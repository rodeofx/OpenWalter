// Copyright 2017 Rodeo FX. All rights reserved.
#include "schemas/walterHdStream/primvar.h"
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/prim.h>

#include "walterUSDCommonUtils.h"
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usdGeom/imageable.h>
#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE

TEST(test_visibility, test1)
{
	std::string stageAsStr = R"(#usda 1.0
	(
	    endTimeCode = 1
	    startTimeCode = 1
	    upAxis = "Y"
	)

	def Xform "triangle" (
	)
	{
	    def Mesh "mesh0"
	    {
	        float3[] extent = [(-0.5, 0, 0), (0.5, 0.7853982, 0)]
	        int[] faceVertexCounts = [3]
	        int[] faceVertexIndices = [0, 1, 2]
	        point3f[] points = [(-0.5, 0, 0), (0.5, 0, 0), (0, 0.7853982, 0)]
	    }
	}
	)";

 	UsdStageRefPtr stage = UsdStage::CreateInMemory();

 	SdfLayerHandle handle = stage->GetSessionLayer();
 	handle->ImportFromString(stageAsStr);

 	// By default the prim is visible
    EXPECT_TRUE(WalterUSDCommonUtils::isVisible(stage, "/triangle/mesh0"));

 	// Hide it and test
    WalterUSDCommonUtils::toggleVisibility(stage, "/triangle/mesh0");
    EXPECT_EQ(WalterUSDCommonUtils::isVisible(stage, "/triangle/mesh0"), false);

 	// Make it visible and test
    WalterUSDCommonUtils::toggleVisibility(stage, "/triangle/mesh0");
    EXPECT_EQ(WalterUSDCommonUtils::isVisible(stage, "/triangle/mesh0"), true);
}
