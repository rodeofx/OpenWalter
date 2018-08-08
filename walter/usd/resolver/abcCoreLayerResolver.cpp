// Copyright 2017 Rodeo FX. All rights reserved.

#include "abcCoreLayerResolver.h"

#include <boost/algorithm/string.hpp>
#include <pxr/base/arch/fileSystem.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/type.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/ar/defineResolver.h>

#define ABC_CORE_LAYER_SEPARATOR ":"

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(AbcCoreLayerResolver, ArResolver)


inline void splitPath(
        const std::string& path, std::vector<std::string>& archives)
{
    std::vector<std::string> parts;
    boost::split(parts, path, boost::is_any_of(ABC_CORE_LAYER_SEPARATOR));

    std::string drive;

    for (const std::string& part : parts)
    {
        if (part.size() == 1)
        {
            // It's a drive. Just save it.
            drive = part;
        }
        else
        {
            // Skip it if the part is empty.
            if (!part.empty())
            {
                if (!drive.empty())
                {
                    // Recontruct the path.
                    archives.push_back(drive + ":" + part);
                }
                else
                {
                    archives.push_back(part);
                }
            }

            // TODO: do we need a path with one letter only? Right now it's
            // filtered out.
            drive.clear();
        }
    }
}

AbcCoreLayerResolver::AbcCoreLayerResolver() :
    ArDefaultResolver()
{}

std::string AbcCoreLayerResolver::ResolveWithAssetInfo(
        const std::string& path, 
        ArAssetInfo* assetInfo)
{
    // Special case: Alembic
    if (boost::iends_with(path, ".abc"))
    {
        // Probably it's a set of Alembic Core Layers. In this way we need to
        // split it and resolve the layers one by one.
        std::vector<std::string> archives;
        splitPath(path, archives);

        std::vector<std::string> resolved;
        resolved.reserve(archives.size());

        // Resolve each archive with the default resolver.
        for (const std::string& archive : archives)
        {
            std::string result =
                BaseType::ResolveWithAssetInfo(archive, assetInfo);

            if (result.empty())
            {
                continue;
            }

            resolved.push_back(result);
        }

        return boost::join(resolved, ABC_CORE_LAYER_SEPARATOR);
    }

    // Use the default resolver.
    return BaseType::ResolveWithAssetInfo(path, assetInfo);
}

VtValue AbcCoreLayerResolver::GetModificationTimestamp(
    const std::string& path,
    const std::string& resolvedPath)
{
    // Special case: Alembic
    if (boost::iends_with(resolvedPath, ".abc"))
    {
        // Probably it's a set of Alembic Core Layers. In this way we need to
        // split it, get the time of each layer and return the latest time.
        std::vector<std::string> archives;
        splitPath(resolvedPath, archives);

        std::vector<double> times;
        times.reserve(archives.size());

        // Look at the time of the each archive.
        for (const std::string& archive : archives)
        {
            double time;
            if (!ArchGetModificationTime(archive.c_str(), &time))
            {
                continue;
            }

            times.push_back(time);
        }

        if (times.empty())
        {
            return VtValue();
        }

        double maxTime = *std::max_element(times.begin(), times.end());

        return VtValue(maxTime);
    }

    // Use the default time.
    return BaseType::GetModificationTimestamp(path, resolvedPath);
}

PXR_NAMESPACE_CLOSE_SCOPE

