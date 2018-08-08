// Copyright 2017 Rodeo FX.  All rights reserved.

#include "mayaUtils.h"
#include "walterUsdUtils.h"
#include "walterUsdConversion.h"
#include "schemas/walterHdStream/lookAPI.h"
#include "schemas/walterHdStream/primvar.h"
#include "schemas/walterHdStream/shader.h"
#include "schemas/walterHdStream/uvTexture.h"

#ifdef MFB_ALT_PACKAGE_NAME

#include <maya/MAnimControl.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMatrixData.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MSelectionList.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/plug/registry.h>
#include <pxr/imaging/hdx/rendererPluginRegistry.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <pxr/usd/usdShade/material.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/regex.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/variant.hpp>
#include <chrono>
#include <unordered_set>
#include <limits>

#include "PathUtil.h"
#include "walterShapeNode.h"
#include "rdoCustomResource.h"
#include "rdoProfiling.h"
#include "schemas/expression.h"
#include "walterAssignment.h"
#include "walterShaderUtils.h"
#include "walterUSDCommonUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE
using namespace WalterMaya;

typedef boost::variant<MObject, SdfPath> ShaderBoostVariant;

const static TfToken xformToken("xformOp:transform");

// Changes given character if it's not allowed in USD name.
struct ExpressionFixName
{
    char operator()(char x) const { return isalnum(x) ? x : '_'; }
};

inline std::string getBoostVariantName(const ShaderBoostVariant& var)
{
    switch (var.which())
    {
        case 0:
            // Hypershade shading network
            return MFnDependencyNode(boost::get<MObject>(var)).name().asChar();
        case 1:
            // USD shading network
            return boost::get<SdfPath>(var).GetElementString();
        default:
            // Should never happen
            return "";
    }
}

std::size_t hash_value(ShaderBoostVariant const& var)
{
    size_t hash = 0;
    boost::hash_combine(hash, boost::hash<int>{}(var.which()));
    boost::hash_combine(
        hash, boost::hash<std::string>{}(getBoostVariantName(var)));
    return hash;
}

UsdStageRefPtr getStage(const ShapeNode* node)
{
    // Get UsdStage from the data.
    return node->getStage();
}

void stdStringArrayToMStringArray(
    std::vector<std::string> const& stdArray,
    MStringArray& mArray)
{
    for (unsigned i = 0; i < stdArray.size(); ++i)
        mArray.append(stdArray[i].c_str());
}

/**
 * @brief Create USD Attribute out of MPlug.
 *
 * @param prim UsdPrim where it's necessary to create an attribute.
 * @param attrPlug The source MPlug to get the type and the data.
 * @param name Name of the attribute.
 * @param custom If it's necessary to put custom meta to the attribute.
 *
 * @return New attribute
 */
UsdAttribute createAttribute(
    UsdPrim& prim,
    const MPlug& attrPlug,
    const TfToken& name,
    bool custom)
{
    const MDataHandle data = attrPlug.asMDataHandle();

    UsdAttribute attr;

    // Convert Maya type to USD type.
    switch (data.numericType())
    {
        case MFnNumericData::kInvalid:
        {
            int v;
            if (data.type() == MFnData::kString)
            {
                // It's a string.
                attr = prim.CreateAttribute(
                    name, SdfValueTypeNames->String, custom);
                std::string str = data.asString().asChar();
                attr.Set(str);
            }
            else if (attrPlug.getValue(v) == MS::kSuccess)
            {
                // This plug doesn't have data. But we can try to get an int.
                attr =
                    prim.CreateAttribute(name, SdfValueTypeNames->Int, custom);
                attr.Set(v);
            }
            break;
        }

        case MFnNumericData::kBoolean:
        {
            // This plug doesn't have data. But we can try to get an int.
            attr = prim.CreateAttribute(name, SdfValueTypeNames->Bool, custom);
            attr.Set(data.asBool());
            break;
        }

        case MFnNumericData::kShort:
        case MFnNumericData::kLong:
        {
            attr = prim.CreateAttribute(name, SdfValueTypeNames->Int, custom);
            attr.Set(data.asInt());
            break;
        }

        case MFnNumericData::kFloat:
        {
            attr = prim.CreateAttribute(name, SdfValueTypeNames->Float, custom);
            attr.Set(data.asFloat());
            break;
        }

        case MFnNumericData::kDouble:
        {
            // Float anyway
            attr = prim.CreateAttribute(name, SdfValueTypeNames->Float, custom);
            attr.Set(static_cast<float>(data.asDouble()));
            break;
        }

        case MFnNumericData::k3Float:
        {
            // TODO: Point, Vector etc...
            attr =
                prim.CreateAttribute(name, SdfValueTypeNames->Color3f, custom);
            const float3& color = data.asFloat3();
            const GfVec3f vec(color[0], color[1], color[2]);
            attr.Set(vec);
            break;
        }
    }

    return attr;
}

/**
 * @brief Convert Maya object to USD object. ATM only walterOverride is
 * supported.
 *
 * @param stage UsdStage where it's necessary to create the shader.
 * @param obj Maya object to convert.
 * @param parentPath Path of the parent object.
 *
 * @return Path of created objects.
 */
SdfPath saveConnectedShader(
    UsdStageRefPtr stage,
    MObject obj,
    const SdfPath& parentPath)
{
    const MFnDependencyNode depNode(obj);
    std::string shaderName = depNode.name().asChar();

    if (depNode.typeName() == "walterOverride")
    {
        // TODO: Skip if already created.
        SdfPath attributesPath = parentPath.AppendChild(TfToken(shaderName));
        UsdShadeMaterial attributesSchema =
            UsdShadeMaterial::Define(stage, attributesPath);
        UsdPrim attributesPrim = attributesSchema.GetPrim();

        // Iterate the attribures of the walterOverride node.
        unsigned int nAttributes = depNode.attributeCount();
        for (unsigned int a = 0; a < nAttributes; a++)
        {
            MObject object = depNode.attribute(a);
            MFnAttribute attr(object);

            // Set good name. Remove the prefix and convert it.
            // "walterSubdivType" -> "subdiv_type"
            bool isUserData = false;
            std::string name =
                attributeNameDemangle(attr.name().asChar(), &isUserData);
            if (name.empty())
            {
                continue;
            }

            // "arnold:attribute:" is hardcoded now because we don't have
            // another options.
            MPlug attrPlug = depNode.findPlug(object);
            createAttribute(
                attributesPrim,
                attrPlug,
                TfToken("arnold:attribute:" + name),
                isUserData);
        }

        return attributesPath;
    }

    return SdfPath(shaderName);
}

/**
 * @brief Save one single connection to the stage.
 *
 * @param stage UsdStage where it's necessary to create the shaders and
 * expressions.
 *
 * @param expression "/*"
 * @param renderLayer "defaultRenderLayer"
 * @param renderTarget "attribute", "displacement", "shader"
 * @param shaderPlug Maya connection
 */
void saveConnection(
    UsdStageRefPtr stage,
    const std::string& expression,
    const std::string& renderLayer,
    const std::string& renderTarget,
    const MPlug& shaderPlug)
{
    MPlugArray connections;
    if (!shaderPlug.connectedTo(connections, true, false) ||
        !connections.length())
    {
        return;
    }

    const MObject connectedNode(connections[0].node());

    // TODO: MTOA outputs displacement nodes like this:
    // MayaNormalDisplacement
    // {
    //  name displacementShader1.message
    // }
    //
    // We need to investigate why. But now we have a spacial case.

    // USD doesn't allow to use special characters as names. We need to mangle
    // them.
    // TODO: consider the name intersections.
    std::string nodeName = expression;
    if (!SdfPath::IsValidIdentifier(nodeName))
    {
        std::transform(
            nodeName.begin(),
            nodeName.end(),
            nodeName.begin(),
            ExpressionFixName());
    }

    // We need to put everything to the materials node.
    SdfPath materialsPath = SdfPath("/materials");

    // Current object path.
    SdfPath expressionPath =
        materialsPath.AppendChild(TfToken(nodeName));

    // Create the expression node.
    auto expressionPrim =
        stage->DefinePrim(expressionPath, TfToken("Expression"));
    // TODO: Some check or assert.
    WalterExpression expressionSchema(expressionPrim);

    // Set expression
    // RND-555: Demangle the expression to convert special regex characters
    // We have to do this because USD converts by it-self backslash to slash
    // and breaks regex expressions...
    expressionSchema.SetExpression(
        WalterCommon::convertRegex(expression));

    // Set connection with the Material.
    SdfPath connectedToPath =
        saveConnectedShader(stage, connectedNode, materialsPath);
    expressionSchema.SetConnection(renderLayer, renderTarget, connectedToPath);
}

// The description is in the header.
std::string constructUSDSessionLayer(MObject obj)
{
    // Maya node
    MFnDagNode dagNode(obj);

    UsdStageRefPtr sceneStage =
        getStage(dynamic_cast<ShapeNode*>(dagNode.userNode()));

    if (!sceneStage)
    {
        return {};
    }

    // If we don't need assignments, we are done. Return what we have.
    std::string session;
    sceneStage->GetSessionLayer()->ExportToString(&session);
    return session;
}

std::string getVariantsLayerAsText(MObject obj)
{
    // Maya node
    MFnDagNode dagNode(obj);

    UsdStageRefPtr sceneStage =
        getStage(dynamic_cast<ShapeNode*>(dagNode.userNode()));

    if (!sceneStage)
    {
        return {};
    }

    return WalterUSDCommonUtils::getVariantsLayerAsText(sceneStage);
}

std::string getPurposeLayerAsText(MObject obj)
{
    // Maya node
    MFnDagNode dagNode(obj);

    UsdStageRefPtr sceneStage =
        getStage(dynamic_cast<ShapeNode*>(dagNode.userNode()));

    if (!sceneStage)
    {
        return {};
    }

    return WalterUSDCommonUtils::getPurposeLayerAsText(sceneStage);
}

std::string getVisibilityLayerAsText(MObject obj)
{
    // Maya node
    MFnDagNode dagNode(obj);

    UsdStageRefPtr sceneStage =
        getStage(dynamic_cast<ShapeNode*>(dagNode.userNode()));

    if (!sceneStage)
    {
        return {};
    }

    return WalterUSDCommonUtils::getVisibilityLayerAsText(sceneStage);
}

void setUSDVisibilityLayer(MObject obj, const std::string& visibility)
{
    // Maya node
    MFnDagNode dagNode(obj);

    // Get USD stuff from the node.
    UsdStageRefPtr sceneStage =
        getStage(dynamic_cast<ShapeNode*>(dagNode.userNode()));

    if (!sceneStage)
    {
        return;
    }

    if(!WalterUSDCommonUtils::setUSDVisibilityLayer(
        sceneStage, visibility.c_str()))
    {
        MGlobal::displayError(
            "[Walter USD] Cant't set the visibility layer to the object " +
            dagNode.name());
    }
}

// The description is in the header.
void setUSDSessionLayer(MObject obj, const std::string& session)
{
    // Maya node
    MFnDagNode dagNode(obj);

    // Get USD stuff from the node.
    UsdStageRefPtr sceneStage =
        getStage(dynamic_cast<ShapeNode*>(dagNode.userNode()));
    if (!sceneStage)
    {
        // It happens when opening a Maya scene. CachedGeometry is not created
        // yet. But it's OK because we will use this layer when creating
        // CacheFileEntry.
        return;
    }

    if (session.empty())
    {
        sceneStage->GetSessionLayer()->Clear();
    }

    else if (!sceneStage->GetSessionLayer()->ImportFromString(session))
    {
        MGlobal::displayError(
            "[Walter USD] Cant't set the session layer to the object " +
            dagNode.name());
        sceneStage->GetSessionLayer()->Clear();
    }
}

void setUSDVariantsLayer(MObject obj, const std::string& variants)
{
    // Maya node
    MFnDagNode dagNode(obj);

    // Get USD stuff from the node.
    UsdStageRefPtr sceneStage =
        getStage(dynamic_cast<ShapeNode*>(dagNode.userNode()));
    if (!sceneStage)
    {
        // It happens when opening a Maya scene. CachedGeometry is not created
        // yet. But it's OK because we will use this layer when creating
        // CacheFileEntry.
        return;
    }

    if(!WalterUSDCommonUtils::setUSDVariantsLayer(
        sceneStage, variants.c_str()))
    {
        MGlobal::displayError(
            "[Walter USD] Cant't set the variants layer to the object " +
            dagNode.name());
    }
}

void setUSDPurposeLayer(MObject obj, const std::string& variants)
{
    // Maya node
    MFnDagNode dagNode(obj);

    // Get USD stuff from the node.
    UsdStageRefPtr sceneStage =
        getStage(dynamic_cast<ShapeNode*>(dagNode.userNode()));

    if (!sceneStage)
    {
        // It happens when opening a Maya scene. CachedGeometry is not created
        // yet. But it's OK because we will use this layer when creating
        // CacheFileEntry.
        return;
    }

    if(!WalterUSDCommonUtils::setUSDPurposeLayer(
        sceneStage, variants.c_str()))
    {
        MGlobal::displayError(
            "[Walter USD] Cant't set the variants layer to the object " +
            dagNode.name());
    }
}

bool getUSDTransform(
    MObject obj,
    const std::string& object,
    double time,
    double matrix[4][4])
{
    // Maya node
    MFnDagNode dagNode(obj);

    // Get USD stuff from the node.
    UsdStageRefPtr stage =
        getStage(dynamic_cast<ShapeNode*>(dagNode.userNode()));
    if (!stage)
    {
        return false;
    }

    UsdPrim prim = stage->GetPrimAtPath(SdfPath(object));
    if (!prim)
    {
        std::string message("[Walter USD] Cant't get the tranform of prim ");
        message += object;
        MGlobal::displayError(message.c_str());
        return false;
    }

    GfMatrix4d gfMatrix;
    bool resetsXformStack;
    UsdGeomXformable(prim).GetLocalTransformation(
        &gfMatrix, &resetsXformStack, time);
    gfMatrix.Get(matrix);

    return true;
}

bool dirUSD(
    const ShapeNode* node,
    const MString& subNodeName,
    MStringArray& result)
{
    // Get USD stuff from the node.
    UsdStageRefPtr stage = getStage(node);
    if (!stage)
    {
        return false;
    }

    std::vector<std::string> result_ = WalterUSDCommonUtils::dirUSD(
        stage, subNodeName.asChar());
    stdStringArrayToMStringArray(result_, result);

    return true;
}

bool propertiesUSD(
    const ShapeNode* node,
    const MString& subNodeName,
    MStringArray& result,
    bool attributeOnly)
{
    // Get USD stuff from the node.
    UsdStageRefPtr stage = getStage(node);
    if (!stage)
    {
        return false;
    }

    std::vector<std::string> result_ = WalterUSDCommonUtils::propertiesUSD(
        stage, subNodeName.asChar(), node->time, attributeOnly);
    stdStringArrayToMStringArray(result_, result);

    return true;
}

bool getVariantsUSD(const ShapeNode* node, MStringArray& result)
{
    // Get USD stuff from the node.
    UsdStageRefPtr stage = getStage(node);
    if (!stage)
    {
        return false;
    }

    std::vector<std::string> result_ =
        WalterUSDCommonUtils::getVariantsUSD(stage);
    stdStringArrayToMStringArray(result_, result);

    return true;
}

bool setVariantUSD(
    ShapeNode* node,
    const MString& subNodeName,
    const MString& variantName,
    const MString& variantSetName)
{
    // Get USD stuff from the node.
    UsdStageRefPtr stage = getStage(node);
    if (!stage)
    {
        return false;
    }

    SdfLayerRefPtr variantLayer =
        WalterUSDCommonUtils::getAnonymousLayer(stage, "variantLayer");
    stage->SetEditTarget(variantLayer);

    WalterUSDCommonUtils::setVariantUSD(stage, subNodeName.asChar(),
        variantName.asChar(), variantSetName.asChar());
    node->refresh();

    return true;
}

bool assignedOverridesUSD(
    const ShapeNode* node,
    const MString& subNodeName,
    MStringArray& result)
{
    // Get USD stuff from the node.
    UsdStageRefPtr stage = getStage(node);
    if (!stage)
    {
        return false;
    }

    UsdTimeCode tc(node->time);

    const SdfPath* primitivePath = WalterCommon::resolveAssignment<SdfPath>(
        subNodeName.asChar(), node->getAssignedOverrides());

    if (!primitivePath)
    {
        return {};
    }

    UsdPrim walterOverride = stage->GetPrimAtPath(*primitivePath);

    std::vector<std::string> overrideDescription;
    for (const auto& attr : walterOverride.GetAttributes())
    {
        int arraySize = -1;
        std::string type, value;
        WalterUsdConversion::getAttributeValueAsString(
            attr, tc, type, value, arraySize);

        std::string description =
            WalterUsdConversion::constuctStringRepresentation(
                attr.GetBaseName().GetText(),
                "Override",
                type,
                value,
                arraySize);

        result.append(description.c_str());
    }

    return true;
}

bool primVarUSD(
    const ShapeNode* node,
    const MString& subNodeName,
    MStringArray& result)
{
    // Get USD stuff from the node.
    UsdStageRefPtr stage = getStage(node);
    if (!stage)
    {
        return false;
    }

    std::vector<std::string> result_ = WalterUSDCommonUtils::primVarUSD(
        stage, subNodeName.asChar(), node->time);
    stdStringArrayToMStringArray(result_, result);

    return true;
}

inline UsdTimeCode getUSDTimeFromMayaTime(
    const WalterMaya::ShapeNode* shapeNode) {

    const MPlug timePlug =
        MFnDependencyNode(shapeNode->thisMObject()).findPlug(ShapeNode::aTime);
    double time;
    if (!timePlug.isConnected())
    {
        time = MAnimControl::currentTime().value();
        shapeNode->time = time / getCurrentFPS();
    }
    else
    {
        time = shapeNode->time * getCurrentFPS();
    }
    return UsdTimeCode(time);
}

void resetUSDTransforms(
    const WalterMaya::ShapeNode* shapeNode,
    const MString& subNodeName) {

    // Get USD stuff from the node.
    UsdStageRefPtr stage = getStage(shapeNode);
    if (!stage)
        return;
    
    UsdPrim prim = shapeNode->getStage()->GetPrimAtPath(
        SdfPath(subNodeName.asChar()));

    UsdTimeCode currentFrame = getUSDTimeFromMayaTime(
        shapeNode);

    UsdAttribute xform = prim.GetAttribute(xformToken);
    
    GfMatrix4d current;
    xform.Get(&current, currentFrame);
    xform.Clear();
    
    xform.Set(current, currentFrame);
}

bool updateUSDTransforms(
    const ShapeNode* shapeNode,
    const std::unordered_map<std::string, MMatrix>& matrices)
{
    RDOPROFILE("");

#ifdef PERFORM_PROFILING
    using namespace std;
    using ChronoClock = chrono::high_resolution_clock;
    using ChronoMS = chrono::microseconds;

    unsigned int objectDuration = 0;
    unsigned int attributeDuration = 0;
    ChronoClock::time_point startTime = ChronoClock::now();
#endif

    // Get USD stuff from the node.
    UsdStageRefPtr stage = getStage(shapeNode);
    if (!stage)
    {
        return false;
    }

    UsdTimeCode currentFrame = getUSDTimeFromMayaTime(
        shapeNode);

    // Put all the edits to the session layer.
    stage->SetEditTarget(stage->GetSessionLayer());


    for (const auto& object : matrices)
    {
#ifdef PERFORM_PROFILING
        ChronoClock::time_point objectTime = ChronoClock::now();
#endif

        const std::string subObjectName = object.first;
        const MMatrix& subObjectTransform = object.second;

        UsdPrim prim = stage->GetPrimAtPath(SdfPath(subObjectName));
      
        if (!prim)
        {
            continue;
        }

        // Don't set the transform of a prim pseudo-root.
        // It's not supported by USD and leads to a SEGFAULT
        if(prim.IsPseudoRoot())
        {
            MString msg = "[walter] Cannot set the transform of the USD pseudo-root: '";
            msg += prim.GetPath().GetText();
            msg +=  "'";
            MGlobal::displayWarning(msg);
            continue;
        }

        // The actual matrix.
        double buffer[4][4];
        subObjectTransform.get(buffer);
        GfMatrix4d matrix(buffer);

        // It's a bottleneck. MakeMatrixXform is extremely slow because it
        // removes all the transformation attributes and creates a new one. We
        // can check if the new one is created and use it directly.
        UsdAttribute xform = prim.GetAttribute(xformToken);
        if (xform.HasValue())
        {
#ifdef PERFORM_PROFILING
            ChronoClock::time_point attributeTime = ChronoClock::now();
#endif

            GfMatrix4d old;
            xform.Get(&old, currentFrame);
            if (old != matrix)
            {
                // TODO: try to set attributes in multiple threads because Set
                // is also a bottleneck.
                xform.Set(matrix, currentFrame);
            }

#ifdef PERFORM_PROFILING
            ChronoClock::time_point finishTime = ChronoClock::now();

            attributeDuration +=
                chrono::duration_cast<ChronoMS>(finishTime - attributeTime)
                    .count();
#endif
        }
        else
        {
            UsdGeomXformable xf(prim);
            UsdGeomXformOp xfOp = xf.MakeMatrixXform();
            xfOp.Set(matrix, currentFrame);
        }

#ifdef PERFORM_PROFILING
        ChronoClock::time_point finishTime = ChronoClock::now();
        objectDuration +=
            chrono::duration_cast<ChronoMS>(finishTime - objectTime).count();
#endif
    }

#ifdef PERFORM_PROFILING
    ChronoClock::time_point finishTime = ChronoClock::now();

    unsigned int totalDuration =
        chrono::duration_cast<ChronoMS>(finishTime - startTime).count();

    printf(
        "[WALTER]: profiling of updateUSDTransforms: total:%ims "
        "UsdPrim:%.02f%% "
        "UsdAttribute::Set:%.02f%%\n",
        totalDuration / 1000,
        float(objectDuration - attributeDuration) * 100 / totalDuration,
        float(attributeDuration) * 100 / totalDuration);
#endif

    return true;
}

bool saveUSDTransforms(
    const MString& objectName,
    const MString& fileName,
    const MTime& start,
    const MTime& end,
    const MTime& step,
    bool resetSessionLayer)
{
    RDOPROFILE("");

    if (step.value() == 0.0)
    {
        MString msg;
        msg.format(
            "[walter] Step time for ^1s is 0. Seems it's an infinite loop.",
            objectName);
        MGlobal::displayError(msg);
        return false;
    }

    // Looking for MFnDependencyNode object in the scene.
    MSelectionList selectionList;
    selectionList.add(objectName);

    MObject obj;
    selectionList.getDependNode(0, obj);

    MFnDependencyNode depNode(obj);
    ShapeNode* shapeNode = dynamic_cast<ShapeNode*>(depNode.userNode());
    
    if(resetSessionLayer)
    {
        setUSDSessionLayer(obj, "");
        shapeNode->setCachedFramesDirty();
    }
    
    const MPlug time = depNode.findPlug(ShapeNode::aTime);

    // Evaluate the requested frames.
    for (MTime t = start; t <= end; t += step)
    {
        MAnimControl::setCurrentTime(t);

        // Evaluate the time graph. It should execute
        // ShapeNode::setInternalValueInContext and save the dirty transforms.
        time.asMTime();

        // RND-647: Unfortunately if the node is not connected to time 
        // we need to force onTimeChanged.
        MFnDependencyNode depNode(shapeNode->thisMObject());
        const MPlug timePlug = depNode.findPlug(ShapeNode::aTime);
        if(!timePlug.isConnected())
        {
            shapeNode->onTimeChanged(std::numeric_limits<float>::min());
        }
        // Save the transforms to the USD stage.
        shapeNode->updateUSDStage();
    }

    UsdStageRefPtr stage = getStage(shapeNode);
    if (!stage->GetSessionLayer()->Export(fileName.asChar()))
    {
        MString msg;
        msg.format(
            "[walter] Can't write USD ^1s. See the terminal for the details.",
            fileName);
        MGlobal::displayError(msg);
        return false;
    }

    return true;
}

/**
 * @brief Gets Perform value resolution to fetch the value of this attribute at
 * the default time or at first available sample if the attribute is sampled.
 *
 * @param attr Attribute to resolve.
 * @param value Value to write.
 * @param time The default time.
 *
 * @return true if there was a value to be read.
 */
template <typename T>
bool usdAttributeGet(
    const UsdAttribute attr,
    T* value,
    UsdTimeCode time = UsdTimeCode::Default())
{
    std::vector<double> times;
    if (attr.GetTimeSamples(&times) && !times.empty())
    {
        time = times[0];
    }

    return attr.Get(value, time);
}

void mergeUSDObject(
    UsdStageRefPtr sceneStage,
    const UsdPrim& parent,
    const std::vector<UsdPrim>& prims)
{
    if (prims.empty())
    {
        return;
    }

    std::vector<UsdPrimRange> allPrims;
    allPrims.reserve(prims.size());
    for (const UsdPrim& prim : prims)
    {
        allPrims.push_back(UsdPrimRange::AllPrims(prim));
    }

    // Get total number of prims.
    int numPrims = 0;
    for (const auto& range : allPrims)
    {
        for (const auto& prim : range)
        {
            numPrims++;
        }
    }

    if (numPrims <= 1)
    {
        return;
    }

    // Read the mesh data for all the prims
    std::vector<VtArray<int>> faceVertexCounts(numPrims);
    std::vector<VtArray<int>> faceVertexIndices(numPrims);
    std::vector<VtArray<GfVec3f>> points(numPrims);
    std::vector<GfMatrix4d> transforms(numPrims);

    int counter = 0;
    // TODO: Multithreaded
    for (const auto& range : allPrims)
    {
        for (const auto& prim : range)
        {
            if (prim.IsA<UsdGeomMesh>())
            {
                UsdGeomMesh mesh(prim);

                UsdAttribute countsAttr = mesh.GetFaceVertexCountsAttr();
                UsdAttribute indicesAttr = mesh.GetFaceVertexIndicesAttr();
                UsdAttribute pointsAttr = mesh.GetPointsAttr();

                if (countsAttr && indicesAttr && indicesAttr)
                {
                    usdAttributeGet(countsAttr, &(faceVertexCounts[counter]));
                    usdAttributeGet(indicesAttr, &(faceVertexIndices[counter]));
                    usdAttributeGet(pointsAttr, &(points[counter]));

                    // TODO: Time
                    transforms[counter] =
                        mesh.ComputeLocalToWorldTransform(1.0);
                }
            }

            counter++;
        }
    }

    // Get the inverse transformation of the parent.
    GfMatrix4d parentInvTransform;
    // Set the purpose of this object.
    if (parent.IsA<UsdGeomImageable>())
    {
        UsdGeomImageable imageable(parent);
        // TODO: Time
        parentInvTransform = imageable.ComputeLocalToWorldTransform(1.0);
        parentInvTransform = parentInvTransform.GetInverse();
    }
    else
    {
        parentInvTransform.SetIdentity();
    }

    // Compute the total amount of data
    int faceVertexCountsSize = 0;
    int faceVertexIndicesSize = 0;
    int pointsSize = 0;

    for (const auto& a : faceVertexCounts)
    {
        faceVertexCountsSize += a.size();
    }

    for (const auto& a : faceVertexIndices)
    {
        faceVertexIndicesSize += a.size();
    }

    for (const auto& a : points)
    {
        pointsSize += a.size();
    }

    // Initialize arrays.
    VtArray<int> newObjectCounts(faceVertexCountsSize);
    VtArray<int> newObjectIndices(faceVertexIndicesSize);
    VtArray<GfVec3f> newObjectPoints(pointsSize);

    // Merge everything together.
    int countsOffset = 0;
    int indicesOffset = 0;
    int pointsOffset = 0;
    // TODO: Multithreaded
    for (int i = 0; i < numPrims; i++)
    {
        // Nothing to do with vertex counts, so we can batch copy them.
        std::copy(
            faceVertexCounts[i].begin(),
            faceVertexCounts[i].end(),
            newObjectCounts.begin() + countsOffset);
        countsOffset += faceVertexCounts[i].size();

        // We need to increase the point offset to each index.
        int faceVertexIndicesCurrentSize = faceVertexIndices[i].size();
        for (int j = 0; j < faceVertexIndicesCurrentSize; j++)
        {
            newObjectIndices[indicesOffset + j] =
                faceVertexIndices[i][j] + pointsOffset;
        }
        indicesOffset += faceVertexIndicesCurrentSize;

        // Relative transformation of the current object.
        const GfMatrix4d& currentTransform = transforms[i] * parentInvTransform;

        // We need to transform each point.
        int pointsCurrentSize = points[i].size();
        for (int j = 0; j < pointsCurrentSize; j++)
        {
            const GfVec3f& p3f = points[i][j];

            // To multiply point (not vector) by matrix we need 4 component
            // vector and the latest component should be 1.0.
            GfVec4f p4f(p3f[0], p3f[1], p3f[2], 1.0f);

            p4f = p4f * currentTransform;

            // Back to 3 components.
            newObjectPoints[pointsOffset + j] = GfVec3f(p4f[0], p4f[1], p4f[2]);
        }
        pointsOffset += pointsCurrentSize;
    }

    // Set render purpose to all the children.
    for (const UsdPrim& prim : prims)
    {
#if 1
        // Set this object as inactive. It's MUCH faster, it's revercable, but
        // the object becames not iterable.
        prim.SetActive(false);
#else
        // Set the render purpose for this object.
        if (!prim.IsA<UsdGeomImageable>())
        {
            continue;
        }

        UsdGeomImageable imageable(prim);
        UsdAttribute purposeAttr = imageable.CreatePurposeAttr();
        purposeAttr.Set(UsdGeomTokens->render);
#endif
    }

    // Generate the name of the new mesh.
    SdfPath path = parent.GetPath();
    path = path.AppendChild(TfToken(path.GetName() + "ProxyShape"));
    // Create the new mesh.
    UsdGeomMesh mesh = UsdGeomMesh::Define(sceneStage, path);

    UsdAttribute faceVertexCountsAttr = mesh.CreateFaceVertexCountsAttr();
    UsdAttribute faceVertexIndicesAttr = mesh.CreateFaceVertexIndicesAttr();
    UsdAttribute pointsAttr = mesh.CreatePointsAttr();
    UsdAttribute purposeAttr = mesh.CreatePurposeAttr();
    UsdAttribute doubleSidedAttr = mesh.CreateDoubleSidedAttr();

    faceVertexCountsAttr.Set(newObjectCounts);
    faceVertexIndicesAttr.Set(newObjectIndices);
    pointsAttr.Set(newObjectPoints);
    purposeAttr.Set(UsdGeomTokens->proxy);
    doubleSidedAttr.Set(true);
}

bool freezeUSDTransforms(const WalterMaya::ShapeNode* userNode)
{
    RDOPROFILE("");

    UsdStageRefPtr sceneStage = getStage(userNode);
    if (!sceneStage)
    {
        // It happens when opening a Maya scene. CachedGeometry is not created
        // yet. But it's OK because we will use this layer when creating
        // CacheFileEntry.
        return false;
    }

    MObject node = userNode->thisMObject();

    // Get all the objects with connected transforms and their parents.
    SdfPathSet allPaths;
    const MPlug transformsPlug(node, ShapeNode::aTransforms);
    assert(!transformsPlug.isNull());
    for (int i = 0, n = transformsPlug.numElements(); i < n; i++)
    {
        const MPlug element = transformsPlug.elementByPhysicalIndex(i);
        const MPlug objectPlug = element.child(ShapeNode::aTransformsObject);
        const MPlug matrixPlug = element.child(ShapeNode::aTransformsMatrix);

        // Skip if there is no connection and if there is no name.
        const MString currentSubObjectName = objectPlug.asString();
        if (currentSubObjectName.length() == 0 || !matrixPlug.isConnected())
        {
            continue;
        }

        // Get and save all the parents.
        SdfPath path(currentSubObjectName.asChar());
        while (path != SdfPath::AbsoluteRootPath() &&
               allPaths.find(path) == allPaths.end())
        {
            allPaths.insert(path);
            path = path.GetParentPath();
        }
    }

    // Looking for the proxy layer.
    SdfLayerRefPtr renderProxyLayer =
        WalterUSDCommonUtils::getAnonymousLayer(sceneStage, "renderProxyLayer");
    renderProxyLayer->Clear();

    // Put all the edits to this layer.
    sceneStage->SetEditTarget(renderProxyLayer);

    // Merge the children of saved objects.
    for (const SdfPath& path : allPaths)
    {
        UsdPrim prim = sceneStage->GetPrimAtPath(path);
        if (!prim)
        {
            // We are here because the stage doesn't have the requested object.
            continue;
        }

        std::vector<UsdPrim> children;

        for (const UsdPrim& c : prim.GetChildren())
        {
            if (allPaths.find(c.GetPath()) != allPaths.end())
            {
                // We don't need to merge the object if it's in the list because
                // we need to keep ability to set the transform of this object.
                continue;
            }

            children.push_back(c);
        }

        mergeUSDObject(sceneStage, prim, children);
    }

    return true;
}

bool unfreezeUSDTransforms(const WalterMaya::ShapeNode* userNode)
{
    RDOPROFILE("");

    UsdStageRefPtr sceneStage = getStage(userNode);
    if (!sceneStage)
    {
        // It happens when opening a Maya scene. CachedGeometry is not created
        // yet. But it's OK because we will use this layer when creating
        // CacheFileEntry.
        return false;
    }

    // When we get it we clear it.
    SdfLayerRefPtr renderProxyLayer =
        WalterUSDCommonUtils::getAnonymousLayer(sceneStage, "renderProxyLayer");
    renderProxyLayer->Clear();

    return true;
}

/**
 * @brief Generate the glsl shader and register it in rdoCustomResource. For now
 * it's just a simple template that depends on the shader name only.
 *
 * @param name The name of the shader
 * @param UDIMs All the UDIM textures
 */
void publishGLSLShader(
    std::string name,
    const std::map<int, std::string>& UDIMs)
{
    // We need this structure to keep the shaders because rdoCustomResource
    // keeps only pointers.
    static std::unordered_map<std::string, std::string> sGLSLShaders;

    std::string shader =
        "-- glslfx version 0.1\n"
        "#import $TOOLS/glf/shaders/simpleLighting.glslfx\n"
        "-- configuration\n"
        "{\n"
        "    \"techniques\": {\n"
        "        \"default\": {\n"
        "            \"surfaceShader\": {\n";
    shader += "                \"source\": [ \"" + name + ".Surface\" ]\n";
    shader +=
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n";
    shader += "-- glsl " + name + ".Surface\n";
    shader +=
        "// This shader is also used for selection. There are no lights in\n"
        "// selection mode, and the shader fails to compile if there is\n"
        "// lighting code.\n"
        "#ifndef NUM_LIGHTS\n"
        "#define NUM_LIGHTS 0\n"
        "#endif\n"
        "vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 "
        "patchCoord)\n"
        "{\n"
        "#if NUM_LIGHTS == 0\n"
        "    return color;\n"
        "#else\n"
        // TODO: For some unknown reason GLSL can have HD_HAS_uv but not
        // HdGet_uv. Investigate this.
        "#if defined(HD_HAS_uv)\n"
        "    vec2 uv = HdGet_uv();\n"
        "#else\n"
        "    vec2 uv = vec2(0.0, 0.0);\n"
        "#endif\n";

    if (UDIMs.empty())
    {
        // Pre-initialize
        shader += "    vec3 texture = HdGet_diffuse();\n";
    }
    else
    {
        // Pre-initialization is not necessary
        shader += "    vec3 texture;\n";
    }

    bool first = true;
    for (const auto& it : UDIMs)
    {
        std::string udimID;
        int udim = it.first - 1;

        if (udim > 0)
        {
            int u = udim % 10;
            int v = udim / 10 % 10;

            std::string conditionUV;
            conditionUV += "uv[0] >= " + std::to_string(u) + ".0 && ";
            conditionUV += "uv[0] < " + std::to_string(u + 1) + ".0 && ";
            conditionUV += "uv[1] >= " + std::to_string(v) + ".0 && ";
            conditionUV += "uv[1] < " + std::to_string(v + 1) + ".0";

            udimID = std::to_string(it.first);
            const std::string condition = first ? "if" : "else if";
            shader += "    " + condition + " (" + conditionUV + ")\n";

            first = false;
        }

        shader += "    {\n";
        shader += "        texture = HdGet_diffuse" + udimID + "();\n";
        shader += "    }\n";
    }

    shader +=
        "    return simpleLighting(color, Peye, Neye, vec4(texture, 1.0));\n"
        "#endif\n"
        "}\n";

    setResource(std::string(name + ".glslfx").c_str(), shader.c_str());
    sGLSLShaders[name] = shader;
}

/**
 * @brief Returns value of specified attribute of the Maya object.
 *
 * @param iSceneStage The USD stage. We don't use it, it's for overloading.
 * @param iShaderNode Thw walterOverride object.
 * @param iAttribute The name of the custom attribute.
 */
std::string getStringValue(
    const UsdStageRefPtr,
    const MObject& iShaderNode,
    const std::string& iAttribute)
{
    const MFnDependencyNode depNode(iShaderNode);

    MString attributeName = MString("walter_") + iAttribute.c_str();
    const MPlug plug = depNode.findPlug(attributeName);
    return plug.asString().asChar();
}

/**
 * @brief Returns value of specified attribute of the prim.
 *
 * @param iSceneStage The USD stage.
 * @param iPath The USD path to the walter override object in the stage.
 * @param iAttribute The name of the attribute.
 */
std::string getStringValue(
    const UsdStageRefPtr iSceneStage,
    const SdfPath iPath,
    const std::string& iAttribute)
{
    const UsdPrim prim = iSceneStage->GetPrimAtPath(iPath);
    if (prim.IsA<UsdShadeMaterial>())
    {
        for (const UsdAttribute attr : prim.GetAttributes())
        {
            const SdfValueTypeName attrTypeName = attr.GetTypeName();
            if (attrTypeName != SdfValueTypeNames->String)
            {
                continue;
            }

            const std::string attrName = attr.GetName();

            std::vector<std::string> splittedName;
            boost::split(splittedName, attrName, boost::is_any_of(":"));

            if (splittedName.back() != iAttribute)
            {
                continue;
            }

            std::string value;
            attr.Get(&value);
            return value;
        }
    }

    return {};
}

/**
 * @brief Returns value of specified attribute of the ShaderBoostVariant.
 *
 * @param iSceneStage The USD stage.
 * @param iAttributeBoostVariant The object to get value from.
 * @param iAttribute The name of the attribute.
 */
std::string getStringValue(
    const UsdStageRefPtr iSceneStage,
    const ShaderBoostVariant* iAttributeBoostVariant,
    const std::string& iAttribute)
{
    if (!iAttributeBoostVariant)
    {
        return {};
    }

    if (iAttributeBoostVariant->which() == 0)
    {
        // It's a Maya shader.
        const MObject& shaderNode =
            boost::get<MObject>(*iAttributeBoostVariant);

        return getStringValue(iSceneStage, shaderNode, iAttribute);
    }
    else if (iAttributeBoostVariant->which() == 1)
    {
        // It's a shader in the USD stage.
        const SdfPath& attributesPath =
            boost::get<SdfPath>(*iAttributeBoostVariant);

        return getStringValue(iSceneStage, attributesPath, iAttribute);
    }
}

/**
 * @brief Parse the given file path and resolve some tokens. It returns list of
 * udim files or one sinle file.
 *
 * @param filePath The given path.
 * @param iSceneStage The USD stage.
 * @param iAttributeBoostVariant The walterOverride that applied to the current
 * object.
 * @param ioTokens The list of the attribute tokens used in the sahder. If this
 * kind of the shader is being processed the first time, the list is empty and
 * it's necessary to fill it. If this kind of the shader is being processed
 * second time, the list is pre-filled with correct values.
 * @param oUDIMs UDIM id to filenmae map. If the given file is not UDIM, then
 * the map will have only one file with index 0.
 */
void parseTextureFilePath(
    std::string filePath,
    const UsdStageRefPtr iSceneStage,
    const ShaderBoostVariant* iAttributeBoostVariant,
    std::map<std::string, std::string>& ioTokens,
    std::map<int, std::string>& oUDIMs,
    bool logErrorIfTokenNotFound)
{
    // Preloaded constants
    static const std::string udim("udim");
    static const std::string attr("attr:");

    bool needToRecomputeTokens = ioTokens.empty();

    // Get all the tokens. It's a set because the token can be specified twice.
    std::unordered_set<std::string> tokens;
    size_t foundOpen = 0;
    while (true)
    {
        foundOpen = filePath.find('<', foundOpen);
        if (foundOpen == std::string::npos)
        {
            break;
        }

        size_t foundClose = filePath.find('>', foundOpen);
        if (foundClose == std::string::npos)
        {
            break;
        }

        tokens.insert(
            filePath.substr(foundOpen + 1, foundClose - foundOpen - 1));

        foundOpen = foundClose;
    }

    // This will be true if the given file has UDIMs.
    bool needUDIMs = false;

    // Resolve the tokens.
    std::unordered_map<std::string, const std::string> values;
    for (const std::string& token : tokens)
    {
        if (boost::starts_with(token, attr))
        {
            std::string attributeName = token.substr(attr.length());

            if (needToRecomputeTokens)
            {
                std::string value = getStringValue(
                    iSceneStage, iAttributeBoostVariant, attributeName);

                // Save it for future.
                ioTokens.emplace(attributeName, value);

                if (!value.empty())
                {
                    values.emplace("<" + token + ">", value);
                }
                else if( logErrorIfTokenNotFound )
                {
                    TF_CODING_ERROR(
                        "Can't get attr token '%s' in the texture '%s'.",
                        token.c_str(),
                        filePath.c_str());
                }
            }
            else
            {
                auto it = ioTokens.find(attributeName);
                if (it != ioTokens.end())
                {
                    values.emplace("<" + token + ">", it->second);
                }
                else
                {
                    TF_CODING_ERROR(
                        "Can't find attr token '%s' in the texture '%s'.",
                        token.c_str(),
                        filePath.c_str());
                }
            }
        }
        else if (boost::algorithm::to_lower_copy(token) == udim)
        {
            // We have UDIMs
            needUDIMs = true;
        }
        else
        {
            TF_CODING_ERROR(
                "Can't resolve token '%s' in the texture.", token.c_str());
        }
    }

    for (const auto& it : values)
    {
        boost::replace_all(filePath, it.first, it.second);
    }

    if (!needUDIMs)
    {
        // We are done. No UDIMs.
        oUDIMs[0] = filePath;
        return;
    }

    // Get all the UDIMs with scanning the filesystem. It can be slow.

    // Regex file filter. test<udim>.exr -> test[0-9][0-9][0-9][0-9].exr
    const boost::regex filter(boost::ireplace_all_copy(
        filePath, "<" + udim + ">", "[0-9][0-9][0-9][0-9]"));

    // Get the position of the <udim> token to generate the UDIM ID from the
    // filename.
    foundOpen =
        boost::algorithm::ifind_first(filePath, "<" + udim + ">").begin() -
        filePath.begin();

    // Directory to look
    const boost::filesystem::path directory =
        boost::filesystem::path(filePath).parent_path();
    if (!boost::filesystem::exists(directory))
    {
        // No directory. No need to scan it.
        return;
    }

    // TODO: limit number of textures, or add a warning message if the number of
    // textures is more than glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max)

    // Default ctor yields past-the-end
    const boost::filesystem::directory_iterator end_itr;
    for (boost::filesystem::directory_iterator i(directory); i != end_itr; ++i)
    {
        // Skip if not a file
        if (!boost::filesystem::is_regular_file(i->status()))
        {
            continue;
        }

        const std::string fileName = i->path().native();

        // Skip if it's not UDIM
        if (!boost::regex_match(fileName, filter))
        {
            continue;
        }

        oUDIMs[std::stoi(fileName.substr(foundOpen, 4))] = fileName;
    }
}

/**
 * @brief Create the USD shading network for the viewport and GLSL shader, so it
 * can be rendered with Hydra.
 *
 * @param iShaderObj MObject that reproduces the maya shader or UsdShadeShader
 * that reproduces USD shader.
 * @param iAttributeBoostVariant The walterOverride that applied to the current
 * object.
 * @param iSceneStage The stage where the shader should be saved.
 * @param iDagNode The current walterStandin node.
 * @param ioTokens The list of the attribute tokens used in the sahder. If this
 * kind of the shader is being processed the first time, the list is empty and
 * it's necessary to fill it. If this kind of the shader is being processed
 * second time, the list is pre-filled with correct values.
 *
 * @return The created material.
 */
template <class T>
UsdShadeMaterial produceGLSLShader(
    const T& iShaderObj,
    const ShaderBoostVariant* iAttributeBoostVariant,
    const std::string& iShaderName,
    UsdStageRefPtr iSceneStage,
    const MFnDagNode& iDagNode,
    std::map<std::string, std::string>& ioTokens,
    bool logErrorIfTokenNotFound)
{
    RDOPROFILE("");

    // UUID will be added to the name of the shader. Hydra caches everything by
    // name, and at this point, we would like to be sure that Hydra really
    // recompiled this shader. Adding UUID to the end of the name is not the
    // best way. It's better to find a way to force the Hydra refresh shader.
    // But it works well at the moment.
    static boost::uuids::random_generator generator;
    std::string uuid = boost::uuids::to_string(generator());
    std::replace(uuid.begin(), uuid.end(), '-', '_');

    // Root path for all the materials.
    static const SdfPath viewportPath("/materials/viewport");
    // Path to the viewport material
    const std::string shaderName = iShaderName + "_" + uuid;
    const SdfPath viewportMaterialPath =
        viewportPath.AppendChild(TfToken(shaderName));

    // Get the shader info.
    std::string textureFilePath;
    GfVec3f color(1.0f, 1.0f, 1.0f);
    walterShaderUtils::getTextureAndColor(iShaderObj, textureFilePath, color);

    std::map<int, std::string> UDIMs;
    if (!textureFilePath.empty())
    {
        parseTextureFilePath(
            textureFilePath,
            iSceneStage,
            iAttributeBoostVariant,
            ioTokens,
            UDIMs,
            logErrorIfTokenNotFound);
    }

    publishGLSLShader(shaderName, UDIMs);

    // Hydra stuff. We need to create a shading network like this:
    // def Material "siSurface" {
    //     rel displayLook:bxdf = </siSurface/Shader>
    // }
    UsdShadeMaterial material =
        UsdShadeMaterial::Define(iSceneStage, viewportMaterialPath);
    WalterHdStreamLookAPI look(material);

    // def Shader "TestShader" {
    //     uniform asset info:filename = @shader.glslfx@
    //
    //     float3 diffuse = (1.0, 1.0, 1.0)
    //     rel connectedSourceFor:diffuse = </Test/Texture.outputs:color>
    //
    //     float2 uv = (0.0, 0.0)
    //     rel connectedSourceFor:uv = </Test/Primvar.outputs:result>
    // }
    SdfPath shaderPath = viewportMaterialPath.AppendChild(TfToken("Shader"));
    WalterHdStreamShader shader(
        UsdShadeShader::Define(iSceneStage, shaderPath).GetPrim());
    shader.CreateFilenameAttr().Set(SdfAssetPath(shaderName + ".glslfx"));

    UsdAttribute uv = shader.GetPrim().CreateAttribute(
        TfToken("uv"), SdfValueTypeNames->Float2, false, SdfVariabilityVarying);
    uv.Set(GfVec2f(0.0f, 0.0f));

    if (UDIMs.empty())
    {
        // Just a diffuse parameter
        UsdAttribute diffuse = shader.GetPrim().CreateAttribute(
            TfToken("diffuse"),
            SdfValueTypeNames->Float3,
            false,
            SdfVariabilityVarying);
        diffuse.Set(color);
    }
    else
    {
        // Create a node with primvar.
        // def Shader "Primvar" {
        //     uniform token info:id = "HwPrimvar_1"
        //     uniform token info:varname = "uv"
        //     float2 outputs:result
        // }
        SdfPath primvarPath =
            viewportMaterialPath.AppendChild(TfToken("Primvar"));
        WalterHdStreamPrimvar primvar(
            UsdShadeShader::Define(iSceneStage, primvarPath).GetPrim());
        primvar.CreateIdAttr().Set(UsdHydraTokens->HwPrimvar_1);
        primvar.CreateVarnameAttr().Set(TfToken("uv"));
        UsdShadeOutput primvarOutput(primvar.GetPrim().CreateAttribute(
            TfToken(UsdShadeTokens->outputs.GetString() + "result"),
            SdfValueTypeNames->Float2,
            false,
            SdfVariabilityVarying));

        // Connect Primvar.outputs:result -> Shader.uv
        UsdShadeInput(uv).ConnectToSource(primvarOutput);

        // Textures
        for (const auto& it : UDIMs)
        {
            std::string udimID;
            if (it.first > 0)
            {
                udimID = std::to_string(it.first);
            }

            // Convert file.
            // TODO: multithreaded.
            const std::string textureFilePath =
                walterShaderUtils::cacheTexture(it.second);

            UsdAttribute diffuse = shader.GetPrim().CreateAttribute(
                TfToken("diffuse" + udimID),
                SdfValueTypeNames->Float3,
                false,
                SdfVariabilityVarying);
            diffuse.Set(color);

            // Create a node with texture.
            // def Shader "Texture1001" {
            //     float2 uv
            //     rel connectedSourceFor:uv = </Test/UvPrimvar.outputs:result>
            //     uniform asset info:filename = @lenna.png@
            //     uniform token info:id = "HwUvTexture_1"
            //     color3f outputs:color
            //     uniform float textureMemory = 10000000
            // }
            SdfPath texturePath =
                viewportMaterialPath.AppendChild(TfToken("Texture" + udimID));
            WalterHdStreamUvTexture texture(
                UsdShadeShader::Define(iSceneStage, texturePath).GetPrim());
            texture.CreateIdAttr().Set(UsdHydraTokens->HwUvTexture_1);
            texture.CreateTextureMemoryAttr().Set(0.01f);
            texture.CreateFilenameAttr().Set(SdfAssetPath(textureFilePath));
            UsdShadeOutput textureOutput(texture.GetPrim().CreateAttribute(
                TfToken(UsdShadeTokens->outputs.GetString() + "color"),
                SdfValueTypeNames->Float3,
                false,
                SdfVariabilityVarying));

            // Connect Texture.outputs:color -> Shader.diffuse
            UsdShadeInput(diffuse).ConnectToSource(textureOutput);

            // Connect Primvar.outputs:result -> Texture.uv
            UsdShadeInput(texture.CreateUvAttr())
                .ConnectToSource(primvarOutput);
        }
    }

    // rel displayLook:bxdf = </siSurface/Shader>
    look.CreateBxdfRel().SetTargets({shaderPath});

    return material;
}

/**
 * @brief Fills oAssignments with all the recognized assignments of the stage.
 *
 * @param sceneStage The USD stage to read the material assignments.
 * @param type "shader", "displacement" or "attribute".
 * @param oAssignments Sub-node name (or expression) to surface shader path map.
 */
void getUSDAssignments(
    UsdStageRefPtr sceneStage,
    const std::string& type,
    std::map<WalterCommon::Expression, ShaderBoostVariant>& oAssignments)
{
    for (const UsdPrim& prim : sceneStage->Traverse())
    {
        if (!prim.IsA<WalterExpression>())
        {
            // WalterExpression is the only way to connect materials at the
            // moment.
            continue;
        }

        // Get expressions.
        WalterExpression expression(prim);
        std::string expressionText = expression.GetExpression();
        // {"defaultRenderLayer": {"shader": materialPath}}
        WalterExpression::AssignmentLayers layers = expression.GetLayers();

        auto layerIt = layers.find("defaultRenderLayer");
        if (layerIt == layers.end())
        {
            continue;
        }

        auto targetIt = layerIt->second.find(type);
        if (targetIt == layerIt->second.end())
        {
            continue;
        }

        // The expression in USD is mangled.
        // TODO: see RendererIndex::insertExpression to make it correctly.
        expressionText= WalterCommon::demangleString(
            WalterCommon::convertRegex(expressionText));

        const SdfPath& materialPath = targetIt->second;

        if (type == "attribute")
        {
            // If we are looking for an attribute, we are done. Otherwise we
            // need to get a shader from the material.
            oAssignments.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(expressionText),
                std::forward_as_tuple(materialPath));
            continue;
        }

        UsdPrim materialPrim = sceneStage->GetPrimAtPath(materialPath);
        if (!materialPrim || !materialPrim.IsA<UsdShadeMaterial>())
        {
            continue;
        }

        // Iterate the connections.
        for (const UsdAttribute& attr : materialPrim.GetAttributes())
        {
            // The name is "arnold:surface"
            const std::string name = attr.GetName();

            std::vector<std::string> splitted;
            boost::split(splitted, name, boost::is_any_of(":"));

            if (splitted.size() < 2)
            {
                continue;
            }

            // Extract the render and the target.
            std::string render = splitted[0];
            std::string target = splitted[1];

            // TODO: other renders?
            if (render != "arnold" || target != "surface")
            {
                continue;
            }

            // Get the connection.
            UsdShadeShader connectedShader =
                walterShaderUtils::getConnectedScheme<UsdShadeShader>(attr);
            if (!connectedShader)
            {
                // No connection.
                continue;
            }

            oAssignments.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(expressionText),
                std::forward_as_tuple(connectedShader.GetPrim().GetPath()));

            break;
        }
    }
}

/**
 * @brief Fills oAssignments with all the recognized assignments of both
 * walterStandin Maya node and USD Stage.
 *
 * @param iObject The walterStandin object to read the assignments.
 * @param iSceneStage The USD stage to read the assignments.
 * @param iType "attribute", "displacement", "shader"
 * @param oAssignments Sub-node name (or expression) to ShaderBoostVariant map.
 * ShaderBoostVariant can have eather SdfPath or MObject.
 */
void getAssignments(
    const MObject& iObject,
    UsdStageRefPtr iSceneStage,
    const std::string& iType,
    std::map<WalterCommon::Expression, ShaderBoostVariant>& oAssignments)
{
    // Sub object (expression) to shader map. Contains all the expressions from
    // both Maya and USD stage.
    assert(oAssignments.empty());

    // Sub object to Maya shader map. All the shader connections from Maya.
    // Strong should be first because we use emplace everywhere.
    getShaderAssignments(iObject, iType, oAssignments);

    // Sub object (expression) to surface shader path map. All the expressions
    // from the USD stage.
    getUSDAssignments(iSceneStage, iType, oAssignments);
}

/**
 * @brief Queries ShaderBoostVariant (which is eather SdfPath or MObject) and
 * returns a map with requested attrTokens and values.
 *
 * @param iSceneStage The USD stage to read the assignments.
 * @param iAttrTokens The list of attr tokens to query.
 * @param iAttributeBoostVariant The object to get values from.
 *
 * @return The map with requested attrTokens and values.
 */
std::map<std::string, std::string> queryAttrTokens(
    const UsdStageRefPtr iSceneStage,
    const std::unordered_set<std::string>& iAttrTokens,
    const ShaderBoostVariant* iAttributeBoostVariant)
{
    std::map<std::string, std::string> map;

    for (const std::string& attrToken : iAttrTokens)
    {
        map[attrToken] =
            getStringValue(iSceneStage, iAttributeBoostVariant, attrToken);
    }

    return map;
}

// The description is in the header.
void processGLSLShaders(
    const ShapeNode* userNode, 
    bool logErrorIfTokenNotFound)
{
    RDOPROFILE("");

    UsdStageRefPtr sceneStage = getStage(userNode);
    if (!sceneStage)
    {
        // It happens when opening a Maya scene.
        return;
    }

    // Maya node
    MObject obj = userNode->thisMObject();
    MFnDagNode dagNode(obj);

    // Looking for the proxy layer.
    SdfLayerRefPtr viewportShaderLayer =
        WalterUSDCommonUtils::getAnonymousLayer(
            sceneStage, "viewportShaderLayer");
    // We need to recreate it to upate Hydra.
    // TODO: it's super ugly, we need another way to update Hydra.
    viewportShaderLayer->Clear();

    // Put all the edits to this layer.
    sceneStage->SetEditTarget(viewportShaderLayer);

    // Sub object (expression) to shader map. Contains all the expressions from
    // both Maya and USD stage.
    std::map<WalterCommon::Expression, ShaderBoostVariant> shaderAssignments;
    getAssignments(obj, sceneStage, "shader", shaderAssignments);
    std::map<WalterCommon::Expression, ShaderBoostVariant> attributeAssignments;
    getAssignments(obj, sceneStage, "attribute", attributeAssignments);

    // Sub oblect to shader hash map. All the shader connections of USD. We use
    // it as a cache to determine if the shader is already applied to the
    // object.
    std::unordered_map<std::string, size_t> assignmentsCache;
    // Shader name to the list of the attribute tokens used in the sahder. We
    // fill it when we output the shader the first time.
    std::unordered_map<std::string, std::unordered_set<std::string>> tokenCache;
    // Shader hash to material map.
    std::map<size_t, UsdShadeMaterial> materialCache;

    // We keep saved queries of walterOverride to avoid query the same requests.
    // size_t is a hash of the walterOverride name and the shader name. We need
    // ordered map in a second template to have the same hash.
    std::map<size_t, std::map<std::string, std::string>> queriedTokensCache;

    for (const UsdPrim& prim : sceneStage->Traverse())
    {
        if (!prim.IsA<UsdGeomImageable>())
        {
            continue;
        }

        SdfPath path = prim.GetPath();
        std::string pathStr = path.GetString();

        // Resolve the expression and get the current material. Reversed because
        // the procedural historically sorts and reverses all the expressions.
        // Since assignments is std::map, it's already sorted.
        // boost::adaptors::reverse wraps an existing range to provide a new
        // range whose iterators behave as if they were wrapped in
        // reverse_iterator.
        const ShaderBoostVariant* shaderBoostVariant =
            WalterCommon::resolveAssignment<ShaderBoostVariant>(
                pathStr, shaderAssignments);
        if (!shaderBoostVariant)
        {
            // If there is no shader, nothing to do.
            continue;
        }

        const ShaderBoostVariant* attributeBoostVariant =
            WalterCommon::resolveAssignment<ShaderBoostVariant>(
                pathStr, attributeAssignments);

        std::string shaderName = getBoostVariantName(*shaderBoostVariant);
        if (shaderName.empty())
        {
            continue;
        }

        // Trying to find the list of attribute tokens for the shader name.
        auto attrTokenIt = tokenCache.find(shaderName);
        // True if the list with attribute tokens existed. It means we already
        // output the shader at least once. And we can compute the full hash
        // without outputting it again.
        bool attrTokenExisted = attrTokenIt != tokenCache.end();
        // Get existed attribute tokens or create a new one.
        std::unordered_set<std::string>& attrTokens =
            attrTokenExisted ? attrTokenIt->second : tokenCache[shaderName];

        // Get hash from name.
        size_t shaderHash = std::hash<std::string>{}(shaderName);
        // Hash of name and all the resolved tokens. It's not complete here.
        // It's necessary to combine it with tokens before using.
        size_t shaderAndAttrTokensHash = shaderHash;

        // Hash from walterOverride object name.
        size_t walterOverrideObjectHash;
        if (attributeBoostVariant)
        {
            walterOverrideObjectHash =
                boost::hash<const ShaderBoostVariant*>{}(attributeBoostVariant);
        }
        else
        {
            walterOverrideObjectHash = 0;
        }

        // We need to combine it with shader because we need to keep unique
        // requests and different shaders need different tokens.
        boost::hash_combine(walterOverrideObjectHash, shaderHash);

        // We don't query walterOverride if we did it before.
        auto queriedTokensIt =
            queriedTokensCache.find(walterOverrideObjectHash);
        if (queriedTokensIt == queriedTokensCache.end())
        {
            // If there are no tokens, queryAttrTokens will return an empty map.
            auto result = queriedTokensCache.emplace(
                walterOverrideObjectHash,
                queryAttrTokens(sceneStage, attrTokens, attributeBoostVariant));
            queriedTokensIt = result.first;
        }

        // We need ordered map to have the same hash.
        std::map<std::string, std::string>& tokens = queriedTokensIt->second;

        // Compute hash of the requested attribute tokens for current material
        // and walterOverride. If there are no tokens, the hash should be 0.
        size_t tokenHash = boost::hash_range(tokens.begin(), tokens.end());

        UsdShadeMaterial material;

        if (attrTokenExisted)
        {
            // We are here because we already produced this shader. So we need
            // to decide if we need another one. We would need another one if
            // the tokens of the shader is now different.
            boost::hash_combine(shaderAndAttrTokensHash, tokenHash);

            // Save that the current object has this shader. We will skip the
            // children material if she shader will be the same.
            assignmentsCache[pathStr] = shaderAndAttrTokensHash;

            // Check if the parent has the same shader with the same tokens.
            auto it = assignmentsCache.find(path.GetParentPath().GetString());
            if (it != assignmentsCache.end() &&
                it->second == shaderAndAttrTokensHash)
            {
                // Skip if the parent has the same shader assigned.
                continue;
            }

            // Check if we already have the same shader with the same tokens.
            auto matIt = materialCache.find(shaderAndAttrTokensHash);
            if (matIt != materialCache.end())
            {
                material = matIt->second;
            }
        }

        if (!material)
        {
            // We are here because we never produce this shder or if we produced
            // it with different tokens.

            // It's not switch/case because of break and continue in a loop.
            if (shaderBoostVariant->which() == 0)
            {
                // It's a Maya shader.
                const MObject& shaderNode =
                    boost::get<MObject>(*shaderBoostVariant);
                if (shaderNode.isNull())
                {
                    continue;
                }

                material = produceGLSLShader(
                    shaderNode,
                    attributeBoostVariant,
                    shaderName,
                    sceneStage,
                    dagNode,
                    tokens,
                    logErrorIfTokenNotFound);
            }
            else if (shaderBoostVariant->which() == 1)
            {
                // It's a shader in the USD stage.
                const SdfPath shaderPath =
                    boost::get<SdfPath>(*shaderBoostVariant);
                if (shaderPath.IsEmpty())
                {
                    continue;
                }

                UsdShadeShader shader =
                    UsdShadeShader::Get(sceneStage, shaderPath);
                // We checked it before
                assert(shader);

                material = produceGLSLShader(
                    shader,
                    attributeBoostVariant,
                    shaderName,
                    sceneStage,
                    dagNode,
                    tokens,
                    logErrorIfTokenNotFound);
            }

            if (!material)
            {
                // No material. There is nothing to bind.
                continue;
            }

            if (!attrTokenExisted)
            {
                // We are here because we never produced this shader before. But
                // we just did it right now and we have all the data to cache
                // it.

                // Save attrTokens for future.
                for (const auto& a : tokens)
                {
                    attrTokens.insert(a.first);
                }

                // Re-compute hash of the requested attribute tokens.
                tokenHash = boost::hash_range(tokens.begin(), tokens.end());

                // Combine it with the name. So we now have the full shader
                // hash.
                boost::hash_combine(shaderAndAttrTokensHash, tokenHash);

                // Save that the current object has this shader. We will skip
                // the children material if she shader will be the same.
                assignmentsCache[pathStr] = shaderAndAttrTokensHash;
            }
        }

        // Save material for future.
        materialCache[shaderAndAttrTokensHash] = material;

        // Connect the newly created material to the current object.
        material.Bind(const_cast<UsdPrim&>(prim));
    }
}

// The description is in the header.
void muteGLSLShaders(const ShapeNode* userNode, bool iSetVisible)
{
    RDOPROFILE("");

    UsdStageRefPtr sceneStage = getStage(userNode);
    if (!sceneStage)
    {
        // It happens when opening a Maya scene.
        return;
    }

    // Maya node
    MObject obj = userNode->thisMObject();
    MFnDagNode dagNode(obj);

    // Looking for the proxy layer.
    SdfLayerRefPtr viewportShaderLayer =
        WalterUSDCommonUtils::getAnonymousLayer(
            sceneStage, "viewportShaderLayer");

    // USD destroys anonymous layer when calling UsdStage::MuteLayer. To avoid
    // it, it's necessary to keep it somethere. We use vector because if we mute
    // the layer twice and unmute once, we still need to protect this layer from
    // destroying.
    static std::vector<SdfLayerRefPtr> sMutedLayersCache;

    if (iSetVisible)
    {
        sceneStage->UnmuteLayer(viewportShaderLayer->GetIdentifier());

        auto it = std::find(
            sMutedLayersCache.begin(),
            sMutedLayersCache.end(),
            viewportShaderLayer);
        if (it != sMutedLayersCache.end())
        {
            sMutedLayersCache.erase(it);
        }
    }
    else
    {
        sceneStage->MuteLayer(viewportShaderLayer->GetIdentifier());

        sMutedLayersCache.push_back(viewportShaderLayer);
    }
}

MString getLayerAsString(
    const WalterMaya::ShapeNode* userNode,
    const MString& layerName)
{
    UsdStageRefPtr sceneStage = getStage(userNode);
    if (!sceneStage)
    {
        // It happens when opening a Maya scene.
        return {};
    }

    std::string layerStr;

    if (layerName.length() == 0)
    {
        sceneStage->Flatten()->ExportToString(&layerStr);
    }
    else if (layerName == "session")
    {
        sceneStage->GetSessionLayer()->ExportToString(&layerStr);
    }
    else
    {
        SdfLayerRefPtr requestedLayer = WalterUSDCommonUtils::getAnonymousLayer(
            sceneStage, layerName.asChar());
        requestedLayer->ExportToString(&layerStr);
    }

    return layerStr.c_str();
}

// The description is in the header.
std::string getMayaStateAsUSDLayer(MObject obj)
{
    // Regular expression looks like this:
    // def Expression "_pSphere1_pSphereShape1"
    // {
    //     rel assign:defaultRenderLayer:shader = </materials/aiStandard1>
    //     string expression = "\\pSphere1\\pSphereShape1"
    // }

    // Trying to get the USD stage from the Maya object.
    UsdStageRefPtr stage;

    // Maya node
    MFnDagNode dagNode(obj);

    // If the stage is not available, just create a new one.
    stage = UsdStage::CreateInMemory();

    const MPlug layersAssignation = dagNode.findPlug("layersAssignation");
    if (layersAssignation.isNull())
    {
        MGlobal::displayWarning(
            "[Walter USD Utils] Can't find " + dagNode.name() +
            ".layersAssignation.");
        return {};
    }

    for (unsigned int i = 0, n = layersAssignation.numElements(); i < n; i++)
    {
        // Get walterStandin.layersAssignation[i]
        const MPlug currentLayerCompound =
            layersAssignation.elementByPhysicalIndex(i);
        // Get walterStandin.layersAssignation[i].layer
        MPlug layerPlug;
        if (!GetChildMPlug(currentLayerCompound, "layer", layerPlug))
        {
            continue;
        }

        MPlugArray connections;
        if (!layerPlug.connectedTo(connections, true, false) ||
            !connections.length())
        {
            continue;
        }

        // The layer is the first connected node. We consider we have only one
        // connection.
        const MFnDependencyNode layer(connections[0].node());
        const std::string layerName = layer.name().asChar();

        // Get walterStandin.layersAssignation[i].shaderConnections
        MPlug shadersPlug;
        if (!GetChildMPlug(
                currentLayerCompound, "shaderConnections", shadersPlug))
        {
            continue;
        }

        for (unsigned j = 0, m = shadersPlug.numElements(); j < m; j++)
        {
            // Get walterStandin.layersAssignation[i].shaderConnections[j]
            const MPlug shadersCompound = shadersPlug.elementByPhysicalIndex(j);
            // Get walterStandin.layersAssignation[].shaderConnections[].abcnode
            MPlug abcnodePlug;
            if (!GetChildMPlug(shadersCompound, "abcnode", abcnodePlug))
            {
                continue;
            }

            // It's an expression. "/pSphere1/pTorus1/pTorusShape1" or "/*".
            const MString abcnode = abcnodePlug.asString().asChar();
            if (!abcnode.length())
            {
                continue;
            }

            // Get walterStandin.layersAssignation[].shaderConnections[].shader
            MPlug shaderPlug;
            if (GetChildMPlug(shadersCompound, "shader", shaderPlug))
            {
                // Put it to USD.
                saveConnection(
                    stage, abcnode.asChar(), layerName, "shader", shaderPlug);
            }

            // The same for displacement
            MPlug dispPlug;
            if (GetChildMPlug(shadersCompound, "displacement", dispPlug))
            {
                // Put it to USD.
                saveConnection(
                    stage,
                    abcnode.asChar(),
                    layerName,
                    "displacement",
                    dispPlug);
            }

            // The same for attributes
            MPlug attrPlug;
            if (GetChildMPlug(shadersCompound, "attribute", attrPlug))
            {
                // Put it to USD.
                saveConnection(
                    stage, abcnode.asChar(), layerName, "attribute", attrPlug);
            }
        }
    }

    // Compose everything together and get the string.
    std::string session;
    stage->Flatten()->ExportToString(&session);

    return session;
}

bool extractAssignments(ShapeNode* iUserNode)
{
    assert(iUserNode);

    ShapeNode::ExpMap& assignments = iUserNode->getAssignments();
    ShapeNode::ExpGroup& groups = iUserNode->getGroups();
    MStringArray& embeddedShaders = iUserNode->getEmbeddedShaders();
    ShapeNode::AssignedOverrides& assignedOverrides =
        iUserNode->getAssignedOverrides();

    assignments.clear();
    groups.clear();
    embeddedShaders.clear();
    assignedOverrides.clear();

    UsdStageRefPtr sceneStage = getStage(iUserNode);
    if (!sceneStage)
    {
        // It happens when opening a Maya scene.
        return false;
    }

    // Get all the expressions.
    for (const UsdPrim& prim : sceneStage->Traverse())
    {
        if (prim.IsA<WalterExpression>())
        {
            // Get expressions.
            WalterExpression expression(prim);
            // USD uses \ as a separator. We use /. We need convert it.
            std::string expressionText = WalterCommon::demangleString(
                WalterCommon::convertRegex(expression.GetExpression()));
            WalterExpression::AssignmentLayers layers = expression.GetLayers();

            // Convert AssignmentLayers to CacheFileEntry::ExpMapPtr.
            // AssignmentLayers is data of this format:
            // {string "layer": {string "target": SdfPath "shader"}}
            for (auto layer : layers)
            {
                for (auto target : layer.second)
                {
                    ShapeNode::LayerKey layerkey(layer.first, target.first);
                    assignments[expressionText][layerkey] =
                        target.second.GetText();
                }
            }

            auto foundLayer = layers.find("defaultRenderLayer");
            if (foundLayer != layers.end())
            {
                SdfPath walterOverridePath = foundLayer->second["attribute"];

                if (!walterOverridePath.IsEmpty())
                {
                    assignedOverrides.emplace(
                        std::piecewise_construct,
                        std::forward_as_tuple(expressionText),
                        std::forward_as_tuple(walterOverridePath));
                }
            }

            std::string group;
            UsdAttribute groupAttr = expression.GetGroupAttr();
            if (groupAttr)
            {
                groupAttr.Get(&group);
            }

            // We need to set it even if the expression doesn't have a group so
            // we can get the list of expressions very fast.
            groups[expressionText] = group;
        }
        else if (prim.IsA<UsdShadeMaterial>())
        {
            // Material.
            embeddedShaders.append(prim.GetPath().GetText());
        }
    }

    return true;
}

void getHydraPlugins(MStringArray& result)
{
    HfPluginDescVector pluginDescriptors;
    HdxRendererPluginRegistry::GetInstance().GetPluginDescs(&pluginDescriptors);

    const PlugRegistry& registry = PlugRegistry::GetInstance();

    for (const HfPluginDesc& pluginDescriptor : pluginDescriptors)
    {
        result.append(pluginDescriptor.id.GetText());
        result.append(pluginDescriptor.displayName.c_str());
    }
}

bool isPseudoRoot(
    const ShapeNode* node,
    const MString& subNodeName)
{
    UsdStageRefPtr stage = getStage(node);
    if (!stage)
        return false;

    return WalterUSDCommonUtils::isPseudoRoot(
        stage, subNodeName.asChar());
}

bool isVisible(
    const ShapeNode* node,
    const MString& subNodeName)
{
    UsdStageRefPtr stage = getStage(node);
    if (!stage)
        return false;

    return WalterUSDCommonUtils::isVisible(
        stage, subNodeName.asChar());
}

bool toggleVisibility(
    const ShapeNode* node,
    const MString& subNodeName) 
{
    UsdStageRefPtr stage = getStage(node);
    if (!stage)
        return false;

    SdfLayerRefPtr visibilityLayer =
        WalterUSDCommonUtils::getAnonymousLayer(stage, "visibilityLayer");
    stage->SetEditTarget(visibilityLayer);

    return WalterUSDCommonUtils::toggleVisibility(
        stage, subNodeName.asChar());  
}

bool setVisibility(
    const ShapeNode* node,
    const MString& subNodeName,
    bool visible) 
{
    UsdStageRefPtr stage = getStage(node);
    if (!stage)
        return false;

    SdfLayerRefPtr visibilityLayer =
        WalterUSDCommonUtils::getAnonymousLayer(stage, "visibilityLayer");
    stage->SetEditTarget(visibilityLayer);

    return WalterUSDCommonUtils::setVisibility(
        stage, subNodeName.asChar(), visible);  
}

bool hideAllExceptedThis(
    const ShapeNode* node,
    const MString& subNodeName) 
{
    UsdStageRefPtr stage = getStage(node);
    if (!stage)
        return false;

    SdfLayerRefPtr visibilityLayer =
        WalterUSDCommonUtils::getAnonymousLayer(stage, "visibilityLayer");
    stage->SetEditTarget(visibilityLayer);

    return WalterUSDCommonUtils::hideAllExceptedThis(
        stage, subNodeName.asChar());  
}

MStringArray primPathsMatchingExpression(
    const ShapeNode* node,
    const MString& expression) 
{
    MStringArray res;

    UsdStageRefPtr stage = getStage(node);
    if (!stage)
        return res;

    std::vector<std::string> primPaths = 
        WalterUSDCommonUtils::primPathsMatchingExpression(
            stage, expression.asChar());  

    for(auto path: primPaths)
        res.append(path.c_str());

    return res;
}

MString setPurpose(
    ShapeNode* node,
    const MString& subNodeName,
    const MString& purpose) 
{
    UsdStageRefPtr stage = getStage(node);

    if (!stage)
        return "default";

    SdfLayerRefPtr purposeLayer =
        WalterUSDCommonUtils::getAnonymousLayer(stage, "purposeLayer");
    stage->SetEditTarget(purposeLayer);

    MString purposeName = WalterUSDCommonUtils::setPurpose(
        stage, subNodeName.asChar(), purpose.asChar()).c_str(); 

    // Re-construct the render-engine, Pixar Hydra bug
    node->refresh();

    return purposeName;
} 

MString getPurpose(
    const ShapeNode* node,
    const MString& subNodeName) 
{
    UsdStageRefPtr stage = getStage(node);
    if (!stage)
        return "default";

    return WalterUSDCommonUtils::purpose(
        stage, subNodeName.asChar()).c_str();
}

void setRenderPurpose(
    ShapeNode* node,
    const MStringArray& purposeArray) 
{
    node->mParams.showRender = false;
    node->mParams.showProxy = false;
    node->mParams.showGuides = false;

    for(uint32_t i=0; i<purposeArray.length(); i++) {
        if(purposeArray[i].asChar() == std::string("render"))
            node->mParams.showRender = true;

        if(purposeArray[i].asChar() == std::string("proxy"))
            node->mParams.showProxy = true;

        if(purposeArray[i].asChar() == std::string("guide"))
            node->mParams.showGuides = true;
    }

    // Re-construct the render-engine, Pixar Hydra bug
    node->refresh();
} 

MStringArray getRenderPurpose(
    const ShapeNode* node) 
{
    MStringArray purposeArray;
    if(node->mParams.showRender)
        purposeArray.append("render");

    if(node->mParams.showProxy)
        purposeArray.append("proxy");

    if(node->mParams.showGuides)
        purposeArray.append("guide");

    return purposeArray;
}

bool savePurposes(
    const MString& objectName,
    const MString& fileName) {

    // Looking for MFnDependencyNode object in the scene.
    MSelectionList selectionList;
    selectionList.add(objectName);

    MObject obj;
    selectionList.getDependNode(0, obj);

    MFnDependencyNode depNode(obj);
    ShapeNode* shapeNode = dynamic_cast<ShapeNode*>(depNode.userNode());
    UsdStageRefPtr stage = getStage(shapeNode);

    SdfLayerRefPtr purposeLayer =
        WalterUSDCommonUtils::getAnonymousLayer(stage, "purposeLayer");

    if (!purposeLayer->Export(fileName.asChar()))
    {
        MString msg;
        msg.format(
            "[walter] Can't write USD ^1s. See the terminal for the details.",
            fileName);

        MGlobal::displayError(msg);
        return false;
    }

    return true;
}

#endif  // MFB_ALT_PACKAGE_NAME
