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
#include ".//tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

WalterTokensType::WalterTokensType() :
    compress("compress", TfToken::Immortal),
    expression("expression", TfToken::Immortal),
    filedata("filedata", TfToken::Immortal),
    filename("filename", TfToken::Immortal),
    grids("grids", TfToken::Immortal),
    group("group", TfToken::Immortal),
    stepScale("stepScale", TfToken::Immortal),
    stepSize("stepSize", TfToken::Immortal),
    velocityFps("velocityFps", TfToken::Immortal),
    velocityOutlierThreshold("velocityOutlierThreshold", TfToken::Immortal),
    velocityScale("velocityScale", TfToken::Immortal),
    volumePadding("volumePadding", TfToken::Immortal),
    allTokens({
        compress,
        expression,
        filedata,
        filename,
        grids,
        group,
        stepScale,
        stepSize,
        velocityFps,
        velocityOutlierThreshold,
        velocityScale,
        volumePadding
    })
{
}

TfStaticData<WalterTokensType> WalterTokens;

PXR_NAMESPACE_CLOSE_SCOPE
