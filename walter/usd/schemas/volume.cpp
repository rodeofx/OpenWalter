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
#include ".//volume.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<WalterVolume,
        TfType::Bases< UsdGeomGprim > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Volume")
    // to find TfType<WalterVolume>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, WalterVolume>("Volume");
}

/* virtual */
WalterVolume::~WalterVolume()
{
}

/* static */
WalterVolume
WalterVolume::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return WalterVolume();
    }
    return WalterVolume(stage->GetPrimAtPath(path));
}

/* static */
WalterVolume
WalterVolume::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Volume");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return WalterVolume();
    }
    return WalterVolume(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
WalterVolume::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<WalterVolume>();
    return tfType;
}

/* static */
bool 
WalterVolume::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
WalterVolume::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
WalterVolume::GetVolumePaddingAttr() const
{
    return GetPrim().GetAttribute(WalterTokens->volumePadding);
}

UsdAttribute
WalterVolume::CreateVolumePaddingAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(WalterTokens->volumePadding,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
WalterVolume::GetStepSizeAttr() const
{
    return GetPrim().GetAttribute(WalterTokens->stepSize);
}

UsdAttribute
WalterVolume::CreateStepSizeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(WalterTokens->stepSize,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
WalterVolume::GetFilenameAttr() const
{
    return GetPrim().GetAttribute(WalterTokens->filename);
}

UsdAttribute
WalterVolume::CreateFilenameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(WalterTokens->filename,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
WalterVolume::GetFiledataAttr() const
{
    return GetPrim().GetAttribute(WalterTokens->filedata);
}

UsdAttribute
WalterVolume::CreateFiledataAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(WalterTokens->filedata,
                       SdfValueTypeNames->UCharArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
WalterVolume::GetStepScaleAttr() const
{
    return GetPrim().GetAttribute(WalterTokens->stepScale);
}

UsdAttribute
WalterVolume::CreateStepScaleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(WalterTokens->stepScale,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
WalterVolume::GetCompressAttr() const
{
    return GetPrim().GetAttribute(WalterTokens->compress);
}

UsdAttribute
WalterVolume::CreateCompressAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(WalterTokens->compress,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
WalterVolume::GetGridsAttr() const
{
    return GetPrim().GetAttribute(WalterTokens->grids);
}

UsdAttribute
WalterVolume::CreateGridsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(WalterTokens->grids,
                       SdfValueTypeNames->StringArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
WalterVolume::GetVelocityScaleAttr() const
{
    return GetPrim().GetAttribute(WalterTokens->velocityScale);
}

UsdAttribute
WalterVolume::CreateVelocityScaleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(WalterTokens->velocityScale,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
WalterVolume::GetVelocityFpsAttr() const
{
    return GetPrim().GetAttribute(WalterTokens->velocityFps);
}

UsdAttribute
WalterVolume::CreateVelocityFpsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(WalterTokens->velocityFps,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
WalterVolume::GetVelocityOutlierThresholdAttr() const
{
    return GetPrim().GetAttribute(WalterTokens->velocityOutlierThreshold);
}

UsdAttribute
WalterVolume::CreateVelocityOutlierThresholdAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(WalterTokens->velocityOutlierThreshold,
                       SdfValueTypeNames->Float,
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
WalterVolume::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        WalterTokens->volumePadding,
        WalterTokens->stepSize,
        WalterTokens->filename,
        WalterTokens->filedata,
        WalterTokens->stepScale,
        WalterTokens->compress,
        WalterTokens->grids,
        WalterTokens->velocityScale,
        WalterTokens->velocityFps,
        WalterTokens->velocityOutlierThreshold,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomGprim::GetSchemaAttributeNames(true),
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
