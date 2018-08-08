// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef _walterShapeNode_h_
#define _walterShapeNode_h_

#include <maya/MMessage.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MStringArray.h>
#include <pxr/usdImaging/usdImagingGL/gl.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp> 
#include <boost/unordered_map.hpp>
#include <set>
#include <vector>
#include <maya/MNodeMessage.h>

#include "PathUtil.h"

// Bool is defined in /usr/include/X11/Intrinsic.h, it conflicts with USD.
#ifdef Bool
#undef Bool
#endif

#include <pxr/usd/usd/stage.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace WalterMaya {

/** @brief Keeps track of animated shapes held in a memory cache. */
class ShapeNode : public MPxSurfaceShape
{
public:
    // Building following structure:
    // {"object", {("layer", "target"): "shader"}}
    typedef std::pair<std::string, std::string> LayerKey;
    typedef std::map<LayerKey, std::string> LayerMap;
    typedef std::map<std::string, LayerMap> ExpMap;

    // First is the expression, second is the group.
    typedef std::map<std::string, std::string> ExpGroup;

    typedef std::map<WalterCommon::Expression, SdfPath> AssignedOverrides;

    static void* creator();
    static MStatus initialize();
    static MStatus uninitialize();

    static const MTypeId id;
    static const MString drawDbClassificationGeometry;
    static const MString drawRegistrantId;

    static MObject aCacheFileName;
    static MObject aTime;
    static MObject aTimeOffset;
    static MObject aBBmode;

    // layersAssignation compound
    static MObject aLayersAssignation;
    static MObject aLayersAssignationLayer;
    static MObject aLayersAssignationSC;
    static MObject aLayersAssignationSCNode;
    static MObject aLayersAssignationSCShader;
    static MObject aLayersAssignationSCDisplacement;
    static MObject aLayersAssignationSCAttribute;

    // expressions compound
    static MObject aExpressions;
    static MObject aExpressionsName;
    static MObject aExpressionsGroupName;
    static MObject aExpressionsWeight;

    static MObject aVisibilities;
    static MObject aRenderables;
    static MObject aAlembicShaders;
    static MObject aEmbeddedShaders;

    // The name of the procedural to render with Arnold.
    static MObject aArnoldProcedural;

    // Read-only attribute with USD session layer as a text.
    static MObject aUSDSessionLayer;

    // Read-only attribute with USD variants layer as a text.
    static MObject aUSDVariantLayer;

    // Read-only attribute with USD purposes layer as a text.
    static MObject aUSDPurposeLayer;

    // Read-only attribute with USD session layer as a text.
    static MObject aUSDMayaStateLayer;

    // Read-only attribute with USD visibility layer as a text.
    static MObject aUSDVisibilityLayer;
    
    // Transform compound
    static MObject aTransforms;
    static MObject aTransformsObject;
    static MObject aTransformsMatrix;

    static MObject aHydraPlugin;

    static MObject aFrozen;

    static const char* nodeTypeName;
    static const char* selectionMaskName;

    mutable double time;
    mutable bool fReadScene;
    mutable bool fReadMaterials;
    mutable bool fLoadGeo;
    mutable bool fBBmode;
    mutable bool bTriggerGeoReload;
    mutable bool fSceneLoaded;
    mutable bool fFrozen;

    enum CacheReadingState {
        kCacheReadingFile,
        kCacheReadingDone
    };

    ShapeNode();
    ~ShapeNode();

    virtual void postConstructor();

    virtual bool isBounded() const;
    virtual MBoundingBox boundingBox() const;

    // When internal attribute values are queried via getAttr or MPlug::getValue
    // this method is called.
    virtual bool getInternalValueInContext(const MPlug& plug,
        MDataHandle& dataHandle, MDGContext& ctx);
    // This method is used by Maya to determine the count of array elements.
    virtual int internalArrayCount(
        const MPlug& plug, const MDGContext& ctx) const;
    virtual bool setInternalValueInContext(const MPlug& plug,
        const MDataHandle& dataHandle, MDGContext& ctx);
    virtual void copyInternalData(MPxNode* source);

    virtual MStringArray    getFilesToArchive( bool shortName = false,
                                               bool unresolvedName = false,
                                               bool markCouldBeImageSequence = false ) const;
    virtual bool match( const MSelectionMask & mask,
        const MObjectArray& componentList ) const;

    virtual MSelectionMask getShapeSelectionMask() const;

    virtual bool excludeAsPluginShape() const;

    // Callback when the walter node is added to the model (create / undo-delete)
    void addedToModelCB();
    // Callback when the walter node is removed from model (delete)
    void removedFromModelCB();

    /* A string that contains all the sub-selected objects separated with a
     * semicolon. */
    void setSubSelectedObjects(const MString& obj);
    const MString& getSubSelectedObjects() const { return fSubSelectedObjects; }

    /**
     * @brief Checks that a regex represents a valid walter expression.
     *
     * @param expression The regex expression to validate.
     *
     * @return True is the expression is valid, false otherwise.
     */
    bool expressionIsMatching(const MString& expression);


    // Originally it's for specifying which plugs should be set dirty based upon
    // an input plug plugBeingDirtied which Maya is marking dirty. We use it to
    // catch dirty event and save dirty plugs to the internal cache.
    virtual MStatus setDependentsDirty(
        const MPlug& plug,
        MPlugArray& plugArray);

    // Reads the internal dirty cache and update the USD stage.
    void updateUSDStage() const;

    /**
     * @brief Regenerate the textures if it's necessary.
     *
     * @param iTexturesEnabled True if the testures should be enabled.
     */
    void updateTextures(bool iTexturesEnabled) const;

    // Clear the USD playback cache.
    void setCachedFramesDirty();

    // This method gets called when connections are made to attributes of this
    // node.
    virtual MStatus connectionMade(
        const MPlug& plug,
        const MPlug& otherPlug,
        bool asSrc);
    virtual MStatus connectionBroken(
        const MPlug& plug,
        const MPlug& otherPlug,
        bool asSrc);

    /**
     * @brief The current visible file names merged in a single string.
     *
     * @return
     */
    const MString resolvedCacheFileName() const;

    /**
     * @brief Saves the stage to the node.
     *
     * @param iStage USD stage.
     */
    void setStage(UsdStageRefPtr iStage);

    /**
     * @brief The current stage.
     *
     * @return The current stage.
     */
    UsdStageRefPtr getStage() const;

    /**
     * @brief Current renderer.
     *
     * @return Current Hydra renderer.
     */
    UsdImagingGLSharedPtr getRenderer();

    /** @brief Called when the loading of the USD file is finished. It's a bit
     * ugly because we call it from SubNode::getCachedGeometry(), but it's done
     * for a reason because getCachedGeometry is the first thing that is called
     * from VP2 when the loading is finished. It should only be called from the
     * main thread. */
    void onLoaded();

    /** @brief Set flag that allows to call onLoaded. We can't call it from a
     * different thread because USD doesn't like it. */
    void setJustLoaded();

    /** @brief Refreshes the render. */
    void refresh();

    /**
     * @brief All the assignments of the current stage.
     */
    ExpMap& getAssignments() { return mAssignments; }
    const ExpMap& getAssignments() const { return mAssignments; }

    /**
     * @brief All the expression groups of the current stage.
     */
    ExpGroup& getGroups() { return mGroups; }
    const ExpGroup& getGroups() const { return mGroups; }

    /**
     * @brief All the assigned overrides of the current stage.
     */
    AssignedOverrides& getAssignedOverrides() { return mOverrides; }
    const AssignedOverrides& getAssignedOverrides() const { return mOverrides; }

    /**
     * @brief All the materials of the current stage.
     */
    MStringArray& getEmbeddedShaders() { return mEmbeddedShaders; }
    const MStringArray& getEmbeddedShaders() const { return mEmbeddedShaders; }

    /**
     * @brief Buffer with session layer. Please note it's not a session layer.
     * It contains a buffer that is created when reading the file. We need to
     * put it to the newly created stage.
     */
    const MString& getSessionLayerBuffer() const { return fSessionLayerBuffer; }

    /**
     * @brief Buffer with variants layer. Please note it's not a variants layer.
     * It contains a buffer that is created when reading the file. We need to
     * put it to the newly created stage.
     */
    const MString& getVariantsLayerBuffer() const { return fVariantsLayerBuffer; }

    /**
     * @brief Buffer with variantspurpose layer. Please note it's not a purpose layer.
     * It contains a buffer that is created when reading the file. We need to
     * put it to the newly created stage.
     */
    const MString& getPurposeLayerBuffer() const { return fPurposeLayerBuffer; }

    /**
     * @brief Buffer with variants layer. Please note it's not a variants layer.
     * It contains a buffer that is created when reading the file. We need to
     * put it to the newly created stage.
     */
    const MString& getVisibilityLayerBuffer() const { return fVisibilityLayerBuffer; }

    /**
     * @brief Called by setInternalValueInContext when the time is changed.
     * If the time is not connect, it is called on scene loading and when saving transforms.
     */
    void onTimeChanged(double previous);

    UsdImagingGL::RenderParams mParams;
    
private:
    // Prohibited and not implemented.
    ShapeNode(const ShapeNode& obj);
    const ShapeNode& operator=(const ShapeNode& obj);

    bool setInternalFileName();
    virtual void getExternalContent(MExternalContentInfoTable& table) const;
    virtual void setExternalContent(const MExternalContentLocationTable& table);


    MCallbackId mNameChangedCallbackId;
    MCallbackId mAttributeChangedCallbackId;
    static void nameChangedCallback(MObject &obj, const MString &prevName, void *clientData);
    static void addAttributeAddedOrRemovedCallback(
        MNodeMessage::AttributeMessage msg,
        MPlug& plug,
        void* clientDat);

    MString fCacheFileName;
    MString fResolvedCacheFileName;
    MIntArray fVisibilities;
    MIntArray fRenderables;

    // See internalArrayCount
    mutable std::vector<std::string> mAssignmentShadersBuffer;

    /* This is the list of the objects selected by user separted by a semicolon.
     * SubSceneOverride::fSubSelectedObjects is the object that is highlighted
     * in the viewport. */
    MString fSubSelectedObjects;

    // We keep the latest loaded session/variants layer on the case the CacheFileEntry is
    // not yet loaded.
    MString fSessionLayerBuffer;
    MString fVariantsLayerBuffer;
    MString fVisibilityLayerBuffer;
    MString fPurposeLayerBuffer;

    // mutable because it's a dirty cache.
    mutable std::set<unsigned int> fDirtyTransforms;
    mutable std::set<unsigned int> fPreviousDirtyTransforms;

    // The list of the frames per object that was already sent to the USD stage.
    std::map<unsigned int, std::set<double>> fCachedFrames;

    UsdStageRefPtr mStage;
    bool mRendererNeedsUpdate;
    UsdImagingGLSharedPtr mRenderer;

    ExpMap mAssignments;
    ExpGroup mGroups;
    AssignedOverrides mOverrides;
    MStringArray mEmbeddedShaders;

    bool mJustLoaded;

    // They are mutable because they just reflect the state of the USD stage.
    // A flag that it's necessary to regenerate the textures.
    mutable bool mDirtyTextures;
    // A lag that is true if layer "viewportShaderLayer" is not muted in the USD
    // stage.
    mutable bool mTexturesEnabled;
};


// Return root ShapeNode from the Maya object name
ShapeNode* getShapeNode(
        const MString& objectName,
        MStatus* returnStatus=NULL);

float getCurrentFPS();


///////////////////////////////////////////////////////////////////////////////
//
// DisplayPref
//
// Keeps track of the display preference.
//
////////////////////////////////////////////////////////////////////////////////

class DisplayPref
{
public:
    enum WireframeOnShadedMode {
        kWireframeOnShadedFull,
        kWireframeOnShadedReduced,
        kWireframeOnShadedNone
    };

    static WireframeOnShadedMode wireframeOnShadedMode();

    static MStatus initCallback();
    static MStatus removeCallback();

private:
    static void displayPrefChanged(void*);

    static WireframeOnShadedMode fsWireframeOnShadedMode;
    static MCallbackId fsDisplayPrefChangedCallbackId;
};


///////////////////////////////////////////////////////////////////////////////
//
// ShapeUI
//
// Displays animated shapes held in a memory cache.
//
////////////////////////////////////////////////////////////////////////////////

class ShapeUI : public MPxSurfaceShapeUI
{
public:

    static void* creator();

    ShapeUI();
    ~ShapeUI();

    virtual void getDrawRequests(const MDrawInfo & info,
                                 bool objectAndActiveOnly,
                                 MDrawRequestQueue & queue);

    // Viewport 1.0 draw
    virtual void draw(const MDrawRequest & request, M3dView & view) const;

    virtual bool select(MSelectInfo &selectInfo, MSelectionList &selectionList,
                        MPointArray &worldSpaceSelectPts ) const;


private:
    // Prohibited and not implemented.
    ShapeUI(const ShapeUI& obj);
    const ShapeUI& operator=(const ShapeUI& obj);

    static MPoint getPointAtDepth( MSelectInfo &selectInfo,double depth);

    // Helper functions for the viewport 1.0 drawing purposes.
    void drawBoundingBox(const MDrawRequest & request, M3dView & view ) const;
    void drawWireframe(const MDrawRequest & request, M3dView & view ) const;
    void drawShaded(const MDrawRequest & request, M3dView & view, bool depthOffset ) const;
    void drawUSD(
            const MDrawRequest& request,
            const M3dView& view,
            bool wireframe) const;

    // Draw Tokens
    enum DrawToken {
        kBoundingBox,
        kDrawWireframe,
        kDrawWireframeOnShaded,
        kDrawSmoothShaded,
        kDrawSmoothShadedDepthOffset
    };
};

}

#endif
