//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include ".//expression.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<WalterExpression,
        TfType::Bases< UsdTyped > >();
    
}

/* virtual */
WalterExpression::~WalterExpression()
{
}

/* static */
WalterExpression
WalterExpression::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return WalterExpression();
    }
    return WalterExpression(stage->GetPrimAtPath(path));
}


/* static */
const TfType &
WalterExpression::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<WalterExpression>();
    return tfType;
}

/* static */
bool 
WalterExpression::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
WalterExpression::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
WalterExpression::GetExpressionAttr() const
{
    return GetPrim().GetAttribute(WalterTokens->expression);
}

UsdAttribute
WalterExpression::CreateExpressionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(WalterTokens->expression,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
WalterExpression::GetGroupAttr() const
{
    return GetPrim().GetAttribute(WalterTokens->group);
}

UsdAttribute
WalterExpression::CreateGroupAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(WalterTokens->group,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
WalterExpression::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        WalterTokens->expression,
        WalterTokens->group,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdTyped::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include <boost/algorithm/string.hpp>

PXR_NAMESPACE_OPEN_SCOPE

std::string WalterExpression::GetExpression() const
{
    std::string expression;
    GetExpressionAttr().Get(&expression);
    return expression;
}

void WalterExpression::SetExpression(const std::string& expression)
{
    UsdAttribute expressionAttr = GetExpressionAttr();
    if (!expressionAttr)
    {
        expressionAttr = CreateExpressionAttr();
    }

    expressionAttr.Set(expression);
}

WalterExpression::AssignmentLayers WalterExpression::GetLayers() const
{
    AssignmentLayers layers;

    // Iterate all the connections with the name starting with "assign:"
    for (const UsdRelationship& relationship : GetPrim().GetRelationships())
    {
        // It should like this: "assign:defaultRenderLayer:shader"
        const std::string name = relationship.GetName().GetText();
        if (!boost::istarts_with(name, "assign:"))
        {
            continue;
        }

        std::vector<std::string> splitted;
        boost::split(splitted, name, boost::is_any_of(":"));

        if (splitted.size() != 3)
        {
            continue;
        }

        // Extract the render layer and the target.
        const std::string layer = splitted[1];
        const std::string target = splitted[2];

        SdfPathVector shaders;
        relationship.GetTargets(&shaders);

        // We don't support multiple connections.
        layers[layer][target] = shaders[0];
    }

    return layers;
}

void WalterExpression::SetConnection(
        const std::string& layer,
        const std::string& target,
        const SdfPath& shader)
{
    TfToken relName("assign:" + layer + ":" + target);

    UsdRelationship rel = GetPrim().GetRelationship(relName);
    if (!rel)
    {
        rel = GetPrim().CreateRelationship(relName, false);
    }

    rel.SetTargets(SdfPathVector{shader});
}

PXR_NAMESPACE_CLOSE_SCOPE
