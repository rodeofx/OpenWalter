// Copyright 2017 Rodeo FX.  All rights reserved.

// Important to include it before gl.h
#include <pxr/imaging/glf/glew.h>

#include "walterShapeNode.h"
#include "mayaUtils.h"
#include "PathUtil.h"
#include "rdoProfiling.h"
#include "walterThreadingUtils.h"
#include "walterUsdUtils.h"

#include <GL/gl.h>
#include <maya/M3dView.h>
#include <maya/MDagPath.h>
#include <maya/MDrawData.h>
#include <maya/MEventMessage.h>
#include <maya/MExternalContentInfoTable.h>
#include <maya/MExternalContentLocationTable.h>
#include <maya/MFileIO.h>
#include <maya/MFileObject.h>
#include <maya/MFnCamera.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MMaterial.h>
#include <maya/MMatrix.h>
#include <maya/MObjectArray.h>
#include <maya/MPointArray.h>
#include <maya/MSelectionList.h>
#include <maya/MSelectionMask.h>
#include <maya/MViewport2Renderer.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/imageable.h>
#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>
#include <boost/algorithm/string.hpp>
#include <cassert>
#include <climits>
#include "walterUSDCommonUtils.h"
#include <maya/MNodeMessage.h>

//==============================================================================
// Error checking
//==============================================================================

#define MCHECKERROR(STAT,MSG)                   \
    if (!STAT) {                                \
        perror(MSG);                            \
        return MS::kFailure;                    \
    }

#define MREPORTERROR(STAT,MSG)                  \
    if (!STAT) {                                \
        perror(MSG);                            \
    }

namespace WalterMaya {

const MTypeId ShapeNode::id(WALTER_MAYA_SHAPE_ID);

const MString ShapeNode::drawDbClassificationGeometry(
    "drawdb/geometry/walterStandin" );

const MString ShapeNode::drawRegistrantId("walterStandin" );

MObject ShapeNode::aCacheFileName;
MObject ShapeNode::aTime;
MObject ShapeNode::aTimeOffset;
MObject ShapeNode::aBBmode;
MObject ShapeNode::aLayersAssignation;
MObject ShapeNode::aLayersAssignationLayer;
MObject ShapeNode::aLayersAssignationSC;
MObject ShapeNode::aLayersAssignationSCNode;
MObject ShapeNode::aLayersAssignationSCShader;
MObject ShapeNode::aLayersAssignationSCDisplacement;
MObject ShapeNode::aLayersAssignationSCAttribute;
MObject ShapeNode::aExpressions;
MObject ShapeNode::aExpressionsName;
MObject ShapeNode::aExpressionsGroupName;
MObject ShapeNode::aExpressionsWeight;
MObject ShapeNode::aVisibilities;
MObject ShapeNode::aRenderables;
MObject ShapeNode::aAlembicShaders;
MObject ShapeNode::aEmbeddedShaders;
MObject ShapeNode::aArnoldProcedural;
MObject ShapeNode::aUSDSessionLayer;
MObject ShapeNode::aUSDVariantLayer;
MObject ShapeNode::aUSDMayaStateLayer;
MObject ShapeNode::aTransforms;
MObject ShapeNode::aUSDPurposeLayer;
MObject ShapeNode::aTransformsObject;
MObject ShapeNode::aTransformsMatrix;
MObject ShapeNode::aFrozen;
MObject ShapeNode::aUSDVisibilityLayer;
MObject ShapeNode::aHydraPlugin;

const char* ShapeNode::nodeTypeName = "walterStandin";
const char* ShapeNode::selectionMaskName = "walterStandin";

void* ShapeNode::creator()
{
    return new ShapeNode;
}

MStatus ShapeNode::initialize()
{
    MStatus stat;
    MFnTypedAttribute typedAttrFn;
    MFnEnumAttribute eAttr;
    MFnUnitAttribute uAttr;
    MFnNumericAttribute nAttr;
    MFnCompoundAttribute cAttr;
    MFnMessageAttribute mAttr;

    // It's here to be loaded by maya before everything else. We need to know
    // the session layer before we open Alembic cache.
    aUSDSessionLayer = typedAttrFn.create(
            "USDSessionLayer",
            "usdsl",
            MFnData::kString,
            MObject::kNullObj,
            &stat);
    typedAttrFn.setHidden(true);
    typedAttrFn.setInternal(true);
    typedAttrFn.setStorable(true);
    stat = MPxNode::addAttribute(aUSDSessionLayer);
    MCHECKERROR(stat, "MPxNode::addAttribute(USDSessionLayer)");

    aUSDVariantLayer = typedAttrFn.create(
            "USDVariantsLayer",
            "usdvl",
            MFnData::kString,
            MObject::kNullObj,
            &stat);
    typedAttrFn.setHidden(true);
    typedAttrFn.setInternal(true);
    typedAttrFn.setStorable(true);
    stat = MPxNode::addAttribute(aUSDVariantLayer);
    MCHECKERROR(stat, "MPxNode::addAttribute(USDVariantLayer)");

    aUSDPurposeLayer = typedAttrFn.create(
        "USDPurposeLayer",
        "usdpl",
        MFnData::kString,
        MObject::kNullObj,
        &stat);
    typedAttrFn.setHidden(true);
    typedAttrFn.setInternal(true);
    typedAttrFn.setStorable(true);
    stat = MPxNode::addAttribute(aUSDPurposeLayer);
    MCHECKERROR(stat, "MPxNode::addAttribute(USDPurposeLayer)");

    aUSDMayaStateLayer = typedAttrFn.create(
        "USDMayaStateLayer",
        "usdmsl",
        MFnData::kString,
        MObject::kNullObj,
        &stat);
    typedAttrFn.setHidden(true);
    typedAttrFn.setInternal(true);
    typedAttrFn.setStorable(false);
    stat = MPxNode::addAttribute(aUSDMayaStateLayer);
    MCHECKERROR(stat, "MPxNode::addAttribute(aUSDMayaStateLayer)");

    aUSDVisibilityLayer = typedAttrFn.create(
            "USDVisibilityLayer",
            "usdvisl",
            MFnData::kString,
            MObject::kNullObj,
            &stat);
    typedAttrFn.setHidden(true);
    typedAttrFn.setInternal(true);
    typedAttrFn.setStorable(true);
    stat = MPxNode::addAttribute(aUSDVisibilityLayer);

    MCHECKERROR(stat, "MPxNode::addAttribute(USDVisibilityLayer)");

    aBBmode = nAttr.create("useBBox", "ubb",
        MFnNumericData::kBoolean, false, &stat);
    nAttr.setInternal(true);
    stat = MPxNode::addAttribute(aBBmode);
    MCHECKERROR(stat, "MPxNode::addAttribute(aBBmode)");

    // time
    aTime = uAttr.create("time", "t", MFnUnitAttribute::kTime);
    uAttr.setStorable(false);
    uAttr.setKeyable(true);
    uAttr.setReadable(true);
    uAttr.setWritable(true);
    uAttr.setInternal(true);
    stat = MPxNode::addAttribute(aTime);
    MCHECKERROR(stat, "MPxNode::addAttribute(aTime)");

    // timeOffset
    aTimeOffset = uAttr.create("timeOffset", "to",MFnUnitAttribute::kTime);
    uAttr.setStorable(true);
    uAttr.setKeyable(true);
    uAttr.setInternal(true);
    stat = MPxNode::addAttribute(aTimeOffset);
    MCHECKERROR(stat, "MPxNode::addAttribute(aTimeOffset)");

    // Generating following compound attribute
    // layersAssignation[]
    //                   |-layer (message)
    //                   +-shaderConnections[]
    //                             |-abcnode (string)
    //                             |-shader (message)
    //                             |-displacement (message)
    //                             +-attribute (message)

    aLayersAssignationSCShader = mAttr.create("shader", "shader");
    mAttr.setHidden(false);
    mAttr.setReadable(true);
    mAttr.setStorable(true);
    mAttr.setWritable(true);

    aLayersAssignationSCDisplacement = mAttr.create(
            "displacement", "displacement");
    mAttr.setHidden(false);
    mAttr.setReadable(true);
    mAttr.setStorable(true);
    mAttr.setWritable(true);

    aLayersAssignationSCAttribute = mAttr.create(
            "attribute", "attribute");
    mAttr.setHidden(false);
    mAttr.setReadable(true);
    mAttr.setStorable(true);
    mAttr.setWritable(true);

    aLayersAssignationSCNode = typedAttrFn.create(
        "abcnode",
        "abcnode",
        MFnStringData::kString,
        MObject::kNullObj);
    typedAttrFn.setHidden(false);
    typedAttrFn.setReadable(true);
    typedAttrFn.setStorable(true);
    typedAttrFn.setWritable(true);

    aLayersAssignationSC = cAttr.create(
            "shaderConnections", "shaderConnections");
    cAttr.setArray(true);
    cAttr.setReadable(true);
    cAttr.setUsesArrayDataBuilder(true);
    cAttr.addChild(aLayersAssignationSCNode);
    cAttr.addChild(aLayersAssignationSCShader);
    cAttr.addChild(aLayersAssignationSCDisplacement);
    cAttr.addChild(aLayersAssignationSCAttribute);

    aLayersAssignationLayer = mAttr.create("layer", "layer");
    typedAttrFn.setHidden(false);
    typedAttrFn.setReadable(true);
    typedAttrFn.setStorable(true);
    typedAttrFn.setWritable(true);

    aLayersAssignation = cAttr.create("layersAssignation", "la");
    cAttr.setArray(true);
    cAttr.setReadable(true);
    cAttr.setUsesArrayDataBuilder(true);
    cAttr.addChild(aLayersAssignationLayer);
    cAttr.addChild(aLayersAssignationSC);

    stat = MPxNode::addAttribute(aLayersAssignation);
    MCHECKERROR(stat, "MPxNode::addAttribute(layersAssignation)");

    // Generating following compound attribute
    // expressions[]
    //            |-expressionname (string)
    //            +-expressiongroupname (string)
    //            +-expressionweight (int)

    aExpressionsName = typedAttrFn.create(
        "expressionname",
        "en",
        MFnStringData::kString,
        MObject::kNullObj);
    typedAttrFn.setHidden(false);
    typedAttrFn.setReadable(true);
    typedAttrFn.setStorable(true);
    typedAttrFn.setWritable(true);

    aExpressionsGroupName = typedAttrFn.create(
        "expressiongroupname",
        "egn",
        MFnStringData::kString,
        MObject::kNullObj);
    typedAttrFn.setHidden(false);
    typedAttrFn.setReadable(true);
    typedAttrFn.setStorable(true);
    typedAttrFn.setWritable(true);

    aExpressionsWeight = nAttr.create(
        "expressionweight", "ew",
        MFnNumericData::kInt, 0, &stat);
    nAttr.setHidden(true);
    nAttr.setReadable(true);
    nAttr.setStorable(true);
    nAttr.setWritable(true);
    stat = MPxNode::addAttribute(aExpressionsWeight);
    MCHECKERROR(stat, "MPxNode::addAttribute(expressionweight)");

    aExpressions = cAttr.create("expressions", "exp");
    cAttr.setArray(true);
    cAttr.setReadable(true);
    cAttr.setUsesArrayDataBuilder(true);
    cAttr.addChild(aExpressionsName);
    cAttr.addChild(aExpressionsGroupName);
    cAttr.addChild(aExpressionsWeight);

    stat = MPxNode::addAttribute(aExpressions);
    MCHECKERROR(stat, "MPxNode::addAttribute(expressions)");

    aVisibilities = nAttr.create(
            "visibilities",
            "vs",
            MFnNumericData::kBoolean,
            true,
            &stat);
    nAttr.setArray(true);
    nAttr.setInternal(true);
    stat = MPxNode::addAttribute(aVisibilities);
    MCHECKERROR(stat, "MPxNode::addAttribute(visibilities)");

    aRenderables = nAttr.create(
            "renderables",
            "rs",
            MFnNumericData::kBoolean,
            true,
            &stat);
    nAttr.setArray(true);
    nAttr.setInternal(true);
    stat = MPxNode::addAttribute(aRenderables);
    MCHECKERROR(stat, "MPxNode::addAttribute(renderables)");

    aAlembicShaders = typedAttrFn.create(
            "alembicShaders",
            "as",
            MFnData::kString,
            MObject::kNullObj,
            &stat);
    typedAttrFn.setArray(true);
    typedAttrFn.setInternal(true);
    typedAttrFn.setStorable(false);
    stat = MPxNode::addAttribute(aAlembicShaders);
    MCHECKERROR(stat, "MPxNode::addAttribute(aAlembicShaders)");

    aEmbeddedShaders = typedAttrFn.create(
            "embeddedShaders",
            "es",
            MFnData::kString,
            MObject::kNullObj,
            &stat);
    typedAttrFn.setArray(true);
    typedAttrFn.setInternal(true);
    typedAttrFn.setStorable(false);
    stat = MPxNode::addAttribute(aEmbeddedShaders);
    MCHECKERROR(stat, "MPxNode::addAttribute(aEmbeddedShaders)");

    aArnoldProcedural = typedAttrFn.create(
            "arnoldProcedural",
            "ap",
            MFnData::kString,
            MObject::kNullObj,
            &stat);
    typedAttrFn.setHidden(false);
    typedAttrFn.setReadable(true);
    typedAttrFn.setStorable(true);
    typedAttrFn.setWritable(true);
    stat = MPxNode::addAttribute(aArnoldProcedural);
    MCHECKERROR(stat, "MPxNode::addAttribute(arnoldProcedural)");

    // Generating following compound attribute
    // transforms[]
    //           |-transformobject (string)
    //           +-transformmatrix (matrix)

    aTransformsObject = typedAttrFn.create(
        "transformobject",
        "tro",
        MFnStringData::kString,
        MObject::kNullObj);
    typedAttrFn.setConnectable(false);
    typedAttrFn.setHidden(false);
    typedAttrFn.setKeyable(false);
    typedAttrFn.setReadable(true);
    typedAttrFn.setStorable(true);
    typedAttrFn.setWritable(true);

    aTransformsMatrix = typedAttrFn.create(
        "transformmatrix",
        "trm",
        MFnStringData::kMatrix,
        MObject::kNullObj);
    typedAttrFn.setConnectable(true);
    typedAttrFn.setInternal(true);
    typedAttrFn.setKeyable(true);
    typedAttrFn.setReadable(true);
    typedAttrFn.setStorable(false);
    typedAttrFn.setWritable(true);

    aTransforms = cAttr.create("transforms", "tr");
    cAttr.setArray(true);
    cAttr.setReadable(true);
    cAttr.addChild(aTransformsObject);
    cAttr.addChild(aTransformsMatrix);

    stat = MPxNode::addAttribute(aTransforms);
    MCHECKERROR(stat, "MPxNode::addAttribute(transforms)");

    aFrozen = nAttr.create(
        "frozenTransforms", "ft",
        MFnNumericData::kBoolean,
        false,
        &stat);
    nAttr.setInternal(true);
    nAttr.setStorable(false);
    stat = MPxNode::addAttribute(aFrozen);
    MCHECKERROR(stat, "MPxNode::addAttribute(aFrozen)");

    aHydraPlugin = typedAttrFn.create(
            "hydraPlugin",
            "hp",
            MFnData::kString,
            MObject::kNullObj,
            &stat);
    typedAttrFn.setInternal(true);
    stat = MPxNode::addAttribute(aHydraPlugin);
    MCHECKERROR(stat, "MPxNode::addAttribute(hydraPlugin)");

    // File name. On the bottom to be sure that everything else is already
    // loaded from the file.
    aCacheFileName = typedAttrFn.create("cacheFileName", "cfn",
        MFnData::kString, MObject::kNullObj, &stat);
    typedAttrFn.setInternal(true);
    stat = MPxNode::addAttribute(aCacheFileName);
    MCHECKERROR(stat, "MPxNode::addAttribute(aCacheFileName)");

    stat = DisplayPref::initCallback();
    MCHECKERROR(stat, "DisplayPref::initCallbacks()");

    return stat;
}

MStatus ShapeNode::uninitialize()
{
    DisplayPref::removeCallback();

    return MStatus::kSuccess;
}

ShapeNode::ShapeNode() :
    bTriggerGeoReload(false),
    fBBmode(false),
    fLoadGeo(false),
    fReadMaterials(false),
    fSceneLoaded(false),
    fFrozen(false),
    fSubSelectedObjects(),
    mRendererNeedsUpdate(false),
    mJustLoaded(false),
    mDirtyTextures(true),
    mTexturesEnabled(false)
{
    mParams.showGuides = true;
    mParams.showProxy = true;
    mParams.showRender = false;
}

ShapeNode::~ShapeNode()
{
    MMessage::removeCallback(
        mNameChangedCallbackId);
}

void ShapeNode::postConstructor()
{
    MObject mobj = thisMObject();
    MFnDependencyNode nodeFn(mobj);
    nodeFn.setName("walterStandinShape#");

    setRenderable(true);

    // Explicitly initialize config when the first walter node is created.
    // When initializing Config, it will access video adapters via WMI and
    // Windows will sometimes send OnPaint message to Maya and thus cause a
    // refresh. The wired OnPaint message will crash VP2 and walter.
    // Config::initialize();

    mNameChangedCallbackId = MNodeMessage::addNameChangedCallback(mobj, nameChangedCallback, nullptr);  
}

void ShapeNode::nameChangedCallback(MObject &obj, const MString &prevName, void *clientData)
{
    MFnDependencyNode node(obj);

    // If the standin is name "walterStandinShape#", it has just been created.
    // Otherwise it's renamed
    MString cmd = prevName == MString("walterStandinShape#")
        ? "callbacks -executeCallbacks -hook \"WALTER_SCENE_CHANGED\" \"ADD_ROOT_ITEM\" \"" + node.name() + "\""
        : "callbacks -executeCallbacks -hook \"WALTER_SCENE_CHANGED\" \"RENAME_ROOT_ITEM\" \"" + node.name() + "\" \"" + prevName + "\"";
    
    MGlobal::executeCommandOnIdle(cmd);
}

bool ShapeNode::isBounded() const
{
    return true;
}

MBoundingBox ShapeNode::boundingBox() const
{
    static const double minBox = 1.0;
    static const MBoundingBox minBBox(
        MPoint(-minBox, -minBox, -minBox), MPoint(minBox, minBox, minBox));

    // Get USD stuff from the node.
    UsdStageRefPtr stage = getStage();
    if (!stage)
    {
        return minBBox;
    }

    // Frame number.
    // TODO: get FPS from Maya.
    UsdTimeCode timeCode(1.0);
    // UsdTimeCode timeCode(time * 24.0);

    UsdGeomImageable imageable =
        UsdGeomImageable::Get(stage, SdfPath::AbsoluteRootPath());
    GfBBox3d bbox = imageable.ComputeWorldBound(
        timeCode, UsdGeomTokens->default_, UsdGeomTokens->render);

    // Push the bbox to Maya.
    const GfRange3d& range = bbox.GetBox();
    const GfVec3d& min = range.GetMin();
    const GfVec3d& max = range.GetMax();

    return MBoundingBox(
        MPoint(min[0], min[1], min[2]), MPoint(max[0], max[1], max[2]));

    return minBBox;
}

bool ShapeNode::getInternalValueInContext(const MPlug& plug,
    MDataHandle& dataHandle, MDGContext& ctx)
{
    if (plug == aCacheFileName) {
        dataHandle.setString(fCacheFileName);
        return true;
    }
    else if (plug == aTime) {
        return false;
    }
    else if (plug == aTimeOffset) {
        return false;
    }
    else if (plug == aBBmode) {
        dataHandle.setBool(fBBmode);
        return true;
    }
    else if (plug == aAlembicShaders)
    {
        unsigned int index = plug.logicalIndex();

        if (index < mAssignmentShadersBuffer.size())
        {
            dataHandle.setString(mAssignmentShadersBuffer[index].c_str());
            return true;
        }
    }
    else if (plug == aUSDSessionLayer)
    {
        std::string session = constructUSDSessionLayer(thisMObject());
        dataHandle.setString(session.c_str());
        return true;
    }
    else if (plug == aUSDVariantLayer)
    {
        std::string layer = getVariantsLayerAsText(thisMObject());
        dataHandle.setString(layer.c_str());
        return true;
    }
    else if (plug == aUSDPurposeLayer)
    {
        std::string layer = getPurposeLayerAsText(thisMObject());
        dataHandle.setString(layer.c_str());
        return true;
    }
    else if (plug == aUSDVisibilityLayer)
    {
        std::string layer = getVisibilityLayerAsText(thisMObject());
        dataHandle.setString(layer.c_str());
        return true;
    }
    else if (plug == aUSDMayaStateLayer)
    {
        std::string layer = getMayaStateAsUSDLayer(thisMObject());
        dataHandle.setString(layer.c_str());
        return true;
    }
    else if (plug == aEmbeddedShaders)
    {
        unsigned int index = plug.logicalIndex();
        if (index < getEmbeddedShaders().length())
        {
            dataHandle.setString(getEmbeddedShaders()[index]);
            return true;
        }
    }
    else if (plug == aFrozen)
    {
        dataHandle.setBool(fFrozen);
        return true;
    }

    return MPxNode::getInternalValueInContext(plug, dataHandle, ctx);
}

int ShapeNode::internalArrayCount(const MPlug& plug, const MDGContext& ctx)
    const
{
    if (plug == aAlembicShaders)
    {
        // Rebuild list of the shaders. The idea that when we ask the number of
        // items in aAlembicShaders, we prepare all the items and when we ask
        // the item, we don't need to rebuild the complex map to the vector.
        mAssignmentShadersBuffer.clear();

        // Get all the objects
        for (const auto& exp : getAssignments())
        {
            // Get all the layers
            for (const auto& layer : exp.second)
            {
                // The current shader name
                const std::string& currentShader = layer.second;
                // Check if we already have it in the vector
                auto found = std::find(
                    mAssignmentShadersBuffer.begin(),
                    mAssignmentShadersBuffer.end(),
                    currentShader);
                if (found == mAssignmentShadersBuffer.end())
                {
                    mAssignmentShadersBuffer.push_back(currentShader);
                }
            }
        }

        return mAssignmentShadersBuffer.size();
    }
    else if (plug == aEmbeddedShaders)
    {
        return getEmbeddedShaders().length();
    }

    return MPxNode::internalArrayCount(plug, ctx);
}

bool ShapeNode::setInternalFileName()
{
    MString newResolvedFileName = resolvedCacheFileName();
    if (newResolvedFileName != fResolvedCacheFileName)
    {
        fResolvedCacheFileName = newResolvedFileName;

        // Remove the stage. It should remove the object from the viewport.
        setStage(UsdStageRefPtr(nullptr));

        // This should clear the caches.
        onLoaded();

        // Start the loading is a different thread.
        WalterThreadingUtils::loadStage(thisMObject());
    }

    return true;
}

bool ShapeNode::setInternalValueInContext(const MPlug& plug,
    const MDataHandle& dataHandle, MDGContext& ctx)
{

    if (plug == aCacheFileName) {
        fCacheFileName = dataHandle.asString();
        return setInternalFileName();
    }
    else if (plug == aTime) {
        MFnDagNode fn(thisMObject());
        MTime time, timeOffset;
        time = dataHandle.asTime();

        MPlug plug = fn.findPlug(aTimeOffset);
        plug.getValue(timeOffset);

        double dtime;
        dtime = time.as(MTime::kSeconds) + timeOffset.as(MTime::kSeconds);
        double previous = this->time;
        this->time = dtime;

        // We want to update the cached shape
        onTimeChanged(previous);

        return false;
    }
    else if (plug == aTimeOffset) {
        MFnDagNode fn(thisMObject());
        MTime time, timeOffset;
        timeOffset = dataHandle.asTime();

        MPlug plug = fn.findPlug(aTime);
        plug.getValue(time);

        double dtime;
        dtime = time.as(MTime::kSeconds) + timeOffset.as(MTime::kSeconds);
        double previous = this->time;
        this->time = dtime;

        // We want to update the cached shape
        onTimeChanged(previous);

        return false;
    }
    else if (plug == aBBmode) {
        this->fBBmode = dataHandle.asBool();
        bTriggerGeoReload = true;
        return true;
    }
    else if (plug == aVisibilities) {
        unsigned index = plug.logicalIndex();
        unsigned length = fVisibilities.length();

        // Extend the size
        if (length <= index) {
            fVisibilities.setLength(index+1);
        }

        // Fill with default values
        while (length < index) {
            fVisibilities[length] = 1;
            length++;
        }

        fVisibilities[index] = dataHandle.asBool();

        if (!MFileIO::isReadingFile())
        {
            // TODO: it's a fast workaround. When we change one visibility from
            // the UI, Maya calls this method for all the visibilities of the
            // layers. We don't need to reload the stage a lot of times at once,
            // so we reload it only if the last visibility is changed.
            std::string fileName = fCacheFileName.asChar();
            if (std::count(fileName.begin(), fileName.end(), ':') == index)
            {
                // This will update internal file name and reload the stage if
                // necessary.
                setInternalFileName();
            }
        }

        // Return false to indicate that Maya can store the attribute's value in
        // the data block. Otherwise the size of array aFiles will not be
        // changed and this attribute will not be saved in Maya file.
        return false;
    }
    else if (plug == aRenderables) {
        unsigned index = plug.logicalIndex();
        unsigned length = fRenderables.length();

        // Extend the size
        if (length <= index) {
            fRenderables.setLength(index+1);
        }

        // Fill with default values
        while (length < index) {
            fRenderables[length] = 1;
            length++;
        }

        fRenderables[index] = dataHandle.asBool();

        // Return false to indicate that Maya can store the attribute's value in
        // the data block. Otherwise the size of array aFiles will not be
        // changed and this attribute will not be saved in Maya file.
        return false;
    }
    else if (plug == aUSDSessionLayer) {
        // Save the session layer in the buffer. If CacheFileEntry is not loaded
        // yet (it happeng on Maya scene opening), setUSDSessionLayer will skip
        // execution and CacheFileEntry will use this buffer.
        fSessionLayerBuffer = dataHandle.asString();
        setUSDSessionLayer(thisMObject(), fSessionLayerBuffer.asChar());
        return false;
    }
    else if (plug == aUSDVisibilityLayer) {
        // Save the Visibilitys layer in the buffer. If CacheFileEntry is not loaded
        // yet (it happeng on Maya scene opening), setUSDVisibilityLayer will skip
        // execution and CacheFileEntry will use this buffer.
        fVisibilityLayerBuffer = dataHandle.asString();
        setUSDVisibilityLayer(thisMObject(), fVisibilityLayerBuffer.asChar());
        return true;
    }
    else if (plug == aUSDVariantLayer) {
        // Save the variants layer in the buffer. If CacheFileEntry is not loaded
        // yet (it happeng on Maya scene opening), setUSDVariantsLayer will skip
        // execution and CacheFileEntry will use this buffer.
        fVariantsLayerBuffer = dataHandle.asString();
        setUSDVariantsLayer(thisMObject(), fVariantsLayerBuffer.asChar());
        return true;
    }
    else if (plug == aUSDPurposeLayer) {
        // Save the Purposes layer in the buffer. If CacheFileEntry is not loaded
        // yet (it happeng on Maya scene opening), setUSDPurposeLayer will skip
        // execution and CacheFileEntry will use this buffer.
        fPurposeLayerBuffer = dataHandle.asString();
        setUSDPurposeLayer(thisMObject(), fPurposeLayerBuffer.asChar());
        return true;
    }
    else if (plug == aFrozen) {
        this->fFrozen = dataHandle.asBool();

        // Reset caches and re-merge everything.
        setCachedFramesDirty();

        return true;
    }
    else if (plug == aHydraPlugin) {
        // The user changed the Hydra plugin. We need to re-create the renderer.
        mRendererNeedsUpdate = true;
    }

    return MPxNode::setInternalValueInContext(plug, dataHandle, ctx);
}

MStringArray ShapeNode::getFilesToArchive(
    bool shortName, bool unresolvedName, bool markCouldBeImageSequence ) const
{
    MStringArray files;

    if(unresolvedName)
    {
        files.append(fCacheFileName);
    }
    else
    {
        //unresolvedName is false, resolve the path via MFileObject.
        MFileObject fileObject;
        fileObject.setRawFullName(fCacheFileName);
        files.append(fileObject.resolvedFullName());
    }

    return files;
}

void ShapeNode::copyInternalData(MPxNode* source)
{
    if (source && source->typeId() == id)
    {
        ShapeNode* node = dynamic_cast<ShapeNode*>(source);

        // We can't copy mStage because we need to have a copy of the stage.
        fCacheFileName = node->fCacheFileName;
        // This should reload the stage and all the additional stuff.
        setInternalFileName();
    }
}

bool ShapeNode::match( const MSelectionMask & mask,
                   const MObjectArray& componentList ) const
{
    MSelectionMask walterMask(ShapeNode::selectionMaskName);
    return mask.intersects(walterMask) && componentList.length()==0;
}

MSelectionMask ShapeNode::getShapeSelectionMask() const
{
    return MSelectionMask(ShapeNode::selectionMaskName);
}

bool ShapeNode::excludeAsPluginShape() const
{
    // Walter node has its own display filter in Show menu.
    // We don't want "Plugin Shapes" to filter out walter nodes.
    return false;
}

void ShapeNode::setSubSelectedObjects(const MString& obj)
{
    fSubSelectedObjects = obj;

    std::string expression = obj.asChar();

    // Get USD stuff from the node.
    if (!mRenderer || !mStage)
    {
        return;
    }

    SdfPathVector selection;

    if (!expression.empty())
    {
        // Generate a regexp object.
        void* regexp = WalterCommon::createRegex(expression);

        UsdPrimRange range(mStage->GetPrimAtPath(SdfPath::AbsoluteRootPath()));
        for (const UsdPrim& prim: range)
        {
            const SdfPath& path = prim.GetPath();
            std::string current = path.GetString();

            // TODO: What if we have '/cube' and '/cubeBig/shape' objects? If we
            // try to select the first one, both of them will be selected.
            if (boost::starts_with(current, expression) ||
                WalterCommon::searchRegex(regexp, current)) {
                selection.push_back(path);
            }
        }

        // Delete a regexp object.
        WalterCommon::clearRegex(regexp);
    }

    mRenderer->SetSelected(selection);
}

bool ShapeNode::expressionIsMatching(const MString& expression)
{
    return WalterUSDCommonUtils::expressionIsMatching(
        mStage, expression.asChar());
}

ShapeNode* getShapeNode(
        const MString& objectName, MStatus* returnStatus)
{
    /* Get ShapeNode from the string. First, create the section. */
    MSelectionList selection;
    MStatus status = selection.add(objectName);
    if (status != MS::kSuccess) {
        if (returnStatus) {
            *returnStatus = status;
        }
        return NULL;
    }

    /* Get the MObject of the dependency node. */
    MObject object;
    status = selection.getDependNode(0, object);
    if (status != MS::kSuccess) {
        if (returnStatus) {
            *returnStatus = status;
        }
        return NULL;
    }

    /* MFnDependencyNode allows the manipulation of dependency graph nodes. */
    MFnDependencyNode depNode(object, &status);
    if (status != MS::kSuccess) {
        if (returnStatus) {
            *returnStatus = status;
        }
        return NULL;
    }

    /* Get the ShapeNode. */
    ShapeNode* userNode = dynamic_cast<ShapeNode*>(depNode.userNode());
    if (returnStatus) {
        if (userNode) {
            *returnStatus = MS::kSuccess;
        } else {
            *returnStatus = MS::kFailure;
        }
    }

    return userNode;
}

float getCurrentFPS()
{
    // Cache it, so no necessary to querry MTime each frame.
    static float currentFPS = -1.0;

    if (currentFPS < 0.0f)
    {
        MTime::Unit unit = MTime::uiUnit();

        switch (unit)
        {
            case MTime::kGames:
                currentFPS = 15.0f;
                break;
            case MTime::kFilm:
                currentFPS = 24.0f;
                break;
            case MTime::kPALFrame:
                currentFPS = 25.0f;
                break;
            case MTime::kNTSCFrame:
                currentFPS = 30.0f;
                break;
            case MTime::kShowScan:
                currentFPS = 48.0f;
                break;
            case MTime::kPALField:
                currentFPS = 50.0f;
                break;
            case MTime::kNTSCField:
                currentFPS = 60.0f;
                break;
            case MTime::k2FPS:
                currentFPS = 2.0f;
                break;
            case MTime::k3FPS:
                currentFPS = 3.0f;
                break;
            case MTime::k4FPS:
                currentFPS = 4.0f;
                break;
            case MTime::k5FPS:
                currentFPS = 5.0f;
                break;
            case MTime::k6FPS:
                currentFPS = 6.0f;
                break;
            case MTime::k8FPS:
                currentFPS = 8.0f;
                break;
            case MTime::k10FPS:
                currentFPS = 10.0f;
                break;
            case MTime::k12FPS:
                currentFPS = 12.0f;
                break;
            case MTime::k16FPS:
                currentFPS = 16.0f;
                break;
            case MTime::k20FPS:
                currentFPS = 20.0f;
                break;
            case MTime::k40FPS:
                currentFPS = 40.0f;
                break;
            case MTime::k75FPS:
                currentFPS = 75.0f;
                break;
            case MTime::k80FPS:
                currentFPS = 80.0f;
                break;
            case MTime::k100FPS:
                currentFPS = 100.0f;
                break;
            case MTime::k120FPS:
                currentFPS = 120.0f;
                break;
            case MTime::k125FPS:
                currentFPS = 125.0f;
                break;
            case MTime::k150FPS:
                currentFPS = 150.0f;
                break;
            case MTime::k200FPS:
                currentFPS = 200.0f;
                break;
            case MTime::k240FPS:
                currentFPS = 240.0f;
                break;
            case MTime::k250FPS:
                currentFPS = 250.0f;
                break;
            case MTime::k300FPS:
                currentFPS = 300.0f;
                break;
            case MTime::k375FPS:
                currentFPS = 375.0f;
                break;
            case MTime::k400FPS:
                currentFPS = 400.0f;
                break;
            case MTime::k500FPS:
                currentFPS = 500.0f;
                break;
            case MTime::k600FPS:
                currentFPS = 600.0f;
                break;
            case MTime::k750FPS:
                currentFPS = 750.0f;
                break;
            case MTime::k1200FPS:
                currentFPS = 1200.0f;
                break;
            case MTime::k1500FPS:
                currentFPS = 1500.0f;
                break;
            case MTime::k2000FPS:
                currentFPS = 2000.0f;
                break;
            case MTime::k3000FPS:
                currentFPS = 3000.0f;
                break;
            case MTime::k6000FPS:
                currentFPS = 6000.0f;
                break;
            default:
                currentFPS = 0.0f;
                break;
        }
    }

    return currentFPS;
}

//==============================================================================
// CLASS DisplayPref
//==============================================================================

DisplayPref::WireframeOnShadedMode DisplayPref::fsWireframeOnShadedMode;
MCallbackId DisplayPref::fsDisplayPrefChangedCallbackId;

MStatus DisplayPref::initCallback()
{
    MStatus stat;

    // Register DisplayPreferenceChanged callback
    fsDisplayPrefChangedCallbackId = MEventMessage::addEventCallback(
        "DisplayPreferenceChanged", DisplayPref::displayPrefChanged, NULL, &stat);
    MCHECKERROR(stat, "MEventMessage::addEventCallback(DisplayPreferenceChanged");

    // Trigger the callback manually to init class members
    displayPrefChanged(NULL);

    return MS::kSuccess;
}

MStatus DisplayPref::removeCallback()
{
    MStatus stat;

    // Remove DisplayPreferenceChanged callback
    MEventMessage::removeCallback(fsDisplayPrefChangedCallbackId);
    MCHECKERROR(stat, "MEventMessage::removeCallback(DisplayPreferenceChanged)");

    return MS::kSuccess;
}

void DisplayPref::displayPrefChanged(void*)
{
    MStatus stat;
    // Wireframe on shaded mode: Full/Reduced/None
    MString wireframeOnShadedActive = MGlobal::executeCommandStringResult(
        "displayPref -q -wireframeOnShadedActive", false, false, &stat);
    if (stat) {
        if (wireframeOnShadedActive == "full") {
            fsWireframeOnShadedMode = kWireframeOnShadedFull;
        }
        else if (wireframeOnShadedActive == "reduced") {
            fsWireframeOnShadedMode = kWireframeOnShadedReduced;
        }
        else if (wireframeOnShadedActive == "none") {
            fsWireframeOnShadedMode = kWireframeOnShadedNone;
        }
        else {
            assert(0);
        }
    }
}

DisplayPref::WireframeOnShadedMode DisplayPref::wireframeOnShadedMode()
{
    return fsWireframeOnShadedMode;
}


//==============================================================================
// CLASS ShapeUI
//==============================================================================

void* ShapeUI::creator()
{
    return new ShapeUI;
}

ShapeUI::ShapeUI()
{}

ShapeUI::~ShapeUI()
{}

void ShapeUI::getDrawRequests(const MDrawInfo & info,
                              bool objectAndActiveOnly,
                              MDrawRequestQueue & queue)
{
    // Get the data necessary to draw the shape
    MDrawData data;
    getDrawData( 0, data );

    // Decode the draw info and determine what needs to be drawn
    M3dView::DisplayStyle  appearance    = info.displayStyle();
    M3dView::DisplayStatus displayStatus = info.displayStatus();

    MDagPath path = info.multiPath();

    switch ( appearance )
    {
        case M3dView::kBoundingBox :
        {
            MDrawRequest request = info.getPrototype( *this );
            request.setDrawData( data );
            request.setToken( kBoundingBox );

            MColor wireframeColor = MHWRender::MGeometryUtilities::wireframeColor(path);
            request.setColor(wireframeColor);

            queue.add( request );
        }break;

        case M3dView::kWireFrame :
        {
            MDrawRequest request = info.getPrototype( *this );
            request.setDrawData( data );
            request.setToken( kDrawWireframe );

            MColor wireframeColor = MHWRender::MGeometryUtilities::wireframeColor(path);
            request.setColor(wireframeColor);

            queue.add( request );
        } break;


        // All of these modes are interpreted as meaning smooth shaded
        // just as it is done in the viewport 2.0.
        case M3dView::kFlatShaded :
        case M3dView::kGouraudShaded :
        default:
        {
            ShapeNode* node = (ShapeNode*)surfaceShape();
            if (!node) break;

            // Get the view to draw to
            M3dView view = info.view();

            const bool needWireframe = ((displayStatus == M3dView::kActive) ||
                                        (displayStatus == M3dView::kLead)   ||
                                        (displayStatus == M3dView::kHilite) ||
                                        (view.wireframeOnShaded()));

            // When we need to draw both the shaded geometry and the
            // wireframe mesh, we need to offset the shaded geometry
            // in depth to avoid Z-fighting against the wireframe
            // mesh.
            //
            // On the hand, we don't want to use depth offset when
            // drawing only the shaded geometry because it leads to
            // some drawing artifacts. The reason is a litle bit
            // subtle. At silouhette edges, both front-facing and
            // back-facing faces are meeting. These faces can have a
            // different slope in Z and this can lead to a different
            // Z-offset being applied. When unlucky, the back-facing
            // face can be drawn in front of the front-facing face. If
            // two-sided lighting is enabled, the back-facing fragment
            // can have a different resultant color. This can lead to
            // a rim of either dark or bright pixels around silouhette
            // edges.
            //
            // When the wireframe mesh is drawn on top (even a dotted
            // one), it masks this effect sufficiently that it is no
            // longer distracting for the user, so it is OK to use
            // depth offset when the wireframe mesh is drawn on top.
            const DrawToken shadedDrawToken = needWireframe ?
                kDrawSmoothShadedDepthOffset : kDrawSmoothShaded;

            // Get the default material.
            //
            // Note that we will only use the material if the viewport
            // option "Use default material" has been selected. But,
            // we still need to set a material (even an unevaluated
            // one), so that the draw request is indentified as
            // drawing geometry instead of drawing the wireframe mesh.
            MMaterial material = MMaterial::defaultMaterial();

            if (view.usingDefaultMaterial()) {
                // Evaluate the material.
                if ( !material.evaluateMaterial(view, path) ) {
                    MStatus stat;
                    // MString msg = MStringResource::getString(kEvaluateMaterialErrorMsg, stat);
                    // perror(msg.asChar());
                }

                // Create the smooth shaded draw request
                MDrawRequest request = info.getPrototype( *this );
                request.setDrawData( data );

                // This draw request will draw all sub nodes using an
                // opaque default material.
                request.setToken( shadedDrawToken );
                request.setIsTransparent( false );

                request.setMaterial( material );
                queue.add( request );
            }
            else if (view.xray()) {
                // Create the smooth shaded draw request
                MDrawRequest request = info.getPrototype( *this );
                request.setDrawData( data );

                // This draw request will draw all sub nodes using in X-Ray mode
                request.setToken( shadedDrawToken );
                request.setIsTransparent( true );

                request.setMaterial( material );
                queue.add( request );
            }
            else {
                // Opaque draw request
                if (1) {
                    // Create the smooth shaded draw request
                    MDrawRequest request = info.getPrototype( *this );
                    request.setDrawData( data );

                    // This draw request will draw opaque sub nodes
                    request.setToken( shadedDrawToken );

                    // USD draw modification: USD needs to render everything in
                    // one pass. So it's necessary to provide the wireframe
                    // color even if it's a regular shading pass.
                    auto wireOnModel = DisplayPref::wireframeOnShadedMode();
                    if (needWireframe &&
                        wireOnModel != DisplayPref::kWireframeOnShadedNone) {
                        MColor wireframeColor =
                            MHWRender::MGeometryUtilities::wireframeColor(path);
                        request.setColor(wireframeColor);
                    }

                    request.setMaterial( material );
                    queue.add( request );
                }

                // Transparent draw request
                if (0) {
                    // Create the smooth shaded draw request
                    MDrawRequest request = info.getPrototype( *this );
                    request.setDrawData( data );

                    // This draw request will draw transparent sub nodes
                    request.setToken( shadedDrawToken );
                    request.setIsTransparent( true );

                    request.setMaterial( material );
                    queue.add( request );
                }
            }

            // create a draw request for wireframe on shaded if
            // necessary.
            if (needWireframe &&
                DisplayPref::wireframeOnShadedMode() != DisplayPref::kWireframeOnShadedNone)
            {
                MDrawRequest wireRequest = info.getPrototype( *this );
                wireRequest.setDrawData( data );
                wireRequest.setToken( kDrawWireframeOnShaded );
                wireRequest.setDisplayStyle( M3dView::kWireFrame );

                MColor wireframeColor = MHWRender::MGeometryUtilities::wireframeColor(path);
                wireRequest.setColor(wireframeColor);

                queue.add( wireRequest );
            }
        } break;
    }
}

void ShapeUI::draw(const MDrawRequest & request, M3dView & view) const
{
    // Initialize GL Function Table.
    // InitializeGLFT();

    // Get the token from the draw request.
    // The token specifies what needs to be drawn.
    DrawToken token = DrawToken(request.token());

    switch( token )
    {
        case kBoundingBox :
            drawBoundingBox( request, view );
            break;

        case kDrawWireframe :
        case kDrawWireframeOnShaded :
            drawWireframe( request, view );
            break;

        case kDrawSmoothShaded :
            drawShaded( request, view, false );
            break;

        case kDrawSmoothShadedDepthOffset :
            drawShaded( request, view, true );
            break;
    }
}

void ShapeUI::drawBoundingBox(const MDrawRequest & request, M3dView & view) const
{
    // Get the surface shape
    ShapeNode* node = (ShapeNode*)surfaceShape();
    if (!node) return;

    // Get the bounding box
    MBoundingBox box = node->boundingBox();

    view.beginGL();
    {
        // TODO: Draw BBox.
    }
    view.endGL();
}

void ShapeUI::drawWireframe(const MDrawRequest & request, M3dView & view) const
{
    // Get the surface shape
    ShapeNode* node = (ShapeNode*)surfaceShape();
    if ( !node ) return;

    MMatrix projMatrix;
    view.projectionMatrix(projMatrix);
    MMatrix modelViewMatrix;
    view.modelViewMatrix(modelViewMatrix);

    MMatrix localToPort = modelViewMatrix * projMatrix;

    view.beginGL();
    {
        // USD render.
        drawUSD(request, view, true);
    }
    view.endGL();
}


void ShapeUI::drawShaded(
    const MDrawRequest & request, M3dView & view, bool depthOffset) const
{
    // Get the surface shape
    ShapeNode* node = (ShapeNode*)surfaceShape();
    if ( !node ) return;

    MMatrix projMatrix;
    view.projectionMatrix(projMatrix);
    MMatrix modelViewMatrix;
    view.modelViewMatrix(modelViewMatrix);

    MMatrix localToNDC = modelViewMatrix * projMatrix;

    M3dView::LightingMode lightingMode;
    view.getLightingMode(lightingMode);
    unsigned int lightCount;
    view.getLightCount(lightCount);

    view.beginGL();

    // USD render.
    drawUSD(request, view, false);

    view.endGL();
}

void ShapeUI::drawUSD(
        const MDrawRequest& request,
        const M3dView& view,
        bool wireframe) const
{
    // Get the surface shape
    ShapeNode* node = (ShapeNode*)surfaceShape();
    assert(node);

    // Get USD stuff from the node.
    UsdStageRefPtr stage = node->getStage();
    UsdImagingGLEngineSharedPtr renderer = node->getRenderer();
    if (!renderer || !stage)
    {
        return;
    }

    GfMatrix4d viewMatrix;
    GfMatrix4d projectionMatrix;

    glGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix.GetArray());
    glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix.GetArray());

    // Get the Maya viewport.
    unsigned int originX;
    unsigned int originY;
    unsigned int width;
    unsigned int height;
    view.viewport(originX, originY, width, height);

    // Put the camera and the viewport to the engine.
    renderer->SetCameraState(
            viewMatrix,
            projectionMatrix,
            GfVec4d(originX, originY, width, height));

    const MColor wireframeColor = request.color();

    // USD render.
    UsdImagingGLRenderParams params;
    params.frame = UsdTimeCode(node->time * getCurrentFPS());
    params.highlight = false;

    if (wireframeColor.r > 1e-6f ||
        wireframeColor.g > 1e-6f ||
        wireframeColor.b > 1e-6f) {
        // Wireframe color is not 0, so we need to draw the wireframe.
        if (wireframe) {
            // It's a wireframe-only mode.
            params.drawMode = UsdImagingGLDrawMode::DRAW_WIREFRAME;
        }
        else {
            // It's a wireframe with shading mode.
			params.highlight = true;
            params.drawMode = UsdImagingGLDrawMode::DRAW_WIREFRAME_ON_SURFACE;
        }

        params.wireframeColor =
            GfVec4f(
                    wireframeColor.r,
                    wireframeColor.g,
                    wireframeColor.b,
                    1.0f);
    }
    else {
        // No wireframe. Draw shaded only.
        params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
    }

    renderer->Render(
            stage->GetPrimAtPath(SdfPath::AbsoluteRootPath()),
            params);
}

//
// Returns the point in world space corresponding to a given
// depth. The depth is specified as 0.0 for the near clipping plane and
// 1.0 for the far clipping plane.
//
MPoint ShapeUI::getPointAtDepth(
    MSelectInfo &selectInfo,
    double     depth)
{
    MDagPath cameraPath;
    M3dView view = selectInfo.view();

    view.getCamera(cameraPath);
    MStatus status;
    MFnCamera camera(cameraPath, &status);

    // Ortho cam maps [0,1] to [near,far] linearly
    // persp cam has non linear z:
    //
    //        fp np
    // -------------------
    // 1. fp - d fp + d np
    //
    // Maps [0,1] -> [np,fp]. Then using linear mapping to get back to
    // [0,1] gives.
    //
    //       d np
    // ----------------  for linear mapped distance
    // fp - d fp + d np

    if (!camera.isOrtho())
    {
        double np = camera.nearClippingPlane();
        double fp = camera.farClippingPlane();

        depth *= np / (fp - depth * (fp - np));
    }

    MPoint     cursor;
    MVector rayVector;
    selectInfo.getLocalRay(cursor, rayVector);
    cursor = cursor * selectInfo.multiPath().inclusiveMatrix();
    short x,y;
    view.worldToView(cursor, x, y);

    MPoint res, neardb, fardb;
    view.viewToWorld(x,y, neardb, fardb);
    res = neardb + depth*(fardb-neardb);

    return res;
}


bool ShapeUI::select(
    MSelectInfo &selectInfo,
    MSelectionList &selectionList,
    MPointArray &worldSpaceSelectPts ) const
{

    MSelectionMask mask(ShapeNode::selectionMaskName);
    if (!selectInfo.selectable(mask)){
        return false;
    }

    // Get the geometry information
    //
    ShapeNode* node = (ShapeNode*)surfaceShape();

    const bool wireframeSelection =
        (M3dView::kWireFrame == selectInfo.displayStyle() ||
         !selectInfo.singleSelection());

    // USD Selection.
    MSelectionMask objectsMask(MSelectionMask::kSelectObjectsMask);
    // selectable() takes MSelectionMask&, not const MSelectionMask. :(.
    if (!selectInfo.selectable(objectsMask))
    {
        return false;
    }

    // Get USD stuff from the node.
    UsdStageRefPtr stage = node->getStage();
    UsdImagingGLEngineSharedPtr renderer = node->getRenderer();
    if (!renderer || !stage)
    {
        return false;
    }

    M3dView view = selectInfo.view();
    GfMatrix4d viewMatrix;
    GfMatrix4d projectionMatrix;

    // We need to get the view and projection matrices for the area of the
    // view that the user has clicked or dragged. Unfortunately the M3dView
    // does not give us that in an easy way. If we extract the view and
    // projection matrices from the M3dView object, it is just for the
    // regular camera. MSelectInfo also gives us the selection box, so we
    // could use that to construct the correct view and projection matrixes,
    // but if we call beginSelect on the view as if we were going to use the
    // selection buffer, Maya will do all the work for us and we can just
    // extract the matrices from OpenGL.

    // Hit record can just be one because we are not going to draw
    // anything anyway. We only want the matrices
    GLuint glHitRecord;
    view.beginSelect(&glHitRecord, 1);

    glGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix.GetArray());
    glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix.GetArray());

    view.endSelect();

    GfVec3d outHitPoint;
    SdfPath outHitPrimPath;
    UsdImagingGLRenderParams params;
    params.frame = UsdTimeCode(node->time * getCurrentFPS());

    if (wireframeSelection)
    {
        params.drawMode = UsdImagingGLDrawMode::DRAW_WIREFRAME;
    }
    else
    {
        params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
    }

    bool didHit = renderer->TestIntersection(
        viewMatrix,
        projectionMatrix,
        GfMatrix4d(1.0),
        stage->GetPrimAtPath(SdfPath::AbsoluteRootPath()),
        params,
        &outHitPoint,
        &outHitPrimPath);

    // Sub-Selection
    if (didHit && selectInfo.displayStatus() == M3dView::kLead)
    {
        UsdPrim prim;

        // It can be an instance object. It's not allowed to get prim. We need
        // to find the first valid prim.
        while (true)
        {
            prim = stage->GetPrimAtPath(outHitPrimPath);

            if (prim.IsValid())
            {
                break;
            }

            outHitPrimPath = outHitPrimPath.GetParentPath();
        }

        assert(prim.IsValid());

        // Check the purpose of selected object.
        if (prim.IsA<UsdGeomImageable>())
        {
            UsdGeomImageable imageable(prim);
            if (imageable.ComputePurpose() == UsdGeomTokens->proxy)
            {
                // It's a proxy. Level up.
                outHitPrimPath = outHitPrimPath.GetParentPath();
            }
        }

        // If object was selected, set sub-selection.
        node->setSubSelectedObjects(outHitPrimPath.GetText());

        // Execute the callback to change the UI.
        // selectPath() returns the tranform, multiPath() returns the shape.
        MDagPath path = selectInfo.multiPath();
        // Form the MEL command. It looks like this:
        // callbacks
        //      -executeCallbacks
        //      -hook walterPanelSelectionChanged
        //      "objectName" "subSelectedObject";
        MString command =
            "callbacks -executeCallbacks "
            "-hook walterPanelSelectionChanged \"";
        command += path.fullPathName();
        command += "\" \"";
        command += outHitPrimPath.GetText();
        command += "\";";
        MGlobal::executeCommandOnIdle(command);
    }
    else
    {
        // If object was not selected, clear sub-selection.
        node->setSubSelectedObjects("");
    }

    if (didHit)
    {
        MSelectionList newSelectionList;
        newSelectionList.add(selectInfo.selectPath());

        // Transform the hit point into the correct space and make it a maya
        // point
        MPoint mayaHitPoint =
            MPoint(outHitPoint[0], outHitPoint[1], outHitPoint[2]);

        selectInfo.addSelection(
            newSelectionList,
            mayaHitPoint,
            selectionList,
            worldSpaceSelectPts,

            // Even though this is an "object", we use the "meshes"
            // selection mask here. This allows us to select usd
            // assemblies that are switched to "full" as well as those
            // that are still collapsed.
            MSelectionMask(MSelectionMask::kSelectMeshes),

            false);
    }

    return didHit;
}

void ShapeNode::getExternalContent(MExternalContentInfoTable& table) const
{
    addExternalContentForFileAttr(table, aCacheFileName);
    MPxSurfaceShape::getExternalContent(table);
}

void ShapeNode::setExternalContent(const MExternalContentLocationTable& table)
{
    setExternalContentForFileAttr(aCacheFileName, table);
    MPxSurfaceShape::setExternalContent(table);
}

MStatus ShapeNode::setDependentsDirty(const MPlug& plug, MPlugArray& plugArray)
{
    if (plug == aTransformsMatrix)
    {
        // We don't want to make is slow so here we only save the number of the
        // dirty connection.
        MPlug array = plug.parent();
        unsigned int index = array.logicalIndex();

        // We are here because the user changed something in the network. This
        // block is not called when playing the animation. So we need to specify
        // that nothing is cached in this plug.
        fCachedFrames[index].clear();

        // Set this plug dirty.
        fDirtyTransforms.insert(index);

        return MStatus::kSuccess;
    }

    return MPxSurfaceShape::setDependentsDirty(plug, plugArray);
}

void ShapeNode::updateUSDStage() const
{
    // We need to do it only if something is dirty.
    if (!fDirtyTransforms.empty())
    {
        RDOPROFILE("");

#ifdef PERFORM_PROFILING
        using namespace std;
        using ChronoClock = chrono::high_resolution_clock;
        using ChronoMS = chrono::microseconds;

        unsigned int executionDuration = 0;
        ChronoClock::time_point startTime = ChronoClock::now();
#endif

        MObject node = thisMObject();

        const MPlug transforms(node, aTransforms);

        // Save all the matrices to the map.
        std::unordered_map<std::string, MMatrix> matrices(
            fDirtyTransforms.size());

        for (int i : fDirtyTransforms)
        {
            const MPlug element = transforms.elementByLogicalIndex(i);
            const MPlug objectPlug =
                element.child(ShapeNode::aTransformsObject);

            std::string subObjectName = objectPlug.asString().asChar();

            if(subObjectName.empty())
            {
                continue;
            }

            const MPlug matrixPlug =
                element.child(ShapeNode::aTransformsMatrix);

#ifdef PERFORM_PROFILING
            ChronoClock::time_point executionTime = ChronoClock::now();
#endif

            // Maya magic to get a matrix from the plug.
            MObject matrixObject;
            matrixPlug.getValue(matrixObject);
            MFnMatrixData matrixData(matrixObject);
            MTransformationMatrix transformationMatrix =
                matrixData.transformation();

            // Finally! It's the actual matrix.
            matrices[subObjectName] = transformationMatrix.asMatrix();

#ifdef PERFORM_PROFILING
            ChronoClock::time_point finishTime = ChronoClock::now();
            executionDuration +=
                chrono::duration_cast<ChronoMS>(finishTime - executionTime)
                    .count();
#endif
        }

        updateUSDTransforms(this, matrices);

        // Save and clear the cache.
        fPreviousDirtyTransforms = fDirtyTransforms;
        fDirtyTransforms.clear();

#ifdef PERFORM_PROFILING
        ChronoClock::time_point usdTime = ChronoClock::now();
#endif


#ifdef PERFORM_PROFILING
        ChronoClock::time_point finishTime = ChronoClock::now();

        unsigned int usdDuration =
            chrono::duration_cast<ChronoMS>(usdTime - startTime).count();
        unsigned int totalDuration =
            chrono::duration_cast<ChronoMS>(finishTime - startTime).count();

        printf(
            "[WALTER]: profiling of updateUSDStage: total:%ims "
            "execution:%.02f%% "
            "updateUSDTransforms:%.02f%%\n",
            totalDuration / 1000,
            float(executionDuration) * 100 / totalDuration,
            float(usdDuration) * 100 / totalDuration);
#endif
    }
}

void ShapeNode::updateTextures(bool iTexturesEnabled) const
{
    if (iTexturesEnabled != mTexturesEnabled)
    {
        mTexturesEnabled = iTexturesEnabled;
        muteGLSLShaders(this, mTexturesEnabled);
    }

    if (mTexturesEnabled && mDirtyTextures)
    {
        processGLSLShaders(this);
        mDirtyTextures = false;
    }
}

void ShapeNode::setCachedFramesDirty()
{
    RDOPROFILE("");

    // Save and clear the cache.
    fPreviousDirtyTransforms = fDirtyTransforms;
    fDirtyTransforms.clear();
    fCachedFrames.clear();

    // Re-freeze transforms if it's necessary.
    if (this->fFrozen)
    {
        freezeUSDTransforms(this);
    }
    else
    {
        unfreezeUSDTransforms(this);
    }
}

void ShapeNode::onTimeChanged(double previous)
{
    if (this->time == previous)
    {
        // There is a bug when the object was moved in one axis during the keyed
        // animation, Maya ignores manual moving with the same axis.
        // Investigation showed that in this case, Maya sends the signal that
        // the time is changed, not the matrix. It looks like it's really the
        // Maya bug. We use a workaround to resolve it. When we receive the
        // signal that the time is changed, but the time value is the same, we
        // update the matrices that were updated last time.
        // TODO: Do we need fCachedFrames[].clear() here? Looks like not because
        // it was already done in the setDependentsDirty. It's called each time
        // before we get here.
        fDirtyTransforms.insert(
            fPreviousDirtyTransforms.begin(), fPreviousDirtyTransforms.end());

        return;
    }

    // Iterate through all the transforms.
    MPlug transforms(thisMObject(), aTransforms);
    for (int i = 0, n = transforms.numElements(); i < n; i++)
    {
        // Check if this plug for this frame is cached.
        std::set<double>& cachedFrames = fCachedFrames[i];

        if (cachedFrames.find(this->time) != cachedFrames.end())
        {
            // Cached.
            continue;
        }

        // Not cached. Make it dirty and set as cached.
        fDirtyTransforms.insert(i);
        cachedFrames.insert(this->time);
    }
}

void ShapeNode::onLoaded()
{
    RDOPROFILE("");

    // Relod assignments and groups of the stage.
    extractAssignments(this);

    // Recompute the USD shaders.
    mDirtyTextures = true;

    // Refresh the renderer.
    mRendererNeedsUpdate = true;

    // Update it in the viewport. We need it to update the bounding box.
    MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());
}

void ShapeNode::setJustLoaded()
{
    mJustLoaded=true;

    // Update it in the viewport. We need it to update the bounding box.
    MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());

    MFnDependencyNode nodeFn(thisMObject());
    if(MFn::kPluginShape == thisMObject().apiType()) {
        MString cmd = 
            "callbacks -executeCallbacks -hook \"WALTER_SCENE_CHANGED\" \"UPDATE_ITEMS\" \"" + nodeFn.name() + "\"";
        MGlobal::executeCommandOnIdle(cmd);
    }
}

void ShapeNode::refresh()
{
    // Refresh the renderer.
    mRendererNeedsUpdate = true;

    // Update it in the viewport. We need it to update the bounding box.
    MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());
}

MStatus ShapeNode::connectionMade(
    const MPlug& plug,
    const MPlug& otherPlug,
    bool asSrc)
{
    if (plug == aTime)
    {
        // It will evaluate setInternalValueInContext and set the internal time.
        // Without this the time is uninitialized when the scene is opened.
        MTime time;
        plug.getValue(time);
        return MStatus::kSuccess;
    }
    else if (plug == aLayersAssignationSCShader)
    {
        // We need to copletely recompute the shaders because otherwise Hydra
        // doesn't update the scene.
        // TODO: We need to figure out how to fix it.
        mDirtyTextures = true;
    }

    return MPxSurfaceShape::connectionMade(plug, otherPlug, asSrc);
}

MStatus ShapeNode::connectionBroken(
    const MPlug& plug,
    const MPlug& otherPlug,
    bool asSrc)
{
    if (plug == aTransformsMatrix)
    {
        // Reset name when the connection with matrix is broken.
        const MPlug array = plug.parent();
        MPlug objectPlug = array.child(aTransformsObject);
        objectPlug.setString("");

        return MStatus::kSuccess;
    }
    else if (plug == aLayersAssignationSCShader)
    {
        // We need to copletely recompute the shaders because otherwise Hydra
        // doesn't update the scene.
        // TODO: We need to figure out how to fix it.
        mDirtyTextures = true;
    }

    return MPxSurfaceShape::connectionBroken(plug, otherPlug, asSrc);
}

const MString ShapeNode::resolvedCacheFileName() const
{
    // If we have several archives, we need to split them and resolve the files
    // separately. Split the string with ":" symbol, expand all the filenames
    // and join it back.
    // We don't use MString::split() because it skips empty lines. We need
    // everything, otherwise we have errors in matching visibilities and layers.
    std::vector<std::string> archives;
    std::string cacheFileName(fCacheFileName.asChar());
    boost::split(archives, cacheFileName, boost::is_any_of(":"));

    MString resolvedFileName;
    for (unsigned i = 0; i < archives.size(); i++)
    {
        if (fVisibilities.length() > i && !fVisibilities[i])
        {
            continue;
        }

        if (resolvedFileName.length())
        {
            resolvedFileName += ":";
        }

        resolvedFileName += archives[i].c_str();
    }

    return resolvedFileName;
}

void ShapeNode::setStage(UsdStageRefPtr iStage)
{
    mRendererNeedsUpdate = true;
    mStage = iStage;
}

UsdStageRefPtr ShapeNode::getStage() const
{
    return mStage;
}

UsdImagingGLEngineSharedPtr ShapeNode::getRenderer()
{
    if (mJustLoaded)
    {
        mJustLoaded = false;

        // Call onLoaded from here because USD doesn't like when it's called
        // from not main thread.
        onLoaded();
    }

    if (mRendererNeedsUpdate || !mRenderer)
    {
        // Updated. No need to update mode.
        mRendererNeedsUpdate = false;

        // Create the new instance of the Hydra renderer.
        mRenderer = std::make_shared<UsdImagingGLEngine>();

        // Get the name of the necessary Hydra plugin.
        MFnDagNode fn(thisMObject());
        MPlug plug = fn.findPlug(aHydraPlugin);
        std::string hydraPlugin = plug.asString().asChar();

        // Set Hydra plugin if it exists.
        if(!hydraPlugin.empty())
        {
            for (const TfToken& current : mRenderer->GetRendererPlugins())
            {
                if( current.GetString() == hydraPlugin)
                {
                    mRenderer->SetRendererPlugin(current);
                    break;
                }
            }
        }
    }

    return mRenderer;
}
}
