// Copyright 2017 Rodeo FX.  All rights reserved.

#include "engine.h"

#include <ai.h>
#include <pxr/usd/sdf/path.h>
#include <cstring>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

struct ProceduralData
{
    ProceduralData(
        const std::string& dso,
        const std::string& prefix,
        const std::string& filePaths,
        RendererEngine* engine,
        const SdfPath& path) :
            mData{dso, prefix, filePaths},
            mEngine(engine),
            mPath(path)
    {}

    RendererPluginData mData;
    RendererEngine* mEngine;
    SdfPath mPath;
    std::vector<float> mTimes;
};

const char* AiNodeLookUpAndGetStr(AtNode* node, const char* param)
{
    if (AiNodeLookUpUserParameter(node, param))
    {
        return AiNodeGetStr(node, param);
    }

    return nullptr;
}

static int arnoldProceduralInit(AtNode* node, void** user_ptr)
{
// Get parameters
// We can keep this logic check for future
    const char* dso = "\n";
    const char* file = AiNodeGetStr(node, "filePaths");
    const char* object = AiNodeGetStr(node, "objectPath");
    const char* sessionLayer = AiNodeGetStr(node, "sessionLayer");
    const char* variantsLayer = AiNodeGetStr(node, "variantsLayer");
    const char* purposeLayer = AiNodeGetStr(node, "purposeLayer");
    const char* mayaStateLayer = AiNodeGetStr(node, "mayaStateLayer");
    const char* visibilityLayer = AiNodeGetStr(node, "visibilityLayer");

    std::vector<std::string> overrides;
    for (const char* o : {sessionLayer, variantsLayer, mayaStateLayer, visibilityLayer, purposeLayer})
    {
        if (o && o[0] != '\0')
        {
            overrides.push_back(o);
        }
    }

    // Get USD stuff
    RendererEngine* engine = RendererEngine::getInstance(file, overrides);

    SdfPath path;
    if(std::string(object) != "")
    {
        path = SdfPath(object);
    }
    else
    {
        AiMsgWarning(
            "[RodeoFX]: Object path is empty in Walter USD Procedural! "
            " Use '/' if you want to load the whole stage.");
    }

    // Get the prefix. We need it because we produce a lot of objects. Each
    // of them should have the unique name. The idea that the root object
    // doesn't have a prefix attribute so that we can use the name as a
    // prefix. All the children should have the prefix because we generate
    // it.
    const char* prefix;
    const char* prefixName = "prefix";
    const AtUserParamEntry* prefixEntry =
        AiNodeLookUpUserParameter(node, prefixName);
    if (prefixEntry)
    {
        prefix = AiNodeGetStr(node, prefixName);
    }
    else
    {
        prefix = AiNodeGetName(node);
    }

    ProceduralData* data = new ProceduralData(dso, prefix, file, engine, path);

    // Get times
    const char* timeName = "frame";
    const AtUserParamEntry* timeEntry =
        AiNodeLookUpUserParameter(node, timeName);
    if (timeEntry)
    {
        int type = AiUserParamGetType(timeEntry);
        switch (type)
        {
            case AI_TYPE_FLOAT:
                data->mTimes.push_back(AiNodeGetFlt(node, timeName));
                break;
            case AI_TYPE_ARRAY:
            {
                AtArray* array = AiNodeGetArray(node, timeName);
                int keys = AiArrayGetNumElements(array) *
                    AiArrayGetNumKeys(array);
                data->mTimes.reserve(keys);
                for (int i = 0; i < keys; i++)
                {
                    data->mTimes.push_back(AiArrayGetFlt(array, i));
                }
            }
            break;
            default:
                break;
        }
    }
    if (data->mTimes.empty())
    {
        data->mTimes.push_back(1.0f);
    }

    // It's possible that motion_start/motion_end are not there if motion blur
    // is disabled.
    const AtNodeEntry* entry = AiNodeGetNodeEntry(node);
    if (AiNodeEntryLookUpParameter(entry, "motion_start"))
    {
        data->mData.motionStart = AiNodeGetFlt(node, "motion_start");
    }
    if (AiNodeEntryLookUpParameter(entry, "motion_end"))
    {
        data->mData.motionEnd = AiNodeGetFlt(node, "motion_end");
    }

    // Save it for future
    *user_ptr = data;

    return 1;
}

static int arnoldProceduralCleanup(void* user_ptr)
{
    ProceduralData* data = reinterpret_cast<ProceduralData*>(user_ptr);
    delete data;

    return 1;
}

static int arnoldProceduralNumNodes(void* user_ptr)
{
    ProceduralData* data = reinterpret_cast<ProceduralData*>(user_ptr);
    return data->mEngine->getNumNodes(data->mPath, data->mTimes);
}

static AtNode* arnoldProceduralGetNode(void* user_ptr, int i)
{
    ProceduralData* data = reinterpret_cast<ProceduralData*>(user_ptr);
    void* result =
        data->mEngine->render(data->mPath, i, data->mTimes, &data->mData);
    return reinterpret_cast<AtNode*>(result);
}

bool arnoldProceduralInitPlugin(void** plugin_user_ptr)
{
    AiMsgInfo(
        "[RodeoFX] Walter USD Procedural %s.%s.%s, built on %s at %s",
        WALTER_MAJOR_VERSION,
        WALTER_MINOR_VERSION,
        WALTER_PATCH_VERSION,
        __DATE__,
        __TIME__);

    if (!AiCheckAPIVersion(AI_VERSION_ARCH, AI_VERSION_MAJOR, AI_VERSION_MINOR))
    {
        AiMsgWarning(
            "[RodeoFX]: Walter USD Procedural is compiled with "
            "Arnold %s but is running with %s",
            AI_VERSION,
            AiGetVersion(nullptr, nullptr, nullptr, nullptr));
    }

    *plugin_user_ptr = nullptr;
    return true;
}

bool arnoldProceduralCleanupPlugin(void* plugin_user_ptr)
{
    RendererEngine::clearCaches();
    return true;
}

AI_PROCEDURAL_NODE_EXPORT_METHODS(WalterMtd);

node_parameters
{
    AiParameterStr("filePaths", "");
    AiParameterStr("objectPath", "");
    AiParameterStr("sessionLayer", "");
    AiParameterStr("variantsLayer", "");
    AiParameterStr("purposeLayer", "");
    AiParameterStr("mayaStateLayer", "");
    AiParameterStr("visibilityLayer", "");
}

node_plugin_initialize
{
    arnoldProceduralInitPlugin(plugin_data);
}
node_plugin_cleanup
{
    arnoldProceduralCleanupPlugin(plugin_data);
}

procedural_init
{
    return arnoldProceduralInit(node, user_ptr);
}
procedural_cleanup
{
    return arnoldProceduralCleanup(user_ptr);
}
procedural_num_nodes
{
    return arnoldProceduralNumNodes(user_ptr);
}
procedural_get_node
{
    return arnoldProceduralGetNode(user_ptr, i);
}

node_loader
{
    if (i > 0)
        return false;

    node->methods = WalterMtd;
    node->output_type = AI_TYPE_NONE;
    node->name = "walter";
    node->node_type = AI_NODE_SHAPE_PROCEDURAL;
    strcpy(node->version, AI_VERSION);
    return true;
}