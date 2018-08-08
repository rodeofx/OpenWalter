// Copyright 2017 Rodeo FX.  All rights reserved.

#include "Scene.h"
#include "FreeCamera.h"
#include "walterUSDCommonUtils.h"

#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/bboxCache.h>
#include <pxr/usd/usdGeom/camera.h>

namespace walter_viewer
{

Scene::Scene(
    const Options& opt,
    FreeCameraPtr camera)
{
    SdfLayerRefPtr root = WalterUSDCommonUtils::getUSDLayer(opt.filePath);
    mStage = UsdStage::Open(root);

    if(camera)
    {
        mCamera = camera;
        mCamera->reset();        

        mRenderer = std::make_shared<UsdImagingGL>();

        setRenderer(opt.renderer);

        mParams.frame = opt.frame;
        mParams.complexity = opt.complexity;
        mParams.highlight = true;

        mDefaultPrim = mStage->GetDefaultPrim();
        if (!mDefaultPrim)
        {
            mDefaultPrim = mStage->GetPseudoRoot();
        }   
    }
}

void Scene::setRenderer(const std::string& name)
{
    if (name == "Embree")
    {
        mRenderer->SetRendererPlugin(TfToken("HdEmbreeRendererPlugin"));
    }
    else
    {
        mRenderer->SetRendererPlugin(TfToken("HdStreamRendererPlugin"));
    }
}

void Scene::resetCamera()
{
    mCamera->reset();
}

void Scene::updateCamera(int height)
{
    mCamera->updateZoom();
    mCamera->updatePan(height);
    mCamera->updateTumble();
}

void Scene::frameSelection()
{
    GfBBox3d bbox = computeStageBBox();
    if (bbox.GetRange().IsEmpty())
    {
        bbox = getDefaultBBox();
    }

    auto center = bbox.ComputeCentroid();
    mCamera->controler().setCenter(center[0], center[1], center[2]);

    const float frameFit = 1.1f;
    float fov = mCamera->GetFieldOfView(GfCamera::FOVVertical);
    float halfFov = fov > 0 ? fov * 0.5f : 0.5f;

    auto range = bbox.ComputeAlignedRange();
    std::array<double, 3> rangeArray{
        {range.GetSize()[0], range.GetSize()[1], range.GetSize()[1]}};

    auto maxSize = *std::max_element(rangeArray.begin(), rangeArray.end());
    float dist = (maxSize * frameFit * 0.5) /
        atan(GfDegreesToRadians(static_cast<double>(halfFov)));

    mCamera->SetFocusDistance(dist);
}

void Scene::select(double xpos, double ypos, int width, int height)
{
    GfVec2d point(xpos / double(width), ypos / double(height));
    point[0] = (point[0] * 2.0 - 1.0);
    point[1] = -1.0 * (point[1] * 2.0 - 1.0);

    auto size = GfVec2d(1.0 / width, 1.0 / height);

    auto pickFrustum =
        mCamera->GetFrustum().ComputeNarrowedFrustum(point, size);

    GfVec3d hitPoint;
    SdfPath hitPrimPath;
    SdfPath hitInstancerPath;
    int hitInstanceIndex;
    int hitElementIndex;

    auto results = mRenderer->TestIntersection(
        pickFrustum.ComputeViewMatrix(),
        pickFrustum.ComputeProjectionMatrix(),
        GfMatrix4d(1.0),
        mStage->GetPseudoRoot(),
        mParams,
        &hitPoint,
        &hitPrimPath,
        &hitInstancerPath,
        &hitInstanceIndex,
        &hitElementIndex);

    mRenderer->ClearSelected();
    if (!hitPrimPath.IsEmpty())
    {
        SdfPathVector selection;
        selection.push_back(hitPrimPath);
        mRenderer->SetSelected(selection);
        printf("[Walter] Prim path:%s\n", hitPrimPath.GetText());
    }
}

void Scene::updateSubdiv(float value)
{
    mParams.complexity += value;
    if (mParams.complexity < 1.f)
    {
        mParams.complexity = 1.f;
    }
    else if (mParams.complexity > 2.f)
    {
        mParams.complexity = 2.f;
    }
}

void Scene::setDrawMode(UsdImagingGLEngine::DrawMode value)
{
    mParams.drawMode = value;
}

void Scene::draw(int width, int height)
{
    auto frustum = mCamera->GetFrustum();

    mRenderer->SetCameraState(
        frustum.ComputeViewMatrix(),
        frustum.ComputeProjectionMatrix(),
        GfVec4d(0, 0, width, height));

    // Add a light positonned at the camera
    auto light = GlfSimpleLight();
    light.SetAmbient(GfVec4f(0.0f));

    auto position = frustum.GetPosition();
    light.SetPosition(GfVec4f(
        static_cast<float>(position[0]),
        static_cast<float>(position[1]),
        static_cast<float>(position[2]),
        1.0f));

    GlfSimpleLightVector lights;
    lights.push_back(light);

    auto material = GlfSimpleMaterial();
    GfVec4f sceneAmbient(0.01f);
    
    mRenderer->SetLightingState(lights, material, sceneAmbient);

    mRenderer->Render(mDefaultPrim, mParams);

    if (this->mFramed)
    {
        auto angles = mCamera->angles();
        mCamera->controler().setRotation(angles[0], angles[1], angles[2]);
    }
    this->mFramed = false;
}

GfBBox3d Scene::getDefaultBBox()
{
    return GfBBox3d(GfRange3d(GfVec3d(-10.0), GfVec3d(10.0)));
}

GfBBox3d Scene::computeStageBBox()
{
    TfTokenVector includedPurposes;
    includedPurposes.push_back(UsdGeomTokens->default_);
    includedPurposes.push_back(UsdGeomTokens->render);
    bool useExtentsHint = true;
    UsdGeomBBoxCache bboxCache(mParams.frame, includedPurposes, useExtentsHint);
    return bboxCache.ComputeWorldBound(mDefaultPrim);
}

void Scene::Export(const std::string& outputPath) const
{
    if(outputPath != "")
    {
        mStage->Export(outputPath);
        std::cout << "[walterViewer] USD stage exported to " << outputPath << '\n';
    }     
}
} // end namespace walter_viewer