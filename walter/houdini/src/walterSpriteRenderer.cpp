// Copyright 2017 Rodeo FX.  All rights reserved.

#include "walterSpriteRenderer.h"

static const char* sBeautyVertexSrc =
    "#version 420\n"
    "layout(location = 0) in vec3 P;"
    "layout(location = 1) in vec2 st;"
    "out vec2 uv;"
    "void main()"
    "{"
    "  uv = st;"
    "  gl_Position = vec4(P, 1.0);"
    "}";

static const char* sBeautyFragmentSrc =
    "#version 420\n"
    "in vec2 uv;"
    "layout (binding = 0) uniform sampler2D colorTexture;"
    "layout (binding = 1) uniform sampler2D depthTexture;"
    "void main()"
    "{"
    "  gl_FragColor = texture(colorTexture, uv);"
    "  gl_FragDepth = texture(depthTexture, uv).x;"
    "}";

static const char* sPickFragmentSrc =
    "#version 420\n"
    "in vec2 uv;"
    "uniform PickSettings"
    "{"
    "  ivec3 glH_PickBaseID;"
    "  ivec3 glH_PickComponentID;"
    "};"
    "layout (binding = 0) uniform sampler2D colorTexture;"
    "layout (binding = 1) uniform sampler2D depthTexture;"
    "out ivec3 pickBaseID;"
    "out ivec3 pickComponentID;"
    "void main()"
    "{"
    "  vec4 Ci = texture(colorTexture, uv);"
    "  float alpha = Ci.a;"
    "  float depth = abs(texture(depthTexture, uv).x);"
    "  if (alpha < 1)"
    "  {"
    "    gl_FragDepth = 1.0;"
    "    pickBaseID = ivec3(0, 0, 0);"
    "    pickComponentID = ivec3(0, 0, 0);"
    "  }"
    "  else"
    "  {"
    "   pickBaseID = glH_PickBaseID;"
    "   pickComponentID = glH_PickComponentID;"
    "   gl_FragDepth = depth;"
    "  }"
    "}";

static const char* sMatteFragmentSrc =
    "#version 420\n"
    "in vec2 uv;"
    "layout (binding = 0) uniform sampler2D colorTexture;"
    "layout (binding = 1) uniform sampler2D depthTexture;"
    "void main()"
    "{"
    "  gl_FragColor = vec4(1.0, 0.699394, 0.0, 0.25);"
    "  gl_FragDepth = texture(depthTexture, uv).x;"
    "}";

std::shared_ptr<WalterSpriteRenderer::GLSLProgram>
    WalterSpriteRenderer::SpriteRenderer::sBeautyShader;
std::shared_ptr<WalterSpriteRenderer::GLSLProgram>
    WalterSpriteRenderer::SpriteRenderer::sPickShader;
std::shared_ptr<WalterSpriteRenderer::GLSLProgram>
    WalterSpriteRenderer::SpriteRenderer::sMatteShader;

void printCompileErrors(GLuint iShaderID)
{
    GLint maxLength = 0;
    glGetShaderiv(iShaderID, GL_INFO_LOG_LENGTH, &maxLength);

    // The maxLength includes the NULL character
    std::vector<GLchar> errorLog(maxLength);
    glGetShaderInfoLog(iShaderID, maxLength, &maxLength, &errorLog[0]);

    printf(
        "ERROR: Shader %i is not compiled:\n{\n%s\n}\n",
        iShaderID,
        &errorLog[0]);
}

WalterSpriteRenderer::GLSLProgram::GLSLProgram(
    const char* iVertexSrc,
    const char* iFragmentSrc)
{
    // Copy the shader strings into GL shaders, and compile them. Then
    // create an executable shader and attach both of the compiled shaders,
    // link this, which matches the outputs of the vertex shader to the
    // inputs of the fragment shader, etc. Add it is then ready to use.
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &iVertexSrc, NULL);
    glCompileShader(vs);

    GLint success = GL_FALSE;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        printCompileErrors(vs);
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &iFragmentSrc, NULL);
    glCompileShader(fs);

    success = GL_FALSE;
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        printCompileErrors(vs);
    }

    mShaderProgram = glCreateProgram();
    glAttachShader(mShaderProgram, fs);
    glAttachShader(mShaderProgram, vs);
    glLinkProgram(mShaderProgram);

    // TODO: Release attached shaders?
}

void WalterSpriteRenderer::GLSLProgram::use()
{
    assert(mShaderProgram);
    glUseProgram(mShaderProgram);
}

WalterSpriteRenderer::GLScopeGuard::GLScopeGuard(RE_Render* r) :
        mRender(r),
        lock(r)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glGetIntegerv(GL_CURRENT_PROGRAM, &mLastProgram);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &mLastTexture);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &mLastVAO);
}

WalterSpriteRenderer::GLScopeGuard::~GLScopeGuard()
{
    glUseProgram(mLastProgram);
    glBindTexture(GL_TEXTURE_2D, mLastTexture);
    glBindVertexArray(mLastVAO);
    glPopAttrib();
}

WalterSpriteRenderer::SpriteRenderer::SpriteRenderer() :
        mVertexArray(0),
        mPointBuffer(0),
        mUVBuffer(0),
        mPickInitialized(false),
        mInitialized(false)
{}

void WalterSpriteRenderer::SpriteRenderer::drawToBufferBegin()
{
    init();

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    if (!mHyraDrawTarget)
    {
        // Initialize hydra framebuffer to viewport width and height.
        mHyraDrawTarget = GlfDrawTarget::New(GfVec2i(viewport[2], viewport[3]));

        mHyraDrawTarget->Bind();
        // Add color and depth buffers.
        mHyraDrawTarget->AddAttachment("color", GL_RGBA, GL_FLOAT, GL_RGBA);
        mHyraDrawTarget->AddAttachment(
            "depth",
            GL_DEPTH_STENCIL,
            GL_UNSIGNED_INT_24_8,
            GL_DEPTH24_STENCIL8);
    }
    else
    {
        mHyraDrawTarget->Bind();
        mHyraDrawTarget->SetSize(GfVec2i(viewport[2], viewport[3]));
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Clear the framebuffer's colour and depth buffers.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void WalterSpriteRenderer::SpriteRenderer::drawToBufferEnd()
{
    mHyraDrawTarget->Unbind();
}

void WalterSpriteRenderer::SpriteRenderer::drawBeauty() const
{
    assert(sBeautyShader);

    sBeautyShader->use();

    drawPoly();
}

void WalterSpriteRenderer::SpriteRenderer::drawPick(
    int* iPickBaseID,
    int* iPickCompID)
{
    initPick(iPickBaseID, iPickCompID);

    assert(sPickShader);
    assert(mUniformPickBuffer);

    sPickShader->use();
    glBindBufferBase(GL_UNIFORM_BUFFER, mUniformPickIndex, mUniformPickBuffer);

    drawPoly();
}

void WalterSpriteRenderer::SpriteRenderer::drawMatte() const
{
    assert(sBeautyShader);

    sMatteShader->use();

    drawPoly();
}

void WalterSpriteRenderer::SpriteRenderer::init()
{
    if (mInitialized)
    {
        return;
    }
    // Never repeat it again.
    mInitialized = true;

    if (!sBeautyShader)
    {
        sBeautyShader =
            std::make_shared<GLSLProgram>(sBeautyVertexSrc, sBeautyFragmentSrc);
    }

    if (!sPickShader)
    {
        sPickShader =
            std::make_shared<GLSLProgram>(sBeautyVertexSrc, sPickFragmentSrc);
    }

    if (!sMatteShader)
    {
        sMatteShader =
            std::make_shared<GLSLProgram>(sBeautyVertexSrc, sMatteFragmentSrc);
    }

    // Geometry to use. These are 4 xyz points to make a quad.
    static const GLfloat points[] = {
        -1.f, -1.f, 0.f, 1.f, -1.f, 0.f, 1.f, 1.f, 0.f, -1., 1., 0.};
    // These are 4 UVs.
    static const GLfloat uvs[] = {0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f};

    // Create a vertex buffer object. It stores an array of data on the
    // graphics adapter's memory. The vertex points in our case.
    glGenBuffers(1, &mPointBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mPointBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

    // The same for UVs.
    glGenBuffers(1, &mUVBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mUVBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);

    // The vertex array object is a descriptor that defines which data from
    // vertex buffer objects should be used as input variables to vertex
    // shaders.
    glGenVertexArrays(1, &mVertexArray);
    glBindVertexArray(mVertexArray);

    // Normally we need to bind vertes buffer objects here. But since
    // Houdini resets them each frame we
}

void WalterSpriteRenderer::SpriteRenderer::initPick(
    int* iPickBaseID,
    int* iPickCompID)
{
    if (mPickInitialized)
    {
        return;
    }
    // Never repeat it again.
    mPickInitialized = true;

    mUniformPickIndex =
        glGetUniformBlockIndex(sPickShader->getID(), "PickSettings");

    GLint blockSize;
    glGetActiveUniformBlockiv(
        sPickShader->getID(),
        mUniformPickIndex,
        GL_UNIFORM_BLOCK_DATA_SIZE,
        &blockSize);

    GLubyte* blockBuffer = (GLubyte*)malloc(blockSize);

    // Query for the offsets of each block variable
    const GLchar* names[] = {"glH_PickBaseID", "glH_PickComponentID"};

    GLuint indices[2];
    glGetUniformIndices(sPickShader->getID(), 2, names, indices);

    GLint offset[2];
    glGetActiveUniformsiv(
        sPickShader->getID(), 2, indices, GL_UNIFORM_OFFSET, offset);

    memcpy(blockBuffer + offset[0], iPickBaseID, 3 * sizeof(GLint));
    memcpy(blockBuffer + offset[1], iPickCompID, 3 * sizeof(GLint));

    glGenBuffers(1, &mUniformPickBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, mUniformPickBuffer);
    glBufferData(GL_UNIFORM_BUFFER, blockSize, blockBuffer, GL_STATIC_DRAW);

    free(blockBuffer);
}

void WalterSpriteRenderer::SpriteRenderer::drawPoly() const
{
    assert(mVertexArray);
    assert(mPointBuffer);
    assert(mUVBuffer);

    glBindVertexArray(mVertexArray);

    // Normally we only should do it on init, but Houdini resets buffers.
    glBindBuffer(GL_ARRAY_BUFFER, mPointBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glBindBuffer(GL_ARRAY_BUFFER, mUVBuffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // Get color and depts buffers as texture.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(
        GL_TEXTURE_2D,
        mHyraDrawTarget->GetAttachment("color")->GetGlTextureName());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(
        GL_TEXTURE_2D,
        mHyraDrawTarget->GetAttachment("depth")->GetGlTextureName());

    // Draw points 0-4 from the currently bound VAO with current in-use
    // shader.
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
