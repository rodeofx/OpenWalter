// Copyright 2017 Rodeo FX. All rights reserved.

#include "walterAbcExportExtensions.h"

#include "AbcWriteJob.h"

#define MNoVersionString

#include <Alembic/AbcMaterial/OMaterial.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/json_parser.hpp>

/**
 * @brief Returns true if the string presents in the given array.
 *
 * @param iArray The given array.
 * @param iMember The string to check.
 *
 * @return True/false.
 */
inline bool presentsIn(const MStringArray& iArray, const MString& iMember)
{
    unsigned int len = iArray.length();
    for (unsigned int i = 0; i < len; i++)
    {
        if (iArray[i] == iMember)
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Save the given parameter to Alembic.
 *
 * @param iName The name of the parameter.
 * @param iParam The parameter as a Json object.
 * @param oProperty The parent Alembic property with all the parameters.
 *
 * @return Return the link name if the parameter is linked.
 */
std::string saveParam(
    const std::string& iName,
    const boost::property_tree::ptree iParam,
    Alembic::Abc::OCompoundProperty& oProperty)
{
    const auto link = iParam.get_optional<std::string>("link");
    if (link)
    {
        return link.get();
    }

    const auto typeOpt = iParam.get_optional<std::string>("type");
    const auto valueOpt = iParam.get_child_optional("value");
    if (!typeOpt || !valueOpt)
    {
        // Can't do anything without type.
        return {};
    }

    const std::string& type = typeOpt.get();
    const auto& value = valueOpt.get();

    if (type == "STRING")
    {
        Alembic::Abc::OStringProperty prop(oProperty, iName);
        prop.set(value.get_value<std::string>());
    }
    else if (type == "BOOL")
    {
        Alembic::Abc::OBoolProperty prop(oProperty, iName);
        prop.set(value.get_value<bool>());
    }
    else if (type == "RGB")
    {
        Alembic::Abc::OC3fProperty color(oProperty, iName);
        Alembic::Abc::C3f colorVal;

        // Get color component by component.
        size_t counter = 0;
        for (const auto& c : value)
        {
            colorVal[counter++] = c.second.get_value<float>();
        }

        color.set(colorVal);
    }
    else if (type == "VECTOR")
    {
        Alembic::Abc::OV3fProperty vector(oProperty, iName);
        Alembic::Abc::V3f vectorVal;

        // Get color component by component.
        size_t counter = 0;
        for (const auto& c : value)
        {
            vectorVal[counter++] = c.second.get_value<float>();
        }

        vector.set(vectorVal);
    }
    else if (type == "FLOAT")
    {
        Alembic::Abc::OFloatProperty prop(oProperty, iName);
        prop.set(value.get_value<float>());
    }
    else if (type == "INT")
    {
        Alembic::Abc::OInt32Property prop(oProperty, iName);
        prop.set(value.get_value<int>());
    }
    else if (type == "BYTE")
    {
        Alembic::Abc::OUcharProperty prop(oProperty, iName);
        prop.set(value.get_value<int>());
    }

    return {};
}

ArnoldLightExporter::ArnoldLightExporter()
{
    MObject plugin = MFnPlugin::findPlugin("walterMtoaConnection");
    mValid = !plugin.isNull();
}

void ArnoldLightExporter::eval(bool isFirst)
{
    if (!isFirst || mLightList.empty())
    {
        return;
    }

    if (!mValid)
    {
        MGlobal::displayError(
            "Can't save Arnold properties because plugin walterMtoaConnection "
            "is not loaded.");
        return;
    }

    // Generate a command to ask walterMtoaConnection to convert the lights.
    // Since walterConvertToArnold opens Arnold session, we need to call it only
    // once with the list of all the lights.
    MString command = "walterConvertToArnold";

    for (MayaLightWriterPtr light : mLightList)
    {
        command += " " + light->mDagPath.partialPathName();
    }

    // The result is Json string with all the lights.
    MString result = MGlobal::executeCommandStringResult(command);

    // Parse it.
    boost::property_tree::ptree tree;
    {
        std::istringstream resultStream(result.asChar());
        boost::property_tree::read_json(resultStream, tree);
    }

    // Check each light separately.
    for (MayaLightWriterPtr light : mLightList)
    {
        std::string lightName = light->mDagPath.partialPathName().asChar();

        const auto& nodes = tree.get_child(lightName);

        if (nodes.empty())
        {
            continue;
        }

        // Save the light information to ".material" schema.
        Alembic::AbcMaterial::OMaterialSchema material(
            light->mSchema.getPtr(), ".material");

        // Set the root node.
        static const std::string target = "arnold";
        material.setNetworkTerminal(target, "light", lightName, "out");

        for (const auto& node : nodes)
        {
            // Process all the nodes.
            const std::string& nodeName = node.first;

            std::string type = node.second.get<std::string>("type");
            if (type.empty())
            {
                continue;
            }

            material.addNetworkNode(nodeName, target, type);

            const auto& params = node.second.get_child("params");

            if (params.empty())
            {
                continue;
            }

            Alembic::Abc::OCompoundProperty property =
                material.getNetworkNodeParameters(nodeName);

            for (const auto& param : params)
            {
                // Process all the parameters.
                const std::string& paramName = param.first;

                // Get name of the linked node. If nothing is linked, this
                // function will create the parameter.
                std::string linked =
                    saveParam(paramName, param.second, property);

                if (linked.empty())
                {
                    // Nothing is linked.
                    continue;
                }

                // This parameter is linked to some node.

                // Split the linked string in case we have components.
                std::vector<std::string> components;
                boost::split(components, linked, boost::is_any_of("."));

                std::string connectedNodeName;
                std::string connectedOutputName;
                if (components.empty())
                {
                    // Boost doesn't bahave like python. If the input doesn't
                    // have '.', the output is empty.
                    connectedNodeName = linked;
                }
                else
                {
                    connectedNodeName = components.front();
                    if (components.size() > 1)
                    {
                        connectedOutputName = components.back();
                    }
                }

                // Save it in Alembic.
                material.setNetworkNodeConnection(
                    nodeName,
                    paramName,
                    connectedNodeName,
                    connectedOutputName);
            }
        }
    }
}

bool ArnoldLightExporter::isLight(const MObject& iObject)
{
    // The list of Maya/Arnold light type.
    // We support all the Maya lights that mtoa supports, cf.
    // https://support.solidangle.com/display/AFMUG/Lights
    static const char* arnoldTypes[] = {"pointLight",
                                        "directionalLight",
                                        "spotLight",
                                        "areaLight",
                                        "aiAreaLight",
                                        "aiLightPortal",
                                        "aiMeshLight",
                                        "aiPhotometricLight",
                                        "aiSkyDomeLight"};
    static const MStringArray arnoldTypesArray(
        arnoldTypes, sizeof(arnoldTypes) / sizeof(arnoldTypes[0]));

    MFnDagNode dagNode(iObject);
    return iObject.hasFn(MFn::kLight) ||
        presentsIn(arnoldTypesArray, dagNode.typeName());
}
