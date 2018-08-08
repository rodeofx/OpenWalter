// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERUSDOPINDEX_H_
#define __WALTERUSDOPINDEX_H_

#include <pxr/usd/sdf/path.h>
#include <boost/noncopyable.hpp>
#include "PathUtil.h"
#include "schemas/expression.h"

PXR_NAMESPACE_USING_DIRECTIVE

class OpDelegate;

// custom specialization of std::hash for SdfPath
namespace std
{
template <> struct hash<SdfPath>
{
    typedef SdfPath argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& path) const noexcept
    {
        return SdfPath::Hash{}(path);
    }
};
}

namespace OpCaboose
{
class ClientAttribute;
// TODO: get rid of double definition
typedef std::shared_ptr<ClientAttribute> ClientAttributePtr;
}

/** @brief It should contain material assignments and the stuff that we would
 * like to cache to use in the future. */
class OpIndex : private boost::noncopyable
{
private:
    template <typename K, typename T> using hashmap = std::unordered_map<K, T>;

public:
    typedef hashmap<std::string, OpCaboose::ClientAttributePtr> NameToAttribute;

    OpIndex();

    /**
     * @brief Resolve the assigned shader using expressions. The difference with
     * resolveMaterialAssignment is this method caches the result and if it was
     * resolved previously, it returns cached path. resolveMaterialAssignment
     * resolves it each time.
     *
     * @param objectName The full name of the object in USD.
     * @param target "shader", "displacement", "attribute"
     *
     * @return The path of the assigned material.
     */
    const SdfPath& getMaterialAssignment(
        const SdfPath& objectPath,
        const std::string& target) const;

    /**
     * @brief Returns attributes of specified path.
     *
     * @param iOverridePath Path to walterOverride object.
     *
     * @return {"attribute name": pointer to attribute}
     */
    const NameToAttribute* getAttributes(SdfPath iOverridePath) const;

private:
    friend class OpDelegate;

    // Building following structure:
    // {"target": {"object": "material"}}
    // In Katana and Maya, we need to get the material only if the material is
    // really assigned to the object and we don't consider inheritance. But here
    // we need to consider inheritance, so we need layer and target at the first
    // layer for fast and easy extracting {"object": "material"} because we need
    // to ignore if it's another target or material is assigned in another
    // render layer.
    typedef std::map<WalterCommon::Expression, SdfPath> ExprToMat;
    typedef hashmap<std::string, ExprToMat> Assignments;

    typedef hashmap<SdfPath, SdfPath> ObjectToShader;
    typedef hashmap<std::string, ObjectToShader> ResolvedAssignments;

    typedef hashmap<SdfPath, NameToAttribute> Attributes;
    /**
     * @brief Save the expression to the index.
     *
     * @param iExpression "/pCube1/pCube2/.*"
     * @param iLayers The structure like this: {"layer": {"target":
     * "material"}}.
     */
    void insertExpression(
        const std::string& iExpression,
        const WalterExpression::AssignmentLayers& iLayers);

    /**
     * @brief Resolve the assigned shader using expressions.
     *
     * @param iObjectName The full name of the object in USD.
     * @param iTarget "shader", "displacement", "attribute"
     *
     * @return The path of the assigned material.
     */
    SdfPath resolveMaterialAssignment(
        const std::string& iObjectName,
        const std::string& iTarget) const;

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
        const OpCaboose::ClientAttributePtr& iAttribute);


    typedef std::mutex Mutex;
    typedef std::lock_guard<Mutex> ScopedLock;
    mutable Mutex mMutex;

    // All the assignments of the cache. We don't need TBB here because we fill
    // it from OpDelegate::populate, when OpEngine is constructed.
    Assignments mAssignments;

    // The assignments that was already resolved. We need it for fast access.
    // TODO: TBB
    mutable ResolvedAssignments mResolvedAssignments;

    // Walter Overrides to Attributes
    Attributes mAttributes;
};

#endif
