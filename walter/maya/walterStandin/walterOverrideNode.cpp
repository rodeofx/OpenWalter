// Copyright 2017 Rodeo FX.  All rights reserved.

#include "walterOverrideNode.h"

#include <assert.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MMessage.h>
#include <maya/MNodeMessage.h>

MTypeId WalterOverride::mId(0x00128540);

// If a problem occurs, this function returns an empty string.
MString getNameFromObj(MObject obj)
{
    MString nodeName;
    // If this object is a MFnDagNode, we should store the dag name.
    // Otherwise, use the MFnDependencyNode name.
    if (obj.hasFn(MFn::kDagNode))
    {
        MFnDagNode dagNode(obj);
        nodeName = dagNode.fullPathName();
    }
    else if (obj.hasFn(MFn::kDependencyNode))
    {
        MFnDependencyNode node(obj);
        nodeName = node.name();
    }
    return nodeName;
}

WalterOverride::WalterOverride()
{
}

WalterOverride::~WalterOverride()
{
}

void* WalterOverride::creator()
{
    return new WalterOverride();
}

MStatus WalterOverride::initialize()
{
    return MS::kSuccess;
}

void WalterOverride::postConstructor()
{
  MObject mobj = thisMObject();
  MFnDependencyNode dep_node(mobj);

  // Set the default node name.
  MString defaultname = typeName() + "#";

  // Save the digit in the node name.
  if( isdigit( defaultname.asChar()[0] ) )
  {
    defaultname = "_" + defaultname;
  }

  dep_node.setName(defaultname);

  // We are unable to call initialize MEL proc because the node is not
  // initialized by maya proprly but we use "#" in the default node name, it
  // means the name will be changed as soon as the node will be completely
  // initialized. The DL_genericShadingNode::nameChanged() will immediately
  // remove this callback.
  MNodeMessage::addNameChangedCallback(mobj, nameChanged, nullptr);
}

void WalterOverride::nameChanged(MObject& node, void *data)
{
  // We need this function only once, so immediately remove current callback.
  MMessage::removeCallback(MMessage::currentCallbackId());

  // Current node.
  MString name = getNameFromObj(node);

  // Call initialize python proc.
  MString cmd =
      "AEwalterOverrideTemplate.walterOverrideInitialize('" +
      name +
      "')";
  MGlobal::executePythonCommandOnIdle(cmd);
}
