// Copyright 2017 Rodeo FX.  All rights reserved.

#include "index.h"

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/reversed.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

void RendererIndex::insertPrim(
        const SdfPath& parentPath, const SdfPath& objectPath)
{
    {
        // Scope because we have another guard mHierarchyMap.
        // TODO: Maybe concurrent_unordered_set is better?
        FastMutexLock lock(mCachedObjectSetLock);
        mCachedObjectSet.insert(objectPath);
    }

    ObjectMap::accessor it;
    mHierarchyMap.insert(it, parentPath);
    it->second.insert(objectPath);
}

bool RendererIndex::isProcessed(const SdfPath& path) const
{
    FastMutexLock lock(mCachedObjectSetLock);
    return mCachedObjectSet.find(path) != mCachedObjectSet.end();
}

int RendererIndex::getNumNodes(const SdfPath& path)
{
    auto it = mNumNodes.find(path);
    if (it != mNumNodes.end())
    {
        return it->second;
    }
    else
    {
        ObjectMap::const_accessor it;
        if (!mHierarchyMap.find(it, path))
        {
            // Nothing is found.
            return 0;
        }

        int numNodes = it->second.size();
        mNumNodes.emplace(path, numNodes);
        return numNodes;
    }
}

void RendererIndex::insertNumNodes(const SdfPath& path, int numNodes)
{
    mNumNodes.emplace(path, numNodes);
}

bool RendererIndex::isChildrenKnown(const SdfPath& path) const
{
    ObjectMap::const_accessor it;
    return mHierarchyMap.find(it, path);
}

bool RendererIndex::isParentKnown(
    const SdfPath& root,
    const SdfPath& path) const
{
    ObjectMap::const_accessor it;
    if (!mHierarchyMap.find(it, root))
    {
        return false;
    }

    for (SdfPath potentialParent: it->second)
    {
        if (path.HasPrefix(potentialParent))
        {
            return true;
        }
    }

    return false;
}

bool RendererIndex::isAssignmentDone() const
{
    return !mAssignments.empty();
}

bool RendererIndex::hasGlobalMaterials() const
{
    SdfPath root("/");
    SdfPath materials("/materials");

    ObjectMap::const_accessor it;
    return mHierarchyMap.find(it, root) || mHierarchyMap.find(it, materials);
}

const SdfPath* RendererIndex::getPath(
        const SdfPath& parentPath, unsigned int i)const
{
    ObjectMap::const_accessor it;
    if (!mHierarchyMap.find(it, parentPath))
    {
        // Nothing is found.
        return nullptr;
    }

    const ObjectSet& set = it->second;
    if (i >= set.size())
    {
        // Out of range.
        return nullptr;
    }

    return &(*std::next(set.begin(), i));
}

bool RendererIndex::getRenderNodeData(
    const SdfPath& path,
    RenderNodeMap::accessor& accessor)
{
    return mRenderNodeMap.insert(accessor, path);
}

bool RendererIndex::getRenderNodeData(
    const SdfPath& path,
    RenderNodeMap::const_accessor& accessor)
{
    return mRenderNodeMap.find(accessor, path);
}

void RendererIndex::insertExpression(
        const SdfPath& expressionPrimPath,
        const SdfPath& rootPath,
        const std::string& expression,
        const WalterExpression::AssignmentLayers& layers)
{
    // The alembic can be loaded as an object. It means it's not in the root and
    // we need to extend the expressions to the full path because we don't want
    // to apply them to all the objects.
    std::string fullExpression = rootPath.GetText();
    // If the latest symbol is '/', remove it.
    if (fullExpression[fullExpression.size()-1] == '/')
    {
        fullExpression = fullExpression.substr(0, fullExpression.size()-1);
    }

    // RND-555: Demangle the expression to convert special regex characters
    fullExpression += WalterCommon::demangleString(
        WalterCommon::convertRegex(expression));

    // Iterate the layers.
    for (auto& l : layers)
    {
        // "defaultRenderLayer"
        TargetToObjShader& currentLayer = mAssignments[l.first];

        for (auto& t : l.second)
        {
            // "shader"
            const std::string& targetName = t.first;

            // Filling mAssignments.
            auto emplaceResult = currentLayer[targetName].emplace(
                std::piecewise_construct,
                std::forward_as_tuple(fullExpression),
                std::forward_as_tuple(t.second));
            if (!emplaceResult.second)
            {
                // The insertion only takes place if no other element in the
                // container has a key equivalent to the one being emplaced.
                // emplaceResult.second is false if the key already existed.
                // Skip it.
                continue;
            }

            // "/materials/aiStandard1"
            SdfPath& material = emplaceResult.first->second;

            // USD converts relative names to absolute names during composition.
            // Make it relative back.
            if (material.GetParentPath() == expressionPrimPath)
            {
                material = material.MakeRelativePath(expressionPrimPath);
            }

            // Material points to the UsdShadeMaterial. Arnold requires a
            // shader. We need to find the shader and save it.
            auto itT = mMaterials.find(material);
            if (itT == mMaterials.end())
            {
                continue;
            }

            const WalterExpression::AssignmentTargets& matTarget = itT->second;

            auto itS = matTarget.find(targetName);
            if (itS == matTarget.end())
            {
                // A special case because a surface shader in Arnold is just
                // "shader"
                if (targetName != "shader" ||
                    (itS = matTarget.find("surface")) == matTarget.end() )
                {
                    continue;
                }
            }

            material = itS->second;
        }

    }
}

void RendererIndex::insertMaterial(
        const SdfPath& material,
        const std::string& target,
        const SdfPath& shader)
{
    mMaterials[material][target] = shader;
}

void RendererIndex::insertAttribute(
        const SdfPath& iOverridePath,
        const std::string& iAttributeName,
        const RendererAttribute& iAttribute)
{
    mAttributes[iOverridePath].emplace(iAttributeName, iAttribute);
}

SdfPath RendererIndex::getShaderAssignment(
        const std::string& objectName,
        const std::string& layer,
        const std::string& target)const
{
    // Looking for requested render layer
    auto layerIt = mAssignments.find(layer);
    if (layerIt == mAssignments.end())
    {
        return SdfPath();
    }

    // Looking for necessary target.
    auto targetIt = layerIt->second.find(target);
    if (targetIt == layerIt->second.end())
    {
        return SdfPath();
    }

    const ObjectToShader& objectList = targetIt->second;

    const SdfPath* shader =
        WalterCommon::resolveAssignment<SdfPath>(objectName, objectList);

    if (!shader)
    {
        return SdfPath();
    }

    return *shader;
}

const NameToAttribute* RendererIndex::getAttributes(SdfPath iOverridePath) const
{
    auto it = mAttributes.find(iOverridePath);
    if (it == mAttributes.end())
    {
        return nullptr;
    }

    const NameToAttribute& attributes = it->second;
    return &attributes;
}

const NameToAttribute* RendererIndex::getObjectAttribute(
    const std::string& iVirtualObjectName,
    const std::string& iLayer)
{
    assert(!iVirtualObjectName.empty());

    // First, try to find the current object in the cache.
    ObjectAttrs::accessor accessor;
    if (mObjectAttributes.find(accessor, iVirtualObjectName))
    {
        // Found!!!
        return &accessor->second;
    }

    // TODO: It's not clear if `accessor` still locks this scope. It shouldn't.

    // Get the name of the walterOverride.
    SdfPath attributePath =
        getShaderAssignment(iVirtualObjectName, iLayer, "attribute");

    // Get the attributes from the walter override object.
    const NameToAttribute* attributes = getAttributes(attributePath);

    if (attributes)
    {
        // Create a list of attributes and copy everything from Walter Override.
        mObjectAttributes.emplace(
            accessor,
            std::piecewise_construct,
            std::forward_as_tuple(iVirtualObjectName),
            std::forward_as_tuple(*attributes));
    }
    else
    {
        // Create a new empty list of attributes.
        mObjectAttributes.emplace(
            accessor,
            std::piecewise_construct,
            std::forward_as_tuple(iVirtualObjectName),
            std::forward_as_tuple());
    }

    // Merge with the attributes of parent.
    if (iVirtualObjectName != "/")
    {
        // Generate the name of the parent.
        // TODO: We could cache even this.
        std::string parentVirtualName;
        size_t found = iVirtualObjectName.find_last_of('/');
        if (found == 0)
        {
            // Special case if the parent is the root.
            parentVirtualName = "/";
        }
        else if (found != std::string::npos)
        {
            parentVirtualName = iVirtualObjectName.substr(0, found);
        }

        if (!parentVirtualName.empty())
        {
            // Recursively get the attributes of the parent.
            const NameToAttribute* parentAttributes =
                getObjectAttribute(parentVirtualName, iLayer);

            if (parentAttributes)
            {
                // Current attributes should be strong, the parent attributes
                // should be weak.
                // TODO: be careful. map::insert has an undefined behaviour.
                // https://cplusplus.github.io/LWG/issue2844
                // It's necessary to check it on all the new compilers.
                accessor->second.insert(
                    parentAttributes->begin(), parentAttributes->end());
            }
        }
    }

    return &accessor->second;
}

const std::string& RendererIndex::getPrefixPath(
    const std::string& iPrefix) const
{
    static const std::string empty;

    StringMap::const_accessor it;
    if (!mPrefixPathsForProcedurals.find(it, iPrefix))
    {
        return empty;
    }

    return it->second;
}

bool RendererIndex::setPrefixPath(
    const std::string& iPrefix,
    const std::string& iPath)
{
    return mPrefixPathsForProcedurals.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(iPrefix),
        std::forward_as_tuple(iPath));
}
