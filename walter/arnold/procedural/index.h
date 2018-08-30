// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __INDEX_H__
#define __INDEX_H__

#include "plugin.h"

#include "PathUtil.h"
#include "schemas/expression.h"

#include <pxr/base/tf/hashmap.h>
#include <pxr/base/tf/hashset.h>
#include <pxr/usd/sdf/path.h>
#include <tbb/concurrent_hash_map.h>

#include <mutex>
#include <unordered_map>

PXR_NAMESPACE_USING_DIRECTIVE

// The object that indexes and keeps USD data for fast access. Generally,
// UsdStage keeps caches but not everything.
class RendererIndex
{
public:
    /** @brief Tbb hash for SdfPath. */
    struct TbbHash
    {
        static size_t hash(const SdfPath& x) { return SdfPath::Hash{}(x); }
        static bool equal(const SdfPath& x, const SdfPath& y) { return x == y; }
    };

    typedef std::mutex Mutex;
    typedef std::lock_guard<Mutex> ScopedLock;
    typedef tbb::concurrent_hash_map<SdfPath, void*, TbbHash> RenderNodeMap;

    // Insert the prim to the index.
    void insertPrim(const SdfPath& parentPath, const SdfPath& objectPath);

    // Return true if the object is in the index.
    bool isProcessed(const SdfPath& path) const;

    // The number of the children (number found is cached for next call).
    int getNumNodes(const SdfPath& path);

    // Insert a number of nodes for a given path (needed for any path that
    // didn't go through the delegate.populate function such as point instancer)
    void insertNumNodes(const SdfPath& path, int numNodes);

    // Return true if children of this path are already in the index
    bool isChildrenKnown(const SdfPath& path) const;

    // Return true if a parent of this path is already in the index.
    bool isParentKnown(const SdfPath& root, const SdfPath& path) const;

    // Assignment are always checked from "/" and on the entire stage, so it
    // doesn't need to be done twice. This function return true if it has
    // already be done.
    bool isAssignmentDone() const;

    // Return true if `/` or `/materials` has already been registered as
    // parent location in the hierarchy map.
    bool hasGlobalMaterials() const;

    // Get children.
    const SdfPath* getPath(const SdfPath& parentPath, unsigned int i)const;

    /**
     * @brief Get the arnold render node data for specific path and provide a
     * writer lock that permits exclusive access to the given enry of the map by
     * a thread. Blocks access to the given enry of the map by other threads.
     *
     * @param path The given path.
     * @param accessor The tbb accessor.
     *
     * @return True if new entry was inserted; false if key was already in the
     * map.
     */
    bool getRenderNodeData(
        const SdfPath& path,
        RenderNodeMap::accessor& accessor);

    /**
     * @brief Get the arnold render node data for specific path and provide a
     * reader lock that permits shared access with other readers.
     *
     * @param path The given path.
     * @param accessor The tbb accessor.
     *
     * @return True if the entry was found; false if key was not found.
     */
    bool getRenderNodeData(
        const SdfPath& path,
        RenderNodeMap::const_accessor& accessor);

    // Save the expression in the index.
    void insertExpression(
            const SdfPath& expressionPrimPath,
            const SdfPath& rootPath,
            const std::string& expression,
            const WalterExpression::AssignmentLayers& layers);

    // Save the material in the index.
    void insertMaterial(
            const SdfPath& material,
            const std::string& target,
            const SdfPath& shader);

    /**
     * @brief Save Walter Override's single attribute.
     *
     * @param iOverridePath Walter Override.
     * @param iAttributeName Attribute name.
     * @param iAttribute The attribute.
     */
    void insertAttribute(
        const SdfPath& iOverridePath,
        const std::string& iAttributeName,
        const RendererAttribute& iAttribute);

    // Get the assigned shader.
    SdfPath getShaderAssignment(
            const std::string& objectName,
            const std::string& layer,
            const std::string& target)const;

    /**
     * @brief Returns the attributes of the Walter Override object.
     *
     * @param iOverridePath Walter Override object
     *
     * @return The pointer to the attributes.
     */
    const NameToAttribute* getAttributes(SdfPath iOverridePath) const;

    /**
     * @brief Returns the attributes of the object. It merges the attributes
     * (strong) with the attributes of all the parents (weak), so we have all
     * the propagated attributes here. It uses a cache to store the object name
     * as a key and all the attributes as a value. Thus, it doesn't resolve the
     * attributes for all the parents each time. It resolves them for the
     * current object and merges them with the parent.
     *
     * @param iVirtualObjectName The virtual name of the object.
     * @param iLayer The current render layer.
     *
     * @return The pointer to the attributes.
     */
    const NameToAttribute* getObjectAttribute(
        const std::string& iVirtualObjectName,
        const std::string& iLayer);

    /**
     * @brief Get the full path of the given prefix.
     *
     * @param iPrefix It's the arnold parameter "prefix" that is used to keep
     * the name of the original procedural arnold node.
     *
     * @return The full path of the original procedural arnold node.
     */
    const std::string& getPrefixPath(const std::string& iPrefix) const;
    /**
     * @brief Save the full path of the given prefix. It is used to generate
     * a 'virtual path'. Such path does not exist in USD stage. Its purpose
     * is to resolve expressions.
     *
     * @param iPrefix It's the arnold parameter "prefix" that is used to keep
     * the name of the original procedural arnold node.
     * @param iPath The full path of the original procedural arnold node.
     *
     * @return Returns true if item is new.
     */
    bool setPrefixPath(const std::string& iPrefix, const std::string& iPath);

private:
    typedef TfHashSet<SdfPath, SdfPath::Hash> ObjectSet;
    typedef tbb::concurrent_hash_map<SdfPath, ObjectSet, TbbHash> ObjectMap;

    // Building following structure:
    // {"layer": {"target": {"object": "shader"}}}
    // In Katana and Maya, we need to get the shader only if the shader is
    // really assigned to the object and we don't consider inheritance. But here
    // we need to consider inheritance, so we need layer and target at the first
    // layer for fast and easy extracting {"object": "shader"} because we need
    // to ignore if it's another target or shader is assigned in another render
    // layer.
    typedef std::map<WalterCommon::Expression, SdfPath> ObjectToShader;
    typedef std::unordered_map<std::string, ObjectToShader> TargetToObjShader;
    typedef std::unordered_map<std::string, TargetToObjShader> Assignments;

    // Building following structure:
    // {"material": {"target": "shader"}}
    typedef TfHashMap<
        SdfPath,
        WalterExpression::AssignmentTargets,
        SdfPath::Hash>
        Materials;

    // Building following structure:
    // {"walterOverride": {"attribute name": "renderer attribute"}}
    typedef TfHashMap<SdfPath, NameToAttribute, SdfPath::Hash> Attributes;
    typedef tbb::concurrent_hash_map<std::string, NameToAttribute> ObjectAttrs;

    typedef std::map<SdfPath, int> NumNodesMap;

    // All the objects that are in the index. They are considered as processed.
    // TODO: If using tbb:concurrent_unordered_set, we can ge rid of
    // mCachedObjectSetLock
    ObjectSet mCachedObjectSet;
    // Key: parent, value: the list of the objects.
    ObjectMap mHierarchyMap;
    // Name to arnold render nodes map for fast access.
    RenderNodeMap mRenderNodeMap;
    // All the assignments of the cache.
    Assignments mAssignments;
    // Materials
    Materials mMaterials;

    // Walter Overrides to Attributes
    Attributes mAttributes;
    // Object to Attributes. List of attributes for each object in the scene. We
    // need it to fast query attributes of the parent object.
    ObjectAttrs mObjectAttributes;

    NumNodesMap mNumNodes;

    // The storage to keep the full paths of the given prefixes. Prefixe is the
    // arnold parameter of the walter procedural that is used to keep the name
    // of the original procedural arnold node. We use this object to track the
    // instances.
    typedef tbb::concurrent_hash_map<std::string, std::string> StringMap;
    StringMap mPrefixPathsForProcedurals;

    // TODO: We probably need to use tbb::spin_mutex here. We need to test it.
    typedef Mutex FastMutex;
    typedef ScopedLock FastMutexLock;

    mutable FastMutex mCachedObjectSetLock;
};

#endif
