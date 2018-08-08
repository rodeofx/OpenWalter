// Copyright 2017 Rodeo FX.  All rights reserved.

#include "walterUSDOpUtils.h"
#include <unordered_map>

// Declare USD debugging messages.
PXR_NAMESPACE_OPEN_SCOPE
TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(WALTER_KATANA, "Output to Katana");
}
PXR_NAMESPACE_CLOSE_SCOPE

OpUtils::Time::Time(
    TimeType iCurrent,
    TimeType iOpen,
    TimeType iClose,
    size_t iSamples) :
        mCurrent(iCurrent)
{
    assert(iSamples);

    mSamples.reserve(iSamples);

    if (iSamples == 1)
    {
        // We use 0.0 because if there are no motion blur, Katana sets shutter
        // open/close times as 0.0 and 1.0.
        mSamples.push_back((TimeType)0);
    }
    else
    {
        for (size_t i = 0; i < iSamples; i++)
        {
            mSamples.push_back(iOpen + (iClose - iOpen) * i / (iSamples - 1));
        }
    }
}

OpUtils::PrivateData::PrivateData(
    OpEngine& iEngine,
    const std::string iKatanaRoot,
    const SdfPath& iCurrentPath,
    TimePtr iTime) :
        mEngine(&iEngine),
        mKatanaRoot(iKatanaRoot),
        mCurrentPath(iCurrentPath),
        mTime(iTime)
{}

std::string OpUtils::getClientAttributeName(const std::string& iName)
{
    // We receive Arnold attributes, and we need to make KTOA understanding
    // them. The only way to do it is remapping them.
    static const std::unordered_map<std::string, std::string> sArnoldAttrs = {
        { "casts_shadows", "arnoldStatements.visibility.AI_RAY_SHADOW" },
        { "disp_autobump", "arnoldStatements.disp_autobump" },
        { "disp_height", "arnoldStatements.disp_height" },
        { "disp_padding", "arnoldStatements.disp_padding" },
        { "disp_zero_value", "arnoldStatements.zero_value" },
        { "matte", "arnoldStatements.matte" },
        { "opaque", "arnoldStatements.opaque" },
        { "primary_visibility", "arnoldStatements.visibility.AI_RAY_CAMERA" },
        { "self_shadows", "arnoldStatements.visibility.AI_RAY_SHADOW" },
        { "subdiv_adaptive_error", "arnoldStatements.subdiv_adaptive_error" },
        { "subdiv_adaptive_metric", "arnoldStatements.subdiv_adaptive_metric" },
        { "subdiv_adaptive_space", "arnoldStatements.subdiv_adaptive_space" },
        { "subdiv_iterations", "arnoldStatements.iterations" },
        { "subdiv_smooth_derivs", "arnoldStatements.subdiv_smooth_derivs" },
        { "subdiv_type", "arnoldStatements.subdiv_type" },
        { "subdiv_uv_smoothing", "arnoldStatements.subdiv_uv_smoothing" },
        { "visibility", "visible" },
        { "visible_in_diffuse", "arnoldStatements.visibility.AI_RAY_DIFFUSE" },
        { "visible_in_diffuse_reflection",
          "arnoldStatements.visibility.AI_RAY_DIFFUSE_REFLECT" },
        { "visible_in_diffuse_transmission",
          "arnoldStatements.visibility.AI_RAY_DIFFUSE_TRANSMIT" },
        { "visible_in_glossy", "arnoldStatements.visibility.AI_RAY_GLOSSY" },
        { "visible_in_reflections",
          "arnoldStatements.visibility.AI_RAY_REFLECTED" },
        { "visible_in_refractions",
          "arnoldStatements.visibility.AI_RAY_REFRACTED" },
        { "visible_in_specular_reflection",
          "arnoldStatements.visibility.AI_RAY_SPECULAR_REFLECT" },
        { "visible_in_specular_transmission",
          "arnoldStatements.visibility.AI_RAY_SPECULAR_TRANSMIT" },
        { "visible_in_volume", "arnoldStatements.visibility.AI_RAY_VOLUME" }
    };

    const auto found = sArnoldAttrs.find(iName);
    if (found != sArnoldAttrs.end())
    {
        return found->second;
    }

    return {};
}
