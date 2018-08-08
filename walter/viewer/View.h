// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERVIEWER_VIEW_H__
#define __WALTERVIEWER_VIEW_H__

#include "Types.h"

#include <pxr/base/gf/vec4d.h>
#include <pxr/imaging/glf/glew.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>

#include <GL/gl.h>
#include <GLFW/glfw3.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace walter_viewer
{

ViewPtr getView();

class Scene;

enum class ButtonPressState { released, click, drag };
enum class ButtonPressed {undefined, left, middle, right };
enum class InteractionMode {camera, selection };

class View
{
    class SRGBContext
    {
    public:
        SRGBContext(GfVec4d clearColor);
        ~SRGBContext();
    };

public:
    View();
    ~View();
    bool show();
    bool isValid() const { return mIsValid; }
    void setScene(ScenePtr scene);
    const ScenePtr scene() { return mScene; }

    void refresh();
    // void mouseReleased();
    void mouseClickCallback(ButtonPressed button);
    void mouseMoveCallback(int x, int y);

    ButtonPressState pressState{ButtonPressState::released};
    ButtonPressed button{ButtonPressed::undefined};

    int width{0};
    int height{0};
    InteractionMode mode{InteractionMode::camera};
    UsdImagingGLEngine::DrawMode drawMode{UsdImagingGLEngine::DRAW_SHADED_SMOOTH};

private:
    GLFWwindow* mWindow;

    bool mIsValid{false};
    bool mFirstRefresh{true};
    ScenePtr mScene;
};


} // end namespace walter_viewer
#endif
