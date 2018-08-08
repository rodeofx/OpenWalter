#include "walterUSDOpEngine.h"

#include "walterUSDOpCaboose.h"
#include "walterUSDOpDelegate.h"
#include "walterUSDOpUtils.h"
#include <FnGeolib/op/FnGeolibOp.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/usd/usd/stage.h>
#include <boost/algorithm/string.hpp>
#include <boost/functional/hash.hpp>
#include <boost/noncopyable.hpp>

/** @brief Static cache for OpEngines */
class EngineRegistry : boost::noncopyable
{
public:
    typedef EngineRegistry This;

    /**
     * @brief Get the instance of this registry singleton.
     *
     * @return This object.
     */
    static EngineRegistry& getInstance()
    {
        return TfSingleton<This>::GetInstance();
    }

    /** @brief Clear mCache */
    void clear() { mCache.clear(); }

    /**
     * @brief Returns the engine.
     *
     * @param iArchives the list of archives.
     *
     * @return The engine.
     */
    OpEngine& getEngine(const std::vector<std::string>& iArchives)
    {
        // It's possible to request the engine at the same time.
        ScopedLock lock(mMutex);

        // Get the hash of the archives.
        size_t hash = boost::hash_range(iArchives.begin(), iArchives.end());

        auto it = mCache.find(hash);
        if (it == mCache.end())
        {
            auto result = mCache.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(hash),
                std::forward_as_tuple(iArchives));
            it = result.first;
        }

        return it->second;
    }

private:
    // Mutex stuff for concurrent access. We don't use TBB stuff because we
    // would like to control hashing.
    typedef std::mutex Mutex;
    typedef std::lock_guard<Mutex> ScopedLock;

    std::map<size_t, OpEngine> mCache;

    Mutex mMutex;
};

TF_INSTANTIATE_SINGLETON(EngineRegistry);

OpEngine& OpEngine::getInstance(const std::vector<std::string>& iArchives)
{
    return EngineRegistry::getInstance().getEngine(iArchives);
}

void OpEngine::clearCaches()
{
    EngineRegistry::getInstance().clear();
}

OpEngine::OpEngine(const std::vector<std::string>& iArchives) :
        mIdentifier(boost::algorithm::join(iArchives, ":"))
{
    SdfLayerRefPtr root = WalterUSDCommonUtils::getUSDLayer(iArchives);
    mStage = UsdStage::Open(root);

    OpDelegate::populate(mStage->GetPseudoRoot(), mIndex);
}

void OpEngine::dress(void* ioClientData)
{
    auto interface =
        reinterpret_cast<Foundry::Katana::GeolibCookInterface*>(ioClientData);

    static std::string currentTimeStr = "system.timeSlice.currentTime";
    FnAttribute::FloatAttribute currentTimeAttr(
        interface->getOpArg(currentTimeStr));

    float currentTime = currentTimeAttr.getValue(0.0, false);

    FnAttribute::StringAttribute variantsListAttr =
        interface->getOpArg("variantsList");

    auto variantsList = variantsListAttr.getNearestSample(currentTime);
    for (auto const* variants : variantsList)
        WalterUSDCommonUtils::setVariantUSD(mStage, variants);
}

void OpEngine::cook(
    const OpUtils::PrivateData* iPrivateData,
    void* ioClientData)
{
    assert(iPrivateData);

    dress(ioClientData);

    const UsdPrim prim = mStage->GetPrimAtPath(iPrivateData->path());

    // Output geo. For example polymesh.
    OpCaboose::cook(prim, iPrivateData, ioClientData);

    if (OpCaboose::skipChildren(prim))
    {
        return;
    }

    // Output children.
    for (const auto& child : prim.GetChildren())
    {
        if (OpCaboose::isSupported(child))
        {
            // We pass this engine as a private data for fast access from the
            // next generated scene graph node.
            // TODO: probably passing the list of the layers is better because
            // it would be possible to create such node in OpScript.
            OpCaboose::cookChild(child, iPrivateData, ioClientData);
        }
    }
}

void OpEngine::cookMasters(
    const OpUtils::PrivateData* iPrivateData,
    void* ioClientData)
{
    assert(iPrivateData);

    mMastersInfoMap = WalterUSDCommonUtils::getMastersInfo(mStage);

    for (const auto& master: mStage->GetMasters())
    {
        if (OpCaboose::isSupported(master))
        {
            std::string katanaName = getKatMasterName(master);
            OpCaboose::cookChild(master, iPrivateData, ioClientData, katanaName);
        }
    }
}

WalterUSDCommonUtils::MasterPrimInfo OpEngine::getMasterPrimInfo(
    const UsdPrim& iPrim)
{
    WalterUSDCommonUtils::MasterPrimInfo masterInfo;

    if (iPrim.IsInMaster() && !iPrim.IsMaster())
    {
        SdfPath masterRoot = iPrim.GetPath().GetPrefixes().front();
        masterInfo = mMastersInfoMap.at(masterRoot.GetName());
    }
    else if (iPrim.IsInstance())
    {
        masterInfo = mMastersInfoMap.at(iPrim.GetMaster().GetName());
    }

    return masterInfo;
}

std::string OpEngine::getMasterName(const UsdPrim& iPrim)
{
    WalterUSDCommonUtils::MasterPrimInfo masterInfo =
        getMasterPrimInfo(iPrim);

    return masterInfo.realName;
}

std::string OpEngine::getKatMasterName(const UsdPrim& iMasterPrim)
{
    WalterUSDCommonUtils::MasterPrimInfo masterInfo =
        mMastersInfoMap.at(iMasterPrim.GetName());

    if (masterInfo.isEmpty())
        return "";

    std::string katanaName = "master_" + masterInfo.realName;
    std::vector<std::string> variantsSelection = masterInfo.variantsSelection;

    for (int i = variantsSelection.size(); i--;)
    {
        katanaName += "_" + variantsSelection[i];
    }

    return katanaName;
}
