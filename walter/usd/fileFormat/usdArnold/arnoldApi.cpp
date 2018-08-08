// Copyright 2018 Rodeo FX.  All rights reserved.

#include "arnoldApi.h"

#include <dlfcn.h>

static const char* sLibAi = "libai.so";

#define INIT_AI_SYMBOL(A) \
    A = (A##FuncPtr)dlsym(mArnoldDSO, #A); \
    if (!A) \
    { \
        printf("[WALTER ERROR] Can't load symbol " #A "\n"); \
    }

ArnoldAPI::ArnoldAPI() : mArnoldDSO(dlopen(sLibAi, RTLD_NOW))
{
    if (!mArnoldDSO)
    {
        printf("[WALTER ERROR] Can't load library %s.\n", sLibAi);
        return;
    }

    INIT_AI_SYMBOL(AiGetVersion);
    // Save the version
    char arch[16];
    AiGetVersion(arch, nullptr, nullptr, nullptr);
    mArch = atoi(arch);

    // Load the symbols
    INIT_AI_SYMBOL(AiASSLoad);
    INIT_AI_SYMBOL(AiBegin);
    INIT_AI_SYMBOL(AiEnd);
    INIT_AI_SYMBOL(AiLoadPlugins);
    INIT_AI_SYMBOL(AiNodeEntryGetName);
    INIT_AI_SYMBOL(AiNodeEntryGetParamIterator);
    INIT_AI_SYMBOL(AiNodeGetBool);
    INIT_AI_SYMBOL(AiNodeGetFlt);
    INIT_AI_SYMBOL(AiNodeGetInt);
    INIT_AI_SYMBOL(AiNodeGetLink);
    INIT_AI_SYMBOL(AiNodeGetMatrix);
    INIT_AI_SYMBOL(AiNodeGetName);
    INIT_AI_SYMBOL(AiNodeGetNodeEntry);
    INIT_AI_SYMBOL(AiNodeGetRGBA);
    INIT_AI_SYMBOL(AiNodeGetRGB);
    INIT_AI_SYMBOL(AiNodeGetStr);
    INIT_AI_SYMBOL(AiNodeGetUInt);
    INIT_AI_SYMBOL(AiNodeGetVec2);
    INIT_AI_SYMBOL(AiNodeGetVec);
    INIT_AI_SYMBOL(AiNodeIsLinked);
    INIT_AI_SYMBOL(AiNodeIteratorDestroy);
    INIT_AI_SYMBOL(AiNodeIteratorFinished);
    INIT_AI_SYMBOL(AiNodeIteratorGetNext);
    INIT_AI_SYMBOL(AiNodeReset);
    INIT_AI_SYMBOL(AiParamGetName);
    INIT_AI_SYMBOL(AiParamGetType);
    INIT_AI_SYMBOL(AiParamIteratorDestroy);
    INIT_AI_SYMBOL(AiParamIteratorFinished);
    INIT_AI_SYMBOL(AiParamIteratorGetNext);
    INIT_AI_SYMBOL(AiUniverseGetNodeIterator);
    INIT_AI_SYMBOL(AiUniverseIsActive);
    INIT_AI_SYMBOL(AiNodeGetArray);
    INIT_AI_SYMBOL(AiArrayGetType);
    INIT_AI_SYMBOL(AiArrayGetRGBFunc);
    INIT_AI_SYMBOL(AiArrayGetFltFunc);
    INIT_AI_SYMBOL(AiArrayGetIntFunc);
    INIT_AI_SYMBOL(AiArrayGetByteFunc);
    INIT_AI_SYMBOL(AiArrayGetStrFunc);
    INIT_AI_SYMBOL(AiArrayGetNumElements);
    INIT_AI_SYMBOL(AiNodeEntryGetType);
    INIT_AI_SYMBOL(AiNodeEntryLookUpParameter);
    INIT_AI_SYMBOL(AiNodeGetPtr);
}

ArnoldAPIPtr ArnoldAPI::create()
{
    ArnoldAPIPtr ai = std::make_shared<ArnoldAPI>();
    if (ai->valid())
    {
        return ai;
    }

    return ArnoldAPIPtr();
}

ArnoldAPI::~ArnoldAPI()
{
    if (mArnoldDSO)
    {
        dlclose(mArnoldDSO);
    }
}

bool ArnoldAPI::valid() const
{
    return mArnoldDSO && mArch == 5;
}
