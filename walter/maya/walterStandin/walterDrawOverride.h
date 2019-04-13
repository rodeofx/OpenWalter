// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef _walterDrawOverride_h_
#define _walterDrawOverride_h_

#ifdef MFB_ALT_PACKAGE_NAME

#include <maya/MPxDrawOverride.h>
#if MAYA_API_VERSION >= 201800
#include <maya/MSelectionContext.h>
#endif

namespace WalterMaya {

// Handles the drawing of the USD stage in the viewport 2.0.
class DrawOverride : public MHWRender::MPxDrawOverride
{
public:
    // Used by the MDrawRegistry to create new instances of this class.
    static MPxDrawOverride* creator(const MObject& obj);

private:

    // Data used by the draw call back.
    class UserData;

    // Invoked by the Viewport 2.0 when it is time to draw.
    static void drawCb(
            const MHWRender::MDrawContext& drawContext,
            const MUserData* data);

    DrawOverride(const MObject& obj);
    virtual ~DrawOverride();

    // Prohibited and not implemented.
    DrawOverride(const DrawOverride& obj);
    const DrawOverride& operator=(const DrawOverride& obj);

    // Overrides of MPxDrawOverride member funtions. 
    virtual MHWRender::DrawAPI supportedDrawAPIs() const;

    virtual bool isBounded(
            const MDagPath& objPath,
            const MDagPath& cameraPath) const;

    virtual MBoundingBox boundingBox(
            const MDagPath& objPath,
            const MDagPath& cameraPath) const;

    virtual bool disableInternalBoundingBoxDraw() const;

    virtual MUserData* prepareForDraw(
            const MDagPath& objPath,
            const MDagPath& cameraPath,
            const MHWRender::MFrameContext& frameContext,
            MUserData* oldData);

#if MAYA_API_VERSION >= 201800

    virtual bool wantUserSelection() const;

    virtual bool userSelect(
        MSelectionInfo &selectInfo,
        const MHWRender::MDrawContext &context,
        MPoint &hitPoint,
        const MUserData *data);

    virtual bool refineSelectionPath (   
        const MSelectionInfo &selectInfo,
        const MRenderItem &hitItem,
        MDagPath &path,
        MObject &components,
        MSelectionMask &objectMask);
#endif

};

}

#endif

#endif
