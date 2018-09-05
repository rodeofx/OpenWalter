// Copyright 2017 Rodeo FX.  All rights reserved.

///////////////////////////////////////////////////////////////////////////////
//
// Walter MEL command
//
// Creates one or more cache files on disk to store attribute data for
// a span of frames.
//
////////////////////////////////////////////////////////////////////////////////

#include "walterCmd.h"

#include "PathUtil.h"
#include "walterShapeNode.h"
#include "walterAssignment.h"
#include "walterAttributes.h"
#include "walterThreadingUtils.h"
#include "walterUsdUtils.h"

#include <AbcExport.h>
#include <maya/MAnimControl.h>
#include <maya/MArgList.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnTransform.h>
#include <maya/MPlugArray.h>
#include <maya/MSyntax.h>
#include <maya/MFnStringData.h>
#include <maya/MFnDependencyNode.h>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_set.hpp>
#include <cfloat>
#include <fstream>
#include <iomanip>
#include <limits>
#include <list>
#include <map>
#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace WalterMaya
{
void* Command::creator()
{
    return new Command();
}

const char* Command::LOCATOR_ATTR_PRIM_PATH = "walterPrimPath";

MSyntax Command::cmdSyntax()
{
    MSyntax syntax;

    syntax.addFlag("-css", "-clearSubSelection", MSyntax::kString);
    syntax.addFlag(
        "-ass", "-addSubSelection", MSyntax::kString, MSyntax::kString);
    syntax.addFlag("-da", "-dirAlembic", MSyntax::kString, MSyntax::kString);
    syntax.addFlag("-pa", "-propsAlembic", MSyntax::kString, MSyntax::kString);
    
    syntax.addFlag(
        "-gva", 
        "-getVariants", 
        MSyntax::kString, 
        MSyntax::kString, 
        MSyntax::kBoolean);
    
    syntax.addFlag(
        "-sva",
        "-setVariant",
        MSyntax::kString,
        MSyntax::kString,
        MSyntax::kString,
        MSyntax::kString);

    syntax.addFlag("-v", "-version", MSyntax::kString, MSyntax::kString);
    syntax.addFlag("-s", "-saveAssignment", MSyntax::kString, MSyntax::kString);
    syntax.addFlag("-s", "-saveAttributes", MSyntax::kString, MSyntax::kString);
    syntax.addFlag(
        "-st", "-saveTransforms", MSyntax::kString, MSyntax::kString);
    syntax.addFlag("-st", "-startTime", MSyntax::kTime);
    syntax.addFlag("-et", "-endTime", MSyntax::kTime);
    syntax.addFlag("-smr", "-simulationRate", MSyntax::kTime);
    syntax.addFlag("-rsl", "-resetSessionLayer", MSyntax::kBoolean);
    syntax.addFlag(
        "-ao",
        "-assignmentObjects",
        MSyntax::kString,
        MSyntax::kString,
        MSyntax::kString);
    syntax.addFlag("-al", "-assignmentLayers", MSyntax::kString);
    syntax.addFlag(
        "-a",
        "-assignment",
        MSyntax::kString,
        MSyntax::kString,
        MSyntax::kString,
        MSyntax::kString);
    syntax.addFlag("-aag", "-alembicAllGroups", MSyntax::kString);

    syntax.addFlag("-ae", "-alembicExpressions", MSyntax::kString);

    syntax.addFlag(
        "-age", "-alembicGroupExpressions", MSyntax::kString, MSyntax::kString);

    syntax.addFlag(
        "-ext", "-exposeTransform", MSyntax::kString, MSyntax::kString);

    syntax.addFlag(
        "-det", "-detachTransform", MSyntax::kString, MSyntax::kString);
  
    syntax.addFlag("-cfd", "-setCachedFramesDirty", MSyntax::kString);

    syntax.addFlag("-e", "-export", MSyntax::kString);

    syntax.addFlag(
        "-gls", "-getLayerAsString", MSyntax::kString, MSyntax::kString);

    syntax.addFlag("-ja", "-joinAll");

    syntax.addFlag("-hp", "-hydraPlugins");

    syntax.addFlag("-j", "-join", MSyntax::kString);

    syntax.addFlag(
        "-cev",
        "-expressionIsMatching",
        MSyntax::kString,
        MSyntax::kString);

    syntax.addFlag(
        "-ipr",
        "-isPseudoRoot",
        MSyntax::kString,
        MSyntax::kString);

    syntax.addFlag(
        "-iv",
        "-isVisible",
        MSyntax::kString,
        MSyntax::kString);

    syntax.addFlag(
        "-sv",
        "-setVisibility",
        MSyntax::kString,
        MSyntax::kString,
        MSyntax::kBoolean);

    syntax.addFlag(
        "-tv",
        "-toggleVisibility",
        MSyntax::kString,
        MSyntax::kString);

    syntax.addFlag(
        "-hat",
        "-hideAllExceptedThis",
        MSyntax::kString,
        MSyntax::kString);

    syntax.addFlag(
        "-pme",
        "-primPathsMatchingExpression",
        MSyntax::kString,
        MSyntax::kString);

    syntax.addFlag(
        "-spp",
        "-setPurpose",
        MSyntax::kString,
        MSyntax::kString,
        MSyntax::kString);

    syntax.addFlag(
        "-gpp",
        "-getPurpose",
        MSyntax::kString,
        MSyntax::kString);

    syntax.addFlag(
        "-srp",
        "-setRenderPurpose",
        MSyntax::kString,
        MSyntax::kString);

    syntax.addFlag(
        "-grp",
        "-getRenderPurpose",
        MSyntax::kString);

    syntax.addFlag(
        "-sp",
        "-savePurposes",
        MSyntax::kString,
        MSyntax::kString);

    syntax.addFlag(
        "-svl",
        "-saveVariantsLayer",
        MSyntax::kString,
        MSyntax::kString);

    syntax.useSelectionAsDefault(true);
    syntax.setObjectType(MSyntax::kSelectionList, 0);

    syntax.enableQuery(true);
    syntax.enableEdit(true);

    return syntax;
}

Command::Command()
{}

Command::~Command()
{}

bool Command::isUndoable() const
{
    return false;
}

bool Command::hasSyntax() const
{
    return true;
}

MStatus Command::doIt(const MArgList& args)
{
    MStatus status;

    MArgDatabase argsDb(syntax(), args, &status);
    if (!status)
        return status;

    if (argsDb.isFlagSet("-clearSubSelection"))
    {
        MString objectName;

        argsDb.getFlagArgument("-clearSubSelection", 0, objectName);

        // Return. Don't execute anything.
        return clearSubSelection(objectName);
    }

    // Check if it's related to the sub-selection.
    if (argsDb.isFlagSet("-addSubSelection"))
    {
        MString objectName;
        MString subNodeName;

        argsDb.getFlagArgument("-addSubSelection", 0, objectName);
        argsDb.getFlagArgument("-addSubSelection", 1, subNodeName);

        // Return. Don't execute anything.
        return addSubSelection(objectName, subNodeName);
    }

    if (argsDb.isFlagSet("-dirAlembic"))
    {
        MString objectName;
        MString subNodeName;

        argsDb.getFlagArgument("-dirAlembic", 0, objectName);
        argsDb.getFlagArgument("-dirAlembic", 1, subNodeName);

        // Return. Don't execute anything else.
        return dirAlembic(objectName, subNodeName);
    }

    if (argsDb.isFlagSet("-propsAlembic"))
    {
        MString objectName;
        MString subNodeName;

        argsDb.getFlagArgument("-propsAlembic", 0, objectName);
        argsDb.getFlagArgument("-propsAlembic", 1, subNodeName);

        // Return. Don't execute anything else.
        return propsAlembic(objectName, subNodeName);
    }

    if (argsDb.isFlagSet("-getVariants"))
    {
        MString objectName;
        MString subNodeName;
        bool recursively;

        argsDb.getFlagArgument("-getVariants", 0, objectName);
        argsDb.getFlagArgument("-getVariants", 1, subNodeName);
        argsDb.getFlagArgument("-getVariants", 2, recursively);

        // Return. Don't execute anything else.
        return getVariants(objectName, subNodeName, recursively);
    }

    if (argsDb.isFlagSet("-setVariant"))
    {
        MString objectName;
        MString subNodeName;
        MString variantName;
        MString variantSetName;

        argsDb.getFlagArgument("-setVariant", 0, objectName);
        argsDb.getFlagArgument("-setVariant", 1, subNodeName);
        argsDb.getFlagArgument("-setVariant", 2, variantName);
        argsDb.getFlagArgument("-setVariant", 3, variantSetName);

        // Return. Don't execute anything else.
        return setVariant(
            objectName, subNodeName, variantName, variantSetName);
    }

    if (argsDb.isFlagSet("-saveAssignment"))
    {
        MString objectName;
        MString fileName;

        argsDb.getFlagArgument("-saveAssignment", 0, objectName);
        argsDb.getFlagArgument("-saveAssignment", 1, fileName);

        // Return. Don't execute anything else.
        if (saveAssignment(objectName, fileName))
        {
            return MS::kSuccess;
        }

        return MS::kFailure;
    }

    if (argsDb.isFlagSet("-saveAttributes"))
    {
        MString objectName;
        MString fileName;

        argsDb.getFlagArgument("-saveAttributes", 0, objectName);
        argsDb.getFlagArgument("-saveAttributes", 1, fileName);

        // Return. Don't execute anything else.
        if (saveAttributes(objectName, fileName))
        {
            return MS::kSuccess;
        }

        return MS::kFailure;
    }

    if (argsDb.isFlagSet("-saveTransforms"))
    {
        MString objectName;
        MString fileName;

        argsDb.getFlagArgument("-saveTransforms", 0, objectName);
        argsDb.getFlagArgument("-saveTransforms", 1, fileName);

        MTime start = MAnimControl::minTime();
        MTime end = MAnimControl::maxTime();
        MTime step = 1.0 / getCurrentFPS();
        if (argsDb.isFlagSet("-startTime"))
        {
            argsDb.getFlagArgument("-startTime", 0, start);
        }
     
        if (argsDb.isFlagSet("-endTime"))
        {
            argsDb.getFlagArgument("-endTime", 0, end);
        }
       
        if (argsDb.isFlagSet("-simulationRate"))
        {
            argsDb.getFlagArgument("-simulationRate", 0, step);
        }
  
        bool resetSessionLayer = true;
        if (argsDb.isFlagSet("-resetSessionLayer"))
        {
            argsDb.getFlagArgument("-resetSessionLayer", 0, resetSessionLayer);
        }

        // Return. Don't execute anything else.
        if (saveUSDTransforms(
                objectName, fileName, start, end, step, resetSessionLayer))
        {
            return MS::kSuccess;
        }

        return MS::kFailure;
    }

    if (argsDb.isFlagSet("-savePurposes"))
    {
        MString objectName;
        MString fileName;

        argsDb.getFlagArgument("-savePurposes", 0, objectName);
        argsDb.getFlagArgument("-savePurposes", 1, fileName);

        // Return. Don't execute anything else.
        if (savePurposes(objectName, fileName))
        {
            return MS::kSuccess;
        }

        return MS::kFailure;
    }

    if (argsDb.isFlagSet("-saveVariantsLayer"))
    {
        MString objectName;
        MString fileName;

        argsDb.getFlagArgument("-saveVariantsLayer", 0, objectName);
        argsDb.getFlagArgument("-saveVariantsLayer", 1, fileName);

        // Return. Don't execute anything else.
        if (saveVariantsLayer(objectName, fileName))
        {
            return MS::kSuccess;
        }

        return MS::kFailure;
    }

    if (argsDb.isFlagSet("-assignmentObjects"))
    {
        MString objectName;
        MString layer;
        MString target;

        argsDb.getFlagArgument("-assignmentObjects", 0, objectName);
        argsDb.getFlagArgument("-assignmentObjects", 1, layer);
        argsDb.getFlagArgument("-assignmentObjects", 2, target);

        // Return. Don't execute anything else.
        return getAssignmentObjects(
            objectName, layer.asChar(), target.asChar());
    }

    if (argsDb.isFlagSet("-assignmentLayers"))
    {
        MString objectName;

        argsDb.getFlagArgument("-assignmentLayers", 0, objectName);

        // Return. Don't execute anything else.
        return getAssignmentLayers(objectName);
    }

    if (argsDb.isFlagSet("-assignment"))
    {
        MString objectName;
        MString subNodeName;
        MString layer;
        MString type;

        argsDb.getFlagArgument("-assignment", 0, objectName);
        argsDb.getFlagArgument("-assignment", 1, subNodeName);
        argsDb.getFlagArgument("-assignment", 2, layer);
        argsDb.getFlagArgument("-assignment", 3, type);

        // Return. Don't execute anything else.
        return getAssignment(objectName, subNodeName, layer, type);
    }

    if (argsDb.isFlagSet("-alembicAllGroups"))
    {
        MString objectName;

        argsDb.getFlagArgument("-alembicAllGroups", 0, objectName);

        // Return. Don't execute anything else.
        return getAlembicAllGroups(objectName);
    }

    if (argsDb.isFlagSet("-alembicExpressions"))
    {
        MString objectName;

        argsDb.getFlagArgument("-alembicExpressions", 0, objectName);

        // Return. Don't execute anything else.
        return getAlembicExpressions(objectName, NULL);
    }

    if (argsDb.isFlagSet("-alembicGroupExpressions"))
    {
        MString objectName;
        MString groupName;

        argsDb.getFlagArgument("-alembicGroupExpressions", 0, objectName);
        argsDb.getFlagArgument("-alembicGroupExpressions", 1, groupName);

        // Return. Don't execute anything else.
        return getAlembicExpressions(objectName, &groupName);
    }

    if (argsDb.isFlagSet("-exposeTransform"))
    {
        MString objectName;
        MString subObjectName;

        argsDb.getFlagArgument("-exposeTransform", 0, objectName);
        argsDb.getFlagArgument("-exposeTransform", 1, subObjectName);

        // Return. Don't execute anything else.
        return exposeTransform(objectName, subObjectName);
    }

    if (argsDb.isFlagSet("-detachTransform"))
    {
        MString objectName;
        MString subObjectName;

        argsDb.getFlagArgument("-detachTransform", 0, objectName);
        argsDb.getFlagArgument("-detachTransform", 1, subObjectName);

        // Return. Don't execute anything else.
        return detachTransform(objectName, subObjectName);
    }

    if (argsDb.isFlagSet("-setCachedFramesDirty"))
    {
        MString objectName;
        argsDb.getFlagArgument("-setCachedFramesDirty", 0, objectName);

        setCachedFramesDirty(objectName);

        // Return. Don't execute anything else.
        return MS::kSuccess;
    }

    if (argsDb.isFlagSet("-getLayerAsString"))
    {
        MString objectName;
        MString layerName;

        argsDb.getFlagArgument("-getLayerAsString", 0, objectName);
        argsDb.getFlagArgument("-getLayerAsString", 1, layerName);

        getLayerAsString(objectName, layerName);

        // Return. Don't execute anything else.
        return MS::kSuccess;
    }

    if (argsDb.isFlagSet("-isPseudoRoot"))
    {
        MString objectName;
        MString subObjectName;

        argsDb.getFlagArgument("-isPseudoRoot", 0, objectName);
        argsDb.getFlagArgument("-isPseudoRoot", 1, subObjectName);

        return isPseudoRoot(objectName, subObjectName);
    }

    if (argsDb.isFlagSet("-toggleVisibility"))
    {
        MString objectName;
        MString subObjectName;

        argsDb.getFlagArgument("-toggleVisibility", 0, objectName);
        argsDb.getFlagArgument("-toggleVisibility", 1, subObjectName);

        return toggleVisibility(objectName, subObjectName);
    }

    if (argsDb.isFlagSet("-setVisibility"))
    {
        MString objectName;
        MString subObjectName;
        bool visibility;

        argsDb.getFlagArgument("-setVisibility", 0, objectName);
        argsDb.getFlagArgument("-setVisibility", 1, subObjectName);
        argsDb.getFlagArgument("-setVisibility", 2, visibility);

        return setVisibility(objectName, subObjectName, visibility);
    }

    if (argsDb.isFlagSet("-hideAllExceptedThis"))
    {
        MString objectName;
        MString subObjectName;

        argsDb.getFlagArgument("-hideAllExceptedThis", 0, objectName);
        argsDb.getFlagArgument("-hideAllExceptedThis", 1, subObjectName);

        return hideAllExceptedThis(objectName, subObjectName);
    }

    if (argsDb.isFlagSet("-primPathsMatchingExpression"))
    {
        MString objectName, expression;

        argsDb.getFlagArgument("-primPathsMatchingExpression", 0, objectName);
        argsDb.getFlagArgument("-primPathsMatchingExpression", 1, expression);

        return primPathsMatchingExpression(objectName, expression);
    }

    if (argsDb.isFlagSet("-setPurpose"))
    {
        MString objectName, subObjectName, purpose;
        argsDb.getFlagArgument("-setPurpose", 0, objectName);
        argsDb.getFlagArgument("-setPurpose", 1, subObjectName);
        argsDb.getFlagArgument("-setPurpose", 2, purpose);
        return setPurpose(objectName, subObjectName, purpose);
    }

    if (argsDb.isFlagSet("-getPurpose"))
    {
        MString objectName, subObjectName;
        argsDb.getFlagArgument("-getPurpose", 0, objectName);
        argsDb.getFlagArgument("-getPurpose", 1, subObjectName);
        return getPurpose(objectName, subObjectName);
    }

    if (argsDb.isFlagSet("-setRenderPurpose"))
    {
        MString objectName, purposeList;
        argsDb.getFlagArgument("-setRenderPurpose", 0, objectName);
        argsDb.getFlagArgument("-setRenderPurpose", 1, purposeList);
        return setRenderPurpose(objectName, purposeList);
    }

    if (argsDb.isFlagSet("-getRenderPurpose"))
    {
        MString objectName;
        argsDb.getFlagArgument("-getRenderPurpose", 0, objectName);
        return getRenderPurpose(objectName);
    }

    if (argsDb.isFlagSet("-isVisible"))
    {
        MString objectName;
        MString subObjectName;

        argsDb.getFlagArgument("-isVisible", 0, objectName);
        argsDb.getFlagArgument("-isVisible", 1, subObjectName);

        return isVisible(objectName, subObjectName);
    }

    if (argsDb.isFlagSet("-export"))
    {
        // Looks like MArgList stores references, so we can't pass the value.
        MString jobArg("-jobArg");

        // Get the parameters.
        MString params;
        argsDb.getFlagArgument("-export", 0, params);

        // Form the arguments for AbcExport.
        MArgList newArgs;
        newArgs.addArg(jobArg);
        newArgs.addArg(params);

        // Create a new instance of AbcExport.
        boost::scoped_ptr<AbcExport> exportCmd(
            reinterpret_cast<AbcExport*>(AbcExport::creator()));

        // Run AbcExport as it's executed by Maya.
        return exportCmd->doIt(newArgs);
    }

    if (argsDb.isFlagSet("-version"))
    {
        /* Return the version. It has following format: 1.2.3[ debug]. */
        MString version;
        version += WALTER_MAJOR_VERSION;
        version += ".";
        version += WALTER_MINOR_VERSION;
        version += ".";
        version += WALTER_PATCH_VERSION;

#ifdef DEBUG
        version += " debug";
#endif

        MPxCommand::setResult(version);

        return MS::kSuccess;
    }

    if (argsDb.isFlagSet("-joinAll"))
    {
        WalterThreadingUtils::joinAll();
        return MS::kSuccess;
    }

    if (argsDb.isFlagSet("-join"))
    {
        MString objectName;

        argsDb.getFlagArgument("-join", 0, objectName);

        join(objectName);

        return MS::kSuccess;
    }

    if (argsDb.isFlagSet("-hydraPlugins"))
    {
        static MStringArray sHydraPlugins;
        if (sHydraPlugins.length() == 0)
        {
            // Init the list of the plugins. All of them are initialized on
            // startup, so we can reuse this array. We can compare it with 0
            // because we know that there is always at least one hydra plugin.
            getHydraPlugins(sHydraPlugins);
        }

        MPxCommand::setResult(sHydraPlugins);
        return MS::kSuccess;
    }

    if (argsDb.isFlagSet("-expressionIsMatching"))
    {
        MString objectName;
        MString expression;

        argsDb.getFlagArgument("-expressionIsMatching", 0, objectName);
        argsDb.getFlagArgument("-expressionIsMatching", 1, expression);

        return expressionIsMatching(objectName, expression);
    }

    return status;
}

// TODO: kill?
MStatus Command::clearSubSelection(const MString& objectName) const
{
    MStatus status;

    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }

    /* Save the sub-selection to update it in the next redraw cycle. */
    userNode->setSubSelectedObjects("");

    return MS::kSuccess;
}

MStatus Command::addSubSelection(
    const MString& objectName,
    const MString& subNodeName) const
{
    MStatus status;

    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }

    userNode->setSubSelectedObjects(subNodeName);
    return MS::kSuccess;
}

MStatus Command::dirAlembic(
    const MString& objectName,
    const MString& subNodeName) const
{
    MStatus status;

    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }

    MStringArray result;
    dirUSD(userNode, subNodeName, result);
    MPxCommand::setResult(result);
    return MS::kSuccess;
}

MStatus Command::propsAlembic(
    const MString& objectName,
    const MString& subNodeName) const
{
    MStatus status;

    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }

    std::string jsonStr = "{ ";
    jsonStr += "\"prim\": \"" + std::string(subNodeName.asChar()) + "\"";

    MStringArray resultArray;
    primVarUSD(userNode, subNodeName, resultArray);
    if(resultArray.length() > 0)
    {   
        jsonStr += ", \"properties\": [ ";
        for(unsigned i=0; i<resultArray.length(); ++i) 
        {
            jsonStr += resultArray[i].asChar();
            if(i < (resultArray.length()-1))
                jsonStr += ", ";
        }
        jsonStr += "]";
    }

    resultArray.clear();
    assignedOverridesUSD(userNode, subNodeName, resultArray);
    if(resultArray.length() > 0)
    {
        jsonStr += ", \"overrides\": [ ";
        for(unsigned i=0; i<resultArray.length(); ++i) 
        {
            jsonStr += resultArray[i].asChar();
            if(i < (resultArray.length()-1))
                jsonStr += ", ";
        }
        jsonStr += "]";
    }

    jsonStr += " }";
    MPxCommand::setResult(jsonStr.c_str());

    return MS::kSuccess;
}

MStatus Command::getVariants(
    const MString& objectName,
    const MString& subNodeName,
    bool recursively) const
{
    MStatus status;

    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }

    // RND-537: Update the variants menu when a path to a walter layer changed.
    // Call this method so the variants off all the layers (for this object) 
    // are available. To-Do: RND-554. Investigate why we have to call joinAll, 
    // join should be enough
    WalterThreadingUtils::joinAll();

    MStringArray resultArray;
    getVariantsUSD(
        userNode,
        subNodeName,
        recursively,
        resultArray);

    MPxCommand::setResult(resultArray);

    return MS::kSuccess;
}

MStatus Command::setVariant(
    const MString& objectName,
    const MString& subNodeName,
    const MString& variantName,
    const MString& variantSetName) const
{
    MStatus status;

    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }

    setVariantUSD(userNode, subNodeName, variantName, variantSetName);

    return MS::kSuccess;
}

MStatus Command::getAssignmentObjects(
    const MString& objectName,
    const std::string& layer,
    const std::string& target) const
{
    MStatus status;

    const ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }

    const ShapeNode::ExpMap& assignments = userNode->getAssignments();

    std::set<std::string> objects;

    BOOST_FOREACH (const auto& exp, assignments)
    {
        BOOST_FOREACH (const auto& l, exp.second)
        {
            if (l.first.first == layer && l.first.second == target)
            {
                objects.insert(exp.first);
                break;
            }
        }
    }

    // Convert std::set to MStringArray
    // TODO: reserve
    MStringArray result;
    BOOST_FOREACH (const auto& o, objects)
    {
        result.append(o.c_str());
    }

    MPxCommand::setResult(result);

    return MS::kSuccess;
}

MStatus Command::getAssignmentLayers(const MString& objectName) const
{
    MStatus status;

    const ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }

    const ShapeNode::ExpMap& assignments = userNode->getAssignments();

    std::set<std::string> layers;
    // Get all the layers
    BOOST_FOREACH (const auto& exp, assignments)
    {
        BOOST_FOREACH (const auto& layer, exp.second)
        {
            layers.insert(layer.first.first);
        }
    }

    // Convert std::set to MStringArray
    // TODO: reserve
    MStringArray result;
    BOOST_FOREACH (const auto& l, layers)
    {
        result.append(l.c_str());
    }

    MPxCommand::setResult(result);

    return MS::kSuccess;
}

MStatus Command::getAssignment(
    const MString& objectName,
    const MString& subNodeName,
    const MString& layer,
    const MString& type) const
{
    MStatus status;

    const ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }

    const ShapeNode::ExpMap& assignments = userNode->getAssignments();

    ShapeNode::ExpMap::const_iterator layersIt =
        assignments.find(subNodeName.asChar());

    MString shader;

    if (layersIt != assignments.end())
    {
        ShapeNode::LayerMap::const_iterator shaderIt = layersIt->second.find(
            ShapeNode::LayerKey(layer.asChar(), type.asChar()));

        if (shaderIt != layersIt->second.end())
        {
            shader = shaderIt->second.c_str();
        }
    }

    MPxCommand::setResult(shader);

    return MS::kSuccess;
}

MStatus Command::getAlembicAllGroups(const MString& objectName) const
{
    MStatus status;

    const ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }

    std::set<std::string> groups;

    for (const auto& group : userNode->getGroups())
    {
        if (!group.second.empty())
        {
            groups.insert(group.second);
        }
    }

    // Convert std::set to MStringArray
    // TODO: reserve
    MStringArray result;
    BOOST_FOREACH (const auto& group, groups)
    {
        result.append(group.c_str());
    }

    MPxCommand::setResult(result);

    return MS::kSuccess;
}

MStatus Command::getAlembicExpressions(
    const MString& objectName,
    const MString* groupName) const
{
    MStatus status;

    const ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }

    std::set<std::string> expressions;
    // We are going to compare std::string, we need to compare it with the same
    // type.
    std::string groupNameStr(groupName ? groupName->asChar() : "");

    for (const auto& group : userNode->getGroups())
    {
        if (!groupName || group.second == groupNameStr)
        {
            expressions.insert(group.first);
        }
    }

    // Convert std::set to MStringArray
    // TODO: reserve
    MStringArray result;
    BOOST_FOREACH (const auto& expression, expressions)
    {
        result.append(expression.c_str());
    }

    MPxCommand::setResult(result);

    return MS::kSuccess;
}

MStatus Command::exposeTransform(
    const MString& objectName,
    const MString& subObjectName) const
{
    MStatus status;
    const ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }

    MObject node = userNode->thisMObject();
    MFnDagNode dagNode(node);

    std::unordered_map<std::string, MPlug> plugCache;

    const MPlug transformsPlug(node, ShapeNode::aTransforms);
    // We can't remove the elements from the middle of the plug array. Instead,
    // we can reuse plugs that are not connected to anything. To reuse them,
    // it's necessary to seve the list of them.
    std::vector<std::pair<const MPlug, const MPlug>> availablePlugs;

    assert(!transformsPlug.isNull());
    for (int i = 0, n = transformsPlug.numElements(); i < n; i++)
    {
        const MPlug element = transformsPlug.elementByPhysicalIndex(i);
        const MPlug objectPlug = element.child(ShapeNode::aTransformsObject);
        const MPlug matrixPlug = element.child(ShapeNode::aTransformsMatrix);

        const MString currentSubObjectName = objectPlug.asString();
        if (currentSubObjectName.length() == 0 || !matrixPlug.isConnected())
        {
            availablePlugs.push_back(std::make_pair(objectPlug, matrixPlug));
            continue;
        }

        // Emplace to avoid overwriting if it already exists.
        plugCache.emplace(currentSubObjectName.asChar(), matrixPlug);
    }

    // Split the sub object name so we have all the parents in the vector.
    // TODO: it's better to use SdfPath but I don't want to include pixar stuff
    // here, so for now it's boost::split and boost::join.
    std::vector<std::string> path, pathres;
    const std::string subObjectNameStr = subObjectName.asChar();
    boost::split(path, subObjectNameStr, boost::is_any_of("/"));
    boost::split(pathres, subObjectNameStr, boost::is_any_of("/"));

    // Looking if we already have a transform connected to this sub-object or to
    // its parents. We are looking starting from the full object name and each
    // iteration we go to its parent. At the same time we save all the objects
    // to pathChildren, so at the end we have the list of objects we need to
    // create.
    std::string firstParentObjectName;
    MPlug* firstParentPlug = NULL;
    std::vector<std::string> pathChildren;
    while (true)
    {
        firstParentObjectName = boost::algorithm::join(path, "/");

        if (firstParentObjectName.empty())
        {
            // Nothing is found.
            break;
        }

        auto found = plugCache.find(firstParentObjectName);
        if (found != plugCache.end())
        {
            // Found.
            firstParentPlug = &(found->second);
            break;
        }

        pathChildren.push_back(path.back());
        path.pop_back();
    }

    std::string currentObjectFullName = firstParentObjectName;
    MObject parent;
    if (firstParentPlug && firstParentPlug->isConnected())
    {
        // We found that we already have the connection to a parent of the
        // requested sub-object. All the created transforms should be the
        // children of this connection.
        MPlugArray connectedTo;
        firstParentPlug->connectedTo(connectedTo, true, false);
        parent = connectedTo[0].node();
    }
    else
    {
        // Nothing is found. We can create all the transforms as children of
        // current walter standin.
        parent = dagNode.parent(0);
    }

    // Dependency graph modifier.
    MDagModifier mod;
    MFnDagNode standinNode(parent);

    std::string currentObjectFullMayaName = std::string(standinNode.fullPathName().asChar());

    // Create the transforms.
    while (!pathChildren.empty())
    {
        MFnDagNode standinNode(parent);

        const std::string& currentObjectName = pathChildren.back();
        currentObjectFullName += "/" + currentObjectName;
        currentObjectFullMayaName += "|" + currentObjectName;
       
        MObject locatorTransform;
        MObject locator;

        int tmp = standinNode.childCount();
        for(uint32_t i = tmp; i--;) {
            auto transformObj = standinNode.child(i);
            MFnDagNode transformDagNode(transformObj);

            if(transformDagNode.fullPathName().asChar() == currentObjectFullMayaName) {
                locator = transformDagNode.child(0);
                locatorTransform = transformObj;
                break;
            }
        }

        // Create a locator node and parent it to the transform.
        if(locatorTransform.isNull())
            locatorTransform = mod.createNode("transform", parent);

        if(locator.isNull())
            locator = mod.createNode("locator", locatorTransform);

        MFnDependencyNode locatorDepNode(locatorTransform);
        const MPlug matrix = locatorDepNode.findPlug("matrix");

        parent = locatorTransform;

        MPlug objectPlug;
        MPlug matrixPlug;
        if (availablePlugs.empty())
        {
            // No available plugs. Create new.
            MPlug ancestor = transformsPlug;
            ancestor.selectAncestorLogicalIndex(
                ancestor.numElements(), ShapeNode::aTransforms);

            objectPlug = ancestor.child(ShapeNode::aTransformsObject);
            matrixPlug = ancestor.child(ShapeNode::aTransformsMatrix);
        }
        else
        {
            // Reuse existing plugs.
            objectPlug = availablePlugs.back().first;
            matrixPlug = availablePlugs.back().second;
            availablePlugs.pop_back();
        }

        // Set transformation from USD stage.
        MFnTransform locatorFnTransform(locatorTransform);
        double transform[4][4];
        if (getUSDTransform(
                node,
                currentObjectFullName,
                userNode->time * getCurrentFPS(),
                transform))
        {
            locatorFnTransform.set(MTransformationMatrix(MMatrix(transform)));
        }

        MString currentObjectFullNameStr(currentObjectFullName.c_str());

        // Set object name.
        objectPlug.setString(currentObjectFullNameStr);
        locatorDepNode.setName(currentObjectName.c_str());

        // Create a string attribute on the locator to store the
        // full path of the USD prim the locator points to.
        MFnStringData stringFn;
        MFnTypedAttribute typedAttribute;
        MObject pathObject  = stringFn.create(currentObjectFullName.c_str());
        MObject pathAttribute = typedAttribute.create(
            LOCATOR_ATTR_PRIM_PATH, 
            LOCATOR_ATTR_PRIM_PATH, 
            MFnData::kString, 
            pathObject);
        typedAttribute.setStorable(true);

        locatorDepNode.addAttribute(pathAttribute);

        // TODO: Disconnect if connected.
        mod.connect(matrix, matrixPlug);
        mod.doIt();

        pathChildren.pop_back();
    }

    MStringArray result;
    std::string currentObjectFullNameRes;
    // Skip the first element since it's emnpty
    for(int i=1; i<pathres.size(); ++i) {
        std::string p = pathres[i];
        currentObjectFullNameRes += "/" + p;
        result.append(currentObjectFullNameRes.c_str());
        result.append(p.c_str());
    }

    MPxCommand::setResult(result);

    return MS::kSuccess;
}

MStatus Command::detachTransform(
    const MString& objectName,
    const MString& subObjectName) const
{
    MStatus status;
    auto* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }
 
    auto node = userNode->thisMObject();
    MDagModifier modifier;

    std::unordered_map<std::string, MPlug> plugCache;

    const MPlug transformsPlug2(node, ShapeNode::aTransforms);

    // We can't remove the elements from the middle of the plug array. Instead,
    // we can reuse plugs that are not connected to anything. To reuse them,
    // it's necessary to seve the list of them.
    std::vector<std::pair<const MPlug, const MPlug>> availablePlugs;

    assert(!transformsPlug2.isNull());
    for (int i = 0, n = transformsPlug2.numElements(); i < n; i++)
    {
        const MPlug element = transformsPlug2.elementByPhysicalIndex(i);
        const MPlug objectPlug = element.child(ShapeNode::aTransformsObject);
        const MPlug matrixPlug = element.child(ShapeNode::aTransformsMatrix);

        const MString currentSubObjectName = objectPlug.asString();
        if (currentSubObjectName.length() == 0 || !matrixPlug.isConnected())
        {
            availablePlugs.push_back(std::make_pair(objectPlug, matrixPlug));
            continue;
        }

        // Emplace to avoid overwriting if it already exists.
        plugCache.emplace(currentSubObjectName.asChar(), matrixPlug);
    }

    // Split the sub object name so we have all the parents in the vector.
    // TODO: it's better to use SdfPath but I don't want to include pixar stuff
    // here, so for now it's boost::split and boost::join.
    std::vector<std::string> path, pathres;
    const std::string subObjectNameStr = subObjectName.asChar();
    boost::split(path, subObjectNameStr, boost::is_any_of("/"));
    boost::split(pathres, subObjectNameStr, boost::is_any_of("/"));

    // Looking if we already have a transform connected to this sub-object or to
    // its parents. We are looking starting from the full object name and each
    // iteration we go to its parent. At the same time we save all the objects
    // to pathChildren, so at the end we have the list of objects we need to
    // create.
    std::string firstParentObjectName;
    MPlug* firstParentPlug = NULL;
    std::vector<std::string> pathChildren;
    while (true)
    {
        firstParentObjectName = boost::algorithm::join(path, "/");

        if (firstParentObjectName.empty())
        {
            // Nothing is found.
            break;
        }

        auto found = plugCache.find(firstParentObjectName);
        if (found != plugCache.end())
        {
            // Found.
            firstParentPlug = &(found->second);
            break;
        }

        pathChildren.push_back(path.back());
        path.pop_back();
        break;
    }

    std::string currentObjectFullName = firstParentObjectName;
    MObject parent;
    if (firstParentPlug)
    {
        // We found that we already have the connection to a parent of the
        // requested sub-object. All the created transforms should be the
        // children of this connection.
        MPlugArray connectedTo;
        firstParentPlug->connectedTo(connectedTo, true, true);
        parent = connectedTo[0].node();
        modifier.deleteNode(parent);
    }
 
    modifier.doIt();
 
    ::resetUSDTransforms(userNode, subObjectName);

    return MS::kSuccess;
}

MStatus Command::setCachedFramesDirty(const MString& objectName) const
{
    MStatus status;
    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }

    // Set animation caches dirty.
    userNode->setCachedFramesDirty();

    // Recreate the viewport materials.
    processGLSLShaders(userNode);

    return MS::kSuccess;
}

MStatus Command::getLayerAsString(
    const MString& objectName,
    const MString& layerName) const
{
    MStatus status;
    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }

    MString layer = ::getLayerAsString(userNode, layerName);
    MPxCommand::setResult(layer);

    return MS::kSuccess;
}

MStatus Command::join(const MString& objectName) const
{
    MStatus status;
    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }

    // This will call onLoaded if it was never called.
    userNode->getRenderer();

    return MS::kSuccess;
}

MStatus Command::expressionIsMatching(
    const MString& objectName,
    const MString& expression) const
{
    MStatus status;
    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
    {
        return status;
    }

    MPxCommand::setResult(
        userNode->expressionIsMatching(expression));

    return MS::kSuccess;
}

MStatus Command::isPseudoRoot(
    const MString& objectName,
    const MString& subObjectName) const
{
    MStatus status;
    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
        return status;

    MPxCommand::setResult(::isPseudoRoot(
        userNode, subObjectName));

    return MS::kSuccess;
}

MStatus Command::isVisible(
    const MString& objectName,
    const MString& subObjectName) const
{
    MStatus status;

    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
        return status;

    MPxCommand::setResult(::isVisible(
        userNode, subObjectName));

    return MS::kSuccess;
}

MStatus Command::toggleVisibility(
    const MString& objectName,
    const MString& subObjectName) const
{
    MStatus status;

    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
        return status;

    MPxCommand::setResult(::toggleVisibility(
        userNode, subObjectName));

    return MS::kSuccess;
}

MStatus Command::setVisibility(
    const MString& objectName,
    const MString& subObjectName,
    bool visibility) const
{
    MStatus status;

    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
        return status;

    MPxCommand::setResult(::setVisibility(
        userNode, subObjectName, visibility));

    return MS::kSuccess;
}

MStatus Command::hideAllExceptedThis(
    const MString& objectName,
    const MString& subObjectName) const
{
    MStatus status;

    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
        return status;

    MPxCommand::setResult(::hideAllExceptedThis(
        userNode, subObjectName));

    return MS::kSuccess;
}

MStatus Command::primPathsMatchingExpression(
    const MString& objectName,
    const MString& expression) const
{
    MStatus status;

    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
        return status;

    MPxCommand::setResult(::primPathsMatchingExpression(
        userNode, expression));

    return MS::kSuccess;
}

MStatus Command::setPurpose(
    const MString& objectName,
    const MString& subObjectName,
    const MString& purpose) const
{
    MStatus status;
    ShapeNode* userNode = getShapeNode(objectName, &status);

    if (!userNode)
        return status;

    MPxCommand::setResult(::setPurpose(
        userNode, subObjectName, purpose));

    return MS::kSuccess;
}

MStatus Command::getPurpose(
    const MString& objectName,
    const MString& subObjectName) const
{
    MStatus status;
    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
        return status;

    MPxCommand::setResult(::getPurpose(
        userNode, subObjectName));

    return MS::kSuccess;
}

MStatus Command::setRenderPurpose(
    const MString& objectName,
    const MString& purposeList) const
{
    MStatus status;
    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
        return status;

    std::string tmp = purposeList.asChar();
    std::vector<std::string> path;
    boost::split(path, tmp, boost::is_any_of(":"));

    MStringArray purposeArray;
    for(auto const& strPurpose: path)
        purposeArray.append(strPurpose.c_str());

    ::setRenderPurpose(userNode, purposeArray);
    return MS::kSuccess;
}

MStatus Command::getRenderPurpose(
    const MString& objectName) const
{
    MStatus status;

    ShapeNode* userNode = getShapeNode(objectName, &status);
    if (!userNode)
        return status;

    MPxCommand::setResult(::getRenderPurpose(userNode));
    return MS::kSuccess;
}

}
