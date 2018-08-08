// Copyright 2017 Rodeo FX.  All rights reserved.

#include "walterShaderUtils.h"
#include "schemas/walterHdStream/shader.h"

#include "rdoProfiling.h"

#include <OpenImageIO/imageio.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <boost/filesystem.hpp>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

OIIO_NAMESPACE_USING

class ShaderNode;
typedef std::shared_ptr<ShaderNode> ShaderNodePtr;

/** @brief The type to attributes map. It represents the important attributes we
 * are interested to scan. */
static const std::unordered_map<std::string, std::vector<std::string>>
    sTypeToPlug = {
        {"aiStandard", {"color"}},
        {"aiStandardSurface", {"baseColor"}},
        {"alCombineColor", {"input1", "input2", "input3"}},
        {"alCombineFloat", {}},
        {"alLayerColor",
         {"layer1",
          "layer2",
          "layer3",
          "layer4",
          "layer5",
          "layer6",
          "layer7",
          "layer8"}},
        {"alLayerFloat", {}},
        {"alRemapColor", {"input"}},
        {"alRemapFloat", {}},
        {"alSurface", {"diffuseColor"}},
        {"alSwitchColor",
         {"inputA", "inputB", "inputC", "inputD", "inputE", "inputF"}},
        {"alSwitchFloat", {}},
        {"anisotropic", {"color"}},
        {"blinn", {"color"}},
        {"lambert", {"color"}},
        {"phong", {"color"}},
        {"phongE", {"color"}},
        {"standard", {"Kd_color"}},
        {"surfaceShader", {"color"}}};

/** @brief This class represents a simple shading node that can store
 * connections to another shaders and the colors. */
class ShaderNode
{
public:
    ShaderNode(
        const std::string& name,
        const std::string& type,
        const std::string& texture = {}) :
            mName(name),
            mType(type),
            mTexture(texture)
    {}

    /**
     * @brief Set the connection to the another shader.
     *
     * @param name Name of the connection.
     * @param shaderNode Connected shader.
     */
    void addPlug(const std::string& name, ShaderNodePtr shaderNode)
    {
        mPlugs[name] = shaderNode;
    }

    /**
     * @brief Set the color.
     *
     * @param name The name of the color.
     * @param color The color to set.
     */
    void addColor(const std::string& name, const GfVec3f& color)
    {
        mColors[name] = color;
    }

    /**
     * @brief Scan the network and return the textures in the plugs using
     * sTypeToPlug map.
     *
     * @param textures Adds found textures there.
     *
     * @return True of the network has textures.
     */
    bool getTextures(std::set<std::string>& textures)
    {
        if (!mTexture.empty())
        {
            // We found a texture, no need to scan.
            textures.insert(mTexture);
            return true;
        }

        // It will be true if we have found a texture.
        bool found = false;

        auto it = sTypeToPlug.find(mType);
        if (it != sTypeToPlug.end())
        {
            // It's known type. We need to scan requested plugs only.
            for (const std::string requestedPlug : it->second)
            {
                auto itRequested = mPlugs.find(requestedPlug);

                if (itRequested == mPlugs.end())
                {
                    // There is no requested plug.
                    continue;
                }

                // Check if the requested plug has a texture.
                if (itRequested->second->getTextures(textures))
                {
                    found = true;
                    // No necessary to continue scanning.
                    break;
                }
            }
        }
        else
        {
            // Unknown type. We need to scan everything.
            for (const auto& plug : mPlugs)
            {
                if (plug.second->getTextures(textures))
                {
                    // Even if we found a texture, there is no guarantee that
                    // this texture is important. So we continue scanning.
                    found = true;
                }
            }
        }

        return found;
    }

    /**
     * @brief Scan the network and get the first available color from the
     * sTypeToPlug map.
     *
     * @param oColor Returned color.
     *
     * @return True if success.
     */
    bool getColor(GfVec3f& oColor)
    {
        // Get the list of attributes depending on the node type.
        auto it = sTypeToPlug.find(mType);
        if (it != sTypeToPlug.end())
        {
            // Check if we have the color stored.
            for (const std::string requestedPlug : it->second)
            {
                auto itRequested = mColors.find(requestedPlug);
                if (itRequested == mColors.end())
                {
                    continue;
                }

                // Found the requested color.
                oColor = itRequested->second;
                return true;
            }
        }

        // We don't have the color stored. Check all the connections.
        for (const auto& plug : mPlugs)
        {
            if (plug.second->getColor(oColor))
            {
                return true;
            }
        }

        return false;
    }

private:
    std::string mName;
    std::string mType;
    std::string mTexture;

    std::unordered_map<std::string, ShaderNodePtr> mPlugs;
    std::unordered_map<std::string, GfVec3f> mColors;
};

/**
 * @brief Returns the color from the plug.
 *
 * @param plug Plug to scan.
 * @param oColor Returned color.
 *
 * @return True if the plug is a color plug.
 */
bool getColor(const MPlug& plug, GfVec3f& oColor)
{
    // Color compounf has three children.
    unsigned int numChildren = plug.numChildren();
    if (numChildren != 3)
    {
        return false;
    }

    bool found[] = {false, false, false};

    // Look into the existing plugin code.
    // devkit/plug-ins/D3DViewportRenderer.cpp, it extracts the
    // "ambientColor" and other attributes.
    for (unsigned int i = 0; i < numChildren; i++)
    {
        // Sometimes the children plugs are not sorted. We need to get the
        // component by the name.
        MPlug child = plug.child(i);
        MString name = child.name();
        char component = name.asChar()[name.length() - 1];
        int index;

        if (component == 'R')
        {
            index = 0;
        }
        else if (component == 'G')
        {
            index = 1;
        }
        else if (component == 'B')
        {
            index = 2;
        }
        else
        {
            // It's not a color. Stop the loop.
            break;
        }

        // Save the color.
        oColor[index] = child.asFloat();
        found[index] = true;
    }

    return found[0] && found[1] && found[2];
}

/**
 * @brief Scan Maya shading natwork and generate similar network with ShaderNode
 * objects. We need it to have fast access to the network and scan it to get
 * the main color and textures.
 *
 * @param shaderObj Maya shader.
 * @param cache Cache with the shaders that was already scanned. We need it to
 * avoid querying Maya objects several times.
 *
 * @return Pinter to the root shader.
 */
ShaderNodePtr getShader(
    const MObject& shaderObj,
    std::unordered_map<std::string, ShaderNodePtr>& cache)
{
    RDOPROFILE("Scan Maya Shader");

    MFnDependencyNode shader(shaderObj);

    // Shader name.
    std::string name = shader.name().asChar();

    // Check if we already scanned this shader.
    auto it = cache.find(name);
    if (it != cache.end())
    {
        return it->second;
    }

    std::string type = shader.typeName().asChar();
    std::string texture;

    if (type == "file" || type == "aiImage")
    {
        // List of attributes where the texture can be placed.
        static const char* attributes[] = {
            "computedFileTextureNamePattern", "fileTextureName", "filename"};

        for (const char* attr : attributes)
        {
            // Get the regular texture name from it.
            MPlug fileTextureName = shader.findPlug(attr);
            if (fileTextureName.isNull())
            {
                continue;
            }

            texture = fileTextureName.asString().asChar();
            break;
        }
    }

    // Create our simple cache node.
    ShaderNodePtr shaderNode =
        std::make_shared<ShaderNode>(name, type, texture);
    // Save it to the cache to avoid querying Maya objects several times.
    cache[name] = shaderNode;

    for (int i = 0, n = shader.attributeCount(); i < n; i++)
    {
        // Iterate all the attributes.
        MPlug plug = shader.findPlug(shader.attribute(i));

        if (plug.isNull())
        {
            continue;
        }

        // Get plug name. No node name, no index, no instanced index, no full
        // attribute path, no alias, return long name.
        std::string plugName =
            plug.partialName(false, false, false, false, false, true).asChar();

        // Get the connections of the plug.
        MPlugArray connections;
        if (!plug.connectedTo(connections, true, false) ||
            !connections.length())
        {
            // Check if it's a color.
            GfVec3f color;
            if (getColor(plug, color))
            {
                shaderNode->addColor(plugName, color);
            }

            continue;
        }

        // It's a connection. Generate connected shader and save it to our node.
        ShaderNodePtr connectedNode = getShader(connections[0].node(), cache);
        shaderNode->addPlug(plugName, connectedNode);
    }

    return shaderNode;
}

/**
 * @brief Scan USD shading natwork and generate similar network with ShaderNode
 * objects. We need it to have fast access to the network and scan it to get
 * the main color and textures.
 *
 * @param shaderObj UsdShadeShader object that represents the shader.
 * @param cache Cache with the shaders that was already scanned. We need it to
 * avoid querying Maya objects several times.
 *
 * @return Pinter to the root shader.
 */
ShaderNodePtr getShader(
    const UsdShadeShader& shaderObj,
    std::unordered_map<std::string, ShaderNodePtr>& cache)
{
    RDOPROFILE("Scan USD Shader");

    // We checked it before in processGLSLShaders
    assert(shaderObj);

    const UsdPrim prim = shaderObj.GetPrim();

    // Shader name.
    std::string name = prim.GetPath().GetElementString();

    // Check if we already scanned this shader.
    auto it = cache.find(name);
    if (it != cache.end())
    {
        return it->second;
    }

    // Get the arnold type of the shader.
    UsdAttribute typeAttr = prim.GetAttribute(TfToken("info:type"));
    TfToken type;
    if (!typeAttr || !typeAttr.Get(&type))
    {
        type = TfToken("unknown");
    }

    // Get the texture file name.
    std::string texture;
    if (type == "image" || type == "MayaFile")
    {
        // Get the regular texture name from it.
        UsdAttribute fileNameAttr = prim.GetAttribute(TfToken("filename"));

        if (fileNameAttr)
        {
            fileNameAttr.Get(&texture);
        }
    }

    // Create our simple cache node.
    ShaderNodePtr shaderNode =
        std::make_shared<ShaderNode>(name, type.GetString(), texture);
    // Save it to the cache to avoid querying Maya objects several times.
    cache[name] = shaderNode;

    // Iterate the connections.
    for (const UsdAttribute& attr : prim.GetAttributes())
    {
        const SdfValueTypeName attrTypeName = attr.GetTypeName();
        if (attrTypeName != SdfValueTypeNames->Color3f &&
            attrTypeName != SdfValueTypeNames->Float3)
        {
            // We need colors only.
            continue;
        }

        const std::string attrName = attr.GetName();

        // Check the connection.
        UsdShadeShader connectedShader =
            walterShaderUtils::getConnectedScheme<UsdShadeShader>(attr);
        if (connectedShader)
        {
            // It's a connection. Generate connected shader and save it to our
            // node.
            ShaderNodePtr connectedNode = getShader(connectedShader, cache);
            shaderNode->addPlug(attrName, connectedNode);
        }
        else
        {
            GfVec3f color;
            if (attr.Get(&color))
            {
                shaderNode->addColor(attrName, color);
            }
        }
    }

    return shaderNode;
}

template <typename T>
void walterShaderUtils::getTextureAndColor(
    const T& shaderObj,
    std::string& oTexture,
    GfVec3f& oColor)
{
    // Shader name to shader cache pointer map. It's a list of shaders that are
    // already converted. It's used to avoid double convertion of the same
    // shader.
    std::unordered_map<std::string, ShaderNodePtr> allShaders;

    // Get the pointer to the shader cache node.
    ShaderNodePtr shader = getShader(shaderObj, allShaders);

    // Get all the textures that connected to the diffuse plug.
    std::set<std::string> textures;
    shader->getTextures(textures);

    // Return the first texture with "dif" in the name.
    for (const std::string& i : textures)
    {
        boost::filesystem::path filename(i);
        if (filename.filename().string().find("dif") != std::string::npos)
        {
            oTexture = i;
            return;
        }
    }

    // We are here because there is no texture with "dif" in the name. Just
    // return the first texture.
    if (!textures.empty())
    {
        oTexture = *textures.begin();
        return;
    }

    // We are here because thare are no textures at all. Return color.
    shader->getColor(oColor);
}

// Explicit instantiations of getTextureAndColor.
template void walterShaderUtils::getTextureAndColor<MObject>(
    const MObject& shaderObj,
    std::string& oTexture,
    GfVec3f& oColor);
template void walterShaderUtils::getTextureAndColor<UsdShadeShader>(
    const UsdShadeShader& shaderObj,
    std::string& oTexture,
    GfVec3f& oColor);

std::string walterShaderUtils::cacheTexture(const std::string& filename)
{
    // Convert file
    boost::filesystem::path filePath(filename);

    if (!boost::filesystem::exists(filePath))
    {
        return filename;
    }

    // Get the file modification time. It's much faster than finding SHA and
    // it's pretty unique per file.
    std::time_t t = boost::filesystem::last_write_time(filePath);
    // Generate the new filename. It contains the old name, the file
    // modification time and it's always EXR.
    const std::string newName =
        filePath.stem().native() + "." + std::to_string(t) + ".exr";
    // The cache directory.
    static const boost::filesystem::path outDir =
        boost::filesystem::temp_directory_path() / "walter_texture_cache";

    // The full file path of the new file. If it already exists, we can use it.
    const boost::filesystem::path newPath = outDir / newName;
    const std::string newFullName = newPath.native();
    if (boost::filesystem::exists(newPath))
    {
        return newFullName;
    }

    // Create temp directory if it doesn't exist.
    if (!boost::filesystem::exists(outDir))
    {
        boost::filesystem::create_directories(outDir);
    }

    // OpenImageIO. Open image.
    ImageInput* in = ImageInput::open(filename);
    if (!in)
    {
        return filename;
    }

    // Get the mipmap number of the best image.
    ImageSpec spec;
    int counter = 0;
    int miplevel = -1;
    while (in->seek_subimage(0, counter, spec))
    {
        // Ideally we need 512x512.
        if (spec.width >= 512 && spec.height >= 512)
        {
            miplevel = counter;
        }
        else
        {
            break;
        }

        counter++;
    }

    // If 0, than nothing to cache, we can use the original. If -1, than there
    // are no mipmaps.
    if (miplevel <= 0)
    {
        in->close();
        return filename;
    }

    // Create OpenImageIO object to output image.
    ImageOutput* out = ImageOutput::create(newFullName);
    if (!out)
    {
        in->close();
        return filename;
    }

    // Write only mipmap we found.
    out->open(newFullName, spec);
    out->copy_image(in);

    // Close files.
    out->close();
    in->close();

    return newFullName;
}
