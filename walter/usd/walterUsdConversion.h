// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERUSDCONVERSION_H_
#define __WALTERUSDCONVERSION_H_

#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/property.h>
#include <pxr/usd/usd/relationship.h>
#include <string>
#include <type_traits>

PXR_NAMESPACE_USING_DIRECTIVE

namespace WalterUSDToString
{
// It's in a separate namespace because it's not possible to specialize
// member functions without explicitly specializing the containing class due
// to C++04, §14.7.3/3.

template <typename T> std::string convert(const T& value);
}

// Sets of functions to cast USD properties into a json.
struct WalterUsdConversion
{
    // Gets the value of an attribute of type T (all excepted strings).
    template <class T>
    static T getAttributeValue(UsdAttribute const& attr, UsdTimeCode const& tc)
    {
        T value;
        attr.Get<T>(&value, tc);
        return value;
    }

    // Gets the value an attribute of basic type T (excepted strings) as a
    // json.
    template <class T>
    static std::string getAttributeValueAsString(
        UsdAttribute const& attr,
        UsdTimeCode const& tc)
    {
        return std::is_same<T, std::string>::value
            ? "\"" + WalterUSDToString::convert(getAttributeValue<T>(attr, tc)) + "\""
            : WalterUSDToString::convert(getAttributeValue<T>(attr, tc));
    }

    // Gets the value of an attribute array of basic type T (excepted strings)
    // as a json.
    template <class T>
    static std::string getArrayAttributeValueAsString(
        UsdAttribute const& attr,
        UsdTimeCode const& tc,
        int maxElementCount,
        int& arraySize)
    {
        VtArray<T> vtArray = getAttributeValue<VtArray<T>>(attr, tc);
        arraySize = (int)vtArray.size();
        unsigned maxCount = maxElementCount > -1 ?
            std::min(maxElementCount, arraySize) :
            arraySize;

        std::string str = "[";
        for (unsigned i = 0; i < maxCount; ++i)
        {
            str += std::is_same<T, std::string>::value
                ? "\"" + WalterUSDToString::convert(vtArray[i]) + "\""
                : WalterUSDToString::convert(vtArray[i]);

            if (i < maxCount - 1)
                str += ", ";
        }
        str += "]";

        return str;
    }
    // Gets the value of an relationship as strings.
    static std::string getRelationShipValueAsString(
        UsdRelationship const& rl,
        UsdTimeCode const& tc);

    // Gets the value of a scalar attribute as strings.
    static std::string getScalarAttributeValueAsString(
        UsdAttribute const& attr,
        UsdTimeCode const& tc);

    // Gets the value of an attribute array as a json array.
    static std::string getArrayAttributeValueAsString(
        UsdAttribute const& attr,
        UsdTimeCode const& tc,
        int maxElementCount,
        int& arraySize);

    // Gets the type and the value of an attribute as a json.
    static bool getAttributeValueAsString(
        UsdAttribute const& attr,
        UsdTimeCode const& tc,
        std::string& propertyType,
        std::string& strValue,
        int& arraySize,
        int maxElementCount = -1);

    // Gets the type and the value of a property as json.
    static bool getPropertyValueAsString(
        UsdProperty const& prop,
        UsdTimeCode const& tc,
        std::string& propertyType,
        std::string& strValue,
        int& arraySize,
        int maxElementCount = -1,
        bool attributeOnly = true);

    // Constructs a single json representing a property.
    static std::string constuctStringRepresentation(
        std::string const& name,
        std::string const& propertyType,
        std::string const& valueType,
        std::string const& strValue,
        int arraySize);
};

#endif // __WALTERUSDCONVERSION_H_
