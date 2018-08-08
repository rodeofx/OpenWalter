// Copyright 2017 Rodeo FX.  All rights reserved.

#include "View.h"
#include "CameraControler.h"
#include "FreeCamera.h"
#include "Scene.h"

#include <pxr/base/gf/gamma.h>

#include <cstdio>

PXR_NAMESPACE_USING_DIRECTIVE

static walter_viewer::ViewPtr sView = nullptr;

static void errorCallback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void keyCallback(
    GLFWwindow* window,
    int key,
    int scancode,
    int action,
    int mods)
{
    if (!sView)
        return;

    if (key == GLFW_KEY_KP_ADD && action == GLFW_PRESS)
    {
        sView->scene()->updateSubdiv(0.1f);
    }

    if (key == GLFW_KEY_KP_SUBTRACT && action == GLFW_PRESS)
    {
        sView->scene()->updateSubdiv(-0.1f);
    }

    if (key == GLFW_KEY_W && action == GLFW_PRESS)
    {
        if (sView->drawMode == UsdImagingGLEngine::DRAW_SHADED_SMOOTH)
        {
            sView->drawMode = UsdImagingGLEngine::DRAW_WIREFRAME;
        }
        else
        {
            sView->drawMode = UsdImagingGLEngine::DRAW_SHADED_SMOOTH;
        }
        sView->scene()->setDrawMode(sView->drawMode);
    }

    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    {
        sView->mode = walter_viewer::InteractionMode::selection;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        sView->mode = walter_viewer::InteractionMode::camera;
    }

    if (key == GLFW_KEY_E && action == GLFW_PRESS)
    {
        sView->scene()->setRenderer("Embree");
    }
    else if (key == GLFW_KEY_S && action == GLFW_PRESS)
    {
        sView->scene()->setRenderer("Stream");
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        sView->scene()->frameSelection();
        sView->scene()->updateCamera(sView->height);
    }
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        sView->scene()->resetCamera();
    }
}

static void mouseButtonCallback(
    GLFWwindow* window,
    int button,
    int action,
    int mods)
{
    if (!sView)
        return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        if (sView->pressState == walter_viewer::ButtonPressState::released)
        {
            sView->pressState = walter_viewer::ButtonPressState::click;
            sView->mouseClickCallback(walter_viewer::ButtonPressed::left);
        }
    }

    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        if (sView->pressState == walter_viewer::ButtonPressState::released)
        {
            sView->pressState = walter_viewer::ButtonPressState::click;
            sView->mouseClickCallback(walter_viewer::ButtonPressed::middle);
        }
    }

    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        if (sView->pressState == walter_viewer::ButtonPressState::released)
        {
            sView->pressState = walter_viewer::ButtonPressState::click;
            sView->mouseClickCallback(walter_viewer::ButtonPressed::right);
        }
    }

    else if (action == GLFW_RELEASE)
    {
        sView->pressState = walter_viewer::ButtonPressState::released;
    }
}

void cursorPosCallback(GLFWwindow*, double x, double y)
{
    if (sView)
    {
        sView->mouseMoveCallback(int(x), int(y));
    }
}

namespace walter_viewer
{

View::View()
{
    if (glfwInit())
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        glfwSetErrorCallback(errorCallback);

        mWindow = glfwCreateWindow(640, 480, "Walter Viewer", nullptr, nullptr);
        if (!mWindow)
        {
            glfwTerminate();
        }
        else
        {
            glfwMakeContextCurrent(mWindow);
            glfwSetWindowPos(mWindow, 200, 200);
            glewExperimental = GL_TRUE;
            GlfGlewInit();
            mIsValid = true;
        }
    }
}

View::~View()
{
    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

bool View::show()
{
    if (mIsValid)
    {
        glfwShowWindow(mWindow);
        glfwSetKeyCallback(mWindow, keyCallback);
        glfwSetMouseButtonCallback(mWindow, mouseButtonCallback);
        glfwSetCursorPosCallback(mWindow, cursorPosCallback);
    }
    return mIsValid;
}

void View::setScene(ScenePtr scene)
{
    mScene = scene;
    mScene->frameSelection();
}

void View::refresh()
{
    while (!glfwWindowShouldClose(mWindow))
    {
        // Check if any events have been activiated (key pressed, mouse moved
        // etc.) and call corresponding response functions.
        glfwPollEvents();

        glfwGetFramebufferSize(mWindow, &width, &height);
        glViewport(0, 0, width, height);

        {
            GfVec4d clearColor(0.333, 0.333, 0.333, 1.0);
            SRGBContext ctx(clearColor);

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (this->pressState == ButtonPressState::drag || mFirstRefresh)
            {
                mScene->updateCamera(height);
                mFirstRefresh = false;
            }

            mScene->draw(width, height);
            // When rendering after a call to UsdImagingGL::TestIntersection,
            // a lot of errors are printing. We clear them.
            // TODO: investigate why it prints so many errors.
            while (glGetError() != GL_NO_ERROR)
            {
            }

            // Swap the screen buffers.
            glfwSwapBuffers(mWindow);
        }
    }
}

void View::mouseClickCallback(ButtonPressed button)
{
    this->button = button;
    if (this->mode == InteractionMode::selection)
    {
        double xpos, ypos;
        glfwGetCursorPos(mWindow, &xpos, &ypos);
        mScene->select(xpos, ypos, width, height);
    }
}

void View::mouseMoveCallback(int x, int y)
{
    if (this->mode == InteractionMode::camera)
    {
        if (this->pressState == ButtonPressState::released)
        {
            mScene->camera()->controler().setPanning(false);
            mScene->camera()->controler().setZoom(0.f);
            mScene->camera()->controler().setPan(0.f, 0.f);
            return;
        }

        if (this->pressState == ButtonPressState::click)
        {
            mScene->camera()->controler().mousePosLastX = x;
            mScene->camera()->controler().mousePosLastY = y;
            this->pressState = ButtonPressState::drag;
        }

        int dx = x - mScene->camera()->controler().mousePosLastX;
        int dy = y - mScene->camera()->controler().mousePosLastY;
        if (dx == 0 && dy == 0)
        {
            return;
        }

        mScene->camera()->controler().mousePosLastX = x;
        mScene->camera()->controler().mousePosLastY = y;

        switch (this->button)
        {
            case ButtonPressed::undefined:
                break;
            case ButtonPressed::left:
                mScene->camera()->controler().updateRotation(dx, dy);
                break;
            case ButtonPressed::middle:
                mScene->camera()->controler().setPanning(true);
                mScene->camera()->controler().setZoom(0.f);
                mScene->camera()->controler().setPan(dx, dy);
                break;
            case ButtonPressed::right:
                mScene->camera()->controler().updateZoom(dx, dy);
                break;
        }
    }
}

View::SRGBContext::SRGBContext(GfVec4d clearColor)
{
    glEnable(GL_FRAMEBUFFER_SRGB_EXT);

    clearColor = GfConvertDisplayToLinear(clearColor);
    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
}

View::SRGBContext::~SRGBContext()
{
    glDisable(GL_FRAMEBUFFER_SRGB_EXT);
}

ViewPtr getView()
{
    if (!sView)
    {
        sView = new View;
    }
    return sView;
}

} // end namespace walter_viewer
