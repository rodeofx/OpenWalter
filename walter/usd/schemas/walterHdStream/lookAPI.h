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
#ifndef WALTERHYDRA_GENERATED_LOOKAPI_H
#define WALTERHYDRA_GENERATED_LOOKAPI_H

/// \file usdHydra/lookAPI.h

#include <pxr/pxr.h>
#include <pxr/usd/usdHydra/api.h>
#include <pxr/usd/usd/apiSchemaBase.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdHydra/tokens.h>

#include <pxr/base/vt/value.h>

#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/matrix4d.h>

#include <pxr/base/tf/token.h>
#include <pxr/base/tf/type.h>

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// HYDRALOOKAPI                                                               //
// -------------------------------------------------------------------------- //

/// \class WalterHdStreamLookAPI
///
/// 
/// 
///
class WalterHdStreamLookAPI : public UsdAPISchemaBase
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Compile-time constant indicating whether or not this class inherits from
    /// UsdTyped. Types which inherit from UsdTyped can impart a typename on a
    /// UsdPrim.
    static const bool IsTyped = false;

    /// Compile-time constant indicating whether or not this class represents a 
    /// multiple-apply API schema. Mutiple-apply API schemas can be applied 
    /// to the same prim multiple times with different instance names. 
    static const bool IsMultipleApply = false;

    /// Construct a WalterHdStreamLookAPI on UsdPrim \p prim .
    /// Equivalent to WalterHdStreamLookAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit WalterHdStreamLookAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a WalterHdStreamLookAPI on the prim held by \p schemaObj .
    /// Should be preferred over WalterHdStreamLookAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit WalterHdStreamLookAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDHYDRA_API
    virtual ~WalterHdStreamLookAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDHYDRA_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a WalterHdStreamLookAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// WalterHdStreamLookAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDHYDRA_API
    static WalterHdStreamLookAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "HydraLookAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid WalterHdStreamLookAPI object is returned upon success. 
    /// An invalid (or empty) WalterHdStreamLookAPI object is returned upon 
    /// failure. See \ref UsdAPISchemaBase::_ApplyAPISchema() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    ///
    USDHYDRA_API
    static WalterHdStreamLookAPI 
    Apply(const UsdPrim &prim);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDHYDRA_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDHYDRA_API
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // BXDF 
    // --------------------------------------------------------------------- //
    /// 
    ///
    USDHYDRA_API
    UsdRelationship GetBxdfRel() const;

    /// See GetBxdfRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDHYDRA_API
    UsdRelationship CreateBxdfRel() const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to: 
    //  - Close the class declaration with }; 
    //  - Close the namespace with PXR_NAMESPACE_CLOSE_SCOPE
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
