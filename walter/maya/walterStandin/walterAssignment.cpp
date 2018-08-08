// Copyright 2017 Rodeo FX.  All rights reserved.

#include "walterAssignment.h"
#include "OWalterExpressionsSchema.h"

#include <Alembic/Abc/All.h>
#include <Alembic/AbcCoreFactory/All.h>
#include <Alembic/AbcCoreLayer/Util.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>

bool saveAssignment(const MString& objectName, const MString& fileName)
{
    // Looking for MFnDependencyNode object in the scene.
    MSelectionList selectionList;
    selectionList.add(objectName);

    MObject obj;
    selectionList.getDependNode(0, obj);

    MFnDependencyNode depNode(obj);

    const MPlug layersAssignation = depNode.findPlug("layersAssignation");
    if (layersAssignation.isNull())
    {
        MString msg;
        msg.format("[walter] Can't find ^1s.layersAssignation", depNode.name());
        MGlobal::displayError(msg);
        return false;
    }

    if (layersAssignation.numElements() <= 0)
    {
        MString msg;
        msg.format(
            "[walter] There are no assignments to save in ^1s", objectName);
        MGlobal::displayError(msg);
        return false;
    }

    // Create Alembic file. Exceptions is a standard alembic way to get an error
    // as a string.
    Alembic::Abc::OArchive archive;
    try
    {
        archive = Alembic::Abc::OArchive(
            Alembic::AbcCoreOgawa::WriteArchive(), fileName.asChar());
    }
    catch (std::exception& exc)
    {
        MString msg;
        msg.format(
            "[walter] Can't write alembic ^1s. Exception:\n^2s",
            fileName,
            exc.what());
        MGlobal::displayError(msg);
        return false;
    }

    OWalterExpressionsSchema expressions = addExpressions(archive.getTop());

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
            continue;
        }

        // The layer is the first connected node. We consider we have only one
        // connection.
        const MFnDependencyNode layer(connections[0].node());
        const std::string layerName = layer.name().asChar();

        // The list of plugs to look up.
        static const char* plugsToFind[] = {
            "shader", "displacement", "attribute"};
        // It's the three maps of sub-node names to assigned matrials.
        typedef std::map<WalterCommon::Expression, MObject> ExpressionToMObject;
        std::vector<ExpressionToMObject> assignments(
            sizeof(plugsToFind) / sizeof(*plugsToFind));
        // Get all the assignments.
        getAllAssignments(
            currentLayerCompound,
            MStringArray(plugsToFind, assignments.size()),
            assignments);

        if (assignments.empty())
        {
            // We are here because currentLayerCompound doesn't have the plug
            // shaderConnections.
            continue;
        }

        // Iterate the result of getAllAssignments.
        for (int n = 0; n < sizeof(plugsToFind) / sizeof(*plugsToFind); n++)
        {
            const auto& subNodeToMaterial = assignments[n];
            for (const auto& item : subNodeToMaterial)
            {
                const MObject node(item.second);
                std::string shader = MFnDependencyNode(node).name().asChar();

                expressions.setExpressionShader(
                    item.first.getExpression(),
                    layerName,
                    plugsToFind[n],
                    shader);
            }
        }
    }

    // Set expression groups
    const MPlug expressionsPlug = depNode.findPlug("expressions");
    if (!expressionsPlug.isNull())
    {
        for (unsigned int i = 0; i < expressionsPlug.numElements(); i++)
        {
            // Get expressions[i]
            const MPlug expressionCompound =
                expressionsPlug.elementByPhysicalIndex(i);

            MPlug namePlug;
            if (!GetChildMPlug(expressionCompound, "en", namePlug))
            {
                continue;
            }

            MPlug groupPlug;
            if (!GetChildMPlug(expressionCompound, "egn", groupPlug))
            {
                continue;
            }

            std::string name(namePlug.asString().asChar());
            std::string group(groupPlug.asString().asChar());

            if (name.empty())
            {
                continue;
            }

            expressions.setExpressionGroup(name, group);
        }
    }

    return true;
}
