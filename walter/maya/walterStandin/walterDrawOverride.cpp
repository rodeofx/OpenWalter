// Copyright 2017 Rodeo FX.  All rights reserved.

#include "walterDrawOverride.h"

#include <pxr/imaging/glf/glew.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/imageable.h>

#include "walterShapeNode.h"
#include "rdoProfiling.h"

#include <GL/gl.h>
#include <maya/MDrawContext.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MUserData.h>
#include <maya/MGlobal.h>
#include <algorithm>

namespace WalterMaya {

PXR_NAMESPACE_USING_DIRECTIVE;
using namespace WalterMaya;

class DrawOverride::UserData : public MUserData
{
public:
    UserData(ShapeNode* node) :
        MUserData(false),
        mShapeNode(node),
        mIsSelected(false)
    {
    }

    // Draw the stage.
    void draw(const MHWRender::MDrawContext& context) const;

    // Form USD Render Params.
    void set(const MColor& wireframeColor, bool isSelected);

    ShapeNode* node() const
    {
        return mShapeNode;
    }

private:
    ShapeNode* mShapeNode;

    GfVec4f mWireframeColor;
    bool mIsSelected;
};

void DrawOverride::UserData::draw(
    const MHWRender::MDrawContext& context) const
{
    RDOPROFILE("");

    // Get USD stuff from the node.
    UsdStageRefPtr stage = mShapeNode->getStage();
    // TODO: const_cast is bad
    UsdImagingGLEngineSharedPtr renderer =
        const_cast<ShapeNode*>(mShapeNode)->getRenderer();
    if (!renderer || !stage)
    {
        return;
    }

    // We need to save the state of the bound program because Hydra doesn't
    // restore it.
    GLint lastProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &lastProgram);

    // Get the matrices from the Maya context.
    const MMatrix view =
        context.getMatrix(MHWRender::MFrameContext::kWorldViewMtx);
    const MMatrix projection =
        context.getMatrix(MHWRender::MFrameContext::kProjectionMtx);

    double viewData[4][4];
    double projectionData[4][4];

    view.get(viewData);
    projection.get(projectionData);

    // Get the Maya viewport.
    int originX;
    int originY;
    int width;
    int height;
    context.getViewportDimensions(originX, originY, width, height);

    const unsigned int displayStyle = context.getDisplayStyle();

    const bool needWireframe =
        !(displayStyle & MHWRender::MFrameContext::kBoundingBox) &&
        (displayStyle & MHWRender::MFrameContext::kWireFrame || mIsSelected);

    // Create render parameters. We can't define them as a member of UserData
    // because Draw should be const.
    UsdImagingGLRenderParams params = mShapeNode->mParams;
    // Frame number.
    params.frame = UsdTimeCode(mShapeNode->time * getCurrentFPS());
    params.highlight = mIsSelected;
    params.wireframeColor = mWireframeColor;

    if (needWireframe)
    {
        if (displayStyle & MHWRender::MFrameContext::kGouraudShaded)
        {
            params.drawMode = UsdImagingGLDrawMode::DRAW_WIREFRAME_ON_SURFACE;
        }
        else
        {
            params.drawMode = UsdImagingGLDrawMode::DRAW_WIREFRAME;
        }
    }
    else
    {
        params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
    }

    GfMatrix4d viewMatrix(viewData);
    GfMatrix4d projectionMatrix(projectionData);

    // Put the camera and the viewport to the engine.
    renderer->SetCameraState(
        viewMatrix, projectionMatrix, GfVec4d(originX, originY, width, height));

    // Lighting. We need to query lights of the scene and provide them to Hydra.
    MStatus status;
    // Take into account only the 8 lights supported by the basic OpenGL
    // profile.
    const unsigned int nbLights =
        std::min(context.numberOfActiveLights(&status), 8u);

    GlfSimpleLightVector lights;
    if (nbLights > 0)
    {
        lights.reserve(nbLights);

        for (unsigned int i = 0; i < nbLights; ++i)
        {
            MFloatPointArray positions;
            MFloatVector direction;
            float intensity;
            MColor color;
            bool hasDirection;
            bool hasPosition;
            status = context.getLightInformation(
                i,
                positions,
                direction,
                intensity,
                color,
                hasDirection,
                hasPosition);
            if (status != MStatus::kSuccess || !hasPosition)
            {
                continue;
            }

            color *= intensity;

            GlfSimpleLight light;
            light.SetAmbient(GfVec4f(0.0f, 0.0f, 0.0f, 0.0f));
            light.SetDiffuse(GfVec4f(color[0], color[1], color[2], 1.0f));
            light.SetPosition(GfVec4f(
                positions[0][0], positions[0][1], positions[0][2], 1.0f));

            lights.push_back(light);
        }
    }

    if (lights.empty())
    {
        // There are no lights. We need to render something anyway, so put the
        // light to the camera position.
        GfVec3d translation = viewMatrix.GetInverse().ExtractTranslation();

        GlfSimpleLight light;
        light.SetAmbient(GfVec4f(0.0f, 0.0f, 0.0f, 0.0f));
        light.SetPosition(
            GfVec4f(translation[0], translation[1], translation[2], 1.0f));

        lights.push_back(light);
    }

    if (!lights.empty())
    {
        static const GfVec4f ambient(0.01f, 0.01f, 0.01f, 1.0f);
        static std::shared_ptr<GlfSimpleMaterial> material;
        if(!material)
        {
            // Init only once.
            material = std::make_shared<GlfSimpleMaterial>();
            material->SetSpecular(GfVec4f(0.05f, 0.05f, 0.05f, 1.0f));
            material->SetShininess(64.0);
        }

        renderer->SetLightingState(lights, *material, ambient);
    }

    {
        RDOPROFILE("Render with Hydra");

        renderer->Render(
            stage->GetPrimAtPath(SdfPath::AbsoluteRootPath()), params);
    }

    // Restore the state of the GL_CURRENT_PROGRAM.
    glUseProgram(lastProgram);

    // Clear OpenGL errors. Because UsdImagingGL::TestIntersection prints them.
    while (glGetError() != GL_NO_ERROR)
    {
    }
}

void DrawOverride::UserData::set(
        const MColor& wireframeColor,
        bool isSelected)
{
    mWireframeColor =
        GfVec4f(
            wireframeColor[0],
            wireframeColor[1],
            wireframeColor[2],
            1.0f);

    mIsSelected = isSelected;
}

void DrawOverride::drawCb(
    const MHWRender::MDrawContext& context,
    const MUserData* userData)

{
    const UserData* data = dynamic_cast<const UserData*>(userData);
    if (data)
    {
        data->draw(context);
    }
}

MHWRender::MPxDrawOverride* DrawOverride::creator(const MObject& obj)
{
    return new DrawOverride(obj);
}

DrawOverride::DrawOverride(const MObject& obj) :
    MPxDrawOverride(obj, &drawCb)
{}

DrawOverride::~DrawOverride()
{}

MHWRender::DrawAPI DrawOverride::supportedDrawAPIs() const
{
    // This draw override supports only OpenGL.
    return MHWRender::kOpenGL | MHWRender::kOpenGLCoreProfile;
}

bool DrawOverride::isBounded(
        const MDagPath& /*objPath*/,
        const MDagPath& /*cameraPath*/) const
{
    return true;
}

MBoundingBox DrawOverride::boundingBox(
    const MDagPath& objPath,
    const MDagPath& cameraPath) const
{
    // Extract the USD stuff.
    MStatus status;
    MFnDependencyNode node(objPath.node(), &status);
    if (!status)
    {
        return MBoundingBox();
    }

    const ShapeNode* shapeNode = dynamic_cast<ShapeNode*>(node.userNode());
    if (!shapeNode)
    {
        return MBoundingBox();
    }

    return shapeNode->boundingBox();
}

bool DrawOverride::disableInternalBoundingBoxDraw() const
{
    // Always return true since we will perform custom bounding box drawing.
    return true;
}

MUserData* DrawOverride::prepareForDraw(
    const MDagPath& objPath,
    const MDagPath& cameraPath,
    const MHWRender::MFrameContext& frameContext,
    MUserData* oldData)
{
    RDOPROFILE("");

    MObject object = objPath.node();

    // Retrieve data cache (create if does not exist)
    UserData* data = dynamic_cast<UserData*>(oldData);
    if (!data)
    {
        // Get the real ShapeNode from the MObject.
        MStatus status;
        MFnDependencyNode node(object, &status);
        if (status)
        {
            data = new UserData(
                    dynamic_cast<ShapeNode*>(node.userNode()));
        }
    }

    if (data)
    {
        // Compute data.
        const MColor wireframeColor =
            MHWRender::MGeometryUtilities::wireframeColor(objPath);

        const MHWRender::DisplayStatus displayStatus =
            MHWRender::MGeometryUtilities::displayStatus(objPath);
        const bool isSelected =
            (displayStatus == MHWRender::kActive) ||
            (displayStatus == MHWRender::kLead)   ||
            (displayStatus == MHWRender::kHilite);

        const unsigned int displayStyle = frameContext.getDisplayStyle();
        bool isTextured = displayStyle & MHWRender::MFrameContext::kTextured;

        // Prepare USD
        data->node()->updateUSDStage();
        data->node()->updateTextures(
            objPath.getDrawOverrideInfo().fEnableTexturing);
        data->set(wireframeColor, isSelected);
    }

    return data;
}

#if MAYA_API_VERSION >= 201800
bool DrawOverride::wantUserSelection() const { 
    return true; 
}

bool DrawOverride::refineSelectionPath(   
    const MSelectionInfo &selectInfo,
    const MRenderItem &hitItem,
    MDagPath &path,
    MObject &components,
    MSelectionMask &objectMask) {

    return MPxDrawOverride::refineSelectionPath(
        selectInfo, hitItem, path, components, objectMask);
}

bool DrawOverride::userSelect(
    MSelectionInfo &selectInfo,
    const MHWRender::MDrawContext &context,
    MPoint &hitPoint,
    const MUserData *data2) {

    const UserData* data = dynamic_cast<const UserData*>(data2);

    ShapeNode* node = data->node();

    // Get USD stuff from the node.
    UsdStageRefPtr stage = node->getStage();
    UsdImagingGLEngineSharedPtr renderer = node->getRenderer();
    if (!renderer || !stage)
    {
        return false;
    }

    M3dView view = M3dView::active3dView();
    GfMatrix4d viewMatrix;
    GfMatrix4d projectionMatrix;

    GLuint glHitRecord;
    view.beginSelect(&glHitRecord, 1);

    glGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix.GetArray());
    glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix.GetArray());

    view.endSelect();

    GfVec3d outHitPoint;
    SdfPath outHitPrimPath;
    UsdImagingGLRenderParams params = node->mParams;
    params.frame = UsdTimeCode(node->time * getCurrentFPS());
    params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
    bool didHit = renderer->TestIntersection(
        viewMatrix,
        projectionMatrix,
        GfMatrix4d(1.0),
        stage->GetPrimAtPath(SdfPath::AbsoluteRootPath()),
        params,
        &outHitPoint,
        &outHitPrimPath);

    // Sub-Selection
    if (didHit)
    {
        UsdPrim prim;

        // It can be an instance object. It's not allowed to get prim. We need
        // to find the first valid prim.
        while (true)
        {
            prim = stage->GetPrimAtPath(outHitPrimPath);

            if (prim.IsValid())
            {
                break;
            }

            outHitPrimPath = outHitPrimPath.GetParentPath();
        }

        assert(prim.IsValid());

        // Check the purpose of selected object.
        if (prim.IsA<UsdGeomImageable>())
        {
            UsdGeomImageable imageable(prim);
            if (imageable.ComputePurpose() == UsdGeomTokens->proxy)
            {
                // It's a proxy. Level up.
                outHitPrimPath = outHitPrimPath.GetParentPath();
            }
        }

        // If object was selected, set sub-selection.
        node->setSubSelectedObjects(outHitPrimPath.GetText());
        hitPoint = MPoint(outHitPoint[0], outHitPoint[1], outHitPoint[2]);

        // Execute the callback to change the UI.
        // selectPath() returns the tranform, multiPath() returns the shape.
        // Form the MEL command. It looks like this:
        // callbacks
        //      -executeCallbacks
        //      -hook walterPanelSelectionChanged
        //      "objectName" "subSelectedObject";
        MString command =
            "callbacks -executeCallbacks "
            "-hook walterPanelSelectionChanged \"";
        command += node->name();
        command += "\" \"";
        command += outHitPrimPath.GetText();
        command += "\";";
        MGlobal::executeCommandOnIdle(command);
    }

    else
    {
        // If object was not selected, clear sub-selection.
        node->setSubSelectedObjects("");
    }

    return didHit;
}
#endif
}
