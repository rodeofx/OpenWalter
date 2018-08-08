// Copyright 2017 Rodeo FX. All rights reserved.

#include "AbcCook.h"
#include "ArrayPropUtils.h"
#include "ScalarPropUtils.h"
#include "ArbitraryGeomParamUtils.h"

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnDataBuilder.h>

namespace WalterIn
{
    // When we output attribute "name", we rename the node, and sometimes a node
    // with the same name already exists. Arnold can successfully track it, but
    // it produces a warning.
    static const NameMap shaderAttributes = {
        {"name", "original_name"}};

    void cookMaterial(
        Alembic::AbcMaterial::IMaterialSchema& iSchema,
        AbcCookPtr ioCook,
        FnAttribute::GroupBuilder& oStaticGb)
    {
        // We don't use GroupBuilder for groups because if we call this function
        // twice, we want to add additional nodes to the current oStaticGb. If
        // we use GroupBuilder for "material" or "node" groups, the second call
        // override these groups. ...
        const std::string material = "material.";
        const std::string nodes = material + "nodes.";
        for (size_t i = 0, e = iSchema.getNumNetworkNodes(); i < e; ++i)
        {
            Alembic::AbcMaterial::IMaterialSchema::NetworkNode node =
                iSchema.getNetworkNode(i);

            // Get name, type and target.
            const std::string name(node.getName());

            std::string type = "<undefined>";
            node.getNodeType(type);

            std::string target = "<undefined>";
            node.getTarget(target);

            // ... But it's good to use GroupBuilder here, because we need to
            // override the specific shading node, if we already have it.
            FnAttribute::GroupBuilder nodeGroup;
            nodeGroup.set("name", FnAttribute::StringAttribute(name));
            nodeGroup.set("srcName", FnAttribute::StringAttribute(name));
            nodeGroup.set("type", FnAttribute::StringAttribute(type));
            nodeGroup.set("target", FnAttribute::StringAttribute(target));
 
            // Fill the parameters.
            Alembic::Abc::ICompoundProperty parameters = node.getParameters();
            FnAttribute::GroupBuilder parametersGroup;
            processUserProperties(
                    ioCook, parameters, parametersGroup, "", &shaderAttributes);

            // Fill the connections.
            FnAttribute::GroupBuilder connectionsGroup;
            size_t numConnections = node.getNumConnections();
            for (size_t i = 0; i < numConnections; ++i)
            {
                std::string inputName, connectedNodeName, connectedOutputName;
                if (!node.getConnection(
                            i,
                            inputName,
                            connectedNodeName,
                            connectedOutputName))
                {
                    continue;
                }

                // The name of the connected output should contain "out" to be
                // recognized by ktoa.
                if (!connectedOutputName.empty())
                {
                    connectedOutputName = "." + connectedOutputName;
                }
                connectedOutputName = "out" + connectedOutputName;

                connectionsGroup.set(
                        inputName,
                        FnAttribute::StringAttribute(
                            connectedOutputName + "@" + connectedNodeName));
            }

            nodeGroup.set("parameters", parametersGroup.build());
            if (numConnections)
            {
                nodeGroup.set("connections", connectionsGroup.build());
            }

            oStaticGb.set(nodes + name, nodeGroup.build());
        }

        // Get terminals.
        const std::string terminals = material + "terminals.";
        std::vector<std::string> targetNames;
        iSchema.getNetworkTerminalTargetNames(targetNames);
        for (auto tit = targetNames.begin(); tit != targetNames.end(); tit++)
        {
            const std::string& targetName = *tit;

            std::vector<std::string> shaderTypes;
            iSchema.getNetworkTerminalShaderTypesForTarget(
                    targetName, shaderTypes);

            for (auto sit = shaderTypes.begin(); sit!=shaderTypes.end(); sit++)
            {
                std::string shaderType = *sit;

                std::string connectedNodeName = "<undefined>";
                std::string connectedOutputName = "out";

                if (!iSchema.getNetworkTerminal(
                            targetName,
                            shaderType,
                            connectedNodeName,
                            connectedOutputName))
                {
                    continue;
                }

                // it should be "arnoldSurface". We need to be sure that the
                // second word is capitalized.
                shaderType[0] = toupper(shaderType[0]);

                oStaticGb.set(
                        terminals + targetName + shaderType,
                        FnAttribute::StringAttribute(connectedNodeName));
                oStaticGb.set(
                        terminals + targetName + shaderType + "Port",
                        FnAttribute::StringAttribute(connectedOutputName));
            }
        }

        // Set style.
        oStaticGb.set(
                material + "style",
                FnAttribute::StringAttribute("network"));
    }

    void cookMaterial(AbcCookPtr ioCook, FnAttribute::GroupBuilder& oStaticGb)
    {
        Alembic::AbcMaterial::IMaterialPtr objPtr(
            new Alembic::AbcMaterial::IMaterial(
                *(ioCook->objPtr), Alembic::AbcGeom::kWrapExisting));

        Alembic::AbcMaterial::IMaterialSchema schema = objPtr->getSchema();

        cookMaterial(schema, ioCook, oStaticGb);

        // Set type.
        oStaticGb.set("type", FnAttribute::StringAttribute("material"));
    }

    void cookLight(AbcCookPtr ioCook, FnAttribute::GroupBuilder& oStaticGb)
    {
        Alembic::AbcGeom::ILightPtr objPtr(new Alembic::AbcGeom::ILight(
            *(ioCook->objPtr), Alembic::AbcGeom::kWrapExisting));

        Alembic::AbcGeom::ILightSchema schema = objPtr->getSchema();
        Alembic::AbcMaterial::IMaterialSchema material(
            schema.getPtr(), ".material");

        cookMaterial(material, ioCook, oStaticGb);

        // Set type.
        oStaticGb.set("type", FnAttribute::StringAttribute("light"));
    }

} //end of namespace WalterIn
