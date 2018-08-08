// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERVIEWER_SCENE_H__
#define __WALTERVIEWER_SCENE_H__

#include "Types.h"

#include <pxr/usd/usd/stage.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>
#include <pxr/usdImaging/usdImagingGL/gl.h>

#include <memory>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace walter_viewer
{
typedef std::shared_ptr<UsdImagingGL> UsdImagingGLRefPtr;

class Scene
{
public:
    struct Options
    {
        float frame {1.f};
        float complexity { 1.1f};
        std::string filePath;
        std::string renderer {"Stream"};
    };

    Scene(
        const Options& opt,
        FreeCameraPtr camera = nullptr);

    void setRenderer(const std::string& name);
    void draw(int width, int height);
    void framed() { mFramed = true; }
    void resetCamera();
    void updateCamera(int height);
    void frameSelection();
    void select(double xpos, double ypos, int width, int height);
    void updateSubdiv(float value);
    void setDrawMode(UsdImagingGLEngine::DrawMode value);
    void Export(const std::string& outputPath) const;
    FreeCameraPtr camera() { return mCamera; }

private:
    static GfBBox3d getDefaultBBox();
    GfBBox3d computeStageBBox();

private:
    bool mFramed{false};
    FreeCameraPtr mCamera;
    UsdStageRefPtr mStage;
    UsdPrim mDefaultPrim;
    UsdImagingGLEngine::RenderParams mParams;
    UsdImagingGLRefPtr mRenderer;
};

} // end namespace walter_viewer
#endif
