// Copyright 2018 Rodeo FX.  All rights reserved.

#include "arnoldTranslator.h"

#include "arnoldApi.h"
#include "schemas/volume.h"

#include <pxr/base/gf/matrix4f.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/shader.h>
#include <boost/algorithm/string.hpp>
#include <boost/preprocessor/array/elem.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/range/algorithm/replace_copy_if.hpp>

PXR_NAMESPACE_OPEN_SCOPE

namespace ArnoldUSDTranslatorImpl
{
static const TfToken sOut("out");

// TODO:
// AI_TYPE_POINTER
// AI_TYPE_NODE
// AI_TYPE_USHORT
// AI_TYPE_HALF
// AI_TYPE_UNDEFINED
// AI_TYPE_NONE
#define AI_CONVERSION_TABLE \
    ((2, (AI_TYPE_BYTE, UChar)))((2, (AI_TYPE_INT, Int)))( \
        (2, (AI_TYPE_UINT, UInt)))((2, (AI_TYPE_BOOLEAN, Bool)))( \
        (2, (AI_TYPE_FLOAT, Float)))((2, (AI_TYPE_RGB, Color3f)))( \
        (2, (AI_TYPE_VECTOR, Vector3f)))((2, (AI_TYPE_VECTOR2, Float2)))( \
        (2, (AI_TYPE_STRING, String)))((2, (AI_TYPE_MATRIX, Matrix4d)))( \
        (2, (AI_TYPE_ENUM, Int)))((2, (AI_TYPE_CLOSURE, Color3f)))( \
        (2, (AI_TYPE_RGBA, Color4f)))

#define AI_CONVERSION_TEMPLATE(r, data, arg) \
    case BOOST_PP_ARRAY_ELEM(0, arg): \
        data = SdfValueTypeNames->BOOST_PP_ARRAY_ELEM(1, arg); \
        break;

#define AI_CONVERSION_ARRAY_TEMPLATE(r, data, arg) \
    case BOOST_PP_ARRAY_ELEM(0, arg): \
        data = SdfValueTypeNames->BOOST_PP_CAT( \
            BOOST_PP_ARRAY_ELEM(1, arg), Array); \
        break;

SdfValueTypeName aiToUsdParamType(uint8_t iArnoldType, bool iIsArray = false)
{
    SdfValueTypeName usdType;

    if (iIsArray)
    {
        switch (iArnoldType)
        {
            BOOST_PP_SEQ_FOR_EACH(
                AI_CONVERSION_ARRAY_TEMPLATE, usdType, AI_CONVERSION_TABLE);
        }
    }
    else
    {
        switch (iArnoldType)
        {
            BOOST_PP_SEQ_FOR_EACH(
                AI_CONVERSION_TEMPLATE, usdType, AI_CONVERSION_TABLE);
        }
    }

    return usdType;
}

#undef AI_CONVERSION_ARRAY_TEMPLATE
#undef AI_CONVERSION_TEMPLATE
#undef AI_CONVERSION_TABLE

void loadArnoldPlugins(ArnoldAPIPtr ioAi)
{
    char* envChar = std::getenv("ARNOLD_PLUGIN_PATH");
    std::string envVar = envChar ? envChar : "";
    std::vector<std::string> envPath;
    boost::split(envPath, envVar, boost::is_any_of(":"));

    envChar = std::getenv("ARNOLD_SHADERLIB_PATH");
    envVar = envChar ? envChar : "";
    std::vector<std::string> envPath2;
    boost::split(envPath2, envVar, boost::is_any_of(":"));

    envPath.insert(envPath.end(), envPath2.begin(), envPath2.end());

    for (auto p : envPath)
    {
        ioAi->AiLoadPlugins(p.c_str());
    }
}

std::string getGoodUSDName(const std::string& iName)
{
    std::string processedString;
    boost::replace_copy_if(
        iName, std::back_inserter(processedString), boost::is_any_of(":"), '_');
    return processedString;
}

UsdShadeShader createUSDShader(
    UsdStageRefPtr ioStage,
    ArnoldAPIPtr ioAi,
    const SdfPath& iPath,
    const AtNode* iNode)
{
    SdfPath path(getGoodUSDName(ioAi->AiNodeGetName(iNode)));
    path = iPath.AppendChild(path.GetNameToken());

    UsdShadeShader shader = UsdShadeShader::Define(ioStage, path);
    UsdPrim prim = shader.GetPrim();

    const AtNodeEntry* entry = ioAi->AiNodeGetNodeEntry(iNode);
    AtParamIterator* paramIterator = ioAi->AiNodeEntryGetParamIterator(entry);

    // Create the type and the target.
    // TODO: it should be a part of UsdShadeShader object but I didn't find it.
    static const TfToken infoType("info:type");
    UsdAttribute infoTypeAttrib = prim.CreateAttribute(
        infoType, SdfValueTypeNames->Token, false, SdfVariabilityVarying);
    infoTypeAttrib.Set(TfToken(ioAi->AiNodeEntryGetName(entry)));

    static const TfToken infoTarget("info:target");
    static const TfToken arnold("arnold");
    UsdAttribute infoTargetAttrib = prim.CreateAttribute(
        infoTarget, SdfValueTypeNames->Token, false, SdfVariabilityVarying);
    infoTargetAttrib.Set(arnold);

    // Iterate parameters.
    while (!ioAi->AiParamIteratorFinished(paramIterator))
    {
        const AtParamEntry* paramEntry =
            ioAi->AiParamIteratorGetNext(paramIterator);
        AtArray* paramArray = nullptr;
        AtString paramName = ioAi->AiParamGetName(paramEntry);
        const TfToken usdParamName(paramName.c_str());

        // Get the type
        uint8_t paramType = ioAi->AiParamGetType(paramEntry);
        SdfValueTypeName usdParamType;
        if (paramType == AI_TYPE_ARRAY)
        {
            paramArray = ioAi->AiNodeGetArray(iNode, paramName);
            uint8_t arrayType = ioAi->AiArrayGetType(paramArray);
            usdParamType = aiToUsdParamType(arrayType, true);
        }
        else
        {
            usdParamType = aiToUsdParamType(paramType);
        }

        if (!usdParamType)
        {
            continue;
        }

        UsdAttribute attrib = prim.CreateAttribute(
            usdParamName, usdParamType, false, SdfVariabilityVarying);

        if (ioAi->AiNodeIsLinked(iNode, paramName.c_str()))
        {
            AtNode* linkedNode =
                ioAi->AiNodeGetLink(iNode, paramName.c_str(), nullptr);
            // TODO: Check if already created.
            UsdShadeShader linkedShader =
                createUSDShader(ioStage, ioAi, iPath, linkedNode);

            UsdShadeConnectableAPI::ConnectToSource(
                attrib, linkedShader.GetPrim().GetPath().AppendProperty(sOut));
        }
        else
        {
            // TODO: It could be a great template
            if (usdParamType == SdfValueTypeNames->UChar)
            {
                int value = ioAi->AiNodeGetInt(iNode, paramName);
                attrib.Set(VtValue(value));
            }
            else if (usdParamType == SdfValueTypeNames->Int)
            {
                int value = ioAi->AiNodeGetInt(iNode, paramName);
                attrib.Set(VtValue(value));
            }
            else if (usdParamType == SdfValueTypeNames->UInt)
            {
                unsigned int value = ioAi->AiNodeGetUInt(iNode, paramName);
                attrib.Set(VtValue(value));
            }
            else if (usdParamType == SdfValueTypeNames->Bool)
            {
                bool value = ioAi->AiNodeGetBool(iNode, paramName);
                attrib.Set(VtValue(value));
            }
            else if (usdParamType == SdfValueTypeNames->Float)
            {
                float value = ioAi->AiNodeGetFlt(iNode, paramName);
                attrib.Set(VtValue(value));
            }
            else if (usdParamType == SdfValueTypeNames->Color3f)
            {
                AtRGB value = ioAi->AiNodeGetRGB(iNode, paramName);
                attrib.Set(VtValue(GfVec3f(value.r, value.g, value.b)));
            }
            else if (usdParamType == SdfValueTypeNames->Color4f)
            {
                AtRGBA value = ioAi->AiNodeGetRGBA(iNode, paramName);
                attrib.Set(
                    VtValue(GfVec4f(value.r, value.g, value.b, value.a)));
            }
            else if (usdParamType == SdfValueTypeNames->Vector3f)
            {
                AtVector value = ioAi->AiNodeGetVec(iNode, paramName);
                attrib.Set(VtValue(GfVec3f(value.x, value.y, value.z)));
            }
            else if (usdParamType == SdfValueTypeNames->Float2)
            {
                AtVector2 value = ioAi->AiNodeGetVec2(iNode, paramName);
                attrib.Set(VtValue(GfVec2f(value.x, value.y)));
            }
            else if (usdParamType == SdfValueTypeNames->String)
            {
                AtString value = ioAi->AiNodeGetStr(iNode, paramName);
                attrib.Set(VtValue(value.c_str()));
            }
            else if (usdParamType == SdfValueTypeNames->Matrix4d)
            {
                AtMatrix matrix = ioAi->AiNodeGetMatrix(iNode, paramName);
                GfMatrix4f gfMatrix(matrix.data);
                attrib.Set(VtValue(gfMatrix));
            }
            else if (usdParamType == SdfValueTypeNames->Color3fArray)
            {
                uint32_t nElements = ioAi->AiArrayGetNumElements(paramArray);
                VtVec3fArray array(nElements);
                for (uint32_t i = 0; i < nElements; i++)
                {
                    AtRGB value = ioAi->AiArrayGetRGBFunc(
                        paramArray, i, __AI_FILE__, __AI_LINE__);
                    array[i] = GfVec3f(value.r, value.g, value.b);
                }
                attrib.Set(array);
            }
            else if (usdParamType == SdfValueTypeNames->FloatArray)
            {
                uint32_t nElements = ioAi->AiArrayGetNumElements(paramArray);
                VtFloatArray array(nElements);
                for (uint32_t i = 0; i < nElements; i++)
                {
                    float value = ioAi->AiArrayGetFltFunc(
                        paramArray, i, __AI_FILE__, __AI_LINE__);
                    array[i] = value;
                }
                attrib.Set(array);
            }
            else if (usdParamType == SdfValueTypeNames->IntArray)
            {
                uint32_t nElements = ioAi->AiArrayGetNumElements(paramArray);
                VtIntArray array(nElements);
                for (uint32_t i = 0; i < nElements; i++)
                {
                    int value = ioAi->AiArrayGetIntFunc(
                        paramArray, i, __AI_FILE__, __AI_LINE__);
                    array[i] = value;
                }
                attrib.Set(array);
            }
            else if (usdParamType == SdfValueTypeNames->UCharArray)
            {
                uint32_t nElements = ioAi->AiArrayGetNumElements(paramArray);
                VtUCharArray array(nElements);
                for (uint32_t i = 0; i < nElements; i++)
                {
                    uint8_t value = ioAi->AiArrayGetByteFunc(
                        paramArray, i, __AI_FILE__, __AI_LINE__);
                    array[i] = value;
                }
                attrib.Set(array);
            }
            else if (usdParamType == SdfValueTypeNames->StringArray)
            {
                uint32_t nElements = ioAi->AiArrayGetNumElements(paramArray);
                VtStringArray array(nElements);
                for (uint32_t i = 0; i < nElements; i++)
                {
                    AtString value = ioAi->AiArrayGetStrFunc(
                        paramArray, i, __AI_FILE__, __AI_LINE__);
                    array[i] = value;
                }
                attrib.Set(array);

                AtString value = ioAi->AiNodeGetStr(iNode, paramName);
                attrib.Set(VtValue(value.c_str()));
            }
        }
    }
    ioAi->AiParamIteratorDestroy(paramIterator);

    return shader;
}

UsdShadeMaterial createUSDMaterial(
    UsdStageRefPtr ioStage,
    ArnoldAPIPtr ioAi,
    const SdfPath& iPath,
    const AtNode* iNode)
{
    UsdShadeMaterial mat = UsdShadeMaterial::Define(ioStage, iPath);

    UsdShadeShader rootShader = createUSDShader(ioStage, ioAi, iPath, iNode);

    // Connect rootShader to mat.
    // TODO: We need to detect if it's a surface or displacement
    static const TfToken arnoldSurface("arnold:surface");
    UsdAttribute attrib = mat.GetPrim().CreateAttribute(
        arnoldSurface, SdfValueTypeNames->Float3, false, SdfVariabilityVarying);
    UsdShadeConnectableAPI::ConnectToSource(
        attrib, rootShader.GetPrim().GetPath().AppendProperty(sOut));

    return mat;
}

WalterVolume createUSDVolume(
    UsdStageRefPtr ioStage,
    ArnoldAPIPtr ioAi,
    const SdfPath& iPath,
    const AtNode* iNode)
{
    WalterVolume volume = WalterVolume::Define(ioStage, iPath);

    const AtNodeEntry* entry = ioAi->AiNodeGetNodeEntry(iNode);

    // It's so complicated with iterating parameters because we can't create
    // AtString without linking to libai.so, and it means we can't query
    // parameters by name. The only way is iterating.
    AtParamIterator* paramIterator = ioAi->AiNodeEntryGetParamIterator(entry);
    // Iterate parameters.
    while (!ioAi->AiParamIteratorFinished(paramIterator))
    {
        const AtParamEntry* paramEntry =
            ioAi->AiParamIteratorGetNext(paramIterator);
        AtString paramName = ioAi->AiParamGetName(paramEntry);
        std::string name(paramName);

        if (name == "matrix")
        {
            AtMatrix matrix = ioAi->AiNodeGetMatrix(iNode, paramName);
            // Convert it to double
            GfMatrix4d gfMatrix(GfMatrix4f(matrix.data));
            volume.AddXformOp(UsdGeomXformOp::TypeTransform).Set(gfMatrix);
        }
        else if (name == "volume_padding")
        {
            float value = ioAi->AiNodeGetFlt(iNode, paramName);
            volume.CreateVolumePaddingAttr().Set(value);
        }
        else if (name == "step_size")
        {
            float value = ioAi->AiNodeGetFlt(iNode, paramName);
            volume.CreateStepSizeAttr().Set(value);
        }
        else if (name == "filename")
        {
            std::string value(ioAi->AiNodeGetStr(iNode, paramName));
            volume.CreateFilenameAttr().Set(value);
        }
        else if (name == "filedata")
        {
            AtArray* paramArray = ioAi->AiNodeGetArray(iNode, paramName);
            uint32_t nElements = ioAi->AiArrayGetNumElements(paramArray);
            VtUCharArray array(nElements);
            for (uint32_t i = 0; i < nElements; i++)
            {
                uint8_t value = ioAi->AiArrayGetByteFunc(
                    paramArray, i, __AI_FILE__, __AI_LINE__);
                array[i] = value;
            }
            volume.CreateFiledataAttr().Set(array);
        }
        else if (name == "step_scale")
        {
            float value = ioAi->AiNodeGetFlt(iNode, paramName);
            volume.CreateStepScaleAttr().Set(value);
        }
        else if (name == "compress")
        {
            bool value = ioAi->AiNodeGetBool(iNode, paramName);
            volume.CreateCompressAttr().Set(value);
        }
        else if (name == "grids")
        {
            if(ioAi->AiParamGetType(paramEntry) == AI_TYPE_ARRAY)
            {
                AtArray* paramArray = ioAi->AiNodeGetArray(iNode, paramName);
                uint32_t nElements = ioAi->AiArrayGetNumElements(paramArray);
                VtStringArray array(nElements);
                for (uint32_t i = 0; i < nElements; i++)
                {
                    AtString value = ioAi->AiArrayGetStrFunc(
                            paramArray, i, __AI_FILE__, __AI_LINE__);
                    array[i] = value;
                }
                volume.CreateGridsAttr().Set(array);
            }
            else
            {
                VtStringArray array(1);
                std::string value(ioAi->AiNodeGetStr(iNode, paramName));
                array[0] = value;
                volume.CreateGridsAttr().Set(array);
            }
        }
        else if (name == "velocity_scale")
        {
            float value = ioAi->AiNodeGetFlt(iNode, paramName);
            volume.CreateVelocityScaleAttr().Set(value);
        }
        else if (name == "velocity_fps")
        {
            float value = ioAi->AiNodeGetFlt(iNode, paramName);
            volume.CreateVelocityFpsAttr().Set(value);
        }
        else if (name == "velocity_outlier_threshold")
        {
            float value = ioAi->AiNodeGetFlt(iNode, paramName);
            volume.CreateVelocityOutlierThresholdAttr().Set(value);
        }
        else if (name == "shader")
        {
            // Bind material to this prim.
            AtNode* shader = (AtNode*)ioAi->AiNodeGetPtr(iNode, paramName);
            if (!shader)
            {
                continue;
            }

            // Generate the path to the shader.
            SdfPath shaderPath(getGoodUSDName(ioAi->AiNodeGetName(shader)));

            // Connect the path and the prim.
            UsdRelationship shaderRel = volume.GetPrim().CreateRelationship(
                UsdShadeTokens->materialBinding, false);
            SdfPathVector targets(1, shaderPath);
            shaderRel.SetTargets(targets);
        }
    }
    ioAi->AiParamIteratorDestroy(paramIterator);

    return volume;
}

bool fillUSDStage(
    UsdStageRefPtr ioStage,
    ArnoldAPIPtr ioAi,
    const std::string& iFile)
{
    // Since it's possible that we didn't create Arnold context, we need to get
    // the current nodes.
    std::set<AtNode*> nodesBeforeLoad;
    AtNodeIterator* nodeIterator = ioAi->AiUniverseGetNodeIterator(AI_NODE_ALL);
    while (!ioAi->AiNodeIteratorFinished(nodeIterator))
    {
        AtNode* node = ioAi->AiNodeIteratorGetNext(nodeIterator);
        nodesBeforeLoad.insert(node);
    }
    ioAi->AiNodeIteratorDestroy(nodeIterator);

    if (ioAi->AiASSLoad(iFile.c_str(), AI_NODE_SHAPE | AI_NODE_SHADER) != 0)
    {
        return false;
    }

    // There is no API to get nodes that was loaded with AiASSLoad. To get them
    // we need to get all the nodes and find the difference.
    std::set<AtNode*> nodesAfterLoad;
    nodeIterator = ioAi->AiUniverseGetNodeIterator(AI_NODE_ALL);
    while (!ioAi->AiNodeIteratorFinished(nodeIterator))
    {
        AtNode* node = ioAi->AiNodeIteratorGetNext(nodeIterator);
        nodesAfterLoad.insert(node);
    }
    ioAi->AiNodeIteratorDestroy(nodeIterator);

    std::set<AtNode*> newNodes;
    std::set_difference(
        nodesAfterLoad.begin(),
        nodesAfterLoad.end(),
        nodesBeforeLoad.begin(),
        nodesBeforeLoad.end(),
        std::inserter(newNodes, newNodes.begin()));

    // Arnold doesn't have inheritance and all the nodes are flat there. In USD
    // it's a good practice to group the shading nodes into materials. To do it
    // we need to know what shaders are "roots". The following code tries to
    // recognize it. Grouping nodes is a classic linked list problem, we
    // simplified it a little bit. We save the connected nodes on the first pass
    // and output the nodes with no connection on the second pass.
    std::set<AtNode*> nodesWithOutput;
    for (const AtNode* node : newNodes)
    {
        const AtNodeEntry* entry = ioAi->AiNodeGetNodeEntry(node);
        int type = ioAi->AiNodeEntryGetType(entry);

        if (type == AI_NODE_SHADER)
        {
            AtParamIterator* paramIterator =
                ioAi->AiNodeEntryGetParamIterator(entry);

            while (!ioAi->AiParamIteratorFinished(paramIterator))
            {
                const AtParamEntry* paramEntry =
                    ioAi->AiParamIteratorGetNext(paramIterator);
                AtString paramName = ioAi->AiParamGetName(paramEntry);
                if (ioAi->AiNodeIsLinked(node, paramName.c_str()))
                {
                    nodesWithOutput.insert(
                        ioAi->AiNodeGetLink(node, paramName.c_str(), nullptr));
                    break;
                }
            }
            ioAi->AiParamIteratorDestroy(paramIterator);
        }
    }

    // Output the nodes
    for (AtNode* node : newNodes)
    {
        const AtNodeEntry* entry = ioAi->AiNodeGetNodeEntry(node);
        // Type is "shader", "shape" etc.
        int type = ioAi->AiNodeEntryGetType(entry);

        if (type == AI_NODE_SHAPE)
        {
            // Nodetype is "volume", "options" etc.
            const std::string nodeType(ioAi->AiNodeEntryGetName(entry));
            if (nodeType != "volume")
            {
                continue;
            }

            SdfPath path(getGoodUSDName(ioAi->AiNodeGetName(node)));
            createUSDVolume(ioStage, ioAi, path, node);
        }
        else if (type == AI_NODE_SHADER)
        {
            if (nodesWithOutput.find(node) != nodesWithOutput.end())
            {
                continue;
            }

            SdfPath path(getGoodUSDName(ioAi->AiNodeGetName(node)));
            createUSDMaterial(ioStage, ioAi, path, node);
        }
    }

    // Delete nodes that we just loaded. We don't need them anymore in the
    // Arnold context.
    for (AtNode* node : newNodes)
    {
        // AiNodeDestroy crashes. AiNodeReset removes all the parameters and
        // Arnold will ignore such node which is not bad in our case.
        ioAi->AiNodeReset(node);
    }

    return true;
}
}

SdfLayerRefPtr ArnoldUSDTranslator::arnoldToUsdLayer(const std::string& iFile)
{
    // Create the layer to populate.
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");

    ArnoldAPIPtr ai = ArnoldAPI::create();
    if (!ai)
    {
        return layer;
    }

    // Create a UsdStage with that root layer.
    UsdStageRefPtr stage = UsdStage::Open(layer);
    stage->SetEditTarget(layer);

    bool sessionCreated = false;
    if (!ai->AiUniverseIsActive())
    {
        ai->AiBegin();
        ArnoldUSDTranslatorImpl::loadArnoldPlugins(ai);
        sessionCreated = true;
    }

    // Generate the stage.
    ArnoldUSDTranslatorImpl::fillUSDStage(stage, ai, iFile);

    if (sessionCreated)
    {
        ai->AiEnd();
    }

    return layer;
}

PXR_NAMESPACE_CLOSE_SCOPE
