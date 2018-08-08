// Copyright 2017 Rodeo FX. All rights reserved.
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>

#include "walterUSDCommonUtils.h"

#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE

TEST(getUSDLayer, testOneIdentifier)
{
    SdfLayerRefPtr rootLayer = WalterUSDCommonUtils::getUSDLayer("A");
    EXPECT_EQ(rootLayer->GetNumSubLayerPaths(), 1);
}

TEST(getUSDLayer, testTwoIdentifiers)
{
    SdfLayerRefPtr rootLayer = WalterUSDCommonUtils::getUSDLayer("A:B");
    EXPECT_EQ(rootLayer->GetNumSubLayerPaths(), 2);
}
