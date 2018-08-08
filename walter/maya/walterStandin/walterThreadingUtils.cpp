// Copyright 2017 Rodeo FX.  All rights reserved.

#include "walterThreadingUtils.h"

#include <maya/MFileIO.h>
#include <maya/MFnDagNode.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>
#include <tbb/concurrent_unordered_set.h>
#include <tbb/tbb.h>
#include <boost/noncopyable.hpp>
#include <chrono>
#include <thread>
#include <limits>

#include "walterShapeNode.h"
#include "walterUSDCommonUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE
using namespace WalterMaya;

class WalterLoader : boost::noncopyable
{
public:
    typedef WalterLoader This;

    /**
     * @brief The instance of this singleton.
     *
     * @return
     */
    static WalterLoader& getInstance()
    {
        return TfSingleton<This>::GetInstance();
    }

    /**
     * @brief Starts loading stage in a background thread.
     *
     * @param iObj Walter standin to load.
     */
    void loadStage(const MObject& iObj)
    {
        mThreads.emplace_back(loadStageTask, this, iObj);
    }

    void joinAll()
    {
        // TODO: mutex
        for (auto& t : mThreads)
        {
            if (t.joinable())
            {
                t.join();
            }
        }

        mThreads.clear();
    }

private:
    /**
     * @brief The thread function that loads the stage.
     *
     * @param iLoader
     * @param iObj
     */
    static void loadStageTask(const WalterLoader* iLoader, MObject iObj)
    {
        // Maya node
        MFnDagNode dagNode(iObj);

        ShapeNode* shape = dynamic_cast<ShapeNode*>(dagNode.userNode());
        if (!shape)
        {
            return;
        }

        // Be sure we get filename when the scene is loaded.
        while (MFileIO::isReadingFile())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        // Get the filename and the session layer.
        std::string fileName = shape->resolvedCacheFileName().asChar();
        std::string session = shape->getSessionLayerBuffer().asChar();
        std::string variants = shape->getVariantsLayerBuffer().asChar();
        std::string visibility = shape->getVisibilityLayerBuffer().asChar();
        std::string purpose = shape->getPurposeLayerBuffer().asChar();

        const MPlug timePlug =
            MFnDependencyNode(shape->thisMObject()).findPlug(ShapeNode::aTime);
        if (!timePlug.isConnected())
        {
            shape->onTimeChanged(std::numeric_limits<float>::min());
        }

        // Root layer
        SdfLayerRefPtr root = WalterUSDCommonUtils::getUSDLayer(fileName);

        // USD stage
        UsdStageRefPtr stage;

        if (!session.empty())
        {
            char layername[100];
            // We need a unique name because USD can open the previous layer if
            // it has the same name.
            // TODO: make a really unique name using UUID.

            sprintf(layername, "anonymous%i.usda", rand());
            SdfLayerRefPtr sessionLayer = SdfLayer::CreateAnonymous(layername);
            if (sessionLayer->ImportFromString(session))
            {
                stage = UsdStage::Open(root, sessionLayer);
            }
        }

        if (!stage)
        {
            // We are here because the session layer is empty or corrupted.
            stage = UsdStage::Open(root);
        }


        if (!variants.empty())
        {
            WalterUSDCommonUtils::setUSDVariantsLayer(
                stage, variants.c_str());
        }

        if (!visibility.empty())
        {
            WalterUSDCommonUtils::setUSDVisibilityLayer(
                stage, visibility.c_str());
        }

        if (!purpose.empty())
        {
            WalterUSDCommonUtils::setUSDPurposeLayer(
                stage, purpose.c_str());
        }

        // Save the stage to the maya node.
        shape->setStage(stage);

        // Callback
        shape->setJustLoaded();
    }

    // The vector with all the created threads.
    std::vector<tbb::tbb_thread> mThreads;
};

TF_INSTANTIATE_SINGLETON(WalterLoader);

void WalterThreadingUtils::loadStage(const MObject& iObj)
{
    WalterLoader::getInstance().loadStage(iObj);
}

void WalterThreadingUtils::joinAll()
{
    WalterLoader::getInstance().joinAll();
}
