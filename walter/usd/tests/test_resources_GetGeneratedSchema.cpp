// Copyright 2017 Rodeo FX. All rights reserved.

/* 
This test check that we can load usda schemas from the Walter resources. 
To do that, we are patching the Usd file 'schemaRegistry.cpp' by overriding 
_GetGeneratedSchema().
*/

#include <pxr/usd/usd/schemaRegistry.h>
#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE


TEST(test_resources_GetGeneratedSchema, test1)
{
    SdfPrimSpecHandle primSpec = UsdSchemaRegistry::GetPrimDefinition(TfToken("ModelAPI"));
    EXPECT_TRUE(primSpec);
}
