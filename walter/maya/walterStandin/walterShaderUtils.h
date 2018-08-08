// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERSHADERUTILS_H_
#define __WALTERSHADERUTILS_H_

#include <pxr/base/gf/vec3f.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace walterShaderUtils
{
/**
 * @brief Scans shading network and returns the main texture or the main color
 * if the texture is not found.
 *
 * @param shaderObj Maya shader as MObject or UsdShadeShader.
 * @param oTexture Returned texture.
 * @param oColor Returned color.
 */
template <typename T>
void getTextureAndColor(
    const T& shaderObj,
    std::string& oTexture,
    GfVec3f& oColor);

/**
 * @brief Checks the image and saves only one specific mipmap to the image in
 * the temporary directory.
 *
 * @param filename Requested image.
 *
 * @return The new image path if the mipmap was successfully copied or the input
 * path otherwise.
 */
std::string cacheTexture(const std::string& filename);

/**
 * @brief USD routine to get the connected scheme.
 *
 * @tparam T The desired scheme.
 * @param attr The destination attribute to querry the connection.
 *
 * @return Requested scheme.
 */
template <class T> T getConnectedScheme(const UsdAttribute& attr)
{
    // Get the connection using ConnectableAPI.
    UsdShadeConnectableAPI connectableAPISource;
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    if (!UsdShadeConnectableAPI::GetConnectedSource(
            attr, &connectableAPISource, &sourceName, &sourceType))
    {
        // No connection.
        return T();
    }

    const UsdPrim prim = connectableAPISource.GetPrim();

    if (!prim || !prim.IsA<T>())
    {
        // Different object.
        return T();
    }

    return T(prim);
}
}

#endif
