// Copyright 2017 Rodeo FX. All rights reserved.

#include "convertToArnold.h"

#include "abcExporterUtils.h"
#include "mtoaScene.h"

#include <json/value.h>
#include <json/writer.h>
#include <maya/MArgDatabase.h>
#include <maya/MSelectionList.h>
#include <maya/MStringArray.h>

/**
 * @brief Converts AtRGB to Json array.
 *
 * @tparam I AtRGB, AtRGBA etc.
 * @param in Input value.
 *
 * @return Json array.
 */
template <class I> inline Json::Value arnToJsonArray(I in)
{
    Json::Value result(Json::arrayValue);

    // It works with Arnold only.
    int size = sizeof(I) / sizeof(float);

    result.resize(size);

    for (int i = 0; i < size; i++)
    {
        result[i] = in[i];
    }

    return result;
}

/**
 * @brief Return Json representation of given Arnold parameter.
 *
 * @param node The node of the parameter.
 * @param param Given parameter.
 *
 * @return Json object.
 */
Json::Value arnParamToJson(const AtNode* node, const AtParamEntry* param)
{
    Json::Value result(Json::objectValue);

    // Parameter name.
    const char* name = AiParamGetName(param);

    // Parameter type.
    int type = AiParamGetType(param);

    if (AiNodeIsLinked(node, name))
    {
        // There is no value because this param is linked to another node. We
        // only need the name of the linked node.
        int comp;
        AtNode* linked = AiNodeGetLink(node, name, &comp);

        std::string linkName = AiNodeGetName(linked);

        if (type == AI_TYPE_RGB || type == AI_TYPE_RGBA)
        {
            switch (comp)
            {
                // if comp >= 0, then the param is linked to a component of the
                // output.
                case 0:
                    linkName += ".r";
                    break;
                case 1:
                    linkName += ".g";
                    break;
                case 2:
                    linkName += ".b";
                    break;
                case 3:
                    linkName += ".a";
                    break;
            }
        }
        else if (
            type == AI_TYPE_VECTOR || type == AI_TYPE_VECTOR ||
            type == AI_TYPE_VECTOR2)
        {
            switch (comp)
            {
                case 0:
                    linkName += ".x";
                    break;
                case 1:
                    linkName += ".y";
                    break;
                case 2:
                    linkName += ".z";
                    break;
            }
        }

        result["link"] = linkName;
    }
    else
    {
        // Get the value.
        switch (type)
        {
            case AI_TYPE_BYTE:
                result["value"] = AiNodeGetByte(node, name);
                break;
            case AI_TYPE_INT:
                result["value"] = AiNodeGetInt(node, name);
                break;
            case AI_TYPE_UINT:
                result["value"] = AiNodeGetUInt(node, name);
                break;
            case AI_TYPE_BOOLEAN:
                result["value"] = AiNodeGetBool(node, name);
                break;
            case AI_TYPE_FLOAT:
                result["value"] = AiNodeGetFlt(node, name);
                break;
            case AI_TYPE_RGB:
                result["value"] = arnToJsonArray(AiNodeGetRGB(node, name));
                break;
            case AI_TYPE_RGBA:
                result["value"] = arnToJsonArray(AiNodeGetRGBA(node, name));
                break;
            case AI_TYPE_VECTOR:
                result["value"] = arnToJsonArray(AiNodeGetVec(node, name));
                break;
            case AI_TYPE_VECTOR2:
                result["value"] = arnToJsonArray(AiNodeGetVec2(node, name));
                break;
            case AI_TYPE_STRING:
                result["value"] = std::string(AiNodeGetStr(node, name));
                break;
            default:
                // Can't detect type.
                return result;
        }
    }

    // Save the type.
    const char* typeName = AiParamGetTypeName(type);
    result["type"] = typeName;

    return result;
}

/**
 * @brief Returns Json object with all the parameters of given Arnold node.
 *
 * @param node Arnold node.
 *
 * @return Json object.
 */
Json::Value arnNodeToJson(const AtNode* node)
{
    Json::Value result(Json::objectValue);

    assert(node);

    const AtNodeEntry* entry = AiNodeGetNodeEntry(node);

    Json::Value params;

    // Iterate all the paramters
    AtParamIterator* nodeParam = AiNodeEntryGetParamIterator(entry);
    while (!AiParamIteratorFinished(nodeParam))
    {
        const AtParamEntry* paramEntry = AiParamIteratorGetNext(nodeParam);
        const char* paramName = AiParamGetName(paramEntry);

        Json::Value param = arnParamToJson(node, paramEntry);
        if (!param.empty())
        {
            // We only need to keep a valid paramter.
            params[paramName] = param;
        }
    }

    AiParamIteratorDestroy(nodeParam);

    if (!params.empty())
    {
        // We only need to keep a valid paramter.
        result["params"] = params;
    }

    result["type"] = AiNodeEntryGetName(entry);

    return result;
}

MSyntax ConvertToArnoldCmd::syntax()
{
    MSyntax syntax;

    syntax.setObjectType(MSyntax::kStringObjects);

    return syntax;
}

MStatus ConvertToArnoldCmd::doIt(const MArgList& args)
{
    MArgDatabase argData(syntax(), args);

    MStringArray objects;
    argData.getObjects(objects);

    Json::Value result(Json::objectValue);

    MTOAScene scene;

    for (unsigned int i = 0; i < objects.length(); i++)
    {
        const MString& currentObject = objects[i];

        // We need a dependency node first. It's the old trick to get it.
        MObject obj;
        MSelectionList sel;
        sel.add(currentObject);

        if (sel.isEmpty())
        {
            continue;
        }

        sel.getDependNode(0, obj);
        MFnDependencyNode depFn(obj);

        // MTOA needs a plug.
        MPlug plug = depFn.findPlug("message");

        // Export nodes to Arnold using MTOA
        AtNodeSet roots;
        CNodeTranslator* translator = scene.exportNode(plug, &roots);
        if (!translator)
        {
            continue;
        }

        AtNode* translatedNode = translator->GetArnoldNode();
        if (!translatedNode)
        {
            continue;
        }

        // We got it. Now we need to convert the Arnold data to string.
        // Traverse the tree to get all the connected nodes.
        AtNodeSet relatedNodes;
        relatedNodes.insert(translatedNode);
        getAllArnoldNodes(translatedNode, &relatedNodes);

        std::string rootName = AiNodeGetName(translatedNode);

        Json::Value nodes(Json::objectValue);

        for (const AtNode* current : relatedNodes)
        {
            std::string currentName = AiNodeGetName(current);
            Json::Value node = arnNodeToJson(current);

            if (!node.empty())
            {
                // We only need to keep valid nodes.
                nodes[currentName] = node;
            }
        }

        if (!nodes.empty())
        {
            result[currentObject.asChar()] = nodes;
        }
    }

    MString resultStr = Json::FastWriter().write(result).c_str();
    MPxCommand::setResult(resultStr);

    return MStatus::kSuccess;
}
