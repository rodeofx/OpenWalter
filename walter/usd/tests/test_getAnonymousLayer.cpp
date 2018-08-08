// Copyright 2017 Rodeo FX. All rights reserved.
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>

#include "walterUSDCommonUtils.h"

#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE

TEST(getAnonymousLayer, testCreateAndGet)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    SdfLayerRefPtr layer = WalterUSDCommonUtils::getAnonymousLayer(stage, "foo");
    EXPECT_TRUE(layer->IsAnonymous());
    EXPECT_EQ(layer->GetDisplayName(), "foo.usd");
}

TEST(getAnonymousLayer, testGet)
{
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous("foo.usd");
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    layer = WalterUSDCommonUtils::getAnonymousLayer(stage, "foo");
    EXPECT_TRUE(layer->IsAnonymous());
    EXPECT_EQ(layer->GetDisplayName(), "foo.usd");
}
