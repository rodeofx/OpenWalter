// Copyright 2017 Rodeo FX.  All rights reserved.

#include "OWalterExpressionsSchema.h"
#include "mayaUtils.h"
#include "walterAssignment.h"

#include <Alembic/Abc/All.h>
#include <Alembic/AbcCoreLayer/Util.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcMaterial/All.h>
#include <maya/MDataHandle.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MSelectionList.h>
#include <unordered_set>

bool saveAttributes(const MString& objectName, const MString& fileName)
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

    // Contain the names of the walterOverride nodes that are already in the
    // Alembic file. We need it to avoid placing the same node several times.
    std::unordered_set<std::string> nodeNameCache;

    // Create Alembic file.
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
    Alembic::Abc::OObject root(archive, Alembic::Abc::kTop);
    Alembic::Abc::OObject materials(root, "materials");

    // Used to mark that an Alembic object or property are meant to be replaced
    // when reading via AbcCoreLayer. Replacing an object or compound property
    // will also replace all of the children encountered and shading network so
    // far.
    Alembic::Abc::MetaData replace;
    Alembic::AbcCoreLayer::SetReplace(replace, true);

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

        MPlug shadersPlug;
        if (!GetChildMPlug(
                currentLayerCompound, "shaderConnections", shadersPlug))
        {
            continue;
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

            // The same for attributes
            MPlug attrPlug;
            if (!GetChildMPlug(shadersCompound, "attribute", attrPlug))
            {
                continue;
            }

            // Get the connected attribute node
            MPlugArray connections;
            if (!attrPlug.connectedTo(connections, true, false) ||
                !connections.length())
            {
                continue;
            }

            const MFnDependencyNode node(connections[0].node());

            std::string nodeName = node.name().asChar();
            if (!nodeNameCache.emplace(nodeName).second)
            {
                // If std::set::emplace successfully inserts the element, the
                // function returns a pair of an iterator and a value of true.
                // If we are here, we already placed walterOverride to the
                // Alembic file.
                continue;
            }

            // TODO: crashes when container.name() is already exported
            Alembic::AbcMaterial::OMaterial matObj(
                materials, nodeName, replace);

            // Setup the material.
            Alembic::AbcMaterial::OMaterialSchema& schema = matObj.getSchema();
            schema.setShader("arnold", "attribute", "walterOverride");

            Alembic::Abc::OCompoundProperty parameters =
                schema.getShaderParameters("arnold", "attribute");

            unsigned int nAttributes = node.attributeCount();
            for (unsigned int a = 0; a < nAttributes; a++)
            {
                MObject object = node.attribute(a);
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

                Alembic::Abc::MetaData meta;
                if (isUserData)
                {
                    // Save that it's a User Data.
                    meta.set("userData", "yes");
                }

                MPlug attrPlug = node.findPlug(object);
                const MDataHandle data = attrPlug.asMDataHandle();

                // Convert Maya type to Alembic type.
                switch (data.numericType())
                {
                    case MFnNumericData::kInvalid:
                    {
                        int v;
                        if (data.type() == MFnData::kString)
                        {
                            // It's a string.
                            Alembic::Abc::OStringProperty property(
                                parameters, name, meta);
                            property.set(data.asString().asChar());
                        }
                        else if (attrPlug.getValue(v) == MS::kSuccess)
                        {
                            // This plug doesn't have data. But we can try
                            // to get an int.
                            Alembic::Abc::OInt32Property property(
                                parameters, name, meta);
                            property.set(v);
                        }
                        break;
                    }

                    case MFnNumericData::kBoolean:
                    {
                        Alembic::Abc::OBoolProperty property(
                            parameters, name, meta);
                        property.set(data.asBool());
                        break;
                    }

                    case MFnNumericData::kShort:
                    case MFnNumericData::kLong:
                    {
                        Alembic::Abc::OInt32Property property(
                            parameters, name, meta);
                        property.set(data.asInt());
                        break;
                    }

                    case MFnNumericData::kFloat:
                    {
                        Alembic::Abc::OFloatProperty property(
                            parameters, name, meta);
                        property.set(data.asFloat());
                        break;
                    }

                    case MFnNumericData::kDouble:
                    {
                        Alembic::Abc::OFloatProperty property(
                            parameters, name, meta);
                        property.set(data.asDouble());
                        break;
                    }

                    case MFnNumericData::k3Float:
                    {
                        Alembic::Abc::OV3fProperty property(
                            parameters, name, meta);
                        const float3& color = data.asFloat3();
                        property.set(Imath::V3f(color[0], color[1], color[2]));
                        break;
                    }

                    default:
                        break;
                }
            }
        }
    }

    return true;
}
