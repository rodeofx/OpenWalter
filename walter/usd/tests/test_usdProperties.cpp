// Copyright 2017 Rodeo FX. All rights reserved.
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usd/stage.h>

#include "walterUSDCommonUtils.h"
#include <string>
#include <vector>

#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE

TEST(propertiesUSD, test_usdProperties)
{
    std::string stageAsStr = R"(#usda 1.0
    (
        endTimeCode = 1
        startTimeCode = 1
        upAxis = "Y"
    )

    def Xform "triangle" (
        customData = {
            bool zUp = 0
        }
    )
    {
        def Mesh "mesh0"
        {
            float3[] extent.timeSamples = {
                1: [(-0.5, 0, 0), (0.5, 0.7853982, 0)],
            }
            int[] faceVertexCounts.timeSamples = {
                1: [3],
            }
            int[] faceVertexIndices.timeSamples = {
                1: [0, 1, 2],
            }
            uniform token orientation = "leftHanded"
            point3f[] points.timeSamples = {
                1: [(-0.5, 0, 0), (0.5, 0, 0), (0, 0.7853982, 0)],
            }
            uniform token subdivisionScheme = "none"
        }
    }
    )";

    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    SdfLayerHandle handle = stage->GetSessionLayer();
    handle->ImportFromString(stageAsStr);

    std::vector<std::string> properties = WalterUSDCommonUtils::propertiesUSD(
        stage,
        "/triangle/mesh0",
        0,
        true);

    EXPECT_EQ(properties[0], "{ \"name\": \"cornerIndices\", \"type\": \"VtArray<int>\", \"arraySize\": 0, \"value\": [] }");
    EXPECT_EQ(properties[1], "{ \"name\": \"cornerSharpnesses\", \"type\": \"VtArray<float>\", \"arraySize\": 0, \"value\": [] }");
    EXPECT_EQ(properties[2], "{ \"name\": \"creaseIndices\", \"type\": \"VtArray<int>\", \"arraySize\": 0, \"value\": [] }");
    EXPECT_EQ(properties[3], "{ \"name\": \"creaseLengths\", \"type\": \"VtArray<int>\", \"arraySize\": 0, \"value\": [] }");
    EXPECT_EQ(properties[4], "{ \"name\": \"creaseSharpnesses\", \"type\": \"VtArray<float>\", \"arraySize\": 0, \"value\": [] }");
    EXPECT_EQ(properties[5], "{ \"name\": \"doubleSided\", \"type\": \"bool\", \"arraySize\": -1, \"value\": 0 }");
    EXPECT_EQ(properties[6], "{ \"name\": \"extent\", \"type\": \"VtArray<GfVec3f>\", \"arraySize\": 2, \"value\": [[-0.500000, 0.000000, 0.000000]] }");
    EXPECT_EQ(properties[7], "{ \"name\": \"faceVaryingLinearInterpolation\", \"type\": \"TfToken\", \"arraySize\": -1, \"value\": \"\" }");
    EXPECT_EQ(properties[8], "{ \"name\": \"faceVertexCounts\", \"type\": \"VtArray<int>\", \"arraySize\": 1, \"value\": [3] }");
    EXPECT_EQ(properties[9], "{ \"name\": \"faceVertexIndices\", \"type\": \"VtArray<int>\", \"arraySize\": 3, \"value\": [0] }");
    EXPECT_EQ(properties[10], "{ \"name\": \"holeIndices\", \"type\": \"VtArray<int>\", \"arraySize\": 0, \"value\": [] }");
}
