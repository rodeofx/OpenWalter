// Copyright 2017 Rodeo FX.  All rights reserved.

#include "plugin.h"

#include "index.h"
#include "rdoArnold.h"

#include <ai.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/transform.h>
#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/curves.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <tbb/atomic.h>
#include <boost/algorithm/string.hpp>
#include <functional>

PXR_NAMESPACE_USING_DIRECTIVE

/**
 * @brief Searches the pre-defined (non-user) parameter entries of a given node
 * looking for a parameter that matches the name string. If found, returns a
 * pointer to the parameter entry. The difference with
 * AiNodeEntryLookUpParameter is that it takes AiNode.
 *
 * @param node Input node.
 * @param name Parameter name that we are looking for.
 *
 * @return If found, returns a pointer to the parameter entry. Otherwise, it
 * returns NULL.
 */
const AtParamEntry* getArnoldParameter(AtNode* node, const char* name)
{
    return AiNodeEntryLookUpParameter(AiNodeGetNodeEntry(node), name);
}

/**
 * @brief Each time it returns a unique string. It's thread safety.
 *
 * @return "0", "1", "2", etc...
 */
inline std::string uniqueString()
{
    // An atomic value is a value that cannot be divided. Thus, thread safety.
    // The advantage of atomic operations is that they are relatively quick
    // compared to locks.
    static tbb::atomic<unsigned int> counter(0);
    return std::to_string(counter.fetch_and_increment());
}

/**
 * @brief Join instance root path and current path. It returns std::string
 * because when resolving shaders, we work with strings.
 *
 * @param iInstanceRootPath The USD path of the root of the current USD
 * instance.
 * @param iPath Current path.
 *
 * @return Resolved/joined full path that is good for the expression resolving.
 */
inline std::string joinPaths(
    const std::string& iInstanceRootPath,
    const SdfPath& iPath)
{
    assert(!iPath.IsEmpty());

    if (iInstanceRootPath.empty())
    {
        // We don't have the instance, so the joined path is the current path.
        return iPath.GetString();
    }

    const std::string& pathStr = iPath.GetString();

    // Concat them. If we are in the instance, the path looks like this:
    // "/__Master_01/normal/path", we don't need the first part.
    return iInstanceRootPath + pathStr.substr(pathStr.find('/', 1));
}

/**
 * @brief Set Arnold motion_start and motion_end.
 *
 * @param iNode Arnold node to set.
 * @param iData Reference to RendererPluginData.
 */
void setMotionStartEnd(AtNode* iNode, const RendererPluginData& iData)
{
    if (iData.motionStart)
    {
        AiNodeSetFlt(iNode, "motion_start", *iData.motionStart);
    }

    if (iData.motionEnd)
    {
        AiNodeSetFlt(iNode, "motion_end", *iData.motionEnd);
    }
}

inline AtNode* createWalterProcedural(
    const RendererPluginData* iData,
    const std::string iName,
    const std::vector<float>& iTimes,
    const std::vector<float>& iXform,
#if AI_VERSION_ARCH_NUM == 4
    const GfRange3d& iRange,
#endif // ARNOLD 4
    const SdfPath& iObjectPath,
    const std::string& iPrefix)
{
    // Create a procedural.
    AtNode* node = AiNode(RDO_WALTER_PROC);
    AiNodeSetStr(node, "name", iName.c_str());

#if AI_VERSION_ARCH_NUM == 4
    AiNodeSetStr(node, "dso", iData->dso.c_str());
#endif // ARNOLD 4

    // Output frame.
    assert(iTimes.size() > 0);
    if (iTimes.size() == 1)
    {
        AiNodeDeclare(node, "frame", "constant FLOAT");
        AiNodeSetFlt(node, "frame", iTimes[0]);
    }
    else
    {
        AiNodeDeclare(node, "frame", "constant ARRAY FLOAT");
        AiNodeSetArray(
            node,
            "frame",
            AiArrayConvert(1, iTimes.size(), AI_TYPE_FLOAT, iTimes.data()));
    }

    if (!iXform.empty())
    {
        AiNodeSetArray(
            node,
            "matrix",
            AiArrayConvert(
                1, iXform.size() / 16, AI_TYPE_MATRIX, iXform.data()));
    }

    // Set Arnold motion_start and motion_end.
    setMotionStartEnd(node, *iData);

#if AI_VERSION_ARCH_NUM == 4
    const GfVec3d& min = iRange.GetMin();
    const GfVec3d& max = iRange.GetMax();

    RdoAiNodeSetVec(node, "min", min[0], min[1], min[2]);
    RdoAiNodeSetVec(node, "max", max[0], max[1], max[2]);

    TF_DEBUG(WALTER_ARNOLD_PLUGIN)
        .Msg(
            "[%s]: Output bounding box: %s min: %f %f %f max: %f %f %f\n",
            __FUNCTION__,
            iName.c_str(),
            min[0],
            min[1],
            min[2],
            max[0],
            max[1],
            max[2]);
#endif // ARNOLD 4

// USD file name.
#if AI_VERSION_ARCH_NUM == 4
    AiNodeDeclare(node, "filePaths", "constant STRING");
#endif
    AiNodeSetStr(node, "filePaths", iData->filePaths.c_str());

// Save SdfPath.
#if AI_VERSION_ARCH_NUM == 4
    AiNodeDeclare(node, "objectPath", "constant STRING");
#endif

    AiNodeSetStr(node, "objectPath", iObjectPath.GetText());

    AiNodeDeclare(node, "prefix", "constant STRING");

    AiNodeSetStr(node, "prefix", iPrefix.c_str());

    return node;
}

// Reverse an attribute of the face. Basically, it converts from the clockwise
// to the counterclockwise and back.
template <class T, class V> void reverseFaceAttribute(T& attr, const V& counts)
{
    size_t counter = 0;

    // TODO: if it's slow, it can be faster with SIMD.
    for (auto npoints : counts)
    {
        for (size_t j = 0; j < npoints / 2; j++)
        {
            size_t from = counter + j;
            size_t to = counter + npoints - 1 - j;
            std::swap(attr[from], attr[to]);
        }

        counter += npoints;
    }
}

// Function template specialization for all the AiNodeSet*
// TODO: AiNodeSetVec, AiNodeSetPnt
template <class A>
void aiNodeSet(AtNode* node, std::string name, const A& value);

template <>
void aiNodeSet<unsigned int>(
    AtNode* node,
    std::string name,
    const unsigned int& value)
{
    if (!getArnoldParameter(node, name.c_str()))
    {
        AiNodeDeclare(node, name.c_str(), "constant UINT");
    }

    AiNodeSetUInt(node, name.c_str(), value);
}

template <>
void aiNodeSet<int>(AtNode* node, std::string name, const int& value)
{
    if (!getArnoldParameter(node, name.c_str()))
    {
        AiNodeDeclare(node, name.c_str(), "constant INT");
    }

    AiNodeSetInt(node, name.c_str(), value);
}

template <>
void aiNodeSet<bool>(AtNode* node, std::string name, const bool& value)
{
    if (!getArnoldParameter(node, name.c_str()))
    {
        AiNodeDeclare(node, name.c_str(), "constant INT");
    }

    AiNodeSetBool(node, name.c_str(), value);
}

template <>
void aiNodeSet<float>(AtNode* node, std::string name, const float& value)
{
    if (!getArnoldParameter(node, name.c_str()))
    {
        AiNodeDeclare(node, name.c_str(), "constant FLOAT");
    }

    AiNodeSetFlt(node, name.c_str(), value);
}

template <>
void aiNodeSet<std::string>(
    AtNode* node,
    std::string name,
    const std::string& value)
{
    if (!getArnoldParameter(node, name.c_str()))
    {
        AiNodeDeclare(node, name.c_str(), "constant STRING");
    }

    AiNodeSetStr(node, name.c_str(), value.c_str());
}

template <>
void aiNodeSet<TfToken>(AtNode* node, std::string name, const TfToken& value)
{
    if (!getArnoldParameter(node, name.c_str()))
    {
        AiNodeDeclare(node, name.c_str(), "constant STRING");
    }

    AiNodeSetStr(node, name.c_str(), value.GetText());
}

template <>
void aiNodeSet<GfVec3f>(AtNode* node, std::string name, const GfVec3f& value)
{
    const AtParamEntry* entry = getArnoldParameter(node, name.c_str());
    if (!entry)
    {
        AiNodeDeclare(node, name.c_str(), "constant RGB");
    }
    else if (AiParamGetType(entry) == AI_TYPE_VECTOR)
    {
        // This parameter exists and the type is Vector. We can't save it as
        // RGB.
        AiNodeSetVec(node, name.c_str(), value[0], value[1], value[2]);
        return;
    }

    AiNodeSetRGB(node, name.c_str(), value[0], value[1], value[2]);
}

template <>
void aiNodeSet<GfVec4f>(AtNode* node, std::string name, const GfVec4f& value)
{
    const AtParamEntry* entry = getArnoldParameter(node, name.c_str());
    if (!entry)
    {
        AiNodeDeclare(node, name.c_str(), "constant RGBA");
    }

    AiNodeSetRGBA(node, name.c_str(), value[0], value[1], value[2], value[3]);
}

template <>
void aiNodeSet<GfVec2f>(AtNode* node, std::string name, const GfVec2f& value)
{
    if (!getArnoldParameter(node, name.c_str()))
    {
        AiNodeDeclare(node, name.c_str(), "constant " RDO_STR_TYPE_VECTOR2);
    }

    RdoAiNodeSetVec2(node, name.c_str(), value[0], value[1]);
}

template <>
void aiNodeSet<uint8_t>(AtNode* node, std::string name, const uint8_t& value)
{
    if (!getArnoldParameter(node, name.c_str()))
    {
        AiNodeDeclare(node, name.c_str(), "constant BYTE");
    }

    AiNodeSetByte(node, name.c_str(), value);
}

template <>
void aiNodeSet<GfMatrix4d>(AtNode* node, std::string name, const GfMatrix4d& value)
{
    if (!getArnoldParameter(node, name.c_str()))
    {
        AiNodeDeclare(node, name.c_str(), "constant MATRIX");
    }
    // Usd GfMatrix4d is using doubles. We need to cast it for Arnold who is using floats
    const double* data = value.GetArray();
    AtMatrix mat;
    mat.data[0][0] = static_cast<float>(data[0]);
    mat.data[0][1] = static_cast<float>(data[1]);
    mat.data[0][2] = static_cast<float>(data[2]);
    mat.data[0][3] = static_cast<float>(data[3]);
    mat.data[1][0] = static_cast<float>(data[4]);
    mat.data[1][1] = static_cast<float>(data[5]);
    mat.data[1][2] = static_cast<float>(data[6]);
    mat.data[1][3] = static_cast<float>(data[7]);
    mat.data[2][0] = static_cast<float>(data[8]);
    mat.data[2][1] = static_cast<float>(data[9]);
    mat.data[2][2] = static_cast<float>(data[10]);
    mat.data[2][3] = static_cast<float>(data[11]);
    mat.data[3][0] = static_cast<float>(data[12]);
    mat.data[3][1] = static_cast<float>(data[13]);
    mat.data[3][2] = static_cast<float>(data[14]);
    mat.data[3][3] = static_cast<float>(data[15]);
    AiNodeSetMatrix(node, name.c_str(), mat);
}

// Return the funstion that pushes USD attribute to Arnold.
template <class U, class A = U>
RendererAttribute attributeToArnold(
    const UsdAttribute& attr,
    const std::string& name,
    UsdTimeCode time)
{
    // Todo: samples.
    U value;

    if (!attr.Get(&value, time))
    {
        // We are here because this parameter is empty. It happens when USD has
        // a connection to this parameter. In this way the USD Prim has both
        // the parameter (to specify its type) and the relationship.
        return RendererAttribute::ArnoldParameterPtr(nullptr);
    }

    return std::make_shared<RendererAttribute::ArnoldParameter>(
        std::bind(aiNodeSet<A>, std::placeholders::_1, name, value));
}

RendererAttribute usdAttributeToArnold(
    const std::string& attributeName,
    const UsdAttribute& attr,
    UsdTimeCode time)
{
    SdfValueTypeName type;

    // Sometimes we have to force the type.
    if (attributeName == "subdiv_iterations")
    {
        type = SdfValueTypeNames->UChar;
    }
    else
    {
        type = attr.GetTypeName();
    }

    // Output the USD attribute depending on the type.
    if (type == SdfValueTypeNames->Bool)
    {
        // Special case: Visibilities. Arnold doesn't have a separate attribute
        // for different types of visibility. Instead, it has a single bitwise
        // visibility attribute. It means we need to save all the flags and
        // combine them.
        uint8_t ray = AI_RAY_UNDEFINED;
        if (attributeName == "casts_shadows")
        {
            ray = AI_RAY_SHADOW;
        }
        else if (attributeName == "primary_visibility")
        {
            ray = AI_RAY_CAMERA;
        }
        else if (attributeName == "visibility")
        {
            ray = AI_RAY_ALL;
        }
        else if (attributeName == "visible_in_diffuse")
        {
            ray = RDO_AI_RAY_DIFFUSE;
        }
        else if (attributeName == "visible_in_glossy")
        {
            ray = RDO_AI_RAY_GLOSSY;
        }
        else if (attributeName == "visible_in_reflections")
        {
            ray = RDO_AI_RAY_REFLECTED;
        }
        else if (attributeName == "visible_in_refractions")
        {
            ray = RDO_AI_RAY_REFRACTED;
        }
#if AI_VERSION_ARCH_NUM != 4
        else if (attributeName == "visible_in_diffuse_reflection")
        {
            ray = AI_RAY_DIFFUSE_REFLECT;
        }
        else if (attributeName == "visible_in_specular_reflection")
        {
            ray = AI_RAY_SPECULAR_REFLECT;
        }
        else if (attributeName == "visible_in_diffuse_transmission")
        {
            ray = AI_RAY_DIFFUSE_TRANSMIT;
        }
        else if (attributeName == "visible_in_specular_transmission")
        {
            ray = AI_RAY_SPECULAR_TRANSMIT;
        }
        else if (attributeName == "visible_in_volume")
        {
            ray = AI_RAY_VOLUME;
        }
#endif

        bool visibilityFlag;
        if (ray && attr.Get(&visibilityFlag, time) && !visibilityFlag)
        {
            // We save the visibilities inverted to be able to combine them with
            // AND operator in outputAttributes().
            return ~ray;
        }

        return attributeToArnold<bool>(attr, attributeName, time);
    }
    else if (type == SdfValueTypeNames->Int)
    {
        return attributeToArnold<int>(attr, attributeName, time);
    }
    else if (type == SdfValueTypeNames->UInt)
    {
        return attributeToArnold<unsigned int>(attr, attributeName, time);
    }
    else if (type == SdfValueTypeNames->Float)
    {
        return attributeToArnold<float>(attr, attributeName, time);
    }
    else if (type == SdfValueTypeNames->Double)
    {
        return attributeToArnold<double, float>(attr, attributeName, time);
    }
    else if (type == SdfValueTypeNames->String)
    {
        return attributeToArnold<std::string>(attr, attributeName, time);
    }
    else if (type == SdfValueTypeNames->Token)
    {
        return attributeToArnold<TfToken>(attr, attributeName, time);
    }
    else if (type == SdfValueTypeNames->Color3f)
    {
        return attributeToArnold<GfVec3f>(attr, attributeName, time);
    }
    else if (type == SdfValueTypeNames->Vector3f)
    {
        return attributeToArnold<GfVec3f>(attr, attributeName, time);
    }
    else if (type == SdfValueTypeNames->Point3f)
    {
        return attributeToArnold<GfVec3f>(attr, attributeName, time);
    }
    else if (type == SdfValueTypeNames->Float2)
    {
        return attributeToArnold<GfVec2f>(attr, attributeName, time);
    }
    else if (type == SdfValueTypeNames->Float3)
    {
        return attributeToArnold<GfVec3f>(attr, attributeName, time);
    }
    else if (type == SdfValueTypeNames->Float4)
    {
        return attributeToArnold<GfVec4f>(attr, attributeName, time);
    }
    else if (type == SdfValueTypeNames->UChar)
    {
        return attributeToArnold<int, uint8_t>(attr, attributeName, time);
    }
    else if (type == SdfValueTypeNames->Matrix4d)
    {
        return attributeToArnold<GfMatrix4d>(attr, attributeName, time);
    }

    return RendererAttribute(nullptr);
}

// Push USD array attribute to Arnold.
// It extracts data from USD attribute, converts the USD data type to the type
// understandable by Arnold, and sets Arnold attribute.
// numVertsArray: a pointer to the face counts to reverce the attribute. If
// NULL, the attribute will not be reverced.
// TODO: Check A and U are the same and don't convert if it's so.
template <class A, class U>
size_t attributeArrayToArnold(
    UsdAttribute attr,
    AtNode* node,
    const char* name,
    uint8_t type,
    const std::vector<float>& times,
    const VtIntArray* numVertsArray)
{
    // Number of motion keys.
    size_t keys = times.size();
    // The number of data items.
    size_t size = 0;

    std::vector<A> vect;
    bool reserved = false;

    for (float time : times)
    {
        VtArray<U> array;
        attr.Get(&array, time);

        if (numVertsArray)
        {
            // Arnold doesn't have an attribute to specify the orientation of
            // data. If the data is in wrong order, we need to reorder it now.
            reverseFaceAttribute(array, *numVertsArray);
        }

        if (!reserved)
        {
            // Reserve the vector.
            size = array.size();
            vect.reserve(size * keys);
            reserved = true;
        }

        vect.insert(vect.end(), array.begin(), array.end());
    }

    if (!vect.empty())
    {
        // Output everything to Arnold only if we have data to output.
        AiNodeSetArray(
            node, name, AiArrayConvert(size, keys, type, vect.data()));
    }

    return size;
}

template <class T>
bool vtToArnold(
    const VtValue& vtValue,
    const VtIntArray& vtIndices,
    const TfToken& name,
    const SdfValueTypeName& typeName,
    const TfToken& interpolation,
    AtNode* node,
    const VtIntArray* numVertsArray,
    const int pointInstanceId)
{
    if (!vtValue.IsHolding<VtArray<T>>())
    {
        return false;
    }
 
    TfToken arnoldName = name;
    bool needDeclare = true;

    // Convert interpolation -> scope
    //
    // USD Interpolation determines how the Primvar interpolates over a
    // geometric primitive:
    // constant One value remains constant over the entire surface
    //          primitive.
    // uniform One value remains constant for each uv patch segment of the
    //         surface primitive (which is a face for meshes).
    // varying Four values are interpolated over each uv patch segment of
    //         the surface. Bilinear interpolation is used for interpolation
    //         between the four values.
    // vertex Values are interpolated between each vertex in the surface
    //        primitive. The basis function of the surface is used for
    //        interpolation between vertices.
    // faceVarying For polygons and subdivision surfaces, four values are
    //             interpolated over each face of the mesh. Bilinear
    //             interpolation is used for interpolation between the four
    //             values.
    //
    // There are four kinds of user-defined data in Arnold:
    //
    // constant constant parameters are data that are defined on a
    //          per-object basis and do not vary across the surface of that
    //          object.
    // uniform uniform parameters are data that are defined on a "per-face"
    //         basis. During subdivision (if appropriate) values are not
    //         interpolated.  Instead, the newly subdivided faces will
    //         contain the value of their "parent" face.
    // varying varying parameters are data that are defined on a per-vertex
    //         basis. During subdivision (if appropriate), the values at the
    //         new vertices are interpolated from the values at the old
    //         vertices. The user should only create parameters of
    //         "interpolatable" variable types (such as floats, colors,
    //         etc.)
    // indexed indexed parameters are data that are defined on a
    //         per-face-vertex basis. During subdivision (if appropriate),
    //         the values at the new vertices are interpolated from the
    //         values at the old vertices, preserving edges where values
    //         were not shared. The user should only create parameters of
    //         "interpolatable" variable types (such as floats, colors,
    //         etc.)
    std::string declaration = (interpolation == UsdGeomTokens->uniform) ?
        "uniform " :
        (interpolation == UsdGeomTokens->varying) ?
        "varying " :
        (interpolation == UsdGeomTokens->vertex) ?
        "varying " :
        (interpolation == UsdGeomTokens->faceVarying) ? "indexed " :
                                                        "constant ";

    int arnoldAPIType;

    if (std::is_same<T, GfVec2f>::value)
    {
        declaration += RDO_STR_TYPE_VECTOR2;
        arnoldAPIType = RDO_AI_TYPE_VECTOR2;

        // A special case for UVs.
        if (name == "uv")
        {
            arnoldName = TfToken("uvlist");
            needDeclare = false;
        }
    }
    else if (std::is_same<T, GfVec3f>::value)
    {
        TfToken role = typeName.GetRole();
        if (role == SdfValueRoleNames->Color) {
            declaration += "RGB";
            arnoldAPIType = AI_TYPE_RGB;
        }
        else {
            declaration += RDO_STR_TYPE_VECTOR;
            arnoldAPIType = RDO_AI_TYPE_VECTOR;
        }

#if AI_VERSION_ARCH_NUM == 4
        // AI_TYPE_VECTOR is distinct to AI_TYPE_POINT only in Arnold 4.
        // So in Arnold 5, this block is redundant with the one above.
        else
        {
            declaration += "VECTOR";
            arnoldAPIType = AI_TYPE_VECTOR;
        }
#endif  // ARNOLD 4
    }
    else if (std::is_same<T, float>::value)
    {
        declaration += "FLOAT";
        arnoldAPIType = AI_TYPE_FLOAT;
    }
    else if (std::is_same<T, int>::value)
    {
        declaration += "INT";
        arnoldAPIType = AI_TYPE_INT;
    }
    else
    {
        // Not supported.
        return false;
    }

    // Declare a user-defined parameter.
    if (needDeclare)
    {
        AiNodeDeclare(node, arnoldName.GetText(), declaration.c_str());
    }

    // Constant USD attributs are provided as an array of one element.
    if(interpolation == UsdGeomTokens->constant) 
    {
        if (std::is_same<T, GfVec3f>::value)
        {
            VtArray<GfVec3f> vecArray = vtValue.Get<VtArray<GfVec3f>>();
            GfVec3f value;
            if (pointInstanceId == -1)
            {
                value = vecArray[0];
            }
            else
            {
                value = vecArray[pointInstanceId];
            }

            TfToken role = typeName.GetRole();
            if (role == SdfValueRoleNames->Color)
            {
                AiNodeSetRGB(
                    node, arnoldName.GetText(), value[0], value[1], value[2]);
            }
            else
            {
                AiNodeSetVec(
                    node, arnoldName.GetText(), value[0], value[1], value[2]);
            }
        }

        else if(std::is_same<T, GfVec2f>::value) 
        {
            auto vector = vtValue.Get<VtArray<GfVec2f>>()[0];
            AiNodeSetVec2(node, arnoldName.GetText(), vector[0], vector[1]);
        }

        else if(std::is_same<T, float>::value) 
        {
            AiNodeSetFlt(node, arnoldName.GetText(), vtValue.Get<VtArray<float>>()[0]);
        }

        else if(std::is_same<T, int>::value) 
        {
            AiNodeSetInt(node, arnoldName.GetText(), vtValue.Get<VtArray<int>>()[0]);
        }
    }

    else 
    {
        const VtArray<T>& rawVal = vtValue.Get<VtArray<T>>();
        AiNodeSetArray(
            node,
            arnoldName.GetText(),
            AiArrayConvert(rawVal.size(), 1, arnoldAPIType, rawVal.data()));

        if (interpolation == UsdGeomTokens->faceVarying)
        {
            const std::string indexName = name.GetString() + "idxs";
            std::vector<unsigned int> indexes;

            if (vtIndices.empty())
            {
                // Arnold doesn't have facevarying iterpolation. It has indexed
                // instead. So it means it's necessary to generate indexes for this
                // type.
                // TODO: Try to generate indexes only once and use it for several
                // primvars.

                indexes.resize(rawVal.size());
                // Fill it with 0, 1, ..., 99.
                std::iota(std::begin(indexes), std::end(indexes), 0);
            }
            else
            {
                // We need to use indexes and we can't use vtIndices because we need
                // unsigned int. Converting int to unsigned int.
                indexes.resize(vtIndices.size());
                std::copy(vtIndices.begin(), vtIndices.end(), indexes.begin());
            }

            // Reverse indexes.
            if (numVertsArray)
            {
                reverseFaceAttribute(indexes, *numVertsArray);
            }

            AiNodeSetArray(
                node,
                indexName.c_str(),
                AiArrayConvert(indexes.size(), 1, AI_TYPE_UINT, indexes.data()));
        }
    }
    return true;
}

/**
 * @brief Compute transform of the given prim.
 *
 * @param iPrim The given prim.
 * @param times The given times.
 *
 * @return Vector of the 4x4 matrices that represent the transforms.
 */
std::vector<float> getPrimTransform(
    const UsdPrim& iPrim,
    const std::vector<float>& times)
{
    std::vector<float> xform;

    // A problem here is that it's not possible to use this to get the transform
    // of non-imageable object:
    // `UsdGeomXformCache(time).GetLocalToWorldTransform(prim)`
    // It crashes for unknown reason. So if the transform is not available on
    // the current prim, we need to manually find transform of the parents.
    UsdPrim prim = iPrim;
    while (!prim.IsA<UsdGeomImageable>())
    {
        prim = prim.GetParent();

        if (prim.GetPath() == SdfPath::AbsoluteRootPath())
        {
            return xform;
        }
    }

    // Output XForm
    UsdGeomImageable imageable(prim);
    size_t keys = times.size();

    xform.reserve(16 * keys);

    for (float time : times)
    {
        GfMatrix4d matrix = imageable.ComputeLocalToWorldTransform(time);

        const double* matrixArray = matrix.GetArray();
        xform.insert(xform.end(), matrixArray, matrixArray + 16);
    }

    return xform;
}

void RendererAttribute::evaluate(AtNode* node) const
{
    if (mFunction)
    {
        (*mFunction)(node);
    }
}

bool RendererAttribute::valid() const
{
    return mFunction != nullptr || mVisibility;
}

bool RendererAttribute::isVisibility() const
{
    return mVisibility;
}

uint8_t RendererAttribute::visibilityFlag() const
{
    return mVisibilityFlag;
}

RendererPlugin::RendererPlugin()
{
#if 0
    TfDebug::Enable(WALTER_ARNOLD_PLUGIN);
#endif
}

bool RendererPlugin::isSupported(const UsdPrim& prim) const
{
    if (!prim)
    {
        return false;
    }
    return prim.IsInstance() || prim.IsA<UsdGeomMesh>() ||
        prim.IsA<UsdGeomCurves>() || prim.IsA<UsdGeomPointInstancer>();
}

bool RendererPlugin::isImmediate(const UsdPrim& prim) const
{
    if (!prim)
    {
        return false;
    }

    // Shaders should be output immediatly.
    return prim.IsA<UsdShadeShader>();
}

// This function is not compatible Arnold 4.
void* RendererPlugin::output(
    const UsdPrim& prim,
    const std::vector<float>& times,
    const void* userData
) const
{
    const RendererPluginData* data =
        reinterpret_cast<const RendererPluginData*>(userData);
    assert(data);

    SdfPath objectPath = prim.GetPath();
    std::string prefix = data->prefix;
    std::string name = prefix + ":" + objectPath.GetText() + ":proc";
    std::vector<float> xform;

    return createWalterProcedural(
        data,
        name,
        times,
        xform,
        objectPath,
        prefix
    );
}

// TODO:
// AiNodeSetBool(node, "inherit_xform", false);
// AiNodeSetBool(node, "invert_normals", true);
// AiNodeSetByte(node, "sidedness", AI_RAY_ALL);
void* RendererPlugin::outputBBox(
    const UsdPrim& prim,
    const std::vector<float>& times,
    const void* userData,
    RendererIndex& index) const
{
    // Get the user data
    const RendererPluginData* data =
        reinterpret_cast<const RendererPluginData*>(userData);
    assert(data);

    const SdfPath path = prim.GetPath();

    // Form the name.
    std::string name = data->prefix + ":" + path.GetText() + ":proc";

    // Output XForm
    std::vector<float> xform = getPrimTransform(prim, times);

#if AI_VERSION_ARCH_NUM == 4
    UsdGeomImageable imageable(prim);

    // Add the bounding box.
    GfBBox3d bbox;
    // We need to combine all the bounding boxes of the all the motion samples.
    bool bboxInitialized = false;
    for (float time : times)
    {
        GfBBox3d current = imageable.ComputeUntransformedBound(
            time, UsdGeomTokens->default_, UsdGeomTokens->render);

        if (bboxInitialized)
        {
            bbox = GfBBox3d::Combine(bbox, current);
        }
        else
        {
            bbox = current;
            bboxInitialized = true;
        }
    }

    // Push the bbox to Arnold.
    const GfRange3d& range = bbox.ComputeAlignedBox();
    const GfVec3d& min = range.GetMin();
    const GfVec3d& max = range.GetMax();

#endif  // ARNOLD 4

    SdfPath objectPath;
    std::string prefix;
    if (prim.IsInstance())
    {
        objectPath = prim.GetMaster().GetPath();
        prefix = data->prefix + "_" + uniqueString();

        // Save the full path that is good for expression resolving.
        index.setPrefixPath(
            prefix, joinPaths(index.getPrefixPath(data->prefix), path));
    }
    else
    {
        objectPath = prim.GetPath();
        prefix = data->prefix;
    }

    return createWalterProcedural(
        data,
        name,
        times,
        xform,
#if AI_VERSION_ARCH_NUM == 4
        range,
#endif // ARNOLD 4
        objectPath,
        prefix);
}

void* RendererPlugin::outputBBoxFromPoint(
    const UsdPrim& prim,
    const int id,
    const std::vector<float>& times,
    const void* userData,
    RendererIndex& index) const
{
    // Get the user data
    const RendererPluginData* data =
        reinterpret_cast<const RendererPluginData*>(userData);
    assert(data);

    UsdGeomPointInstancer instancer(prim);

    float averageTime =
        std::accumulate(times.begin(), times.end(), 0.0f) / times.size();

    // Get prototype prim path (to set objectPath)
    // [NOTE]: Not sure we should take the average time as the reference
    // for point cloud topology...
    VtIntArray protoIndices;
    if (!instancer.GetProtoIndicesAttr().Get(&protoIndices, averageTime))
    {
        return nullptr;
    }

    SdfPathVector protoPaths;
    instancer.GetPrototypesRel().GetTargets(&protoPaths);
    const SdfPath& protoPath = protoPaths[protoIndices[id]];

    // Form the name.
    std::string name = data->prefix + ":" + protoPath.GetText() + ":proc_" +
        std::to_string(id);

    // Compute Xform from point
    std::vector<float> xform;
    xform.reserve(16 * times.size());

    const UsdAttribute positionsAttr = instancer.GetPositionsAttr();
    const UsdAttribute scalesAttr = instancer.GetScalesAttr();
    const UsdAttribute orientationsAttr = instancer.GetOrientationsAttr();

    for (const float time : times)
    {
        VtVec3fArray positions, scales;
        VtQuathArray orientations;

        scalesAttr.Get(&scales, static_cast<double>(time));
        orientationsAttr.Get(&orientations, static_cast<double>(time));
        positionsAttr.Get(&positions, static_cast<double>(time));

        GfTransform xfo;
        xfo.SetScale(scales[id]);
        xfo.SetRotation(GfRotation(orientations[id]));
        xfo.SetTranslation(positions[id]);

        const double* matrixArray = xfo.GetMatrix().GetArray();
        xform.insert(xform.end(), matrixArray, matrixArray + 16);
    }

#if AI_VERSION_ARCH_NUM == 4

    UsdStageRefPtr stage = prim.GetStage();
    const UsdPrim& protoPrim = stage->GetPrimAtPath(protoPath);
    UsdGeomImageable imageable(protoPrim);

    // Add the bounding box.
    GfBBox3d bbox;
    // We need to combine all the bounding boxes of the all the motion samples.
    bool bboxInitialized = false;
    for (float time : times)
    {
        GfBBox3d current = imageable.ComputeUntransformedBound(
            time, UsdGeomTokens->default_, UsdGeomTokens->render);

        if (bboxInitialized)
        {
            bbox = GfBBox3d::Combine(bbox, current);
        }
        else
        {
            bbox = current;
            bboxInitialized = true;
        }
    }

    // Push the bbox to Arnold.
    const GfRange3d& range = bbox.ComputeAlignedBox();
    const GfVec3d& min = range.GetMin();
    const GfVec3d& max = range.GetMax();
#endif // ARNOLD 4

    AtNode* node = createWalterProcedural(
        data,
        name,
        times,
        xform,
#if AI_VERSION_ARCH_NUM == 4
        range,
#endif // ARNOLD 4
        protoPath,
        data->prefix);
    outputPrimvars(prim, averageTime, node, nullptr, id);
    return node;
}

void* RendererPlugin::outputReference(
    const UsdPrim& prim,
    const std::vector<float>& times,
    const void* userData,
    RendererIndex& index) const
{
    assert(prim);

    std::string name = prim.GetStage()->GetSessionLayer()->GetIdentifier() +
        ":" + prim.GetPath().GetText();

    AtNode* node = nullptr;

    if (prim.IsA<UsdGeomMesh>())
    {
        node = outputGeomMesh(prim, times, name.c_str(), userData);
    }
    else if (prim.IsA<UsdGeomCurves>())
    {
        node = outputGeomCurves(prim, times, name.c_str(), userData);
    }
    else if (prim.IsA<UsdShadeShader>())
    {
        node = outputShader(prim, times, name.c_str());
    }

    if (node)
    {
        // TODO: get rid of this. We don't support it.
        static const std::string layer = "defaultRenderLayer";

        SdfPath path = prim.GetPath();

        // Walter overrides.
        const NameToAttribute* attributes =
            getAssignedAttributes(path, layer, userData, index, nullptr);

        outputAttributes(node, path, attributes, index, layer, userData, true);
    }

    // Return node.
    return node;
}

void RendererPlugin::establishConnections(
    const UsdPrim& prim,
    void* data,
    RendererIndex& index) const
{
    AtNode* node = reinterpret_cast<AtNode*>(data);

    if (!prim.IsA<UsdShadeShader>())
    {
        return;
    }

    UsdShadeShader shader(prim);

    for (const UsdShadeInput& input : shader.GetInputs())
    {
        UsdShadeConnectableAPI connectableAPISource;
        TfToken sourceName;
        UsdShadeAttributeType sourceType;
        if (!UsdShadeConnectableAPI::GetConnectedSource(
                input, &connectableAPISource, &sourceName, &sourceType))
        {
            continue;
        }

        std::string inputName = input.GetBaseName().GetString();
        // Convert USD namespaces to Arnold components. We need it because we
        // did conversion from Arnold components to USD namespaces in Alembic to
        // USD plugin.
        std::replace(inputName.begin(), inputName.end(), ':', '.');

        RendererIndex::RenderNodeMap::const_accessor accessor;
        if (!index.getRenderNodeData(
                connectableAPISource.GetPrim().GetPath(), accessor))
        {
            continue;
        }

        AtNode* source = reinterpret_cast<AtNode*>(accessor->second);

        if (sourceName.IsEmpty() || sourceName == "out")
        {
            AiNodeLink(source, inputName.c_str(), node);
        }
        else
        {
            // The difference between this and AiNodeLink is that a specific
            // component can be selected for both the source output and the
            // target input, so that the connection would only affect that
            // component (the other components can be linked independently).
            AiNodeLinkOutput(
                source, sourceName.GetText(), node, inputName.c_str());
        }
    }
}

void* RendererPlugin::render(
    const UsdPrim& prim,
    const std::vector<float>& times,
    void* renderData,
    const void* userData,
    RendererIndex& index) const
{
    size_t keys = times.size();
    std::vector<float> averageTime(
        1, std::accumulate(times.begin(), times.end(), 0.0f) / keys);

    // Get the user data
    const RendererPluginData* data =
        reinterpret_cast<const RendererPluginData*>(userData);
    assert(data);

    SdfPath primPath = prim.GetPath();
    std::string name = data->prefix + ":" + primPath.GetText();

    // TODO: get rid of this. We don't support it.
    static const std::string layer = "defaultRenderLayer";

    // We need to know if the object is visible before we create the node.
    uint8_t visibility;
    const NameToAttribute* attributes =
        getAssignedAttributes(primPath, layer, userData, index, &visibility);

    // We should also consider standard USD visibility flag.
    UsdGeomImageable imageable = UsdGeomImageable(prim);
    bool invisibleFromUSD = imageable &&
        imageable.ComputeVisibility(averageTime[0]) == UsdGeomTokens->invisible;

    bool canRender = imageable
        ? imageable.ComputePurpose() != UsdGeomTokens->proxy && imageable.ComputePurpose() != UsdGeomTokens->guide
        : true;

    if (visibility == AI_RAY_UNDEFINED || invisibleFromUSD || !canRender)
    {
        // The object is not visible. Skip it.
        TF_DEBUG(WALTER_ARNOLD_PLUGIN)
            .Msg(
                "[%s]: Skipping %s because it's not visible\n",
                __FUNCTION__,
                primPath.GetText());
        return outputEmptyNode(name);
    }

    AtNode* node = AiNode("ginstance");

    AiNodeSetStr(node, "name", name.c_str());

    AiNodeSetPtr(node, "node", renderData);

    // Make it visibile.
    AiNodeSetByte(node, "visibility", visibility);

    // All the attributes.
    outputAttributes(node, primPath, attributes, index, layer, userData, false);

    return node;
}

RendererAttribute RendererPlugin::createRendererAttribute(
    const UsdAttribute& attr,
    UsdTimeCode time) const
{
    return usdAttributeToArnold(attr.GetBaseName(), attr, time);
}

AtNode* RendererPlugin::outputGeomMesh(
    const UsdPrim& prim,
    const std::vector<float>& times,
    const char* name,
    const void* userData) const
{
    TF_DEBUG(WALTER_ARNOLD_PLUGIN)
        .Msg("[%s]: Render: %s\n", __FUNCTION__, name);

    size_t keys = times.size();
    std::vector<float> averageTime(
        1, std::accumulate(times.begin(), times.end(), 0.0f) / keys);

    // Create a polymesh.
    AtNode* node = AiNode("polymesh");

    AiNodeSetStr(node, "name", name);
    AiNodeSetBool(node, "smoothing", true);
    AiNodeSetByte(node, "visibility", 0);

    const RendererPluginData* data =
        reinterpret_cast<const RendererPluginData*>(userData);
    assert(data);

    setMotionStartEnd(node, *data);

    // Get mesh.
    UsdGeomMesh mesh(prim);

    // Get orientation.
    bool reverse = false;
    {
        TfToken orientation;
        if (mesh.GetOrientationAttr().Get(&orientation))
        {
            reverse = (orientation == UsdGeomTokens->leftHanded);
        }
    }

    // Eval vertex counts of faces.
    VtIntArray numVertsArray;
    mesh.GetFaceVertexCountsAttr().Get(&numVertsArray, averageTime[0]);
    // Type conversion
    {
        std::vector<unsigned char> numVertsVec(
            numVertsArray.begin(), numVertsArray.end());
        AiNodeSetArray(
            node,
            "nsides",
            AiArrayConvert(
                numVertsVec.size(), 1, AI_TYPE_BYTE, &(numVertsVec[0])));
    }

    // TODO: output motion samples only if they exist.
    // Vertex indices
    attributeArrayToArnold<unsigned int, int>(
        mesh.GetFaceVertexIndicesAttr(),
        node,
        "vidxs",
        AI_TYPE_UINT,
        averageTime,
        reverse ? &numVertsArray : nullptr);

    // Eval points.
    attributeArrayToArnold<GfVec3f, GfVec3f>(
        mesh.GetPointsAttr(),
        node,
        "vlist",
        RDO_AI_TYPE_VECTOR,
        times,
        nullptr);

    // Output normals.
    size_t numNormals = attributeArrayToArnold<GfVec3f, GfVec3f>(
        mesh.GetNormalsAttr(), node, "nlist", AI_TYPE_VECTOR, times, nullptr);

    if (numNormals)
    {
        // Generate a range with sequentially increasing values and use them as
        // normal indexes.
        std::vector<unsigned int> normIdxArray(numNormals);
        std::iota(normIdxArray.begin(), normIdxArray.end(), 0);
        if (reverse)
        {
            reverseFaceAttribute(normIdxArray, numVertsArray);
        }
        AiNodeSetArray(
            node,
            "nidxs",
            AiArrayConvert(
                normIdxArray.size(), 1, AI_TYPE_UINT, normIdxArray.data()));
    }

    outputPrimvars(
        prim, averageTime[0], node, reverse ? &numVertsArray : nullptr);

    // Return node.
    return node;
}

AtNode* RendererPlugin::outputGeomCurves(
    const UsdPrim& prim,
    const std::vector<float>& times,
    const char* name,
    const void* userData) const
{
    TF_DEBUG(WALTER_ARNOLD_PLUGIN)
        .Msg("[%s]: Render: %s\n", __FUNCTION__, name);

    size_t keys = times.size();
    std::vector<float> averageTime(
        1, std::accumulate(times.begin(), times.end(), 0.0f) / keys);

    // Create curves.
    AtNode* node = AiNode("curves");

    AiNodeSetStr(node, "name", name);
    AiNodeSetByte(node, "visibility", 0);

    const RendererPluginData* data =
        reinterpret_cast<const RendererPluginData*>(userData);
    assert(data);

    setMotionStartEnd(node, *data);

    // Get the basis. From the Arnold docs: Can choose from Bezier, B-Spline,
    // Catmull-Rom, Linear.
    int basis = 3;
    if (prim.IsA<UsdGeomBasisCurves>())
    {
        // TODO: use a scope_pointer for curves and basisCurves.
        UsdGeomBasisCurves basisCurves(prim);
        TfToken curveType;
        basisCurves.GetTypeAttr().Get(&curveType, averageTime[0]);
        if (curveType == UsdGeomTokens->cubic)
        {
            TfToken basisType;
            basisCurves.GetBasisAttr().Get(&basisType, averageTime[0]);

            if (basisType == UsdGeomTokens->bezier)
            {
                basis = 0;
            }
            else if (basisType == UsdGeomTokens->bspline)
            {
                basis = 1;
            }
            else if (basisType == UsdGeomTokens->catmullRom)
            {
                basis = 2;
            }
        }
    }

    AiNodeSetInt(node, "basis", basis);

    // Get curves.
    UsdGeomCurves curves(prim);

    // Eval vertex counts of faces.
    attributeArrayToArnold<int, int>(
        curves.GetCurveVertexCountsAttr(),
        node,
        "num_points",
        AI_TYPE_UINT,
        times,
        nullptr);

    // TODO: output motion samples only if they exist.
    // Eval points.
    attributeArrayToArnold<GfVec3f, GfVec3f>(
        curves.GetPointsAttr(),
        node,
        "points",
        RDO_AI_TYPE_VECTOR,
        times,
        nullptr);

    // Eval widths.
    attributeArrayToArnold<float, float>(
        curves.GetWidthsAttr(), node, "radius", AI_TYPE_FLOAT, times, nullptr);

    outputPrimvars(prim, averageTime[0], node, nullptr);

    // Return node.
    return node;
}

AtNode* RendererPlugin::outputShader(
    const UsdPrim& prim,
    const std::vector<float>& times,
    const char* name) const
{
    if (!prim.IsA<UsdShadeShader>())
    {
        return nullptr;
    }

    size_t keys = times.size();
    std::vector<float> averageTime(
        1, std::accumulate(times.begin(), times.end(), 0.0f) / keys);

    // Check if it's an Arnold shader.
    UsdAttribute shaderTargetAttr = prim.GetAttribute(TfToken("info:target"));
    TfToken shaderTarget;
    shaderTargetAttr.Get(&shaderTarget, averageTime[0]);
    if (shaderTarget != "arnold")
    {
        return nullptr;
    }

    UsdAttribute shaderTypeAttr = prim.GetAttribute(TfToken("info:type"));
    TfToken shaderType;
    shaderTypeAttr.Get(&shaderType, averageTime[0]);

    // Create shader.
    AtNode* node = AiNode(shaderType.GetText());

    // Set the name
    AiNodeSetStr(node, "name", name);

    // Output XForm
    static const char* matrix = "matrix";
    if (getArnoldParameter(node, matrix))
    {
        std::vector<float> xform = getPrimTransform(prim, times);
        if (!xform.empty())
        {
            // 16 is the size of regular matrix 4x4
            // TODO: output motion samples only if they exist.
            AiNodeSetArray(
                node,
                matrix,
                AiArrayConvert(
                    1, xform.size() / 16, AI_TYPE_MATRIX, xform.data()));
        }
    }

    // Iterate all the paramters.
    for (const UsdAttribute& attr : prim.GetAttributes())
    {
        if (!attr.GetNamespace().IsEmpty())
        {
            continue;
        }

        const TfToken parameterName = attr.GetName();
        if (parameterName == "name")
        {
            // Skip parameter with name "name" because it will rename the node.
            continue;
        }
        RendererAttribute rndAttr = usdAttributeToArnold(
            parameterName.GetString(), attr, averageTime[0]);
        rndAttr.evaluate(node);
    }

    return node;
}

void RendererPlugin::outputPrimvars(
    const UsdPrim& prim,
    float time,
    AtNode* node,
    const VtIntArray* numVertsArray,
    const int pointInstanceId) const
{
    assert(prim);
    UsdGeomImageable imageable = UsdGeomImageable(prim);
    assert(imageable);

    for (const UsdGeomPrimvar& primvar : imageable.GetPrimvars())
    {
        TfToken name;
        SdfValueTypeName typeName;
        TfToken interpolation;
        int elementSize;

        primvar.GetDeclarationInfo(
            &name, &typeName, &interpolation, &elementSize);

        if(pointInstanceId > -1)
        {
            assert(interpolation == UsdGeomTokens->uniform);
            interpolation = UsdGeomTokens->constant;
        }

        // Resolve the value
        VtValue vtValue;
        VtIntArray vtIndices;
        if (interpolation == UsdGeomTokens->constant)
        { 
            if (!primvar.Get(&vtValue, time))
            {
                continue;
            }
        }

        else if (interpolation == UsdGeomTokens->faceVarying && primvar.IsIndexed())
        {
            // It's an indexed value. We don't want to flatten it because it
            // breaks subdivs.
            if (!primvar.Get(&vtValue, time))
            {
                continue;
            }

            if (!primvar.GetIndices(&vtIndices, time))
            {
                continue;
            }
        }
        else
        {
            // USD comments suggest using using the ComputeFlattened() API
            // instead of Get even if they produce the same data.
            if (!primvar.ComputeFlattened(&vtValue, time))
            {
                continue;
            }
        }

        if (vtToArnold<GfVec2f>(
                vtValue,
                vtIndices,
                name,
                typeName,
                interpolation,
                node,
                numVertsArray,
                pointInstanceId))
        { /* Nothing to do */
        }
        else if (vtToArnold<GfVec3f>(
                     vtValue,
                     vtIndices,
                     name,
                     typeName,
                     interpolation,
                     node,
                     numVertsArray,
                     pointInstanceId))
        { /* Nothing to do */          
        }
        else if (vtToArnold<float>(
                     vtValue,
                     vtIndices,
                     name,
                     typeName,
                     interpolation,
                     node,
                     numVertsArray,
                     pointInstanceId))
        { /* Nothing to do */
        }
        else if (vtToArnold<int>(
                     vtValue,
                     vtIndices,
                     name,
                     typeName,
                     interpolation,
                     node,
                     numVertsArray,
                     pointInstanceId))
        { /* Nothing to do */
        }
    }
}

const NameToAttribute* RendererPlugin::getAssignedAttributes(
    const SdfPath& path,
    const std::string& layer,
    const void* userData,
    RendererIndex& index,
    uint8_t* oVisibility) const
{
    const std::string& prefix =
        reinterpret_cast<const RendererPluginData*>(userData)->prefix;

    // Form the full path considering instancing that is good for expression
    // resolving. The first part is saved in the index. The second part is the
    // cuurent USD path.
    std::string virtualObjectName =
        joinPaths(index.getPrefixPath(prefix), path);

    uint8_t visibility = AI_RAY_ALL;

    // Get attributes.
    const NameToAttribute* attributes =
        index.getObjectAttribute(virtualObjectName, layer);

    // Compute the visibility.
    if (attributes && oVisibility)
    {
        for (const auto& attr : *attributes)
        {
            if (attr.second.isVisibility())
            {
                // Combine all the visibilities to the single attribute.
                visibility &= attr.second.visibilityFlag();
            }
        }

        *oVisibility = visibility;
    }

    return attributes;
}

void RendererPlugin::outputAttributes(
    AtNode* node,
    const SdfPath& path,
    const NameToAttribute* attributes,
    RendererIndex& index,
    const std::string& layer,
    const void* userData,
    bool forReference) const
{
    const std::string& prefix =
        reinterpret_cast<const RendererPluginData*>(userData)->prefix;

    // Output attributes to Arnold.
    if (forReference && attributes)
    {
        // TODO: Query parents, get attributes right from the USD Prim.
        // Get attributes.
        if (attributes)
        {
            for (const auto& attr : *attributes)
            {
                attr.second.evaluate(node);
            }
        }
    }

    // Form the full path considering instancing that is good for expression
    // resolving. The first part is saved in the index. The second part is the
    // cuurent USD path.
    std::string virtualObjectName =
        joinPaths(index.getPrefixPath(prefix), path);

    if (forReference)
    {
        // Set the displacement.
        outputShadingAttribute(
            node, virtualObjectName, index, layer, "displacement");
    }
    else
    {
        // Set the shader.
        outputShadingAttribute(node, virtualObjectName, index, layer, "shader");
    }
}

void RendererPlugin::outputShadingAttribute(
    AtNode* node,
    const std::string& objectName,
    RendererIndex& index,
    const std::string& layer,
    const std::string& target) const
{
    // Get shader
    SdfPath shaderPath = index.getShaderAssignment(objectName, layer, target);
    if (shaderPath.IsEmpty())
    {
        return;
    }

    bool isDisplacement = target == "displacement";
    RendererIndex::RenderNodeMap::accessor accessor;
    bool newEntry = index.getRenderNodeData(shaderPath, accessor);

    if (newEntry)
    {
        // We are here if getRenderNodeData created a new entry.
        // Try to find the node in Arnold nodes on the case the node was
        // created in the maya scene, and it was not exported to USD or
        // Alembic.
        // TODO: It's not an effective way. It's much better to export all
        // the necessary nodes to the session layer. But it's much more
        // complicated.
        AtNode* shaderNode = AiNodeLookUpByName(shaderPath.GetText());

        if (!shaderNode && isDisplacement)
        {
            // MTOA adds ".message" to the displacement nodes.
            shaderPath = shaderPath.AppendProperty(TfToken("message"));
            // Try again with a new name.
            shaderNode = AiNodeLookUpByName(shaderPath.GetText());
        }

        if (shaderNode)
        {
            accessor->second = shaderNode;
        }
    }

    // Read and release the accessor.
    void* shaderData = accessor->second;
    accessor.release();

    if (shaderData)
    {
        std::string arrayName = isDisplacement ? "disp_map" : target;

        AtArray* shaders = AiArrayAllocate(1, 1, AI_TYPE_NODE);
        AiArraySetPtr(shaders, 0, shaderData);
        AiNodeSetArray(node, arrayName.c_str(), shaders);
    }
}

void* RendererPlugin::outputEmptyNode(const std::string& iNodeName) const
{
#if AI_VERSION_ARCH_NUM == 4
    // Everything works well with Arnold 4.
    return nullptr;
#else
    // Output a very small point that is visible only for volume rays
    static const float points[] = {0.0f, 0.0f, 0.0f};
    static const float radius[] = {1e-9f};
    AtNode* node = AiNode("points");
    AiNodeSetStr(node, "name", iNodeName.c_str());
    AiNodeSetByte(node, "visibility", AI_RAY_VOLUME);
    AiNodeSetByte(node, "sidedness", AI_RAY_UNDEFINED);
    AiNodeSetArray(
        node,
        "points",
        AiArrayConvert(
            sizeof(points) / sizeof(points[0]) / 3, 1, AI_TYPE_VECTOR, points));
    AiNodeSetArray(
        node,
        "radius",
        AiArrayConvert(
            sizeof(radius) / sizeof(radius[0]), 1, AI_TYPE_FLOAT, radius));
    return node;
#endif
}
