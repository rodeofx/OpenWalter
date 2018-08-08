// Copyright 2017 Rodeo FX.  All rights reserved.

#include "walterUSDOpIndex.h"

#include <boost/algorithm/string.hpp>
#include "walterUSDOpUtils.h"

OpIndex::OpIndex()
{}

const SdfPath& OpIndex::getMaterialAssignment(
    const SdfPath& objectPath,
    const std::string& target) const
{
    ScopedLock lock(mMutex);
    // Getting the necessary target. If it's not exist, we have to create it
    // because we need to cache the result.
    ObjectToShader& objects = mResolvedAssignments[target];

    // Looking for necessary material. Returns a pair consisting of an iterator
    // to the inserted element, or the already-existing element if no insertion
    // happened, and a bool denoting whether the insertion took place. True for
    // Insertion, False for No Insertion.
    const auto result = objects.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(objectPath),
        std::forward_as_tuple());

    SdfPath& assigned = result.first->second;

    if (result.second)
    {
        // There was no object name in the index. We just created it, so we need
        // to replace it with the good value.
        assigned = resolveMaterialAssignment(objectPath.GetString(), target);
    }

    return assigned;
}

const OpIndex::NameToAttribute* OpIndex::getAttributes(
    SdfPath iOverridePath) const
{
    auto it = mAttributes.find(iOverridePath);
    if (it == mAttributes.end())
    {
        return nullptr;
    }

    const NameToAttribute& attributes = it->second;
    return &attributes;
}

void OpIndex::insertExpression(
    const std::string& iExpression,
    const WalterExpression::AssignmentLayers& iLayers)
{
    // WalterExpression::AssignmentLayers is {"layer": {"target": "material"}}
    // WalterExpression::AssignmentTargets is {"target": "material"}

    static const std::string defaultRenderLayer = "defaultRenderLayer";

    const WalterExpression::AssignmentTargets* targets = nullptr;

    if (iLayers.size() == 1)
    {
        // We use the only available layer.
        targets = &iLayers.begin()->second;
    }
    else
    {
        // In the case there are several layers, we use defaultRenderLayer.
        auto layerIt = iLayers.find(defaultRenderLayer);
        if (layerIt != iLayers.end())
        {
            targets = &layerIt->second;
        }
    }

    if (!targets)
    {
        return;
    }

    const std::string fullExpression =
        WalterCommon::demangleString(iExpression);

    for (const auto& t : *targets)
    {
        // targetName is "shader"
        const std::string& targetName = t.first;
        const SdfPath& materialPath = t.second;

        mAssignments[targetName].emplace(
            std::piecewise_construct,
            std::forward_as_tuple(fullExpression),
            std::forward_as_tuple(materialPath));
    }
}

SdfPath OpIndex::resolveMaterialAssignment(
    const std::string& iObjectName,
    const std::string& iTarget) const
{
    // Looking for necessary target.
    auto targetIt = mAssignments.find(iTarget);
    if (targetIt == mAssignments.end())
    {
        return SdfPath();
    }

    const ExprToMat& expressionList = targetIt->second;

    const SdfPath* shader =
        WalterCommon::resolveAssignment<SdfPath>(iObjectName, expressionList);

    if (!shader)
    {
        return SdfPath();
    }

    return *shader;
}

void OpIndex::insertAttribute(
    const SdfPath& iOverridePath,
    const std::string& iAttributeName,
    const OpCaboose::ClientAttributePtr& iAttribute)
{
    mAttributes[iOverridePath].emplace(iAttributeName, iAttribute);
}
