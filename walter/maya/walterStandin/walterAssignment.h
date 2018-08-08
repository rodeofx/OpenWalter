// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERASSIGNMENT_H_
#define __WALTERASSIGNMENT_H_

// Several utilities to save/load assignment from Alembic files.

#include "PathUtil.h"
#include "mayaUtils.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <map>

/**
 * @brief Iterates the plug with the assignments and fills the map to with all
 * the connections that was found.
 *
 * @param layerCompound The plug that represents
 * walterStandin.layersAssignation[n]
 * @param plugsToQuerry The list of plugs to query. Fox example if the list is
 * {"shader", "displacement"}, then following plugs will be queried:
 * * walterStandin.layersAssignation[].shaderConnections[].shader
 * * walterStandin.layersAssignation[].shaderConnections[].displacement
 * @param oAssignments The sub-node names to assigned matrials maps for each
 * requested plug.
 */
template <typename T>
void getAllAssignments(
    MPlug layerCompound,
    MStringArray plugsToQuerry,
    T& oAssignments)
{
    MPlug shadersPlug;
    if (!GetChildMPlug(layerCompound, "shaderConnections", shadersPlug))
    {
        return;
    }

    for (unsigned int j = 0; j < shadersPlug.numElements(); j++)
    {
        // Get walterStandin.layersAssignation[i].shaderConnections[j]
        const MPlug shadersCompound = shadersPlug.elementByPhysicalIndex(j);
        // Get walterStandin.layersAssignation[].shaderConnections[].abcnode
        MPlug abcnodePlug;
        if (!GetChildMPlug(shadersCompound, "abcnode", abcnodePlug))
        {
            continue;
        }

        const MString abcnode = abcnodePlug.asString().asChar();
        if (!abcnode.length())
        {
            continue;
        }

        for (int i = 0, n = plugsToQuerry.length(); i < n; i++)
        {
            // walterStandin.layersAssignation[].shaderConnections[].xxx
            MPlug shaderPlug;
            if (!GetChildMPlug(shadersCompound, plugsToQuerry[i], shaderPlug))
            {
                continue;
            }

            // Get the connected shader name
            MPlugArray connections;
            if (!shaderPlug.connectedTo(connections, true, false) ||
                !connections.length())
            {
                continue;
            }

            oAssignments[i].emplace(
                std::piecewise_construct,
                std::forward_as_tuple(abcnode.asChar()),
                std::forward_as_tuple(connections[0].node()));
        }
    }
}

/**
 * @brief Querry the connections of the walter standin to get the list of all
 * the shaders connected to the default render layer.
 *
 * @param node Walter Standin node.
 * @param type "shader", "displacement" or "attribute".
 * @param oAssignments The sub-node names to assigned matrials output map.
 */
template <class T>
void getShaderAssignments(
    MObject node,
    const std::string& type,
    T& oAssignments)
{
    MFnDependencyNode depNode(node);

    const MPlug layersAssignation = depNode.findPlug("layersAssignation");
    if (layersAssignation.isNull())
    {
        printf(
            "[walter] Can't find %s.layersAssignation.",
            depNode.name().asChar());
        return;
    }

    if (layersAssignation.numElements() <= 0)
    {
        return;
    }

    for (unsigned int i = 0; i < layersAssignation.numElements(); i++)
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
            // Layer should be conneted, but it's not.
            continue;
        }

        // The layer is the first connected node. We consider we have only one
        // connection.
        const MFnDependencyNode layer(connections[0].node());
        const std::string layerName = layer.name().asChar();

        if (layerName != "defaultRenderLayer")
        {
            // We don't need it.
            continue;
        }

        // The list of plugs to look up.
        const char* plugsToFind[] = {type.c_str()};
        T* assignmentsPtr = &oAssignments;
        // Get all the assignments.
        getAllAssignments(
            currentLayerCompound,
            MStringArray(
                plugsToFind, sizeof(plugsToFind) / sizeof(*plugsToFind)),
            assignmentsPtr);

        // We found everything. No need to continue.
        break;
    }
}

// Save all the assignments from given walter node to a separate Alembic file.
bool saveAssignment(const MString& objectName, const MString& fileName);

#endif
