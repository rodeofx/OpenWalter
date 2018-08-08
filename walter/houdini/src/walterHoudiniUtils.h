// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __walterHoudiniUtils_h__
#define __walterHoudiniUtils_h__

#include <pxr/usd/usd/stage.h>
#include <pxr/usdImaging/usdImagingGL/gl.h>

PXR_NAMESPACE_USING_DIRECTIVE

typedef std::shared_ptr<UsdImagingGL> UsdImagingGLRefPtr;

namespace WalterHoudiniUtils
{
// Return the stage
UsdStageRefPtr getStage(const std::string& stageName);

// Return the Hydra renderer
UsdImagingGLRefPtr getRenderer(const std::string& stageName);

// Fill the output with the object names
void getObjectList(
    const std::string& stageName,
    const std::string& primPath,
    bool geomOnly,
    std::vector<std::string>& oList);

template <class To, class From> inline To MatrixConvert(const From& from)
{
    // It's very ugly. Can't beleive GfMatrix4d can't take a pointer to the
    // data.
    return To(
        from[0][0],
        from[0][1],
        from[0][2],
        from[0][3],
        from[1][0],
        from[1][1],
        from[1][2],
        from[1][3],
        from[2][0],
        from[2][1],
        from[2][2],
        from[2][3],
        from[3][0],
        from[3][1],
        from[3][2],
        from[3][3]);
}
}

#endif
