// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __MTOASCENE_H_
#define __MTOASCENE_H_

#include <translators/NodeTranslator.h>

#include "mtoaVersion.h"

#if WALTER_MTOA_VERSION >= 10400
#include <unordered_set>
typedef std::unordered_set<AtNode*> AtNodeSet;
#endif

// Since MTOA 1.4 files like CMayaScene, CArnoldSession, CRenderSession are no
// longer part of the API. But they are necessary to save Arnold shading
// network. This class contains all the communication with CMayaScene etc. In
// the case the MTOA is less than 1.4, we access CMayaScene directly. Otherwise,
// we are trying to load the symbols from the shared library and use them.
class MTOAScene {
    public:
        MTOAScene();
        ~MTOAScene();

        // Export given plug to Arnold shading networks using MTOA. The root
        // nodes are added to the AtNodeSet. If current MTOA is 2.1, `nodes` is
        // never changed.
        CNodeTranslator* exportNode(const MPlug& plug, AtNodeSet* nodes);

    private:
#if WALTER_MTOA_VERSION >= 10400
        void *fSession;
#else
        CArnoldSession* fSession;
#endif

        // Pointer to CMayaScene::End()
        MStatus (*mEnd)();

        // Pointer to
        // CNodeTranslator* CArnoldSession::ExportNode(
        //      const MPlug& shaderOutputPlug,
        //      AtNodeSet* nodes=NULL,
        //      AOVSet* aovs=NULL,
        //      bool initOnly=false,
        //      int instanceNumber = -1,
        //      MStatus* stat=NULL);
        // Available in MTOA 2.0
        CNodeTranslator* (*mExportNode)(
                void*, const MPlug&, AtNodeSet*, void*, bool, int, void*);

        // Pointer to
        // CNodeTranslator* CArnoldSession::ExportNode(
        //     const MPlug& shaderOutputPlug,
        //     bool initOnly,
        //     int instanceNumber,
        //     MStatus* stat)
        // Available in MTOA 2.1
        CNodeTranslator* (*mExportNodeA)(void*, const MPlug&, bool, int, void*);
};

#endif
