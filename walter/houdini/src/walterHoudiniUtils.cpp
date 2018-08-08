// Copyright 2017 Rodeo FX.  All rights reserved.

#include "walterHoudiniUtils.h"

#include "walterUSDCommonUtils.h"

#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/curves.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <boost/range/adaptors.hpp>

// Cache for USD data
class USDDataRegistry : boost::noncopyable
{
public:
    typedef USDDataRegistry This;

    static This& getInstance()
    {
        return TfSingleton<This>::GetInstance();
    }

    // Return the stage
    UsdStageRefPtr getStage(const std::string& stageName)
    {
        auto it = mStages.find(stageName);
        if (it == mStages.end())
        {
            SdfLayerRefPtr root = WalterUSDCommonUtils::getUSDLayer(stageName);

            auto result = mStages.emplace(stageName, UsdStage::Open(root));
            it = result.first;
        }

        return it->second;
    }

    // Return the Hydra renderer
    UsdImagingGLRefPtr getRenderer(const std::string& stageName)
    {
        auto it = mRenderers.find(stageName);
        if (it == mRenderers.end())
        {
            auto result =
                mRenderers.emplace(stageName, std::make_shared<UsdImagingGL>());
            it = result.first;
        }

        return it->second;
    }

private:
    // We need to cache the stage because we don't load it from the disk
    // directly, instead, we generate it in the memory from the pipeline layers.
    // That's why standard USD stage caching system doesn't work for us.
    std::unordered_map<std::string, UsdStageRefPtr> mStages;

    // We need renderers cache because it should be a separate renderer object
    // per stage, otherwise it crashes. And we need a separate cache because it
    // should be initialized in the correct OpenGL context, so we can't
    // initialize it when we initialize the stage.
    std::unordered_map<std::string, UsdImagingGLRefPtr> mRenderers;
};

TF_INSTANTIATE_SINGLETON(USDDataRegistry);

UsdStageRefPtr WalterHoudiniUtils::getStage(const std::string& stageName)
{
    return USDDataRegistry::getInstance().getStage(stageName);
}

UsdImagingGLRefPtr WalterHoudiniUtils::getRenderer(const std::string& stageName)
{
    return USDDataRegistry::getInstance().getRenderer(stageName);
}

void WalterHoudiniUtils::getObjectList(
    const std::string& stageName,
    const std::string& primPath,
    bool geomOnly,
    std::vector<std::string>& oList)
{
    UsdStageRefPtr stage = getStage(stageName);

    // TODO: reserve

    UsdPrim root;
    if (!primPath.empty() && SdfPath::IsValidPathString(primPath))
    {
        root = stage->GetPrimAtPath(SdfPath(primPath));
    }
    else
    {
        root = stage->GetPseudoRoot();
    }

    UsdPrimRange range(root);
    for (const UsdPrim& prim : range)
    {
        if (!geomOnly || prim.IsA<UsdGeomMesh>() || prim.IsA<UsdGeomCurves>())
        {
            oList.push_back(prim.GetPath().GetString());
        }
    }
}
