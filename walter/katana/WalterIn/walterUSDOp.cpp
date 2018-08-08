// Copyright 2017 Rodeo FX.  All rights reserved.

#include "walterUSDOp.h"

#include "walterUSDOpEngine.h"
#include "walterUSDOpUtils.h"

#include <FnPluginSystem/FnPlugin.h>

class WalterInUSDOp : public Foundry::Katana::GeolibOp
{
public:
    static void setup(Foundry::Katana::GeolibSetupInterface& ioInterface)
    {
        ioInterface.setThreading(
            Foundry::Katana::GeolibSetupInterface::ThreadModeConcurrent);
    }

    /**
     * @brief The entry point for each scene graph node.
     *
     * @param ioInterface A reference to a valid GeolibCookInterface object.
     */
    static void cook(Foundry::Katana::GeolibCookInterface& ioInterface)
    {
        // We passed the engine as a private data previously.
        // TODO: probably passing the list of the layers is better because it
        // would be possible to create such node in OpScript.
        OpUtils::PrivateData* privateData =
            reinterpret_cast<OpUtils::PrivateData*>(
                ioInterface.getPrivateData());
        assert(privateData);

        privateData->engine().cook(privateData, &ioInterface);
    }
};

DEFINE_GEOLIBOP_PLUGIN(WalterInUSDOp)

void cookUSDRoot(
    Foundry::Katana::GeolibCookInterface& ioInterface,
    const std::vector<std::string>& iArchives)
{
    W_DEBUG(
        "i:%s o:%s",
        ioInterface.getInputLocationPath().c_str(),
        ioInterface.getOutputLocationPath().c_str());

    FnAttribute::StringAttribute pathFromRootAttr =
        ioInterface.getOpArg("pathFromRoot");

    // False means don't throw an error if error occurs.
    std::string rootStr = pathFromRootAttr.getValue(std::string{}, false);
    SdfPath rootPath =
        rootStr.empty() ? SdfPath::AbsoluteRootPath() : SdfPath(rootStr);

    // Time related paramteters.
    static std::string currentTimeStr = "system.timeSlice.currentTime";
    static std::string shutterOpenStr = "system.timeSlice.shutterOpen";
    static std::string shutterCloseStr = "system.timeSlice.shutterClose";
    static std::string numSamplesStr = "system.timeSlice.numSamples";

    FnAttribute::FloatAttribute currentTimeAttr(
        ioInterface.getOpArg(currentTimeStr));
    FnAttribute::FloatAttribute shutterOpenAttr(
        ioInterface.getOpArg(shutterOpenStr));
    FnAttribute::FloatAttribute shutterCloseAttr(
        ioInterface.getOpArg(shutterCloseStr));
    FnAttribute::IntAttribute numSamplesAttr(
        ioInterface.getOpArg(numSamplesStr));

    float currentTime = currentTimeAttr.getValue(0.0, false);
    float shutterOpen = shutterOpenAttr.getValue(0.0, false);
    float shutterClose = shutterCloseAttr.getValue(0.0, false);
    int numSamples = numSamplesAttr.getValue(0, false);

    OpUtils::TimePtr time = std::make_shared<OpUtils::Time>(
        currentTime, shutterOpen, shutterClose, numSamples);

    OpEngine& engine = OpEngine::getInstance(iArchives);

    // Att the children PrivateDatas will have this outputLocationPath.
    const OpUtils::PrivateData privateData(
        engine, ioInterface.getOutputLocationPath(), rootPath, time);

    engine.cook(&privateData, &ioInterface);

    // If cookInstances is true we need to cook Masters locations.
    // Then any instances will be cooked as Katana instances and point to the
    // corresponding Masters location.
    auto cookInstancesAttr = ioInterface.getOpArg("cookInstances");
    int cookInstances =
        FnAttribute::IntAttribute(cookInstancesAttr).getValue(0, false);

    if (cookInstances)
        engine.cookMasters(&privateData, &ioInterface);
}

void registerUSDPlugins()
{
    REGISTER_PLUGIN(WalterInUSDOp, "WalterInUSD", 0, 1);
}
