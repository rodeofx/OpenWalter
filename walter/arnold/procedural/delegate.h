// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __DELEGATE_H__
#define __DELEGATE_H__

#include "index.h"
#include "plugin.h"

#include <pxr/usd/usd/prim.h>
#include <tbb/concurrent_hash_map.h>

PXR_NAMESPACE_USING_DIRECTIVE

class RendererDelegate
{
public:
    RendererDelegate(RendererIndex& index, RendererPlugin& plugin);

    // Populate index with all the objects.
    void populate(const UsdPrim& root);

private:
    // A cache with the necessary objects.
    RendererIndex& mIndex;
    // Renderer. For now it's Arnold only. But in future we will be able to
    // initialize other render plugins.
    RendererPlugin& mPlugin;

    // We use int as a dummy type. We only need to lock if the same SdfPath is
    // in process.
    typedef tbb::concurrent_hash_map<SdfPath, int, RendererIndex::TbbHash>
        SdfPathMutex;
    SdfPathMutex mPopulateGuard;
};

#endif
