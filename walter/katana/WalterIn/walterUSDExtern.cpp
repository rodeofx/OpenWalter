// Copyright 2017 Rodeo FX.  All rights reserved.

#include "walterUSDExtern.h"
#include "walterUSDCommonUtils.h"
#include "walterUSDOpEngine.h"
#include <boost/algorithm/string.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

void WalterUSDExtern::setIdentifier(const char* identifiers)
{
    mPrimsCount = 0;
    mFlattenVariants = "";

    if ((identifiers != 0) && (identifiers[0] == '\0'))
        return;

    mIdentifiers.clear();
    boost::split(mIdentifiers, identifiers, boost::is_any_of(":"));
}

void WalterUSDExtern::extractVariantsUsd()
{
    OpEngine& engine = OpEngine::getInstance(mIdentifiers);
    std::vector<std::string> variants =
        WalterUSDCommonUtils::getVariantsUSD(
            engine.getUsdStage(), "", true);

    mFlattenVariants = "";
    mPrimsCount = variants.size();
    for (int i = 0; i < variants.size(); ++i)
    {
        mFlattenVariants += variants[i];
        if (variants.size() > 1 && i < (variants.size() - 1))
            mFlattenVariants += "|";
    }
}

std::string& WalterUSDExtern::getVariantsUsd()
{
    return mFlattenVariants;
}

int WalterUSDExtern::getPrimsCount()
{
    return mPrimsCount;
}

std::string WalterUSDExtern::setVariantUsd(
    const char* primPath,
    const char* variantSetName,
    const char* variantName)
{
    std::string variants = primPath + std::string("{") + variantSetName +
        std::string("=") + variantName + std::string("}");

    return setVariantUsd(variants.c_str());
}

std::string WalterUSDExtern::setVariantUsd(const char* variants)
{
    OpEngine& engine = OpEngine::getInstance(mIdentifiers);
    WalterUSDCommonUtils::setVariantUSD(engine.getUsdStage(), variants);
    return variants;
}

extern "C" {
WalterUSDExtern* WalterUSDExtern_new()
{
    return new WalterUSDExtern();
}

void WalterUSDExtern_setIdentifier(
    WalterUSDExtern* walterExtern,
    const char* flattenIdentifiers)
{
    walterExtern->setIdentifier(flattenIdentifiers);
}

void WalterUSDExtern_extractVariantsUsd(WalterUSDExtern* walterExtern)
{
    walterExtern->extractVariantsUsd();
}

const char* WalterUSDExtern_getVariantsUsd(WalterUSDExtern* walterExtern)
{
    return walterExtern->getVariantsUsd().c_str();
}

const char* WalterUSDExtern_setVariantUsd(
    WalterUSDExtern* walterExtern,
    const char* primPath,
    const char* variantSetName,
    const char* variantName)
{
    return walterExtern->setVariantUsd(primPath, variantSetName, variantName)
        .c_str();
}

void WalterUSDExtern_setVariantUsdFlat(
    WalterUSDExtern* walterExtern,
    const char* variants)
{
    walterExtern->setVariantUsd(variants);
}

int WalterUSDExtern_getPrimsCount(WalterUSDExtern* walterExtern)
{
    return walterExtern->getPrimsCount();
}
}
