// Copyright 2018 Rodeo FX.  All rights reserved.

#ifndef __ARNOLD_API__
#define __ARNOLD_API__

#include <ai.h>
#include <memory>

class ArnoldAPI;
typedef std::shared_ptr<ArnoldAPI> ArnoldAPIPtr;

class ArnoldAPI
{
public:
    static ArnoldAPIPtr create();

    ArnoldAPI();
    ~ArnoldAPI();

    int GetArchVersion() const { return valid() ? mArch : -1; }

    bool valid() const;

    typedef int (*AiASSLoadFuncPtr)(const char* filename, int mask);
    typedef void (*AiBeginFuncPtr)();
    typedef void (*AiEndFuncPtr)();
    typedef const char* (
        *AiGetVersionFuncPtr)(char* arch, char* major, char* minor, char* fix);
    typedef void (*AiLoadPluginsFuncPtr)(const char* directory);
    typedef const char* (*AiNodeEntryGetNameFuncPtr)(const AtNodeEntry* nentry);
    typedef AtParamIterator* (*AiNodeEntryGetParamIteratorFuncPtr)(
        const AtNodeEntry* nentry);
    typedef bool (
        *AiNodeGetBoolFuncPtr)(const AtNode* node, const AtString param);
    typedef float (
        *AiNodeGetFltFuncPtr)(const AtNode* node, const AtString param);
    typedef int (
        *AiNodeGetIntFuncPtr)(const AtNode* node, const AtString param);
    typedef AtNode* (*AiNodeGetLinkFuncPtr)(
        const AtNode* node,
        const char* input,
        int* comp);
    typedef AtMatrix (
        *AiNodeGetMatrixFuncPtr)(const AtNode* node, const AtString param);
    typedef const char* (*AiNodeGetNameFuncPtr)(const AtNode* node);
    typedef const AtNodeEntry* (*AiNodeGetNodeEntryFuncPtr)(const AtNode* node);
    typedef AtRGBA (
        *AiNodeGetRGBAFuncPtr)(const AtNode* node, const AtString param);
    typedef AtRGB (
        *AiNodeGetRGBFuncPtr)(const AtNode* node, const AtString param);
    typedef AtString (
        *AiNodeGetStrFuncPtr)(const AtNode* node, const AtString param);
    typedef unsigned int (
        *AiNodeGetUIntFuncPtr)(const AtNode* node, const AtString param);
    typedef AtVector2 (
        *AiNodeGetVec2FuncPtr)(const AtNode* node, const AtString param);
    typedef AtVector (
        *AiNodeGetVecFuncPtr)(const AtNode* node, const AtString param);
    typedef bool (
        *AiNodeIsLinkedFuncPtr)(const AtNode* node, const char* input);
    typedef void (*AiNodeIteratorDestroyFuncPtr)(AtNodeIterator* iter);
    typedef bool (*AiNodeIteratorFinishedFuncPtr)(const AtNodeIterator* iter);
    typedef AtNode* (*AiNodeIteratorGetNextFuncPtr)(AtNodeIterator* iter);
    typedef void (*AiNodeResetFuncPtr)(AtNode* node);
    typedef AtString (*AiParamGetNameFuncPtr)(const AtParamEntry* pentry);
    typedef uint8_t (*AiParamGetTypeFuncPtr)(const AtParamEntry* pentry);
    typedef void (*AiParamIteratorDestroyFuncPtr)(AtParamIterator* iter);
    typedef bool (*AiParamIteratorFinishedFuncPtr)(const AtParamIterator* iter);
    typedef const AtParamEntry* (*AiParamIteratorGetNextFuncPtr)(
        AtParamIterator* iter);
    typedef AtNodeIterator* (*AiUniverseGetNodeIteratorFuncPtr)(
        unsigned int node_mask);
    typedef bool (*AiUniverseIsActiveFuncPtr)();
    typedef AtArray* (
        *AiNodeGetArrayFuncPtr)(const AtNode* node, const AtString param);
    typedef uint8_t (*AiArrayGetTypeFuncPtr)(const AtArray* array);
    typedef AtRGB (*AiArrayGetRGBFuncFuncPtr)(
        const AtArray* a,
        uint32_t i,
        const char*,
        int line);
    typedef float (*AiArrayGetFltFuncFuncPtr)(
        const AtArray* a,
        uint32_t i,
        const char*,
        int line);
    typedef int (*AiArrayGetIntFuncFuncPtr)(
        const AtArray* a,
        uint32_t i,
        const char*,
        int line);
    typedef uint8_t (*AiArrayGetByteFuncFuncPtr)(
        const AtArray* a,
        uint32_t i,
        const char*,
        int line);
    typedef AtString (*AiArrayGetStrFuncFuncPtr)(
        const AtArray* a,
        uint32_t i,
        const char*,
        int line);
    typedef uint32_t (*AiArrayGetNumElementsFuncPtr)(const AtArray* array);
    typedef int (*AiNodeEntryGetTypeFuncPtr)(const AtNodeEntry* nentry);
    typedef const AtParamEntry* (*AiNodeEntryLookUpParameterFuncPtr)(
        const AtNodeEntry* nentry,
        const AtString param);
    typedef void* (
        *AiNodeGetPtrFuncPtr)(const AtNode* node, const AtString param);

    AiASSLoadFuncPtr AiASSLoad;
    AiBeginFuncPtr AiBegin;
    AiGetVersionFuncPtr AiGetVersion;
    AiEndFuncPtr AiEnd;
    AiLoadPluginsFuncPtr AiLoadPlugins;
    AiNodeEntryGetNameFuncPtr AiNodeEntryGetName;
    AiNodeEntryGetParamIteratorFuncPtr AiNodeEntryGetParamIterator;
    AiNodeGetBoolFuncPtr AiNodeGetBool;
    AiNodeGetFltFuncPtr AiNodeGetFlt;
    AiNodeGetIntFuncPtr AiNodeGetInt;
    AiNodeGetLinkFuncPtr AiNodeGetLink;
    AiNodeGetMatrixFuncPtr AiNodeGetMatrix;
    AiNodeGetNameFuncPtr AiNodeGetName;
    AiNodeGetNodeEntryFuncPtr AiNodeGetNodeEntry;
    AiNodeGetRGBAFuncPtr AiNodeGetRGBA;
    AiNodeGetRGBFuncPtr AiNodeGetRGB;
    AiNodeGetStrFuncPtr AiNodeGetStr;
    AiNodeGetUIntFuncPtr AiNodeGetUInt;
    AiNodeGetVec2FuncPtr AiNodeGetVec2;
    AiNodeGetVecFuncPtr AiNodeGetVec;
    AiNodeIsLinkedFuncPtr AiNodeIsLinked;
    AiNodeIteratorDestroyFuncPtr AiNodeIteratorDestroy;
    AiNodeIteratorFinishedFuncPtr AiNodeIteratorFinished;
    AiNodeIteratorGetNextFuncPtr AiNodeIteratorGetNext;
    AiNodeResetFuncPtr AiNodeReset;
    AiParamGetNameFuncPtr AiParamGetName;
    AiParamGetTypeFuncPtr AiParamGetType;
    AiParamIteratorDestroyFuncPtr AiParamIteratorDestroy;
    AiParamIteratorFinishedFuncPtr AiParamIteratorFinished;
    AiParamIteratorGetNextFuncPtr AiParamIteratorGetNext;
    AiUniverseGetNodeIteratorFuncPtr AiUniverseGetNodeIterator;
    AiUniverseIsActiveFuncPtr AiUniverseIsActive;
    AiNodeGetArrayFuncPtr AiNodeGetArray;
    AiArrayGetTypeFuncPtr AiArrayGetType;
    AiArrayGetRGBFuncFuncPtr AiArrayGetRGBFunc;
    AiArrayGetFltFuncFuncPtr AiArrayGetFltFunc;
    AiArrayGetIntFuncFuncPtr AiArrayGetIntFunc;
    AiArrayGetByteFuncFuncPtr AiArrayGetByteFunc;
    AiArrayGetStrFuncFuncPtr AiArrayGetStrFunc;
    AiArrayGetNumElementsFuncPtr AiArrayGetNumElements;
    AiNodeEntryGetTypeFuncPtr AiNodeEntryGetType;
    AiNodeEntryLookUpParameterFuncPtr AiNodeEntryLookUpParameter;
    AiNodeGetPtrFuncPtr AiNodeGetPtr;

private:
    // The library address
    void* mArnoldDSO;

    // The Arnold version
    int mArch;
};

#endif
