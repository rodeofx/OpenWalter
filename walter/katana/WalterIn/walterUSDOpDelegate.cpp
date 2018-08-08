// Copyright 2017 Rodeo FX.  All rights reserved.

#include "walterUSDOpDelegate.h"
#include "walterUSDOpCaboose.h"

#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/material.h>
#include <boost/algorithm/string.hpp>

void OpDelegate::populate(const UsdPrim& iRoot, OpIndex& oIndex)
{
    UsdPrimRange range(iRoot);
    for (const UsdPrim& prim : range)
    {
        // We already processed everything and since all the materials are
        // loaded, we can work with expressions.
        if (prim.IsA<WalterExpression>())
        {
            // Get expressions.
            WalterExpression expression(prim);
            // Insert them into a cache.
            oIndex.insertExpression(
                expression.GetExpression(), expression.GetLayers());
        }
        else if (prim.IsA<UsdShadeMaterial>())
        {
            // Check and save WalterOverrides.
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
                    // This block saves Walter Overrides.
                    // We need to save the arnold attributes if any.
                    if (target != "attribute" && splitted.size() >= 3)
                    {
                        continue;
                    }

                    // Get the renderer attribute
                    OpCaboose::ClientAttributePtr clientAttribute =
                        OpCaboose::createClientAttribute(attr);
                    if (!clientAttribute->valid())
                    {
                        continue;
                    }

                    // Save it to the index.
                    oIndex.insertAttribute(
                        prim.GetPath(), attr.GetBaseName(), clientAttribute);
                }
            }
        }
    }
}
