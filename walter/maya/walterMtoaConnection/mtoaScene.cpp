// Copyright 2017 Rodeo FX.  All rights reserved.

#include "mtoaScene.h"

#include <link.h>

#include <boost/foreach.hpp>

// Returns true if str ends with ending
inline bool endsWith(const std::string& str, const std::string& ending)
{
    if (ending.size() > str.size()) {
        return false;
    }

    return std::equal(ending.rbegin(), ending.rend(), str.rbegin());
}

MTOAScene::MTOAScene()
        :
        mEnd(NULL),
        fSession(NULL),
        mExportNode(NULL),
        mExportNodeA(NULL)
{
    void* maya = dlopen(NULL, RTLD_NOW);
    link_map* map = reinterpret_cast<link_map*>(maya)->l_next;
    void* mtoa = NULL;

    // Looking for mtoa.so
    static const std::string looking = "/mtoa.so";
    while (map) {
        if (endsWith(map->l_name, looking)) {
            // Found
            mtoa = map;
            break;
        }

        map = map->l_next;
    }

    if (!mtoa) {
        AiMsgError("[EXPORT] Error loading mtoa.so");
        return;
    }

    // Get MStatus CMayaScene::Begin(ArnoldSessionMode)
    MStatus (*begin)(int);
    *(void **)(&begin) =
        dlsym(mtoa, "_ZN10CMayaScene5BeginE17ArnoldSessionMode");

    // Get CArnoldSession* CMayaScene::GetArnoldSession();
    void* (*getArnoldSession)();
    *(void **)(&getArnoldSession) =
        dlsym(mtoa, "_ZN10CMayaScene16GetArnoldSessionEv");

    // Get CRenderSession* CMayaScene::GetRenderSession();
    void* (*getRenderSession)();
    *(void **)(&getRenderSession) =
        dlsym(mtoa, "_ZN10CMayaScene16GetRenderSessionEv");

    // Get MStatus CMayaScene::Export(MSelectionList* selected = NULL)
    MStatus (*expor)(void*);
#if MAYA_API_VERSION >= 201800 // mtoa >= 3.0.1 (maya 2018)
    *(void **)(&expor) =
        dlsym(mtoa, "_ZN10CMayaScene6ExportEPN8Autodesk4Maya16OpenMaya2018000014MSelectionListE");
#else
    *(void **)(&expor) =
        dlsym(mtoa, "_ZN10CMayaScene6ExportEP14MSelectionList");
#endif

    // Get CArnoldSession::ExportNode of MTOA 2.0
    const char* symbol =
        "_ZN14CArnoldSession10ExportNodeERK5MPlugPNSt3tr113"
        "unordered_setIP6AtNodeNS3_4hashIS6_EESt8equal_toIS6_ESaIS6_EEEPSt3"
        "setI4CAOVSt4lessISF_ESaISF_EEbiP7MStatus";
    *(void **)(&mExportNode) = dlsym(mtoa, symbol);

const char* symbolA = nullptr;
#if MAYA_API_VERSION >= 201800 // mtoa >= 3.0.1 (maya 2018)
    // Get CArnoldSession::ExportNode of MTOA 3.0.1
    symbolA = "_ZN14CArnoldSession10ExportNodeERKN8Autodesk4Maya16OpenMaya201800005MPlugEbiPNS2_7MStatusE";
#else
    // Get CArnoldSession::ExportNode of MTOA >= 2.1
    symbolA = "_ZN14CArnoldSession10ExportNodeERK5MPlugbiP7MStatus";
#endif

    *(void**)(&mExportNodeA) = dlsym(mtoa, symbolA);

    // Get CMayaScene::End()
    *(void **)(&mEnd) = dlsym(mtoa, "_ZN10CMayaScene3EndEv");

    if (!begin ||
        !getArnoldSession ||
        !getRenderSession ||
        !expor ||
        (!mExportNode && !mExportNodeA) ||
        !mEnd) {
        // Something is not loaded. Return.
        AiMsgError("[EXPORT] Error loading symbols from mtoa.so");
        return;
    }

    // Call MStatus CMayaScene::Begin(MTOA_SESSION_ASS)
    (*begin)(MTOA_SESSION_ASS);
    // Call CMayaScene::GetArnoldSession()
    fSession = (*getArnoldSession)();
    // Call CMayaScene::GetRenderSession()
    void *renderSession = (*getRenderSession)();

    // TODO:
    // renderSession->SetOutputAssMask(16)
    // fSession->SetExportFilterMask(16)
    // renderSession->SetForceTranslateShadingEngines(true)

    // Call CMayaScene::Export(NULL)
    (*expor)(NULL);
}

MTOAScene::~MTOAScene()
{
    if (mEnd) {
        // Call CMayaScene::End()
        (*mEnd)();
    }
}

CNodeTranslator* MTOAScene::exportNode(const MPlug& plug, AtNodeSet* nodes)
{
    if (mExportNode) {
        // Call fSession->ExportNode(plug, nodes)
        return (*mExportNode)(fSession, plug, nodes, NULL, false, -1, NULL);
    }

    if (mExportNodeA)
    {
        return (*mExportNodeA)(fSession, plug, false, -1, NULL);
    }

    return nullptr;
}
