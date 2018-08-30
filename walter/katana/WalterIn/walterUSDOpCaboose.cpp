// Copyright 2017 Rodeo FX.  All rights reserved.

#include "walterUSDOpCaboose.h"

#include "schemas/volume.h"
#include "walterUSDOpEngine.h"
#include "walterUSDOpUtils.h"

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnDataBuilder.h>
#include <FnGeolib/op/FnGeolibOp.h>
#include <FnGeolibServices/FnXFormUtil.h>
#include <pxr/usd/usdGeom/curves.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/shader.h>
#include <usdKatana/utils.h>
#include <boost/algorithm/string.hpp>

#ifdef USE_OPENVDB
#include <openvdb/io/File.h>
#include <openvdb/openvdb.h>
#endif

namespace OpCabooseImpl
{
/**
 * @brief Calls GroupBuilder::set. We need it to be able to std::bind set.
 *
 * @param ioGB GroupBuilder to call set().
 * @param iPath First argument to set.
 * @param iAttr Second argument to set.
 */
void setGroupBuilder(
    void* ioGB,
    const std::string& iPath,
    const FnAttribute::Attribute& iAttr)
{
    auto groupBuilder = reinterpret_cast<FnAttribute::GroupBuilder*>(ioGB);
    groupBuilder->set(iPath, iAttr);
}

/**
 * @brief Push USD array attribute to Katana. It extracts data from USD
 * attribute, converts the USD data type to the type understandable by Katana,
 * and sets Katana attribute.
 *
 * TODO: Can we use either PxrUsdKatanaUtils::ConvertVtValueToKatAttr or
 * _ConvertGeomAttr instead?
 *
 * @param KatanaType Katana attribute type. Example FnAttribute::IntAttribute.
 * @param UsdType The type of the single element of the USD array.
 * @param TupleSize Katana tuple size. For float it's 1, for point it's 3.
 * @param iAttr Attribute that will be passed to Katana.
 * @param iName Katana name of the new attribute.
 * @param iTime The sample times.
 * @param oStaticGb Output group builder where the new attribute will be
 * created.
 *
 * @return Number of the elements in the created Katana attribute.
 */
template <typename KatanaType, typename UsdType, unsigned int TupleSize = 1>
size_t attributeArrayToKatana(
    UsdAttribute iAttr,
    const char* iName,
    const OpUtils::TimePtr iTime,
    FnAttribute::GroupBuilder& oStaticGb)
{
    if (!iAttr.IsValid() || !iAttr.HasValue())
    {
        return 0;
    }

    // Number of motion keys.
    size_t keys = iTime->size();
    // The number of data items.
    size_t size = 0;

    // The data builder to combine animation samples. If there are no samples,
    // it's not used.
    FnAttribute::DataBuilder<KatanaType> dataBuilder;

    for (auto time : iTime->samples())
    {
        VtArray<UsdType> array;
        iAttr.Get(&array, iTime->current() + time);

        if (array.empty())
        {
            continue;
        }

        // TODO: std::is_same<typename KatanaType::value_type, UsdType>::value
        std::vector<typename KatanaType::value_type> value;

        // TODO: It will not cast the type. If KatanaType and UsdType are
        // different, it will produce wrong data.
        size = array.size() * TupleSize;
        auto dataBegin =
            reinterpret_cast<typename KatanaType::value_type*>(array.data());
        auto dataEnd = dataBegin + size;

        value.reserve(size);
        value.insert(value.end(), dataBegin, dataEnd);

        if (keys == 1)
        {
            oStaticGb.set(
                iName, KatanaType(&(value.front()), value.size(), TupleSize));
        }
        else
        {
            dataBuilder.set(value, time);
        }
    }

    if (keys > 1)
    {
        oStaticGb.set(iName, dataBuilder.build());
    }

    return size;
}

/**
 * @brief Convert USD interpolation to Katana scope.
 *
 * @param iInterpolation Interpolation
 *
 * @return Scope
 */
FnAttribute::StringAttribute interpolationToScope(const TfToken& iInterpolation)
{
    static const FnAttribute::StringAttribute faceAttr("face");
    static const FnAttribute::StringAttribute pointAttr("point");
    static const FnAttribute::StringAttribute primAttr("primitive");
    static const FnAttribute::StringAttribute vertexAttr("vertex");

    if (iInterpolation == UsdGeomTokens->faceVarying)
    {
        return vertexAttr;
    }
    else if (iInterpolation == UsdGeomTokens->varying)
    {
        return pointAttr;
    }
    else if (iInterpolation == UsdGeomTokens->vertex)
    {
        // see below
        return pointAttr;
    }
    else if (iInterpolation == UsdGeomTokens->uniform)
    {
        return faceAttr;
    }

    return primAttr;
}

/**
 * @brief Converts USD role name to Katana type string.
 *
 * @param iRoleName USD role, like Point/Vector/Normal/Color
 * @param iTypeStr It will be used if the role is unknown
 * @param oInputTypeAttr Type name recognizable by Katana
 * @param oElementSizeAttr Katana element's size.
 *
 * @return True if success.
 */
bool KTypeAndSizeFromUsdVec4(
    TfToken const& iRoleName,
    const char* iTypeStr,
    FnAttribute::Attribute* oInputTypeAttr)
{
    if (iRoleName == SdfValueRoleNames->Point)
    {
        *oInputTypeAttr = FnAttribute::StringAttribute("point4");
    }
    else if (iRoleName == SdfValueRoleNames->Vector)
    {
        *oInputTypeAttr = FnAttribute::StringAttribute("vector4");
    }
    else if (iRoleName == SdfValueRoleNames->Normal)
    {
        *oInputTypeAttr = FnAttribute::StringAttribute("normal4");
    }
    else if (iRoleName == SdfValueRoleNames->Color)
    {
        *oInputTypeAttr = FnAttribute::StringAttribute("color4");
    }
    else if (iRoleName.IsEmpty())
    {
        *oInputTypeAttr = FnKat::StringAttribute(iTypeStr);
    }
    else
    {
        return false;
    }

    return true;
}

/**
 * @brief A simple utility to copy data from VtVec4fArray to vector.
 */
void ConvertArrayToVector(const VtVec4fArray& a, std::vector<float>* r)
{
    r->resize(a.size() * 4);
    size_t i = 0;
    for (const auto& vec : a)
    {
        for (int c = 0; c < 4; c++)
        {
            (*r)[i++] = vec[c];
        }
    }

    TF_VERIFY(i == r->size());
}

/**
 * @brief As it's clear from the name it converts any VtValue to Katana
 * attribute set. PxrUsdKatanaUtils::ConvertVtValueToKatCustomGeomAttr doesn't
 * support all the types, and we need this function to have Point4.
 *
 * @param iVal Input VtValue
 * @param iElementSize The size of an element
 * @param iRoleName USD role, like Point/Vector/Normal/Color
 * @param oValueAttr Katana value.
 * @param oInputTypeAttr Type name recognizable by Katana
 * @param oElementSizeAttr Katana element's size.
 */
void convertVtValueToKatCustomGeomAttr(
    const VtValue& iVal,
    int iElementSize,
    const TfToken& iRoleName,
    FnAttribute::Attribute* oValueAttr,
    FnAttribute::Attribute* oInputTypeAttr,
    FnAttribute::Attribute* oElementSizeAttr)
{
    if (iVal.IsHolding<GfVec4f>())
    {
        if (KTypeAndSizeFromUsdVec4(iRoleName, "point4", oInputTypeAttr))
        {
            const GfVec4f rawVal = iVal.Get<GfVec4f>();
            FnAttribute::FloatBuilder builder(/* tupleSize = */ 4);
            std::vector<float> vec(rawVal.data(), rawVal.data() + 4);
            builder.set(vec);
            *oValueAttr = builder.build();
        }
        return;
    }
    if (iVal.IsHolding<VtArray<GfVec4f>>())
    {
        if (KTypeAndSizeFromUsdVec4(iRoleName, "float", oInputTypeAttr))
        {
            const VtArray<GfVec4f> rawVal = iVal.Get<VtArray<GfVec4f>>();
            std::vector<float> vec;
            ConvertArrayToVector(rawVal, &vec);
            FnKat::FloatBuilder builder(/* tupleSize = */ 4);
            builder.set(vec);
            *oValueAttr = builder.build();
        }
        return;
    }
    if (iVal.IsHolding<VtArray<bool>>())
    {
        const VtArray<bool> rawVal = iVal.Get<VtArray<bool>>();
        std::vector<int> vec(rawVal.size());
        for (int i=0; i<rawVal.size(); ++i)
        {
            vec[i] = rawVal[i];
        }

        FnKat::IntBuilder builder(/* tupleSize = */ 1);
        builder.set(vec);
        *oValueAttr = builder.build();

        return;
    }
    // else
    PxrUsdKatanaUtils::ConvertVtValueToKatCustomGeomAttr(
        iVal,
        iElementSize,
        iRoleName,
        oValueAttr,
        oInputTypeAttr,
        oElementSizeAttr);
}

/**
 * @brief Generates geometry arbitrary attribute for Katana.
 *
 * @param iVtValue The data of the attribute.
 * @param vtIndices The indexes of the attribute. If this is not emply, the
 * generated arbitrary attribute will be an indexed attribute.
 * @param iElementSize The size of the element. For example it's 3 for point.
 * @param iType USD type.
 * @param iInterpolation USD interpolation. TODO: is it enough to use scope?
 * @param iScopeAttr Katana scope.
 *
 * @return GroupAttribute that can be directly used as arbitrary attr.
 */
FnAttribute::Attribute convertVtValueToArbitrary(
    const VtValue& iVtValue,
    const VtIntArray& vtIndices,
    int iElementSize,
    const SdfValueTypeName& iType,
    const TfToken& iInterpolation,
    const FnAttribute::StringAttribute& iScopeAttr)
{
    // Convert value to the required Katana attributes to describe it.
    FnAttribute::Attribute valueAttr;
    FnAttribute::Attribute inputTypeAttr;
    FnAttribute::Attribute elementSizeAttr;
    convertVtValueToKatCustomGeomAttr(
        iVtValue,
        iElementSize,
        iType.GetRole(),
        &valueAttr,
        &inputTypeAttr,
        &elementSizeAttr);

    // Bundle them into a group attribute
    FnAttribute::GroupBuilder attrBuilder;
    attrBuilder.set("scope", iScopeAttr);
    attrBuilder.set("inputType", inputTypeAttr);
    if (elementSizeAttr.isValid())
    {
        attrBuilder.set("elementSize", elementSizeAttr);
    }

    std::string valueAttributeName;
    if (!vtIndices.empty())
    {
        // If we have indexes, we need to output it as indexed attribute.
        valueAttributeName = "indexedValue";
        attrBuilder.set(
            "index",
            FnAttribute::IntAttribute(vtIndices.data(), vtIndices.size(), 1));
    }
    else
    {
        valueAttributeName = "value";
    }

    attrBuilder.set(valueAttributeName, valueAttr);

    // Note that 'varying' vs 'vertex' require special handling, as in
    // Katana they are both expressed as 'point' scope above. To get
    // 'vertex' interpolation we must set an additional
    // 'interpolationType' attribute.  So we will flag that here.
    const bool vertexInterpolationType =
        (iInterpolation == UsdGeomTokens->vertex);
    if (vertexInterpolationType)
    {
        attrBuilder.set(
            "interpolationType", FnAttribute::StringAttribute("subdiv"));
    }

    return attrBuilder.build();
}

/**
 * @brief Output the bound to the Katana group builder.
 *
 * @param iImageable The UsdGeomImageable with a bound.
 * @param iTime The sample times.
 * @param oStaticGb Output group builder where the new attribute will be
 * created.
 * @param isLocal Whether to return the local bound or the untransformed
 * bound (default).
 */
void createBound(
    const UsdGeomImageable& iImageable,
    const OpUtils::TimePtr iTime,
    FnAttribute::GroupBuilder& oStaticGb,
    bool isLocal=false)
{
    // Get the bounding box.
    GfBBox3d bbox;
    // We need to combine all the bounding boxes of the all the motion
    // samples.
    bool bboxInitialized = false;
    for (auto t : iTime->samples())
    {
        GfBBox3d current =  isLocal
            ? iImageable.ComputeLocalBound(
                t, UsdGeomTokens->default_, UsdGeomTokens->render)
            : iImageable.ComputeUntransformedBound(
                t, UsdGeomTokens->default_, UsdGeomTokens->render);

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

    const GfRange3d& range = bbox.ComputeAlignedBox();
    const GfVec3d& min = range.GetMin();
    const GfVec3d& max = range.GetMax();

    // Output to the group builder.
    FnKat::DoubleBuilder builder(1);
    builder.set({min[0], max[0], min[1], max[1], min[2], max[2]});
    oStaticGb.set("bound", builder.build());
}

/**
 * @brief Outputs the primvars of the object to the Katana group builder.
 *
 * @param iPrim The USD prim to output.
 * @param iTime The sample times.
 * @param oStaticGb Output group builder where the new attribute will be
 * created.
 */
void cookGeomImageable(
    const UsdPrim& iPrim,
    const OpUtils::TimePtr iTime,
    FnAttribute::GroupBuilder& oStaticGb)
{
    UsdGeomImageable imageable(iPrim);

    const bool isCurve = iPrim.IsA<UsdGeomCurves>();

    // Create primvars. We don't use PxrUsdKatanaGeomGetPrimvarGroup because it
    // requires PxrUsdKatanaUsdInPrivateData and PxrUsdKatanaUsdInArgs. The next
    // block is very close to PxrUsdKatanaUsdInPrivateData from readPrim.cpp
    // from USD.
    FnAttribute::GroupBuilder primvarsBuilder;
    for (const UsdGeomPrimvar& primvar : imageable.GetPrimvars())
    {
        // Katana backends (such as KTOA) are not prepared to handle groups of
        // primvars under geometry.arbitrary, which leaves us without a
        // ready-made way to incorporate namespaced primvars like
        // "primvars:skel:jointIndices".  Until we untangle that, skip importing
        // any namespaced primvars.
        if (primvar.NameContainsNamespaces())
        {
            continue;
        }

        TfToken name;
        TfToken interpolation;
        SdfValueTypeName typeName;
        int elementSize;

        // GetDeclarationInfo inclues all namespaces other than "primvars:" in
        // 'name'
        primvar.GetDeclarationInfo(
            &name, &typeName, &interpolation, &elementSize);

        // Name: this will eventually need to know how to translate namespaces
        std::string gdName = name;
        // KTOA needs UV coordinates to be named "st".
        if (gdName == "uv")
        {
            gdName = "st";
        }

        // Convert interpolation -> scope
        FnAttribute::StringAttribute scopeAttr;
        if (isCurve && interpolation == UsdGeomTokens->vertex)
        {
            // it's a curve, so "vertex" == "vertex"
            scopeAttr = FnAttribute::StringAttribute("vertex");
        }
        else
        {
            scopeAttr = interpolationToScope(interpolation);
        }

        // Resolve the value
        VtValue vtValue;
        VtIntArray vtIndices;
        if (interpolation == UsdGeomTokens->faceVarying && primvar.IsIndexed())
        {
            // It's an indexed value. We don't want to flatten it because it
            // breaks subdivs.
            if (!primvar.Get(&vtValue, iTime->current()))
            {
                continue;
            }

            if (!primvar.GetIndices(&vtIndices, iTime->current()))
            {
                continue;
            }
        }
        else
        {
            // USD comments suggest using using the ComputeFlattened() API
            // instead of Get even if they produce the same data.
            if (!primvar.ComputeFlattened(&vtValue, iTime->current()))
            {
                continue;
            }
        }

        primvarsBuilder.set(
            gdName,
            OpCabooseImpl::convertVtValueToArbitrary(
                vtValue,
                vtIndices,
                elementSize,
                typeName,
                interpolation,
                scopeAttr));
    }

    oStaticGb.set("geometry.arbitrary", primvarsBuilder.build());
}

/**
 * @brief Outputs xform of the object to the Katana group builder.
 *
 * @param iPrim The USD prim to output.
 * @param iTime The sample times.
 * @param oStaticGb Output group builder where the new attribute will be
 * created.
 */
void cookGeomXformable(
    const UsdPrim& iPrim,
    const OpUtils::TimePtr iTime,
    FnAttribute::GroupBuilder& oStaticGb)
{
    UsdGeomXformable xformable(iPrim);

    // This block closely matches to PxrUsdKatanaReadXformable() of
    // readXformable.cpp.
    // Get the ordered xform ops for the prim.
    bool resetsXformStack;
    std::vector<UsdGeomXformOp> orderedXformOps =
        xformable.GetOrderedXformOps(&resetsXformStack);

    FnAttribute::GroupBuilder gb;

    // For each xform op, construct a matrix containing the
    // transformation data for each time sample it has.
    int opCount = 0;
    for (const auto& xformOp : orderedXformOps)
    {
        FnAttribute::DoubleBuilder matBuilder(16);
        for (auto time : iTime->samples())
        {
            GfMatrix4d mat = xformOp.GetOpTransform(iTime->current() + time);

            // Convert to vector.
            const double* matArray = mat.GetArray();
            std::vector<double>& matVec = matBuilder.get(time);

            matVec.resize(16);
            for (int i = 0; i < 16; ++i)
            {
                matVec[i] = matArray[i];
            }
        }

        gb.set("matrix" + std::to_string(opCount++), matBuilder.build());
    }

    // Only set an 'xform' attribute if xform ops were found.
    //
    if (!orderedXformOps.empty())
    {
        FnAttribute::GroupBuilder xformGb;
        xformGb.setGroupInherit(false);

        // Reset the location to the origin if the xform op
        // requires the xform stack to be reset.
        if (resetsXformStack)
        {
            xformGb.set("origin", FnAttribute::DoubleAttribute(1));
        }

        FnAttribute::DoubleAttribute matrixAttr =
            FnGeolibServices::FnXFormUtil::CalcTransformMatrixAtExistingTimes(
                gb.build())
                .first;

        xformGb.set("matrix", matrixAttr);

        oStaticGb.set("xform", xformGb.build());
    }
}

void cookGeomMesh(
    const UsdPrim& iPrim,
    const OpUtils::TimePtr iTime,
    FnAttribute::GroupBuilder& oStaticGb)
{
    // Get mesh.
    UsdGeomMesh mesh(iPrim);

    oStaticGb.set("type", FnAttribute::StringAttribute("polymesh"));

    // Eval vertex counts of faces.
    VtIntArray numVertsArray;
    mesh.GetFaceVertexCountsAttr().Get(&numVertsArray, iTime->current());
    // Katana doesn't have "Face Vertex Counts". Instead, it has startIndex. We
    // need to convert "Face Vertex Counts" to Katana's startIndex.
    // FVC:        [4, 4, 3, 4]
    // startIndex: [0, 4, 8, 11, 15]
    {
        size_t numVertsArraySize = numVertsArray.size();
        std::vector<int> startIndex;
        startIndex.reserve(numVertsArraySize + 1);

        startIndex.push_back(0);

        for (size_t i = 0; i < numVertsArraySize; i++)
        {
            startIndex.push_back(startIndex[i] + numVertsArray[i]);
        }

        oStaticGb.set(
            "geometry.poly.startIndex",
            FnAttribute::IntAttribute(startIndex.data(), startIndex.size(), 1));
    }

    attributeArrayToKatana<FnAttribute::IntAttribute, int>(
        mesh.GetFaceVertexIndicesAttr(),
        "geometry.poly.vertexList",
        iTime,
        oStaticGb);

    attributeArrayToKatana<FnAttribute::FloatAttribute, GfVec3f, 3>(
        mesh.GetPointsAttr(), "geometry.point.P", iTime, oStaticGb);

    attributeArrayToKatana<FnAttribute::FloatAttribute, GfVec3f, 3>(
        mesh.GetNormalsAttr(), "geometry.vertex.N", iTime, oStaticGb);
}

void cookShadeShader(
    const UsdPrim& iPrim,
    const OpUtils::TimePtr iTime,
    FnAttribute::GroupBuilder& oStaticGb)
{
    // TODO: Can we use _GatherShadingParameters although it's private?

    // Get shader.
    UsdShadeShader shader(iPrim);

    FnAttribute::GroupBuilder parametersBuilder;
    FnAttribute::GroupBuilder connectionsBuilder;

    const std::string name = iPrim.GetName().GetString();

    for (const UsdShadeInput& input : shader.GetInputs())
    {
        std::string inputName = input.GetBaseName();
        // Convert USD namespaces to Arnold components. We need it because we
        // did conversion from Arnold components to USD namespaces in Alembic to
        // USD plugin.
        std::replace(inputName.begin(), inputName.end(), ':', '.');

        // Extract connection
        UsdShadeConnectableAPI source;
        TfToken outputName;
        UsdShadeAttributeType sourceType;

        if (UsdShadeConnectableAPI::GetConnectedSource(
                input, &source, &outputName, &sourceType))
        {
            std::string outputNameStr = outputName.GetString();
            if (outputNameStr != "out")
            {
                outputNameStr = "out." + outputNameStr;
            }

            connectionsBuilder.set(
                inputName,
                FnAttribute::StringAttribute(
                    outputNameStr + "@" + source.GetPath().GetName()));
        }

        // Extract value
        UsdAttribute attr = input.GetAttr();
        VtValue vtValue;
        if (!attr.Get(&vtValue, iTime->current()))
        {
            continue;
        }

        parametersBuilder.set(
            inputName,
            PxrUsdKatanaUtils::ConvertVtValueToKatAttr(vtValue, true));
    }

    // Query shader type. It's "standard_surface"
    static const TfToken typeToken("info:type");
    TfToken type;
    iPrim.GetAttribute(typeToken).Get(&type, iTime->current());

    // Target is usually render name. "arnold"
    static const TfToken targetToken("info:target");
    TfToken target;
    iPrim.GetAttribute(targetToken).Get(&target, iTime->current());

    oStaticGb.set("name", FnAttribute::StringAttribute(name));
    oStaticGb.set("srcName", FnAttribute::StringAttribute(name));
    oStaticGb.set("type", FnAttribute::StringAttribute(type.GetString()));
    oStaticGb.set("target", FnAttribute::StringAttribute(target.GetString()));
    oStaticGb.set("parameters", parametersBuilder.build());
    oStaticGb.set("connections", connectionsBuilder.build());
}

void cookShadeMaterial(
    const UsdPrim& iPrim,
    const OpUtils::TimePtr iTime,
    FnAttribute::GroupBuilder& oStaticGb)
{
    // Get material.
    UsdShadeMaterial material(iPrim);

    FnAttribute::GroupBuilder nodesBuilder;
    FnAttribute::GroupBuilder terminalsBuilder;

    // Create the ports. Example: "arnoldSurface" and "arnoldSurfacePort"
    // attributes
    for (const UsdAttribute& attr : iPrim.GetAttributes())
    {
        if (!UsdShadeConnectableAPI::HasConnectedSource(attr))
        {
            continue;
        }

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

        // Get the connection using ConnectableAPI.
        UsdShadeConnectableAPI connectableAPISource;
        TfToken sourceName;
        UsdShadeAttributeType sourceType;
        if (!UsdShadeConnectableAPI::GetConnectedSource(
                attr, &connectableAPISource, &sourceName, &sourceType))
        {
            // Should never happen because we already checked it.
            assert(false);
        }

        // Title surface -> Surface
        target[0] = toupper(target[0]);

        terminalsBuilder.set(
            render + target,
            FnAttribute::StringAttribute(
                connectableAPISource.GetPrim().GetName()));
        terminalsBuilder.set(
            render + target + "Port", FnAttribute::StringAttribute("out"));
    }

    // Iterate the shaders.
    for (const UsdPrim& child : iPrim.GetDescendants())
    {
        W_DEBUG(
            "Checking child %s %s",
            child.GetTypeName().GetText(),
            child.GetPath().GetText());

        FnAttribute::GroupBuilder shadingNode;
        if (child.IsA<UsdShadeShader>())
        {
            cookShadeShader(child, iTime, shadingNode);
        }
        nodesBuilder.set(child.GetName().GetString(), shadingNode.build());
    }

    static const FnAttribute::StringAttribute materialStrAttr("material");
    static const FnAttribute::StringAttribute networkStrAttr("network");

    oStaticGb.set("type", materialStrAttr);
    oStaticGb.set("material.nodes", nodesBuilder.build());
    oStaticGb.set("material.terminals", terminalsBuilder.build());
    oStaticGb.set("material.style", networkStrAttr);
}

/**
 * @brief Return assignment of the specified path
 *
 * @param iPath the path of the object.
 * @param iTarget "shader", "displacement", "attribute".
 * @param ioIndex
 *
 * @return the path to the material or walterOVerride object.
 */
SdfPath getAssignment(
    const SdfPath& iPath,
    const std::string& iTarget,
    const OpIndex& iIndex,
    bool checkParent = true)
{

    const SdfPath assignmentPath =
        iIndex.getMaterialAssignment(iPath, iTarget);

    SdfPath parentAssignmentPath;
    if (!iPath.IsRootPrimPath() && checkParent)
    {
        parentAssignmentPath =
            iIndex.getMaterialAssignment(iPath.GetParentPath(), iTarget);
    }

    if (assignmentPath == parentAssignmentPath)
    {
        return SdfPath::EmptyPath();
    }

    return assignmentPath;
}

/**
 * @brief Outputs material assign of the object to the Katana group builder.
 *
 * @param iPrim The USD prim to output.
 * @param iPrivateData The object that is generated by us for each node we
 * are going to produce.
 * @param oStaticGb Output group builder where the new attribute will be
 * created.
 * @param isParentImageable Whether the parent prim is a UsdGeomImageable or
 * not. If not, getAssignment won't check parent assignment for inheritance.
 */
void cookWalterAssignment(
    const UsdPrim& iPrim,
    const OpUtils::PrivateData* iPrivateData,
    FnAttribute::GroupBuilder& oStaticGb,
    bool isParentImageable)
{
    OpUtils::TimePtr time = iPrivateData->time();
    SdfPath path = iPrivateData->path();
    OpIndex& index = iPrivateData->engine().index();
    std::string masterName = iPrivateData->engine().getMasterName(iPrim);

    static const std::string surfaceTarget = "shader";
    static const std::string displacementTarget = "displacement";
    static const std::string attributeTarget = "attribute";

    // If masterName exists, it means that this prim is under a Master prim
    // and we need to specify a path on which to get assignement.
    // e.g.: if prim in Katana is located at /master_shape/cube, we need to
    //       look for assignment on /shape/cube in Usd.
    if (masterName != "")
    {
        SdfPath primPath = iPrim.GetPrimPath();
        SdfPath masterRoot = primPath.GetPrefixes().front();

        std::string newPrimPath = primPath.GetString();
        boost::replace_first(newPrimPath, masterRoot.GetName(), masterName);

        path = SdfPath(newPrimPath);
    }

    // Shader assignment. First, we need to get the standard material
    // binding. If it exists, it should win.
    UsdRelationship binding = UsdShadeMaterial::GetBindingRel(iPrim);
    SdfPath surfaceMaterialPath;
    if (binding)
    {
        SdfPathVector targetPaths;
        binding.GetForwardedTargets(&targetPaths);
        if ((targetPaths.size() == 1) && targetPaths.front().IsPrimPath())
        {
            surfaceMaterialPath = targetPaths.front();
        }
    }

    // If the standard binding doesn't exist, we need to evaluate our
    // expressions.
    if (surfaceMaterialPath.IsEmpty())
    {
        surfaceMaterialPath = OpCabooseImpl::getAssignment(
            path,
            surfaceTarget,
            index,
            isParentImageable
        );
    }

    // TODO: it's necessary to figure out if we need to assign displacement
    // if the standard binding exists. Now we do assign on the top of the
    // standard binding.
    const SdfPath displacementMaterialPath = OpCabooseImpl::getAssignment(
        path,
        displacementTarget,
        index,
        isParentImageable
    );

    if (!surfaceMaterialPath.IsEmpty() &&
        !displacementMaterialPath.IsEmpty())
    {
        // We need to assign both surface and displacement. But in Katana
        // there is only one material input. So we can't connect the
        // material, we just output both surface and displacement to this
        // current node and the shader is embedded.
        // TODO: can we reuse GroupBuilder from the matrial we created
        // previously?
        FnAttribute::GroupBuilder materialBuilder;
        OpCabooseImpl::cookShadeMaterial(
            iPrim.GetStage()->GetPrimAtPath(surfaceMaterialPath),
            time,
            materialBuilder);

        FnAttribute::GroupBuilder displacementBuilder;
        OpCabooseImpl::cookShadeMaterial(
            iPrim.GetStage()->GetPrimAtPath(displacementMaterialPath),
            time,
            displacementBuilder);

        oStaticGb.deepUpdate(materialBuilder.build());
        oStaticGb.deepUpdate(displacementBuilder.build());
    }
    else if (!surfaceMaterialPath.IsEmpty())
    {
        const std::string material =
            iPrivateData->root() + surfaceMaterialPath.GetString();

        oStaticGb.set(
            "materialAssign", FnAttribute::StringAttribute(material));
    }
    else if (!displacementMaterialPath.IsEmpty())
    {
        const std::string material =
            iPrivateData->root() + displacementMaterialPath.GetString();

        oStaticGb.set(
            "materialAssign", FnAttribute::StringAttribute(material));
    }

    // walterOverride attributes
    const SdfPath attributePath = OpCabooseImpl::getAssignment(
        path,
        attributeTarget,
        index,
        isParentImageable
    );

    if (!attributePath.IsEmpty())
    {
        const OpIndex::NameToAttribute* attributes =
            index.getAttributes(attributePath);
        if (attributes)
        {
            FnAttribute::GroupBuilder attributesBld;
            for (const auto& attr : *attributes)
            {
                attr.second->evaluate(&attributesBld);
            }
            oStaticGb.deepUpdate(attributesBld.build());
        }
    }
}

/**
 * @brief Output WalterVolume schema to the Katana group builder.
 *
 * @param iPrim The USD prim to output.
 * @param iTime The sample times.
 * @param oStaticGb Output group builder where the new attribute will be
 * created.
 */
void cookWalterVolume(
    const UsdPrim& iPrim,
    const OpUtils::TimePtr iTime,
    FnAttribute::GroupBuilder& oStaticGb)
{
    static const FnAttribute::StringAttribute volumeStrAttr("volume");
    static const FnAttribute::StringAttribute volumeplugStrAttr("volumeplugin");
    static const FnAttribute::FloatAttribute stepSizeStrAttr(0.0f);

    oStaticGb.set("type", volumeStrAttr);
    oStaticGb.set("geometry.type", volumeplugStrAttr);
    oStaticGb.set("geometry.step_size", stepSizeStrAttr);
    oStaticGb.set("rendererProcedural.node", volumeStrAttr);

    // The mapping from USD name to the Katana name.
    static const std::unordered_map<std::string, std::string> attributeMap = {
        {"filename", "rendererProcedural.args.filename"},
        {"grids", "rendererProcedural.args.grids"},
        {"stepScale", "rendererProcedural.args.step_scale"},
        {"stepSize", "rendererProcedural.args.step_size"},
        {"velocityFps", "rendererProcedural.args.velocity_fps"},
        {"velocityOutlierThreshold",
         "rendererProcedural.args.velocity_outlier_threshold"},
        {"velocityScale", "rendererProcedural.args.velocity_scale"},
        {"volumePadding", "rendererProcedural.args.volume_padding"}};

    std::string filename;

    // Iterate all the attributes.
    for (const UsdAttribute& attr : iPrim.GetAttributes())
    {
        std::string baseName = attr.GetBaseName().GetString();
        auto found = attributeMap.find(baseName);
        if (found == attributeMap.end())
        {
            // If it is not in the map, we don't support it.
            continue;
        }

        VtValue vtValue;
        if (!attr.Get(&vtValue, iTime->current()))
        {
            continue;
        }

        if (baseName == "filename")
        {
            filename = vtValue.Get<std::string>();
        }

        oStaticGb.set(
            found->second,
            PxrUsdKatanaUtils::ConvertVtValueToKatAttr(vtValue, true));
    }

#ifdef USE_OPENVDB
    // Ass file doesn't contain the information about bounds. The only way to
    // get it is open VDB file. Since we are going to read metadata only, it
    // should be fast.
    if (!filename.empty())
    {
        static std::atomic_bool sIsInitialized(false);
        if (!sIsInitialized.load())
        {
            openvdb::initialize();
            sIsInitialized = true;
        }

        // Trying to open OpenVDB to get the information about bounding box.
        std::unique_ptr<openvdb::io::File> vdb(new openvdb::io::File(filename));
        if (vdb->open())
        {
            openvdb::math::Vec3d bbMin =
                openvdb::math::Vec3d(std::numeric_limits<double>::max());
            openvdb::math::Vec3d bbMax =
                openvdb::math::Vec3d(-std::numeric_limits<double>::max());

            // Read the metadata of all the grids. It will not load the grid
            // data.
            openvdb::GridPtrVecPtr grids = vdb->readAllGridMetadata();
            for (const auto& grid : *grids)
            {
                const openvdb::Vec3i& bbMinI = grid->metaValue<openvdb::Vec3i>(
                    openvdb::GridBase::META_FILE_BBOX_MIN);
                const openvdb::Vec3i& bbMaxI = grid->metaValue<openvdb::Vec3i>(
                    openvdb::GridBase::META_FILE_BBOX_MAX);

                // Intersect them.
                bbMin = openvdb::math::minComponent(
                    bbMin, grid->indexToWorld(bbMinI));
                bbMax = openvdb::math::maxComponent(
                    bbMax, grid->indexToWorld(bbMaxI));
            }

            // Output to Katana.
            std::vector<double> bound = {bbMin.x(),
                                         bbMax.x(),
                                         bbMin.y(),
                                         bbMax.y(),
                                         bbMin.z(),
                                         bbMax.z()};
            FnKat::DoubleBuilder builder(1);
            builder.set(bound);
            oStaticGb.set("bound", builder.build());
        }
    }
#endif
}

void cookInstance(
    const UsdPrim& iPrim,
    const OpUtils::TimePtr iTime,
    FnAttribute::GroupBuilder& oStaticGb,
    std::string instanceSourceLocation)
{
    OpCabooseImpl::createBound(
        UsdGeomImageable{iPrim}, iTime, oStaticGb, false/*isLocal*/);

    oStaticGb.set("type", FnAttribute::StringAttribute("instance"));
    oStaticGb.set(
        "geometry.instanceSource",
        FnAttribute::StringAttribute(instanceSourceLocation)
    );
}

void cookAsRendererProcedural(
    const UsdPrim& iPrim,
    const OpUtils::TimePtr iTime,
    FnAttribute::GroupBuilder& oStaticGb,
    std::string engineIdentifier)
{
    OpCabooseImpl::createBound(
        UsdGeomImageable{iPrim}, iTime, oStaticGb, true/*isLocal*/);

    oStaticGb.set(
        "type", FnAttribute::StringAttribute("renderer procedural"));
    oStaticGb.set(
        "rendererProcedural.procedural",
        FnAttribute::StringAttribute("walter"));
    oStaticGb.set(
        "rendererProcedural.node", FnAttribute::StringAttribute("walter"));
    oStaticGb.set(
        "rendererProcedural.args.filePaths",
        FnAttribute::StringAttribute(engineIdentifier));
    oStaticGb.set(
        "rendererProcedural.args.objectPath",
        FnAttribute::StringAttribute(iPrim.GetPath().GetString()));
    oStaticGb.set(
        "rendererProcedural.includeCameraInfo",
        FnAttribute::StringAttribute("None"));
    oStaticGb.set(
        "rendererProcedural.args.__outputStyle",
        FnAttribute::StringAttribute("typedArguments"));
    oStaticGb.set(
        "rendererProcedural.args.__skipBuiltins",
        FnAttribute::IntAttribute(1));
    oStaticGb.set(
        "rendererProcedural.args.frame",
        FnAttribute::FloatAttribute(iTime->current()));
}


void setAllAttrs(
    Foundry::Katana::GeolibCookInterface& interface,
    const FnAttribute::GroupAttribute& attrs)
{
    for (int64_t i = 0; i < attrs.getNumberOfChildren(); ++i)
    {
        interface.setAttr(attrs.getChildName(i), attrs.getChildByIndex(i));
    }
}

} // namespace OpCabooseImpl

void OpCaboose::ClientAttribute::evaluate(
    OpCaboose::ClientAttribute::Carrier iCarrier) const
{
    if (mFunction)
    {
        (*mFunction)(iCarrier);
    }
}

bool OpCaboose::isSupported(const UsdPrim& iPrim)
{
    // We check HasAuthoredTypeName because it's possible that the object
    // doesn't have any type but it has children. We want to load children in
    // this way.
    return !iPrim.HasAuthoredTypeName() || iPrim.IsA<UsdGeomXform>() ||
        iPrim.IsA<UsdGeomScope>() || iPrim.IsA<UsdGeomMesh>() ||
        iPrim.IsA<UsdShadeMaterial>() || iPrim.IsA<WalterVolume>() ||
        iPrim.IsA<UsdGeomPointInstancer>();
}

bool OpCaboose::skipChildren(const UsdPrim& iPrim)
{
    return iPrim.IsA<UsdShadeMaterial>() || iPrim.IsA<UsdShadeShader>() ||
        iPrim.IsA<UsdGeomPointInstancer>();
}

void OpCaboose::cook(
    const UsdPrim& iPrim,
    const OpUtils::PrivateData* iPrivateData,
    void* ioClientData)
{
    W_DEBUG("%s %s", iPrim.GetTypeName().GetText(), iPrim.GetPath().GetText());

    assert(ioClientData);

    auto interface =
        reinterpret_cast<Foundry::Katana::GeolibCookInterface*>(ioClientData);

    OpUtils::TimePtr time = iPrivateData->time();
    const SdfPath& path = iPrivateData->path();
    OpIndex& index = iPrivateData->engine().index();

    bool isInstance = iPrim.IsInstance();

    FnAttribute::GroupBuilder staticBld;

    if (iPrim.IsA<UsdGeomImageable>() && iPrim.GetName() != "materials")
    {
        UsdGeomImageable imageable(iPrim);
        if(imageable.ComputeVisibility() == TfToken("invisible"))
        {
            return;
        }

        OpCabooseImpl::cookGeomImageable(iPrim, time, staticBld);

        // Cook Walter Assignment only for primitives that are not instances and
        // not a group of instances.
        // This test make sense as long as groups are homogeneous, i.e. they
        // either contains only instances or only realy geometries but not both.
        UsdPrimSiblingRange children = iPrim.GetChildren();
        if (!isInstance && (children.empty() || !children.front().IsInstance()))
        {
            OpCabooseImpl::cookWalterAssignment(
                iPrim,
                iPrivateData,
                staticBld,
                iPrim.GetParent().IsA<UsdGeomImageable>()
            );
        }
    }

    if (iPrim.IsA<UsdGeomXformable>() && !isInstance)
    {
        // We shouldn't produce the transform for instances because we use the
        // procedural to render them and the procedural will take care about the
        // transforms.
        OpCabooseImpl::cookGeomXformable(iPrim, time, staticBld);
    }

    if (iPrim.IsA<UsdGeomMesh>())
    {
        OpCabooseImpl::cookGeomMesh(iPrim, time, staticBld);
    }
    else if (iPrim.IsA<UsdShadeMaterial>())
    {
        OpCabooseImpl::cookShadeMaterial(iPrim, time, staticBld);
    }
    else if (iPrim.IsA<WalterVolume>())
    {
        OpCabooseImpl::cookWalterVolume(iPrim, time, staticBld);
    }
    else if (iPrim.IsA<UsdGeomPointInstancer>())
    {
        OpCabooseImpl::cookAsRendererProcedural(
            iPrim,
            time,
            staticBld,
            iPrivateData->engine().getIdentifier()
        );
    }
    else if (isInstance)
    {
        auto cookInstancesAttr = interface->getOpArg("cookInstances");
        int cookInstances =
            FnAttribute::IntAttribute(cookInstancesAttr).getValue(0, false);

        // Cook instances as Katana instances.
        // Note: Because of some bugs with the USD Arnold Procedural, instances are
        //       always expanded as Katana instances (cookInstances=True).
        //       cf. RND-661
        if (cookInstances)
        {
            std::string masterName =
                iPrivateData->engine().getKatMasterName(iPrim.GetMaster());

            std::string katMasterLocation =
                interface->getRootLocationPath() + "/" + masterName;

            OpCabooseImpl::cookGeomXformable(iPrim, time, staticBld);
            OpCabooseImpl::cookInstance(
                iPrim,
                time,
                staticBld,
                katMasterLocation
            );
        }

        // Cook instances as renderer procedurals.
        else
        {
            OpCabooseImpl::cookAsRendererProcedural(
                iPrim,
                time,
                staticBld,
                iPrivateData->engine().getIdentifier()
            );
        }
    }
    else if (iPrim.IsMaster())
    {
        staticBld.set("type", FnAttribute::StringAttribute("instance source"));
    }
    else
    {
        // Set the default node type. By default it's a group.
        staticBld.set("type", FnAttribute::StringAttribute("group"));
    }

    OpCabooseImpl::setAllAttrs(*interface, staticBld.build());

    // Check if we need to set Arnold defaults.
    // TODO: We should do it on the root only.
    FnAttribute::StringAttribute addDefaultAttrs =
        interface->getOpArg("addDefaultAttrs");
    if (addDefaultAttrs.getValue("Arnold", false) == "Arnold")
    {
        // We need smoothing because Arnold doesn't read normals it this
        // attribute is off.
        interface->setAttr(
            "arnoldStatements.smoothing", FnAttribute::IntAttribute(1));
    }
}

void OpCaboose::cookChild(
    const UsdPrim& iPrim,
    const OpUtils::PrivateData* iPrivateData,
    void* ioClientData,
    std::string iCustomName)
{
    assert(ioClientData);

    auto interface =
        reinterpret_cast<Foundry::Katana::GeolibCookInterface*>(ioClientData);

    FnAttribute::GroupAttribute childOpArgs = interface->getOpArg();

    SdfPath path = iPrim.GetPath();
    std::string name = (iCustomName != "")? iCustomName: path.GetName();

    auto privateData = new OpUtils::PrivateData(
        iPrivateData->engine(),
        iPrivateData->root(),
        path,
        iPrivateData->time());

    interface->createChild(
        name,
        "WalterInUSD",
        childOpArgs,
        Foundry::Katana::GeolibCookInterface::ResetRootFalse,
        privateData,
        OpUtils::PrivateData::kill);
}

OpCaboose::ClientAttributePtr OpCaboose::createClientAttribute(
    const UsdAttribute& iAttr,
    UsdTimeCode iTime)
{
    VtValue vtValue;
    if (!iAttr.Get(&vtValue, iTime))
    {
        return {};
    }

    std::string attributeName = iAttr.GetBaseName();
    std::string attributeKatanaName =
        OpUtils::getClientAttributeName(attributeName);

    FnAttribute::Attribute attr;

    if (!attributeKatanaName.empty())
    {
        // It's arnold attribute, and should be represented in Katana as a
        // single atribute in arnoldStatements group.
        attr = PxrUsdKatanaUtils::ConvertVtValueToKatAttr(vtValue, false);
    }
    else
    {
        // Since it doesn't have a special case name, it's a custom attribute.
        // We need to create a special description in "geometry.arbitrary"
        // group.
        static const std::string arbitrary = "geometry.arbitrary.";
        attributeKatanaName = arbitrary + iAttr.GetBaseName().GetString();

        int elementSize = 1;
        iAttr.GetMetadata(UsdGeomTokens->elementSize, &elementSize);

        TfToken interpolation;
        if (!iAttr.GetMetadata(UsdGeomTokens->interpolation, &interpolation))
        {
            interpolation = UsdGeomTokens->constant;
        }

        FnAttribute::StringAttribute scopeAttr =
            OpCabooseImpl::interpolationToScope(interpolation);

        VtIntArray vtIndices;
        attr = OpCabooseImpl::convertVtValueToArbitrary(
            vtValue,
            vtIndices,
            elementSize,
            iAttr.GetTypeName(),
            interpolation,
            scopeAttr);
    }

    if (!attr.isValid())
    {
        return {};
    }

    auto function =
        std::make_shared<OpCaboose::ClientAttribute::AttributeSetFn>(std::bind(
            &OpCabooseImpl::setGroupBuilder,
            std::placeholders::_1,
            attributeKatanaName,
            attr));
    return std::make_shared<OpCaboose::ClientAttribute>(function);
}
