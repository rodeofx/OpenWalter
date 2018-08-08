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
#ifndef WALTER_TOKENS_H
#define WALTER_TOKENS_H

/// \file walterUsdExtras/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include ".//api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \hideinitializer
#define WALTER_TOKENS \
    (compress) \
    (expression) \
    (filedata) \
    (filename) \
    (grids) \
    (group) \
    (stepScale) \
    (stepSize) \
    (velocityFps) \
    (velocityOutlierThreshold) \
    (velocityScale) \
    (volumePadding)

/// \anchor WalterTokens
///
/// <b>WalterTokens</b> provides static, efficient TfToken's for
/// use in all public USD API
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// WalterTokens also contains all of the \em allowedTokens values declared
/// for schema builtin attributes of 'token' scene description type.
/// Use WalterTokens like so:
///
/// \code
///     gprim.GetVisibilityAttr().Set(WalterTokens->invisible);
/// \endcode
///
/// The tokens are:
/// \li <b>compress</b> - WalterVolume
/// \li <b>expression</b> - WalterExpression
/// \li <b>filedata</b> - WalterVolume
/// \li <b>filename</b> - WalterVolume
/// \li <b>grids</b> - WalterVolume
/// \li <b>group</b> - WalterExpression
/// \li <b>stepScale</b> - WalterVolume
/// \li <b>stepSize</b> - WalterVolume
/// \li <b>velocityFps</b> - WalterVolume
/// \li <b>velocityOutlierThreshold</b> - WalterVolume
/// \li <b>velocityScale</b> - WalterVolume
/// \li <b>volumePadding</b> - WalterVolume
TF_DECLARE_PUBLIC_TOKENS(WalterTokens, WALTERUSDEXTRAS_API, WALTER_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
