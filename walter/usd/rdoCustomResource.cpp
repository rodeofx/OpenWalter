// Copyright 2017 Rodeo FX. All rights reserved.

#include "rdoCustomResource.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <pxr/base/plug/plugin.h>
#include <pxr/base/tf/fileUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/layer.h>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

std::unordered_map<std::string, const char*> resource;

// Split the given path and return the vector of directories.
std::vector<std::string> splitPath(const boost::filesystem::path &src)
{
    std::vector<std::string> elements;
    for (const auto &p : src)
    {
        elements.push_back(p.c_str());
    }

    return elements;
}

void setResource(const char* filename, const char* contents)
{
    std::vector<std::string> pathVect = splitPath(filename);
    while (!pathVect.empty())
    {
        std::string joined = boost::algorithm::join(pathVect, "/");

        resource[joined] = contents;

        pathVect.erase(pathVect.begin());
    }
}

const char* getResource(const char* filename)
{
    std::vector<std::string> pathVect = splitPath(filename);
    while (!pathVect.empty())
    {
        std::string joined = boost::algorithm::join(pathVect, "/");

        auto it = resource.find(joined);
        if (it != resource.end())
        {
            return it->second;
        }

        pathVect.erase(pathVect.begin());
    }

    return nullptr;
}

// USD overrides
extern "C"
TfToken _GetShaderPath(char const* shader)
{
    return TfToken(shader);
}

extern "C"
std::istream* _GetFileStream(const char* filename)
{
    // Try to search in the saved resources first.
    const char* data = getResource(filename);
    if (data)
    {
        return new std::istringstream(data);
    }

    return new std::ifstream(filename);
}

extern "C"
SdfLayerRefPtr _GetGeneratedSchema(const PlugPluginPtr &plugin)
{
    // Look for generatedSchema in Resources.
    const std::string filename = TfStringCatPaths(
            plugin->GetResourcePath(),
            "generatedSchema.usda");

    const char* data = getResource(filename.c_str());
    if (data)
    {
        SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(filename);
        if (layer->ImportFromString(data))
        {
            return layer;
        }
    }

    return TfIsFile(filename) ? SdfLayer::OpenAsAnonymous(filename) : TfNullPtr;
}
