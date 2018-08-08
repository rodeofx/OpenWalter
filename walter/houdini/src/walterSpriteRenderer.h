// Copyright 2017 Rodeo FX.  All rights reserved.

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <glcorearb.h>

#include <RE/RE_Render.h>
#include <pxr/imaging/glf/drawTarget.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace WalterSpriteRenderer
{
/** @brief A handle for a single OpenGL shader. */
class GLSLProgram
{
public:
    /**
     * @brief Initialize GLSL shader and save the ID.
     *
     * @param iVertexSrc the source of the vertex shader.
     * @param iFragmentSrc the source of the fragment shader.
     */
    GLSLProgram(const char* iVertexSrc, const char* iFragmentSrc);

    /** @brief Installs the shader program object as part of current rendering
     * state. */
    void use();

    /**
     * @brief Returns the ID of the shader program object.
     *
     * @return ID of the shader program object.
     */
    GLuint getID() { return mShaderProgram; }

private:
    GLuint mShaderProgram;
};

/** @brief It saves the OpenGL state when constructed and restores it when
 * deconstructed. */
class GLScopeGuard
{
public:
    GLScopeGuard(RE_Render* r);
    ~GLScopeGuard();

private:
    GLint mLastProgram;
    GLint mLastTexture;
    GLint mLastVAO;
    RE_Render* mRender;
    RE_RenderAutoLock lock;
};

/** @brief A helper to simplify rendering to the external framebuffer and move
 * the rendered data back to the current framebuffer. We need for two reasons:
 * 1) Transfer data from the raytracer plugin to the current framebuffer.
 * 2) Replace the color of the rendered object to produce the Houdini ID pass.
 */
class SpriteRenderer
{
public:
    SpriteRenderer();

    /** @brief Changes the current framebuffer. All the OpenGL draws will be
     * performed in the external texture framebuffer. */
    void drawToBufferBegin();
    /** @brief Restores framebuffer back to normal. */
    void drawToBufferEnd();
    /** @brief Draws the color and the depth data saved with
     * drawToBufferBegin/drawToBufferEnd to the current framebuffer. */
    void drawBeauty() const;
    /** @brief Draws the Houdini pass ID from the data saved with
     * drawToBufferBegin/drawToBufferEnd to the current framebuffer. */
    void drawPick(int* iPickBaseID, int* iPickCompID);
    /** @brief Draws the matte pass from the data saved with
     * drawToBufferBegin/drawToBufferEnd to the current framebuffer. */
    void drawMatte() const;

private:
    /** @brief Init the OpenGL buffers. */
    void init();
    /** @brief Init the OpenGL buffers for Houdini pass ID. */
    void initPick(int* iPickBaseID, int* iPickCompID);
    /** @brief Draw a quad that takes all the area of the viewport. */
    void drawPoly() const;

    // OpenGL buffer for render to.
    GlfDrawTargetRefPtr mHyraDrawTarget;

    // OpenGL gometry buffers.
    GLuint mVertexArray;
    GLuint mPointBuffer;
    GLuint mUVBuffer;

    // Flags that the buffers are already initialized.
    bool mPickInitialized;
    bool mInitialized;

    // The index of the binding point of the uniform block of the fragment
    // shader.
    GLuint mUniformPickIndex;
    // The name of a buffer object to bind to the specified binding point.
    GLuint mUniformPickBuffer;

    // The shaders. They are static because we don't need to produce a shader
    // per object.
    static std::shared_ptr<GLSLProgram> sBeautyShader;
    static std::shared_ptr<GLSLProgram> sPickShader;
    static std::shared_ptr<GLSLProgram> sMatteShader;
};
}
