// Copyright 2017 Rodeo FX. All rights reserved.

/*
This test check that we can load glslfx from the Walter resources.
To do that we are patching the Usd file 'glslfx.cpp' by overriding
_GetFileStream() (that is called in GlfGLSLFX constructor).
*/

#include <pxr/imaging/glf/glslfx.h>
#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE

const std::string sources = R"(// line 50 "fallbackSurface.glslfx"

vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord)
{
    // lighting
    color.rgb = FallbackLighting(Peye.xyz, Neye, color.rgb);
    return color;
}

)";

TEST(test_resources_GetFileStream, fallbackSurface)
{
    GlfGLSLFX glslfx("fallbackSurface.glslfx");
    EXPECT_TRUE(glslfx.IsValid());
    EXPECT_EQ(glslfx.GetSurfaceSource(), sources);
}

