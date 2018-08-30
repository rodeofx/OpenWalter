// Copyright 2017 Rodeo FX.  All rights reserved.

#include "delegate.h"

#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/material.h>
#include <boost/algorithm/string.hpp>

#include "schemas/expression.h"

PXR_NAMESPACE_USING_DIRECTIVE

RendererDelegate::RendererDelegate(
    RendererIndex& index,
    RendererPlugin& plugin) :
        mIndex(index),
        mPlugin(plugin)
{}

void RendererDelegate::populate(const UsdPrim& root)
{
    const SdfPath rootPath = root.GetPath();
    bool childrenKnown = mIndex.isChildrenKnown(rootPath);

    // This accessor will freeze if there is another accessor in different
    // thread with the same rootPath.
    SdfPathMutex::accessor it;
    if (!mPopulateGuard.insert(it, rootPath))
    {
        // We already did it before. Skip.
        return;
    }

    // TODO: limit type.
    UsdPrimRange range(root);
    for (const UsdPrim& prim : range)
    {
        // We need to process the expressions at the end because we need all the
        // materials loaded.
        if (prim.IsA<WalterExpression>())
        {
            continue;
        }

        const SdfPath primPath = prim.GetPath();

        // This will check if, in the hierarchy of the root path, a parent prim
        // of primPath has already been added to the index.
        // If it's the case, it means that a walter procedural will be created
        // for it (the parent) and so children will be populate at this time.
        if (mIndex.isParentKnown(rootPath, primPath))
        {
            continue;
        }

        // In some cases like a bbox generated from a "point instance" a path could
        // already be added to the index, but not from the same parent. In this case
        // we can't skip to early.
        bool skipLater = false;

        // Skip if it's already cached. In USD it's possible to have several
        // parents. We need to be sure that we output the object only once.
        if (rootPath != primPath && mIndex.isProcessed(primPath))
        {
            if (childrenKnown)
            {
                continue;
            }
            else
            {
                skipLater = true;
            }
        }

        if (mPlugin.isSupported(prim))
        {
            mIndex.insertPrim(rootPath, primPath);
        }
        else if (mPlugin.isImmediate(prim))
        {
            mIndex.insertPrim(rootPath, primPath);
        }

        if (skipLater)
        {
            continue;
        }

        if (prim.IsA<UsdShadeMaterial>())
        {
            // Iterate the connections.
            for (const UsdAttribute& attr : prim.GetAttributes())
            {
                // The name is "arnold:surface"
                std::string name = attr.GetName();

                std::vector<std::string> splitted;
                boost::split(splitted, name, boost::is_any_of(":"));

                if (splitted.size() < 2)
                {
                    continue;
                }

                // Extract the render and the target.
                std::string render = splitted[0];
                std::string target = splitted[1];

                // TODO: other renders?
                if (render != "arnold")
                {
                    continue;
                }

                if (!UsdShadeConnectableAPI::HasConnectedSource(attr))
                {
                    // This block saved Walter Overrides.
                    // We need to save the arnold attributes if any.
                    if (target != "attribute" && splitted.size() >= 3)
                    {
                        continue;
                    }

                    // Get the renderer attribute.
                    RendererAttribute rendererAttribute =
                        mPlugin.createRendererAttribute(attr);
                    if (!rendererAttribute.valid())
                    {
                        continue;
                    }

                    // Save it to the index.
                    mIndex.insertAttribute(
                        primPath, attr.GetBaseName(), rendererAttribute);

                    continue;
                }

                // Get the connection using ConnectableAPI.
                UsdShadeConnectableAPI connectableAPISource;
                TfToken sourceName;
                UsdShadeAttributeType sourceType;
                if (!UsdShadeConnectableAPI::GetConnectedSource(
                        attr, &connectableAPISource, &sourceName, &sourceType))
                {
                    // Should never happen because we already checked it.
                    continue;
                }

                mIndex.insertMaterial(
                    primPath, target, connectableAPISource.GetPrim().GetPath());
            }
        }
    }

    // If the locations '/' or '/materials' were not already "scanned" we don't
    // want to check for expression and assignment as materials may not be
    // retrieved to do it properly. So skip the Expression scan until
    // '/materials' has been scanned in the loop above.
    //
    // If materials are retrieved and the check for expression and assignment
    // are already done there is nothing else to do and we can skip the last
    // loop on WalterExpression.
    if (!mIndex.hasGlobalMaterials() || mIndex.isAssignmentDone())
    {
        return;
    }

    // Always check assignment from the pseudo root "/" whatever the given
    // root prim was as material are usually under "/" directly.
    UsdPrim pseudoRoot = root.GetStage()->GetPseudoRoot();
    range = UsdPrimRange(pseudoRoot);

    for (const UsdPrim& prim : range)
    {
        // We already processed everything and since all the materials are
        // loaded, we can work with expressions.
        if (!prim.IsA<WalterExpression>())
        {
            continue;
        }

        // Get expressions.
        WalterExpression expression(prim);
        std::string expressionText = expression.GetExpression();
        WalterExpression::AssignmentLayers layers = expression.GetLayers();

        // Insert them into a cache.
        mIndex.insertExpression(
            prim.GetPath(), pseudoRoot.GetPath(), expressionText, layers);
    }
}
