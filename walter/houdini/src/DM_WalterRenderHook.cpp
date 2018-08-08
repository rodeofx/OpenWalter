// Copyright 2017 Rodeo FX.  All rights reserved.

#include <GL/gl.h>

#include <DM/DM_GeoDetail.h>
#include <DM/DM_RenderTable.h>
#include <DM/DM_SceneHook.h>
#include <DM/DM_VPortAgent.h>
#include <GR/GR_Primitive.h>
#include <RE/RE_Render.h>
#include <SOP/SOP_Node.h>
#include <UT/UT_Map.h>

#include <pxr/imaging/glf/drawTarget.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include "GU_WalterPackedImpl.h"
#include "walterHoudiniUtils.h"
#include "walterSpriteRenderer.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

typedef std::pair<exint, GA_Index> PrimUniqueID;
typedef std::set<PrimUniqueID> WirefameSet;

/**
 * @brief Extracts the WalterPacked intrinsinc attributes 
 *
 * @param iPrim The pointer to the GEO_Primitive object.
 * @param oFileName The USD stage name will be writtent here.
 * @param oRendererName The Hudra renderer plugin name.
 * @param oPrimPath The USD path to the prim will be there.
 * @param oTransform The local transform will be there.
 * @param oFrame The frame will be there.
 */
void getPrimData(
    const GEO_Primitive* iPrim,
    std::string& oFileName,
    std::string& oRendererName,
    SdfPath& oPrimPath,
    UT_Matrix4D& oTransform,
    float& oFrame)

{
    assert(iPrim);

    UT_String fileName;
    iPrim->getIntrinsic(
        iPrim->findIntrinsic(WalterHoudini::GU_WalterPackedImpl::usdFileName), fileName);
    oFileName = fileName.toStdString();

    UT_String rendererName;
    iPrim->getIntrinsic(
        iPrim->findIntrinsic(WalterHoudini::GU_WalterPackedImpl::rendererName), rendererName);
    oRendererName = rendererName.toStdString();

    UT_String primPath;
    iPrim->getIntrinsic(
        iPrim->findIntrinsic(WalterHoudini::GU_WalterPackedImpl::usdPrimPath), primPath);

    // Generate SdfPath
    if (primPath && primPath.length() > 0)
    {
        oPrimPath = SdfPath(primPath.toStdString());
    }
    else
    {
        oPrimPath = SdfPath::EmptyPath();
    }

    iPrim->getLocalTransform4(oTransform);

    iPrim->getIntrinsic(
        iPrim->findIntrinsic(WalterHoudini::GU_WalterPackedImpl::usdFrame), oFrame);

}


/**
 * @brief Draw the USD object in the current OpenGL state. It sets up Hydra and
 * calls the drawing methods.
 *
 * @param iFileName The name of the USD stage to draw.
 * @param iPrimPath The path to the prim to draw in the USD stage.
 * @param iView The view matrix.
 * @param iProjection The projection matrix.
 * @param iWidth The width of the current viewport.
 * @param iHeight The height of the current viewport.
 * @param iParams The Hydra params. The frame can be changed.
 */
void drawWalterPrim(
    const std::string& iRendererName,
    const std::string& iFileName,
    const SdfPath& iPrimPath,
    const UT_Matrix4D& iView,
    const UT_Matrix4D& iProjection,
    int iWidth,
    int iHeight,
    const UsdImagingGLEngine::RenderParams& iParams)
{
    UsdStageRefPtr stage = WalterHoudiniUtils::getStage(iFileName);
    UsdImagingGLRefPtr renderer = WalterHoudiniUtils::getRenderer(iFileName);

    if (stage && renderer)
    {
        // Set renderer plugin
        // TODO: set only when the renderer change.
        for (const TfToken& current : renderer->GetRendererPlugins())
        {
            if( current.GetString() == iRendererName )
            {
                renderer->SetRendererPlugin(current);
                break;
            }
        }        

        // Draw all.
        UsdPrim prim;
        if (iPrimPath.IsEmpty() || iPrimPath == SdfPath::AbsoluteRootPath())
        {
            prim = stage->GetPseudoRoot();
        }
        else
        {
            prim = stage->GetPrimAtPath(iPrimPath);
        }

        if (prim)
        {
            // Hydra.
            auto viewMatrix = WalterHoudiniUtils::MatrixConvert<GfMatrix4d>(iView);

            renderer->SetCameraState(
                viewMatrix,
                WalterHoudiniUtils::MatrixConvert<GfMatrix4d>(iProjection),
                GfVec4d(0, 0, iWidth, iHeight));


            // Add a light positonned at the camera
            auto light = GlfSimpleLight();
            light.SetAmbient(GfVec4f(0.0f));

            GfVec3d translation = viewMatrix.GetInverse().ExtractTranslation();
            light.SetPosition(GfVec4f(
                static_cast<float>(translation[0]),
                static_cast<float>(translation[1]),
                static_cast<float>(translation[2]),
                1.0f));

            GlfSimpleLightVector lights;
            lights.push_back(light);

            auto material = GlfSimpleMaterial();
            GfVec4f sceneAmbient(0.01f);
            
            renderer->SetLightingState(lights, material, sceneAmbient);

            renderer->Render(prim, iParams);
        }
    }
}

} // end anonymous namespace

class DM_WalterRenderHook : public DM_SceneRenderHook
{
public:
    DM_WalterRenderHook(DM_VPortAgent& vport, WirefameSet& iWireframedObjects) :
            DM_SceneRenderHook(vport, DM_VIEWPORT_ALL_3D),
            mRefCount(0),
            mWireframedObjects(iWireframedObjects)
    {
        static const float colorData[] = {1.0f, 0.699394f, 0.0f, 0.5f};
        mParams.wireframeColor.Set(colorData);
    }

    virtual ~DM_WalterRenderHook() {}

    virtual bool supportsRenderer(GR_RenderVersion version)
    {
        return version >= GR_RENDER_GL3;
    }

    // Because this hook is bound to multiple passes, differentiate between the
    // various passes in our render method.
    virtual bool render(RE_Render* r, const DM_SceneHookData& hook_data)
    {
        // SideFX support told us to use this to fix shading problems. Without
        // those lines, all the other objects are black because Hydra modifies
        // OpenGL state. It works well starting from 16.5.349.
        r->unbindPipeline();
        r->invalidateCachedState();

        // View matrix.
        const UT_Matrix4D view =
            viewport().getViewStateRef().getTransformMatrix();
        // Projection matrix.
        const UT_Matrix4D projection =
            viewport().getViewStateRef().getProjectionMatrix();

        // Viewport.
        int width = viewport().getViewStateRef().getViewWidth();
        int height = viewport().getViewStateRef().getViewHeight();

        // It changes the framebuffer to draw.
        mSprite.drawToBufferBegin();

        // Iterate the objects in the object list.
        for (int i = 0; i < viewport().getNumOpaqueObjects(); i++)
        {
            DM_GeoDetail detail = viewport().getOpaqueObject(i);

            if (!detail.isValid() || detail.getNumDetails() == 0)
            {
                continue;
            }

            // The SOP the detail represents.
            OP_Node* op = detail.getSop();
            if (!op)
            {
                continue;
            }

            SOP_Node* sop = reinterpret_cast<SOP_Node*>(op);
            const GU_Detail* gdp = sop->getLastGeo();
            if (!gdp)
            {
                continue;
            }

            exint detailID = gdp->getUniqueId();

            // Transform of the object.
            const UT_Matrix4D objXForm = detail.getDetailTransform(0);

            const GEO_PrimList list = gdp->primitives();

            // Iterate all the prims
            const GEO_Primitive* prim =
                list.head(GA_PrimCompat::TypeMask::fullMask());

            if(!prim)
            {
                continue;
            }

            while (prim)
            {
                if (prim->getTypeDef().getId() == WalterHoudini::GU_WalterPackedImpl::typeId())
                {
                    std::string fileName;
                    float frame;
                    std::string rendererName;
                    SdfPath primPath;
                    UT_Matrix4D transform;

                    getPrimData(
                        prim,
                        fileName,
                        rendererName,
                        primPath,
                        transform,
                        frame);
                    mParams.frame = frame;

                    // Check the cache if we should draw wireframe.
                    const auto it = mWireframedObjects.find(
                        std::make_pair(detailID, prim->getMapIndex()));
                    if (it == mWireframedObjects.end())
                    {
                        mParams.drawMode =
                            UsdImagingGLEngine::DRAW_SHADED_SMOOTH;
                    }
                    else
                    {
                        mParams.drawMode =
                            UsdImagingGLEngine::DRAW_WIREFRAME_ON_SURFACE;
                    }

                    drawWalterPrim(
                        rendererName,
                        fileName,
                        primPath,
                        transform * objXForm * view,
                        projection,
                        width,
                        height,
                        mParams);
                }

                prim = list.next(prim, GA_PrimCompat::TypeMask::fullMask());
            }
        }

        // Clear the cache.
        mWireframedObjects.clear();

        // It restores the framebuffer.
        mSprite.drawToBufferEnd();

        mSprite.drawBeauty();

        // Allow other hooks bound to this pass to render by returning false
        return false;
    }

    virtual void viewportClosed()
    {
        // For some unknown reasons, this virtual method is never called.
    }

    // Reference count so that the DM_WalterSceneHook knows when to delete this
    // object.
    int bumpRefCount(bool inc)
    {
        mRefCount += (inc ? 1 : -1);
        return mRefCount;
    }

private:
    int mRefCount;

    UsdImagingGLEngine::RenderParams mParams;

    WalterSpriteRenderer::SpriteRenderer mSprite;

    WirefameSet& mWireframedObjects;
};

class GUI_WalterPrimFramework : public GR_Primitive
{
public:
    GUI_WalterPrimFramework(
        const GR_RenderInfo* info,
        const char* cache_name,
        const GEO_Primitive* prim,
        WirefameSet& iWireframedObjects) :
            GR_Primitive(info, cache_name, GA_PrimCompat::TypeMask(0)),
            mWireframedObjects(iWireframedObjects)
    {}

    virtual ~GUI_WalterPrimFramework() {}

    virtual const char* className() const { return "GUI_WalterPrimFramework"; }

    // See if the primitive can be consumed by this GR_Primitive. Only
    // primitives from the same detail will ever be passed in. If the primitive
    // hook specifies GUI_HOOK_COLLECT_PRIMITIVES then it is possible to have
    // this called more than once for different GR_Primitives.  A GR_Primitive
    // that collects multiple GT or GEO primitives is responsible for keeping
    // track of them (in a list, table, tree, etc).
    virtual GR_PrimAcceptResult acceptPrimitive(
        GT_PrimitiveType t,
        int geo_type,
        const GT_PrimitiveHandle& ph,
        const GEO_Primitive* prim)
    {
        static const int walterPrimID = WalterHoudini::GU_WalterPackedImpl::typeId().get();
        if (geo_type == walterPrimID)
        {
            return GR_PROCESSED;
        }

        return GR_NOT_PROCESSED;
    }

    // Called whenever the parent detail is changed, draw modes are changed,
    // selection is changed, or certain volatile display options are changed
    // (such as level of detail).
    virtual void update(
        RE_Render* r,
        const GT_PrimitiveHandle& primh,
        const GR_UpdateParms& p)
    {
        // Fetch the GEO primitive from the GT primitive handle
        const GEO_Primitive* prim = nullptr;
        getGEOPrimFromGT<GEO_Primitive>(primh, prim);

        if (!prim)
        {
            return;
        }

        float frame;
        getPrimData(
            prim, mFileName, mRendererName, mPrimPath, mTransform, frame);
        mParams.frame = frame;

        mID = std::make_pair(
            prim->getDetail().getUniqueId(), prim->getMapIndex());
    }

    // Called to do a variety of render tasks (beauty, wire, shadow, object
    // pick)
    virtual void render(
        RE_Render* r,
        GR_RenderMode render_mode,
        GR_RenderFlags flags,
        GR_DrawParms draw_parms)
    {
        // See comments in newRenderHook(). Since GR_Primitive::render is called
        // before DM_WalterRenderHook::render, we can form the set here and
        // clean it up after reading in DM_WalterRenderHook::render.
        if (render_mode == GR_RENDER_BEAUTY && flags & GR_RENDER_FLAG_WIRE_OVER)
        {
            mWireframedObjects.insert(mID);
            return;
        }

        bool isPick = render_mode == GR_RENDER_OBJECT_PICK;
        bool isMatte = render_mode == GR_RENDER_MATTE;

        // For now we only support pick, matte and wireframe.
        if (!isPick && !isMatte)
        {
            return;
        }

        RE_Uniform* base = r->getUniform(RE_UNIFORM_PICK_BASE_ID);
        RE_Uniform* comp = r->getUniform(RE_UNIFORM_PICK_COMPONENT_ID);
        int* baseVec = (int*)base->getValue();
        int* compVec = (int*)comp->getValue();

        WalterSpriteRenderer::GLScopeGuard scopeRestore(r);

        // It changes the framebuffer to draw.
        mSprite.drawToBufferBegin();

        // From SideFX support.
        UT_Matrix4D projection =
            r->getUniform(RE_UNIFORM_PROJECT_MATRIX)->getMatrix4();
        UT_Matrix4D view = r->getUniform(RE_UNIFORM_VIEW_MATRIX)->getMatrix4();
        UT_Matrix4D objXForm =
            r->getUniform(RE_UNIFORM_OBJECT_MATRIX)->getMatrix4();

        // TODO: Is there a way to get rid of this? It's the only OpenGL call in
        // this file.
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        drawWalterPrim(
            mRendererName,
            mFileName,
            mPrimPath,
            mTransform * objXForm * view,
            projection,
            viewport[2],
            viewport[3],
            mParams);

        // It restores the framebuffer.
        mSprite.drawToBufferEnd();

        if (isPick)
        {
            mSprite.drawPick(baseVec, compVec);
        }
        else
        {
            mSprite.drawMatte();
        }
    }

    // Render this primitive for picking, where pick_type is defined as one of
    // the pickable bits in GU_SelectType.h (like GU_PICK_GEOPOINT) return the
    // number of picks
    virtual int renderPick(
        RE_Render* r,
        const GR_DisplayOption* opt,
        unsigned int pick_type,
        GR_PickStyle pick_style,
        bool has_pick_map)
    {
        // For some unknown reason it's never called.
        return 0;
    }

private:
    WalterSpriteRenderer::SpriteRenderer mSprite;
    UsdImagingGLEngine::RenderParams mParams;

    std::string mFileName;
    SdfPath mPrimPath;
    std::string mRendererName;
    float mFrame;
    UT_Matrix4D mTransform;

    PrimUniqueID mID;
    WirefameSet& mWireframedObjects;
};

class DM_WalterSceneHook : public DM_SceneHook
{
public:
    DM_WalterSceneHook() : DM_SceneHook("WalterScene", 0) {}
    virtual ~DM_WalterSceneHook() {}

    virtual DM_SceneRenderHook* newSceneRender(
        DM_VPortAgent& vport,
        DM_SceneHookType type,
        DM_SceneHookPolicy policy)
    {
        // Only for 3D views (persp, top, bottom; not the UV viewport)
        if (!(vport.getViewType() & DM_VIEWPORT_ALL_3D))
        {
            return nullptr;
        }

        DM_WalterRenderHook* hook = nullptr;

        // Create only 1 per viewport.
        const int id = vport.getUniqueId();
        UT_Map<int, DM_WalterRenderHook*>::iterator it = mSceneHooks.find(id);

        if (it != mSceneHooks.end())
        {
            // Found existing hook for this viewport, reuse it.
            hook = it->second;
        }
        else
        {
            // No hook for this viewport; create it.
            hook = new DM_WalterRenderHook(vport, mWireframedObjects);
            mSceneHooks[id] = hook;
        }

        // Increase reference count on the render so we know when to delete it.
        hook->bumpRefCount(true);

        return hook;
    }

    virtual void retireSceneRender(
        DM_VPortAgent& vport,
        DM_SceneRenderHook* hook)
    {
        // If the ref count is zero, we're the last retire call. delete the
        // hook.
        // RND-613: When closing Houdini, (if there is at least one open
        // viewport), the Houdini process is not released for some unknown
        // reason. If all the Houdini viewports are closed BEFORE closing
        // Houdini, there is no issue. It looks like when Houdini exit,
        // the "viewport delete" calls are not done the same way than when just
        // closing a scene view tab...
        if (static_cast<DM_WalterRenderHook*>(hook)->bumpRefCount(false) == 0)
        {
            // Remove from the map and delete the hook.
            const int id = vport.getUniqueId();
            UT_Map<int, DM_WalterRenderHook*>::iterator it =
                mSceneHooks.find(id);

            if (it != mSceneHooks.end())
            {
                mSceneHooks.erase(id);
            }

            delete hook;
        }
    }

    WirefameSet& getWireframedObjects() { return mWireframedObjects; }

private:
    // Keeps a hook per viewport.
    UT_Map<int, DM_WalterRenderHook*> mSceneHooks;
    // List of GEO_Primitives that should be drawn with wireframe. See comments
    // in newRenderHook().
    WirefameSet mWireframedObjects;
};

class GUI_WalterPrimHook : public GUI_PrimitiveHook
{
public:
    GUI_WalterPrimHook(WirefameSet& iWireframedObjects) :
            GUI_PrimitiveHook("Walter Prim Hook"),
            mWireframedObjects(iWireframedObjects)
    {}
    virtual ~GUI_WalterPrimHook() {}

    // If the geo_prim is a WalterPacked, return a new GUI_WalterPrimFramework.
    virtual GR_Primitive* createPrimitive(
        const GT_PrimitiveHandle& gt_prim,
        const GEO_Primitive* geo_prim,
        const GR_RenderInfo* info,
        const char* cache_name,
        GR_PrimAcceptResult& processed)
    {
        if (geo_prim->getTypeId() != WalterHoudini::GU_WalterPackedImpl::typeId())
        {
            return nullptr;
        }

        // We're going to process this prim and prevent any more lower-priority
        // hooks from hooking on it. Alternatively, GR_PROCESSED_NON_EXCLUSIVE
        // could be passed back, in which case any hooks of lower priority would
        // also be called.
        processed = GR_PROCESSED;

        // In this case, we aren't doing anything special, like checking
        // attribs to see if this is a flagged native primitive we want
        // to hook on.
        return new GUI_WalterPrimFramework(
            info, cache_name, geo_prim, mWireframedObjects);
    }

private:
    WirefameSet& mWireframedObjects;
};

void newRenderHook(DM_RenderTable* table)
{
    DM_WalterSceneHook* hook = new DM_WalterSceneHook();

    // Register the hook so that it executes after the beauty pass. We use Scene
    // Hook to draw everything at once. It's much faster than drawing objects
    // one by one.
    table->registerSceneHook(hook, DM_HOOK_BEAUTY, DM_HOOK_AFTER_NATIVE);

    // We draw all the objects of the scene in DM_SceneRenderHook::render at
    // once. But we don't know if we need to draw objects with wireframe from
    // there. So, if we draw wireframed objects in GR_Primitive::render, it's
    // very slow because each wire object will be drawn twice. To avoid it, we
    // collect the indexes of wireframed objects and use them in
    // DM_SceneRenderHook::render when we draw all the scene at once. It's the
    // cache we keep the indexes.
    WirefameSet& wireframedObjects = hook->getWireframedObjects();

    // The priority only matters if multiple hooks are assigned to the same
    // primitive type. If this is the case, the hook with the highest priority
    // (largest priority value) is processed first. We use GEO Hook to draw
    // selection. Unfortunately, there is no option to draw pass ID in a scene
    // hook, and we need to draw it one by one.
    GA_PrimitiveTypeId type = WalterHoudini::GU_WalterPackedImpl::typeId();
    table->registerGEOHook(new GUI_WalterPrimHook(wireframedObjects), type, 0);
}
