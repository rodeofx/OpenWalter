// Copyright 2017 Rodeo FX. All rights reserved.
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/prim.h>

#include "walterUSDCommonUtils.h"

#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE


TEST(setUSDVariantsLayer, test1)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    std::string variantData = "#usda 1.0\n\ndef \"Foo\"\n{\n}\n\n";

    bool result = WalterUSDCommonUtils::setUSDVariantsLayer(stage, variantData.c_str());
    EXPECT_TRUE(result);

    UsdPrim prim(stage->GetPrimAtPath(SdfPath("/Foo")));
    EXPECT_TRUE(prim);

    EXPECT_EQ(WalterUSDCommonUtils::getVariantsLayerAsText(stage), variantData);
}
