// Copyright 2017 Rodeo FX.  All rights reserved.

#include "engine.h"

#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <boost/algorithm/string.hpp>
#include <boost/noncopyable.hpp>
#include <unordered_map>
#include "walterUSDCommonUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE

// Cache for RendererEngines
class EngineRegistry : boost::noncopyable
{
public:
    typedef EngineRegistry This;

    static EngineRegistry& getInstance()
    {
        return TfSingleton<This>::GetInstance();
    }

    // Clear mCache.
    void clear() { mCache.clear(); }

    RendererEngine& getEngine(
        const std::string& fileName,
        const std::vector<std::string>& layers)
    {
        // It's possible that the procedurals request the engine at the same
        // time.
        ScopedLock lock(mMutex);

        // TODO: Initialize and keep the cache with UsdStage ID or the hash, not
        // with the filename.
        auto it = mCache.find(fileName);
        if (it == mCache.end())
        {
            auto result = mCache.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(fileName),
                std::forward_as_tuple(fileName, layers));
            it = result.first;
        }

        return it->second;
    }

private:
    typedef std::mutex Mutex;
    typedef std::lock_guard<Mutex> ScopedLock;

    std::unordered_map<std::string, RendererEngine> mCache;

    Mutex mMutex;
};

TF_INSTANTIATE_SINGLETON(EngineRegistry);

RendererEngine* RendererEngine::getInstance(
    const std::string& fileName,
    const std::vector<std::string>& layers)
{
    RendererEngine& engine =
        EngineRegistry::getInstance().getEngine(fileName, layers);
    return &engine;
}

void RendererEngine::clearCaches()
{
    EngineRegistry::getInstance().clear();
}

RendererEngine::RendererEngine(
    const std::string& fileName,
    const std::vector<std::string>& layers) :
        mPlugin(),
        mIndex(),
        mDelegate(mIndex, mPlugin)
{
    SdfLayerRefPtr root = WalterUSDCommonUtils::getUSDLayer(fileName);

    if (!layers.empty())
    {
        // Create the layer-container with the sub-layers.
        char layername[100];
        sprintf(layername, "anonymous%i.usda", rand());
        SdfLayerRefPtr overrideLayer = SdfLayer::CreateAnonymous(layername);
        // List of the layer IDs to put it to the container.
        std::vector<std::string> allLayers;
        allLayers.reserve(layers.size());
        // We need this cache to be sure that the layer will not be killed.
        std::vector<SdfLayerRefPtr> cacheLayers;
        cacheLayers.reserve(layers.size());

        for (const std::string& layer : layers)
        {
            // We need a unique name because USD can open the previous layer if
            // it has the same name.
            // TODO: make a really unique name using UUID.
            sprintf(layername, "anonymous%i.usda", rand());
            SdfLayerRefPtr overrideLayer = SdfLayer::CreateAnonymous(layername);
            if (overrideLayer->ImportFromString(layer))
            {
                allLayers.push_back(overrideLayer->GetIdentifier());
                cacheLayers.push_back(overrideLayer);
            }
        }

        // Put all the sub-layers.
        overrideLayer->SetSubLayerPaths(allLayers);

        // Form the stage.
        mStage = UsdStage::Open(root, overrideLayer);
    }

    if (!mStage)
    {
        mStage = UsdStage::Open(root);
    }
}

int RendererEngine::getNumNodes(
    const SdfPath& path,
    const std::vector<float>& times)
{
    const UsdPrim prim = mStage->GetPrimAtPath(path);
    if (prim)
    {
        if (prim.IsA<UsdGeomPointInstancer>())
        {
            UsdGeomPointInstancer instancer(prim);
            UsdAttribute positionsAttr = instancer.GetPositionsAttr();
            if (!positionsAttr.HasValue())
            {
                return 0;
            }
            float averageTime =
                std::accumulate(times.begin(), times.end(), 0.0f) /
                times.size();
            VtVec3fArray positions;
            positionsAttr.Get(&positions, static_cast<double>(averageTime));

            return static_cast<int>(positions.size());
        }

        prepare(prim);
        // 2x because we need to output a reference and an inctance for each
        // object.
        return 2 * mIndex.getNumNodes(path);
    }
    return 0;
}

void* RendererEngine::render(
    const SdfPath& path,
    int id,
    const std::vector<float>& times,
    const void* userData)
{
    const UsdPrim parentPrim = mStage->GetPrimAtPath(path);
    if (parentPrim.IsA<UsdGeomPointInstancer>())
    {
        return mPlugin.outputBBoxFromPoint(
            parentPrim, id, times, userData, mIndex);
    }

    // TODO: Can be slow.
    int numNodes = mIndex.getNumNodes(path);
    // It's impossible that numNodes is 0 at this point because render() is
    // called from arnoldProceduralGetNode. And arnoldProceduralGetNode is
    // called after arnoldProceduralNumNodes. If the number of nodes is 0,
    // render() should never be called.
    assert(numNodes > 0);

    // Since we output a reference and an inctance for each object, the real id
    // is id % numNodes. With this we can use variants if variant is 0, we need
    // to output a reference object. Otherwise we need to output an instance. We
    // use following terminology: a reference is an invisible object that
    // contains the actual mesh data, and an instance is an object with the
    // transformation matrix and it uses the data of the instance to draw a
    // mesh.
    int realID = id % numNodes;
    int variant = id / numNodes;

    const SdfPath* primPath = mIndex.getPath(path, realID);
    if (!primPath)
    {
        // Just in case.
        return nullptr;
    }

    // Check if it's a shader or something.
    const UsdPrim prim = mStage->GetPrimAtPath(*primPath);

    if (prim.IsInstance() && prim.IsInstanceable())
    {
        // We are here because user specified he wants to render precisely this
        // object. We know that it was the user because we never create bounding
        // boxes that point to instances. Instead, we continue iteration inside
        // the master object as it's the "continuation" of the instance. So we
        // need to continue iteration inside the master, outputBBox will
        // automatically handle it.
        if (variant == 0)
        {
            return mPlugin.outputBBox(prim, times, userData, mIndex);
        }

        return nullptr;
    }

    if (mPlugin.isImmediate(prim))
    {
        if (variant == 0)
        {
            RendererIndex::RenderNodeMap::accessor accessor;
            mIndex.getRenderNodeData(*primPath, accessor);
            // First pass of shading nodes. We need to create Arnold nodes
            // without any connections. That's why we output null.
            accessor->second =
                mPlugin.outputReference(prim, times, userData, mIndex);
            return nullptr;
        }
        else
        {
            RendererIndex::RenderNodeMap::const_accessor accessor;
            if (mIndex.getRenderNodeData(*primPath, accessor))
            {
                // The second pass. Add the connections and output the node.
                mPlugin.establishConnections(prim, accessor->second, mIndex);
                return accessor->second;
            }
        }

        return nullptr;
    }

    if (path == *primPath)
    {
        // We are here because we need to output a reference or an instance.
        if (variant == 0)
        {
            RendererIndex::RenderNodeMap::accessor accessor;
            mIndex.getRenderNodeData(*primPath, accessor);

            if (!accessor->second)
            {
                // Output reference and save the node.
                accessor->second =
                    mPlugin.outputReference(prim, times, userData, mIndex);
                return accessor->second;
            }
        }
        else
        {
            RendererIndex::RenderNodeMap::const_accessor accessor;

            if (mIndex.getRenderNodeData(*primPath, accessor) &&
                accessor->second)
            {
                // Output instance. Render object.
                return mPlugin.render(
                    prim, times, accessor->second, userData, mIndex);
            }
        }
    }
    else if (variant == 0)
    {
        // We are here because we need to output the procedural that can contain
        // other procedurals/instances/references.
        return mPlugin.outputBBox(prim, times, userData, mIndex);
    }

    return nullptr;
}

void RendererEngine::prepare(const UsdPrim& root)
{
    mDelegate.populate(root);
}
