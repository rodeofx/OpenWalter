// Copyright 2017 Rodeo FX. All rights reserved.
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usd/stage.h>

#include "walterUSDCommonUtils.h"
#include <string>
#include <vector>

#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE

TEST(setVariantUSD, testSetVariantOnChildPrimFromParent)
{
    const char* primPath = "/Foo/Bar";
    const char* variantSetName = "colors";
    const char* variantName = "red";

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdPrim prim = stage->DefinePrim(SdfPath(primPath));

    UsdVariantSet colorsVset = prim.GetVariantSets().AddVariantSet(variantSetName);
    colorsVset.AddVariant(variantName);
    colorsVset.AddVariant("green");

    colorsVset.SetVariantSelection("green");

    const char* parentPrimPath = "/Foo";
    std::string variants = parentPrimPath + std::string("{") + variantSetName +
        std::string("=") + variantName + std::string("}");

    WalterUSDCommonUtils::setVariantUSD(stage, variants.c_str());

    std::vector<std::string> result =
        WalterUSDCommonUtils::getVariantsUSD(stage);

    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(colorsVset.GetVariantSelection(), std::string(variantName));
}
