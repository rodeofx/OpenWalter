// Copyright 2017 Rodeo FX.  All rights reserved.

#include "walterUsdConversion.h"
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec3f.h>

namespace WalterUSDToString
{
#define MY_SCALARS \
    (bool)(int)(long)(float)(double)(unsigned int)(unsigned char)(unsigned long)

#define MY_VECTORS \
    (GfVec2d)(GfVec3d)(GfVec4d)(GfVec2f)(GfVec3f)(GfVec4f)(GfVec2i)(GfVec3i)( \
        GfVec4i)

#define MY_MATRICES (GfMatrix2d)(GfMatrix3d)(GfMatrix4d)

#define MY_QUATS (GfQuatd)(GfQuatf)

// Generate extern template declarations in the header
#define MY_SCALAR_TEMPLATE(r, data, arg) \
    template <> std::string convert<arg>(const arg& value) \
    { \
        return std::to_string(value); \
    }

// Generate extern template declarations in the header
#define MY_VECTOR_TEMPLATE(r, data, arg) \
    template <> std::string convert<arg>(const arg& value) \
    { \
        return convertDim(value, arg::dimension); \
    }

// Generate extern template declarations in the header
#define MY_MATRIX_TEMPLATE(r, data, arg) \
    template <> std::string convert<arg>(const arg& value) \
    { \
        return convertDim(value, arg::numRows * arg::numColumns); \
    }

// Generate extern template declarations in the header
#define MY_QUAT_TEMPLATE(r, data, arg) \
    template <> std::string convert<arg>(const arg& value) \
    { \
        const typename arg::ScalarType* rawBuffer = \
            value.GetImaginary().GetArray(); \
        std::string str = "[";\
        for (unsigned j = 0; j < 3; ++j) \
        { \
            str += convert(rawBuffer[j]); \
            str += ", "; \
        } \
        str += convert(value.GetReal()); \
        str += "]"; \
        return str; \
    }

template <typename T> std::string convertDim(const T& value, int dim)
{
    const typename T::ScalarType* rawBuffer = value.GetArray();

    std::string str = "[";
    for (unsigned j = 0; j < dim; ++j)
    {
        str += convert(rawBuffer[j]);
        if (j < dim - 1)
            str += ", ";
    }
    str += "]";
    return str;
}

BOOST_PP_SEQ_FOR_EACH(MY_SCALAR_TEMPLATE, _, MY_SCALARS)
BOOST_PP_SEQ_FOR_EACH(MY_VECTOR_TEMPLATE, _, MY_VECTORS)
BOOST_PP_SEQ_FOR_EACH(MY_MATRIX_TEMPLATE, _, MY_MATRICES)
BOOST_PP_SEQ_FOR_EACH(MY_QUAT_TEMPLATE, _, MY_QUATS)

template <> std::string convert<std::string>(const std::string& value)
{
    return value;
}

#undef MY_SCALARS
#undef MY_VECTORS
#undef MY_MATRICES
#undef MY_QUATS
#undef MY_SCALAR_TEMPLATE
#undef MY_VECTOR_TEMPLATE
#undef MY_MATRIX_TEMPLATE
#undef MY_QUAT_TEMPLATE
}

PXR_NAMESPACE_USING_DIRECTIVE

std::string WalterUsdConversion::getScalarAttributeValueAsString(
    UsdAttribute const& attr,
    UsdTimeCode const& tc)
{
    std::string strValue;
    const SdfValueTypeName attrTypeName = attr.GetTypeName();

    // Basic type
    if (attrTypeName == SdfValueTypeNames->Bool)
        strValue = getAttributeValueAsString<bool>(attr, tc);

    else if (attrTypeName == SdfValueTypeNames->UChar)
        strValue = getAttributeValueAsString<uint8_t>(attr, tc);

    else if (attrTypeName == SdfValueTypeNames->Int)
        strValue = getAttributeValueAsString<int32_t>(attr, tc);

    else if (attrTypeName == SdfValueTypeNames->UInt)
        strValue = getAttributeValueAsString<uint32_t>(attr, tc);

    else if (attrTypeName == SdfValueTypeNames->Int64)
        strValue = getAttributeValueAsString<int64_t>(attr, tc);

    else if (attrTypeName == SdfValueTypeNames->UInt64)
        strValue = getAttributeValueAsString<uint64_t>(attr, tc);

    else if (attrTypeName == SdfValueTypeNames->Float)
        strValue = getAttributeValueAsString<float>(attr, tc);

    else if (attrTypeName == SdfValueTypeNames->Double)
        strValue = getAttributeValueAsString<double>(attr, tc);

    else if (
        attrTypeName == SdfValueTypeNames->String ||
        attrTypeName == SdfValueTypeNames->Token ||
        attrTypeName == SdfValueTypeNames->Asset)
        strValue = getAttributeValueAsString<std::string>(attr, tc);

    // Double array/matrix
    else if (attrTypeName == SdfValueTypeNames->Matrix2d)
        strValue = getAttributeValueAsString<GfMatrix2d>(attr, tc);

    else if (attrTypeName == SdfValueTypeNames->Matrix3d)
        strValue = getAttributeValueAsString<GfMatrix3d>(attr, tc);

    else if (
        attrTypeName == SdfValueTypeNames->Matrix4d ||
        attrTypeName == SdfValueTypeNames->Frame4d)
        strValue = getAttributeValueAsString<GfMatrix4d>(attr, tc);

    else if (attrTypeName == SdfValueTypeNames->Quatd)
        strValue = getAttributeValueAsString<GfQuatd>(attr, tc);

    else if (attrTypeName == SdfValueTypeNames->Double2)
        strValue = getAttributeValueAsString<GfVec2d>(attr, tc);

    else if (
        attrTypeName == SdfValueTypeNames->Double3 ||
        attrTypeName == SdfValueTypeNames->Color3d ||
        attrTypeName == SdfValueTypeNames->Vector3d ||
        attrTypeName == SdfValueTypeNames->Normal3d ||
        attrTypeName == SdfValueTypeNames->Point3d)
        strValue = getAttributeValueAsString<GfVec3d>(attr, tc);

    else if (
        attrTypeName == SdfValueTypeNames->Double4 ||
        attrTypeName == SdfValueTypeNames->Color4d)
        strValue = getAttributeValueAsString<GfVec4d>(attr, tc);

    // Float array/matrix
    else if (attrTypeName == SdfValueTypeNames->Quatf)
        strValue = getAttributeValueAsString<GfQuatf>(attr, tc);

    else if (attrTypeName == SdfValueTypeNames->Float2)
        strValue = getAttributeValueAsString<GfVec2f>(attr, tc);

    else if (
        attrTypeName == SdfValueTypeNames->Float3 ||
        attrTypeName == SdfValueTypeNames->Color3f ||
        attrTypeName == SdfValueTypeNames->Vector3f ||
        attrTypeName == SdfValueTypeNames->Normal3f ||
        attrTypeName == SdfValueTypeNames->Point3f)
        strValue = getAttributeValueAsString<GfVec3f>(attr, tc);

    else if (
        attrTypeName == SdfValueTypeNames->Float4 ||
        attrTypeName == SdfValueTypeNames->Color4f)
        strValue = getAttributeValueAsString<GfVec4f>(attr, tc);

    // Int array/matrix
    else if (attrTypeName == SdfValueTypeNames->Int2)
        strValue = getAttributeValueAsString<GfVec2i>(attr, tc);

    else if (attrTypeName == SdfValueTypeNames->Int3)
        strValue = getAttributeValueAsString<GfVec3i>(attr, tc);

    else if (attrTypeName == SdfValueTypeNames->Int4)
        strValue = getAttributeValueAsString<GfVec4i>(attr, tc);

    return strValue;
}

std::string WalterUsdConversion::getArrayAttributeValueAsString(
    UsdAttribute const& attr,
    UsdTimeCode const& tc,
    int maxElementCount,
    int& arraySize)
{
    std::string strValue;
    const SdfValueTypeName& attrTypeName = attr.GetTypeName();

    // Basic type
    if (attrTypeName == SdfValueTypeNames->BoolArray)
        strValue = getArrayAttributeValueAsString<bool>(
            attr, tc, maxElementCount, arraySize);

    else if (attrTypeName == SdfValueTypeNames->UCharArray)
        strValue = getArrayAttributeValueAsString<uint8_t>(
            attr, tc, maxElementCount, arraySize);

    else if (attrTypeName == SdfValueTypeNames->IntArray)
        strValue = getArrayAttributeValueAsString<int32_t>(
            attr, tc, maxElementCount, arraySize);

    else if (attrTypeName == SdfValueTypeNames->UIntArray)
        strValue = getArrayAttributeValueAsString<uint32_t>(
            attr, tc, maxElementCount, arraySize);

    else if (attrTypeName == SdfValueTypeNames->Int64Array)
        strValue = getArrayAttributeValueAsString<int64_t>(
            attr, tc, maxElementCount, arraySize);

    else if (attrTypeName == SdfValueTypeNames->UInt64Array)
        strValue = getArrayAttributeValueAsString<uint64_t>(
            attr, tc, maxElementCount, arraySize);

    else if (attrTypeName == SdfValueTypeNames->FloatArray)
        strValue = getArrayAttributeValueAsString<float>(
            attr, tc, maxElementCount, arraySize);

    else if (attrTypeName == SdfValueTypeNames->DoubleArray)
        strValue = getArrayAttributeValueAsString<double>(
            attr, tc, maxElementCount, arraySize);

    else if (
        attrTypeName == SdfValueTypeNames->StringArray ||
        attrTypeName == SdfValueTypeNames->TokenArray ||
        attrTypeName == SdfValueTypeNames->AssetArray)
        strValue = getArrayAttributeValueAsString<std::string>(
            attr, tc, maxElementCount, arraySize);

    // Double array/matrix
    else if (attrTypeName == SdfValueTypeNames->Matrix2dArray)
        strValue = getArrayAttributeValueAsString<GfMatrix2d>(
            attr, tc, maxElementCount, arraySize);

    else if (attrTypeName == SdfValueTypeNames->Matrix3dArray)
        strValue = getArrayAttributeValueAsString<GfMatrix3d>(
            attr, tc, maxElementCount, arraySize);

    else if (
        attrTypeName == SdfValueTypeNames->Matrix4dArray ||
        attrTypeName == SdfValueTypeNames->Frame4dArray)
        strValue = getArrayAttributeValueAsString<GfMatrix4d>(
            attr, tc, maxElementCount, arraySize);

    else if (attrTypeName == SdfValueTypeNames->QuatdArray)
        strValue = getArrayAttributeValueAsString<GfQuatd>(
            attr, tc, maxElementCount, arraySize);

    else if (attrTypeName == SdfValueTypeNames->Double2Array)
        strValue = getArrayAttributeValueAsString<GfVec2d>(
            attr, tc, maxElementCount, arraySize);

    else if (
        attrTypeName == SdfValueTypeNames->Double3Array ||
        attrTypeName == SdfValueTypeNames->Color3dArray ||
        attrTypeName == SdfValueTypeNames->Vector3dArray ||
        attrTypeName == SdfValueTypeNames->Normal3dArray ||
        attrTypeName == SdfValueTypeNames->Point3dArray)
        strValue = getArrayAttributeValueAsString<GfVec3d>(
            attr, tc, maxElementCount, arraySize);

    else if (
        attrTypeName == SdfValueTypeNames->Double4Array ||
        attrTypeName == SdfValueTypeNames->Color4dArray)
        strValue = getArrayAttributeValueAsString<GfVec4d>(
            attr, tc, maxElementCount, arraySize);

    // Float array/matrix
    else if (attrTypeName == SdfValueTypeNames->QuatfArray)
        strValue = getArrayAttributeValueAsString<GfQuatf>(
            attr, tc, maxElementCount, arraySize);

    else if (attrTypeName == SdfValueTypeNames->Float2Array)
        strValue = getArrayAttributeValueAsString<GfVec2f>(
            attr, tc, maxElementCount, arraySize);

    else if (
        attrTypeName == SdfValueTypeNames->Float3Array ||
        attrTypeName == SdfValueTypeNames->Color3fArray ||
        attrTypeName == SdfValueTypeNames->Vector3fArray ||
        attrTypeName == SdfValueTypeNames->Normal3fArray ||
        attrTypeName == SdfValueTypeNames->Point3fArray)
        strValue = getArrayAttributeValueAsString<GfVec3f>(
            attr, tc, maxElementCount, arraySize);

    else if (
        attrTypeName == SdfValueTypeNames->Float4Array ||
        attrTypeName == SdfValueTypeNames->Color4fArray)
        strValue = getArrayAttributeValueAsString<GfVec4f>(
            attr, tc, maxElementCount, arraySize);

    // Int array/matrix
    else if (attrTypeName == SdfValueTypeNames->Int2Array)
        strValue = getArrayAttributeValueAsString<GfVec2i>(
            attr, tc, maxElementCount, arraySize);

    else if (attrTypeName == SdfValueTypeNames->Int3Array)
        strValue = getArrayAttributeValueAsString<GfVec3i>(
            attr, tc, maxElementCount, arraySize);

    else if (attrTypeName == SdfValueTypeNames->Int4Array)
        strValue = getArrayAttributeValueAsString<GfVec4i>(
            attr, tc, maxElementCount, arraySize);

    return strValue;
}

std::string WalterUsdConversion::getRelationShipValueAsString(
    const UsdRelationship& rl,
    UsdTimeCode const& tc)
{
    SdfPathVector targets;
    rl.GetTargets(&targets);

    std::string strValue = "[";
    for (auto it = targets.begin(); it != targets.end(); ++it)
        strValue += it->GetString();
    strValue += "]";

    return strValue;
}

bool WalterUsdConversion::getAttributeValueAsString(
    UsdAttribute const& attr,
    UsdTimeCode const& tc,
    std::string& propertyType,
    std::string& strValue,
    int& arraySize,
    int maxElementCount)
{
    SdfValueTypeName attrTypeName;

    attrTypeName = attr.GetTypeName();
    propertyType = attrTypeName.GetType().GetTypeName();

    if (attrTypeName.IsScalar())
    {
        strValue = getScalarAttributeValueAsString(attr, tc);
        return true;
    }

    else if (attrTypeName.IsArray())
    {
        strValue = getArrayAttributeValueAsString(
            attr, tc, maxElementCount, arraySize);
        return true;
    }

    return false;
}

bool WalterUsdConversion::getPropertyValueAsString(
    UsdProperty const& prop,
    UsdTimeCode const& tc,
    std::string& propertyType,
    std::string& strValue,
    int& arraySize,
    int maxElementCount,
    bool attributeOnly)
{
    SdfValueTypeName attrTypeName;

    if (prop.Is<UsdAttribute>())
    {
        getAttributeValueAsString(
            prop.As<UsdAttribute>(),
            tc,
            propertyType,
            strValue,
            arraySize,
            maxElementCount);
        return true;
    }

    else if (!attributeOnly && prop.Is<UsdRelationship>())
    {
        propertyType = prop.GetBaseName().GetText();
        const UsdRelationship& rl = prop.As<UsdRelationship>();
        strValue = getRelationShipValueAsString(rl, tc);
        return true;
    }

    return false;
}

std::string WalterUsdConversion::constuctStringRepresentation(
    std::string const& name,
    std::string const& propertyType,
    std::string const& valueType,
    std::string const& strValue,
    int arraySize)
{
    std::string jsonStr = "{ ";
    jsonStr += "\"name\": \"" + name + "\"";
    jsonStr += ", \"type\": \"" + valueType + "\"";
    jsonStr += ", \"arraySize\": " + std::to_string(arraySize);
    jsonStr += ", \"value\": " + strValue;
    jsonStr += " }";

    return jsonStr;
}
