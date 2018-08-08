// Copyright 2017 Rodeo FX. All rights reserved.

/* 
Those tests check that we can load good tokens from the Walter resources when
calling the HdStPackage* functions.
To do that we are patching the Usd file 'package.cpp' by overriding _GetShaderPath().
*/

#include <pxr/imaging/hdSt/package.h>
#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE


TEST(test_resources_GetShaderPaths, fallbackSurface)
{
    TfToken s = HdStPackageFallbackSurfaceShader();
    EXPECT_EQ(s, "fallbackSurface.glslfx");
}

TEST(test_resources_getShaderPaths, ptexTexture)
{
    TfToken s = HdStPackagePtexTextureShader();
    EXPECT_EQ(s, "ptexTexture.glslfx");
}

TEST(test_resources_getShaderPaths, renderPassShader)
{
    TfToken s = HdStPackageRenderPassShader();
    EXPECT_EQ(s, "renderPassShader.glslfx");
}

TEST(test_resources_getShaderPaths, fallbackLightingShader)
{
    TfToken s = HdStPackageFallbackLightingShader();
    EXPECT_EQ(s, "fallbackLightingShader.glslfx");
}

TEST(test_resources_getShaderPaths, lightingIntegrationShader)
{
    TfToken s = HdStPackageLightingIntegrationShader();
    EXPECT_EQ(s, "lightingIntegrationShader.glslfx");
}
