// Copyright 2017 Rodeo FX. All rights reserved.

#ifndef __ABCCORELAYERRESOLVER_H_
#define __ABCCORELAYERRESOLVER_H_

#include <pxr/usd/ar/defaultResolver.h>

PXR_NAMESPACE_OPEN_SCOPE

class AbcCoreLayerResolver : public ArDefaultResolver
{
    typedef ArDefaultResolver BaseType;

public:
    AbcCoreLayerResolver();

    // Returns the resolved filesystem path for the file identified by path.
    virtual std::string ResolveWithAssetInfo(
            const std::string& path,
            ArAssetInfo* assetInfo) override;

    // Return a value representing the last time the asset identified by path
    // was modified. resolvedPath is the resolved path of the asset.
    virtual VtValue GetModificationTimestamp(
            const std::string& path,
            const std::string& resolvedPath) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif

