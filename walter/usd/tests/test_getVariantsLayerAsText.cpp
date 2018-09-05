// Copyright 2017 Rodeo FX. All rights reserved.
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/variantSets.h>

#include <string>
#include <vector>
#include "walterUSDCommonUtils.h"

#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE


TEST(getVariantsLayerAsText, test1)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    UsdPrim prim = stage->DefinePrim(SdfPath("/Foo"));
    UsdVariantSets vsets = prim.GetVariantSets();

    UsdVariantSet colorsVset = vsets.AddVariantSet("colors");
    colorsVset.AddVariant("red");
    colorsVset.AddVariant("green");
    colorsVset.AddVariant("blue");
    colorsVset.SetVariantSelection("green");

    std::vector<std::string> result = WalterUSDCommonUtils::getVariantsUSD(
        stage, "", true);
    EXPECT_EQ(result[0], "{ \"prim\": \"/Foo\", \"variants\": [{ \"set\": \"colors\", \"names\": [\"blue\", \"green\", \"red\"], \"selection\": \"green\" } ]  }");
}
