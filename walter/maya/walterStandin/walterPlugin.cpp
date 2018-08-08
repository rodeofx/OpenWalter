// Copyright 2017 Rodeo FX.  All rights reserved.

#include "walterShapeNode.h"
#include "walterDrawOverride.h"
#include "walterCmd.h"
#include "walterOverrideNode.h"

#include <maya/MFnPlugin.h>
#include <maya/MPlugArray.h>
#include <maya/MDrawRegistry.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionMask.h>
#include <maya/MSceneMessage.h>
#include <maya/MDGMessage.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
using namespace WalterMaya;

MStatus createMenu()
{
    MString cmd;

    // File Open dialog
    cmd += "proc int _walterCacheFileNameCB(";
    cmd += "\tstring $plug,";
    cmd += "\tstring $fileName,";
    cmd += "\tstring $fileType)";
    cmd += "{";
    cmd += "\tsetAttr $plug -type \"string\" $fileName;";
    cmd += "\treturn true;";
    cmd += "}";
    cmd += "proc _walterCacheFileNameBrowser(string $cmd)";
    cmd += "{";
    cmd += "\tfileBrowser($cmd, \"ABC File\", \"\", 0);";
    cmd += "}";

    cmd += "global string $gMainCreateMenu;";
    cmd += "ModCreateMenu($gMainCreateMenu);";
    cmd += "string $walterCreateNodeCmd = \""
        "string $node = `createNode walterStandin`;"
        "setAttr ($node + \\\".overrideTexturing\\\") 0;"
        "setAttr ($node + \\\".overrideEnabled\\\") 1;"
        "string $plug = $node + \\\".cacheFileName\\\";"
        "connectAttr -f time1.outTime ($node + \\\".time\\\");"
        "_walterCacheFileNameBrowser(\\\"_walterCacheFileNameCB $plug\\\");\";";
    cmd += "string $walterCreateNodeLabel = \"Walter Standin\";";
    cmd += "string $walterStandinMenuItem = \"walterStandinMenuItem\";";
    cmd += "if( !stringArrayContains("
        "$walterStandinMenuItem, `menu -q -ia $gMainCreateMenu`) )";
    cmd += "{";
    cmd += "\tsetParent -menu $gMainCreateMenu;";
    cmd += "\tmenuItem "
        "-divider true "
        "-dividerLabel \"Rodeo Walter\" "
        "($walterStandinMenuItem+\"Divider\");";
    cmd += "\tmenuItem "
        "-label $walterCreateNodeLabel "
        "-command $walterCreateNodeCmd $walterStandinMenuItem;";
    cmd += "}";
    cmd += "else";
    cmd += "{";
    cmd += "\tmenuItem -e "
        "-label $walterCreateNodeLabel "
        "-command $walterCreateNodeCmd "
        "$walterStandinMenuItem;";
    cmd += "}";

    return MGlobal::executeCommandOnIdle(cmd);
}

MStatus loadAETemplates()
{
    // AETemplate for walterOverride
    MString cmd =
        "import AEwalterOverrideTemplate\n"
        "import AEwalterStandinTemplate\n";
    return MGlobal::executePythonCommand(cmd);
}

MStatus deleteMenu()
{
    MString cmd;
    cmd += "global string $gMainCreateMenu;";
    cmd += "string $walterStandinMenuItem = \"walterStandinMenuItem\";";
    cmd += "if( stringArrayContains("
        "$walterStandinMenuItem, `menu -q -ia $gMainCreateMenu`) )";
    cmd += "{";
    cmd += "\tdeleteUI -menuItem ($walterStandinMenuItem+\"Divider\");";
    cmd += "\tdeleteUI -menuItem $walterStandinMenuItem;";
    cmd += "}";

    return MGlobal::executeCommand(cmd);
}

MCallbackId beforeNewSceneID;
MCallbackId beforeOpenSceneID;
MCallbackId afterOpenSceneID;

void onBeforeNewScene(void* userData)
{
    // Only execute this callback in interactive state.
    // Otherwise it will crash when rendered in the farm.
    if (MGlobal::kInteractive == MGlobal::mayaState()) 
    {
        // Hide the panel (qt widget)
        MString cmd =
            "import walterPanel\n"
            "walterPanel.hidePanel()\n";
        MGlobal::executePythonCommand(cmd);
    }
}

void onBeforeOpenScene(void* userData)
{
    // Only execute this callback in interactive state.
    // Otherwise it will crash when rendered in the farm.
    if (MGlobal::kInteractive == MGlobal::mayaState()) 
    {
        // Refreshs the panel (qt widget) to remove
        // the tree items associated with walter 
        // standins of the previous scene.
        MString cmd =
            "import walterPanel\n"
            "walterPanel.refreshPanel()\n";
        MGlobal::executePythonCommand(cmd);
    }
}

void onAfterOpenScene(void* userData)
{   
    // Only execute this callback in interactive state.
    // Otherwise it will crash when rendered in the farm.
    if (MGlobal::kInteractive == MGlobal::mayaState()) 
    {
        // Refreshs the panel (qt widget) to update
        // the tree items associated with walter
        // standinsof the current scene.
        MString cmd =
            "import walterPanel\n"
            "walterPanel.refreshPanel()\n";
        MGlobal::executePythonCommand(cmd);
    }
}

// Add callback when a dag is added/removed to support undo/redo
MCallbackId addNodeID;
MCallbackId removeNodeID;
MCallbackId connectNodeID;

void onAddCB(MObject& node, void* clientData) 
{
    if (MGlobal::kInteractive == MGlobal::mayaState()) {

        MFnDependencyNode nodeFn(node);

        if(MFn::kPluginShape == node.apiType()) {
            MString cmd = 
                "callbacks -executeCallbacks -hook \"WALTER_SCENE_CHANGED\" \"ADD_ROOT_ITEM\" \"" + nodeFn.name() + "\"";
            MGlobal::executeCommandOnIdle(cmd);
        }
    }
}

void onRemoveCB(MObject& node, void* clientData)
{
    if (MGlobal::kInteractive == MGlobal::mayaState()) {
        MFnDependencyNode nodeFn(node);

        if(MFn::kPluginShape == node.apiType()) {
            MString cmd = 
                "callbacks -executeCallbacks -hook \"WALTER_SCENE_CHANGED\" \"REMOVE_ROOT_ITEM\" \"" + nodeFn.name() + "\"";
            MGlobal::executeCommandOnIdle(cmd);
        }
    }
}

void onConnectCB(MPlug& srcPlug, MPlug& destPlug, bool made, void* clientData)
{
    if (MGlobal::kInteractive == MGlobal::mayaState()) {

        MFnDependencyNode srcNodeFn(srcPlug.node());
        MFnDependencyNode destNodeFn(destPlug.node());

        MString primPath = "";
        MString action = made ? "ATTACH_TRANSFORM" : "DETACH_TRANSFORM";
        for(int i=0; i<srcNodeFn.attributeCount(); ++i) {
            MObject attrObject = srcNodeFn.attribute(i);

            if(attrObject.apiType() == MFn::kTypedAttribute) {
                MFnTypedAttribute attribute(attrObject);

                MObject defaultCustomData;
                attribute.getDefault(defaultCustomData);

                if(attribute.name() == Command::LOCATOR_ATTR_PRIM_PATH) {
                    MFnStringData data(defaultCustomData);
                    primPath = data.string();

                    if(primPath.length() > 0) {
                        MString cmd = 
                            "callbacks -executeCallbacks -hook \"WALTER_TRANFORM_LOCATOR_DELETED\" \"" + action + "\" \"" + destNodeFn.name() + "\" \"" + primPath + "\"";
                        MGlobal::executeCommandOnIdle(cmd);
                    }
                }
            }
        }
    }
}

MStatus initializePlugin( MObject obj )
{
    MStatus status;

    beforeNewSceneID = MSceneMessage::addCallback(
        MSceneMessage::kBeforeNew, onBeforeNewScene );

    beforeOpenSceneID = MSceneMessage::addCallback(
        MSceneMessage::kBeforeOpen, onBeforeOpenScene );

    afterOpenSceneID = MSceneMessage::addCallback(
        MSceneMessage::kAfterOpen, onAfterOpenScene );

    addNodeID = MDGMessage::addNodeAddedCallback (
        onAddCB, "dependNode", NULL, &status);

    removeNodeID = MDGMessage::addNodeRemovedCallback (
        onRemoveCB, "dependNode", NULL, &status);

    connectNodeID = MDGMessage::addPreConnectionCallback (
        onConnectCB, NULL, &status);

    MFnPlugin plugin( obj, "RodeoFX", "1.0", "Any");

    status = plugin.registerShape(
        ShapeNode::nodeTypeName, 
        ShapeNode::id,
        ShapeNode::creator, 
        ShapeNode::initialize,
        ShapeUI::creator,
        &ShapeNode::drawDbClassificationGeometry );

    if (!status) {
        status.perror("registerNode");
        return status;
    }

    if (MGlobal::kInteractive == MGlobal::mayaState()) {

        status = MHWRender::MDrawRegistry::registerDrawOverrideCreator(
            ShapeNode::drawDbClassificationGeometry,
            ShapeNode::drawRegistrantId,
            DrawOverride::creator);
        if (!status) {
            status.perror("registerGeometryDrawCreator");
            return status;
        }
    }

    status = plugin.registerCommand(
        "walterStandin", Command::creator, Command::cmdSyntax );
    if (!status) {
        status.perror("registerCommand");
        return status;
    }

    MGlobal::executeCommand("setNodeTypeFlag -threadSafe true walterStandin");

    status = plugin.registerNode(
            "walterOverride",
            WalterOverride::mId,
            WalterOverride::creator,
            WalterOverride::initialize);

    if (MGlobal::kInteractive == MGlobal::mayaState()) {
        status = createMenu();
        if (!status) {
            status.perror("Can't create the Walter menu.");
            return status;
        }

        status = loadAETemplates();
        if (!status) {
            status.perror("Can't load AE Templates.");
            return status;
        }
    }

    std::cout << "[RodeoFX] walterStandin " <<
    MAJOR_VERSION << "." << MINOR_VERSION << "." << PATCH_VERSION <<
    ", built on " << __DATE__ << " at " << __TIME__ << std::endl;
    return MS::kSuccess;
}

MStatus uninitializePlugin( MObject obj )
{
    MStatus   status;

    MSceneMessage::removeCallback(beforeNewSceneID);
    MSceneMessage::removeCallback(beforeOpenSceneID);
    MSceneMessage::removeCallback(afterOpenSceneID);
    MDGMessage::removeCallback(addNodeID);
    MDGMessage::removeCallback(removeNodeID);
    MDGMessage::removeCallback(connectNodeID);

    MFnPlugin plugin( obj );

    if (MGlobal::kInteractive == MGlobal::mayaState()) {
        // Deregister the one we registered in initializePlugin().
        status = MHWRender::MDrawRegistry::deregisterDrawOverrideCreator(
            ShapeNode::drawDbClassificationGeometry,
            ShapeNode::drawRegistrantId);
        if (!status) {
            status.perror("deregisterDrawOverrideCreator");
            return status;
        }
    }

    status = ShapeNode::uninitialize();
    if (!status) {
        status.perror("ShapeNode::uninitialize()");
        return status;
    }

    status = plugin.deregisterNode( ShapeNode::id );
    if (!status) {
        status.perror("deregisterNode");
        return status;
    }

    status = plugin.deregisterCommand( "walterStandin" );
    if (!status) {
        status.perror("deregisterCommand");
        return status;
    }

    if (MGlobal::kInteractive == MGlobal::mayaState()) {
        status = deleteMenu();
        if (!status) {
            status.perror("Can't delete Walter menu.");
            return status;
        }
    }

    return MS::kSuccess;
}

