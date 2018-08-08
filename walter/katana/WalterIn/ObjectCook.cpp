#include "AbcCook.h"
#include "ArrayPropUtils.h"
#include "ScalarPropUtils.h"
#include "ArbitraryGeomParamUtils.h"


namespace WalterIn
{

bool isBoundBox(const Alembic::Abc::PropertyHeader & iPropHeader)
{

    if (iPropHeader.getDataType().getExtent() != 6)
    {
        return false;
    }

    Alembic::Util::PlainOldDataType podType =
        iPropHeader.getDataType().getPod();
    if (podType != Alembic::Util::kFloat64POD &&
        podType != Alembic::Util::kFloat32POD)
    {
        return false;
    }

    std::string interp = iPropHeader.getMetaData().get("interpretation");
    if (interp != "box")
    {
        return false;
    }

    return true;
}

int64_t getTupleSize(const Alembic::AbcCoreAbstract::PropertyHeader & iHeader)
{
    int64_t extent = iHeader.getDataType().getExtent();
    if (iHeader.getMetaData().get("interpretation") == "box")
    {
        if (extent == 6)
        {
            extent = 3;
        }
        else if (extent == 4)
        {
            extent = 2;
        }
    }

    if (extent == 0)
    {
        extent = 1;
    }

    return extent;
}

void evalObject(AbcCookPtr ioCookPtr, const OpArgs & iArgs,
                FnAttribute::GroupBuilder & oGb,
                Alembic::Abc::IUInt64ArrayProperty * iIdsProperty)
{

    if (ioCookPtr->visProp.valid())
    {
        Alembic::Util::int8_t visValue = -1;
        Alembic::Abc::ISampleSelector ss(iArgs.getAbcFrameTime());
        ioCookPtr->visProp.get(visValue, ss);
        if (visValue > -1)
        {
            oGb.set("visible", FnAttribute::IntAttribute(visValue));
        }
    }

    if (!ioCookPtr->objPtr)
    {
        return;
    }

    std::vector< ArrayProp >::iterator at = ioCookPtr->arrayProps.begin();
    for (; at != ioCookPtr->arrayProps.end(); ++at)
    {
        arrayPropertyToAttr(*at, iArgs, oGb, iIdsProperty);
    }

    std::vector< ScalarProp >::iterator st = ioCookPtr->scalarProps.begin();
    for (; st != ioCookPtr->scalarProps.end(); ++st)
    {
        scalarPropertyToAttr(*st, iArgs, oGb);
    }

    std::vector< IndexedGeomParamPair >::iterator it =
        ioCookPtr->forcedExpandProps.begin();
    for (; it != ioCookPtr->forcedExpandProps.end(); ++it)
    {
        indexedParamToAttr(*it, iArgs, oGb);
    }

    Alembic::AbcGeom::IXformPtr xformPtr = Alembic::Util::dynamic_pointer_cast<
        Alembic::AbcGeom::IXform, Alembic::Abc::IObject >(ioCookPtr->objPtr);
    if (xformPtr)
    {
        evalXform(xformPtr->getSchema(), iArgs, oGb);
        return;
    }

    Alembic::AbcGeom::INuPatchPtr patchPtr =
        Alembic::Util::dynamic_pointer_cast<Alembic::AbcGeom::INuPatch,
            Alembic::Abc::IObject >(ioCookPtr->objPtr);
    if (patchPtr)
    {
        evalNuPatch(patchPtr->getSchema(), iArgs, false, oGb);
        return;
    }

    Alembic::AbcGeom::ICurvesPtr curvesPtr =
        Alembic::Util::dynamic_pointer_cast<Alembic::AbcGeom::ICurves,
            Alembic::Abc::IObject >(ioCookPtr->objPtr);
    if (curvesPtr)
    {
        evalCurves(curvesPtr->getSchema(), iArgs, oGb);
        return;
    }

    Alembic::AbcGeom::ICameraPtr cameraPtr =
        Alembic::Util::dynamic_pointer_cast<Alembic::AbcGeom::ICamera,
            Alembic::Abc::IObject >(ioCookPtr->objPtr);
    if (cameraPtr)
    {
        evalCamera(cameraPtr->getSchema(), iArgs, oGb);
        return;
    }
}

// fills in the static group, and the animated array and static props
void initAbcCook(AbcCookPtr ioCookPtr,
                 FnAttribute::GroupBuilder & oStaticGb)
{

    const Alembic::AbcCoreAbstract::ObjectHeader & header =
        ioCookPtr->objPtr->getHeader();

    Alembic::AbcGeom::IVisibilityProperty visProp =
        Alembic::AbcGeom::GetVisibilityProperty(*ioCookPtr->objPtr);

    if (visProp.valid() && visProp.isConstant())
    {
        Alembic::Util::int8_t visValue = -1;
        visProp.get(visValue);
        if (visValue > -1)
        {
            oStaticGb.set("visible", FnAttribute::IntAttribute(visValue));
        }
    }
    else
    {
        if (visProp.valid())
            ioCookPtr->animatedSchema = true;
        ioCookPtr->visProp = visProp;
    }

    if (Alembic::AbcGeom::IXform::matches(header))
    {
        cookXform(ioCookPtr, oStaticGb);
    }
    else if (Alembic::AbcGeom::IPolyMesh::matches(header))
    {
        cookPolyMesh(ioCookPtr, oStaticGb);
    }
    else if (Alembic::AbcGeom::ISubD::matches(header))
    {
        cookSubd(ioCookPtr, oStaticGb);
    }
    else if (Alembic::AbcGeom::ICamera::matches(header))
    {
        cookCamera(ioCookPtr, oStaticGb);
    }
    else if (Alembic::AbcGeom::IFaceSet::matches(header))
    {
        cookFaceset(ioCookPtr, oStaticGb);
    }
    else if (Alembic::AbcGeom::ICurves::matches(header))
    {
        cookCurves(ioCookPtr, oStaticGb);
    }
    else if (Alembic::AbcGeom::INuPatch::matches(header))
    {
        cookNuPatch(ioCookPtr, oStaticGb);
    }
    else if (Alembic::AbcGeom::IPoints::matches(header))
    {
        cookPoints(ioCookPtr, oStaticGb);
    }
    else if (Alembic::AbcMaterial::IMaterial::matches(header))
    {
        cookMaterial(ioCookPtr, oStaticGb);
    }
    else if (Alembic::AbcGeom::ILight::matches(header))
    {
        cookLight(ioCookPtr, oStaticGb);
    }
    else
    {
        // just set this up as a group, we'll ignore all the properties for now
        oStaticGb.set("type", FnAttribute::StringAttribute("group"));
    }
}

namespace
{
    std::string safeAttrName(const std::string & name)
    {
        std::string result = name;

        for (size_t i = 0; i < result.size(); ++i)
        {
            if (result[i] == '.')
            {
                result[i] = '_';
            }
        }

        return result;
    }
}

void processUserProperties(AbcCookPtr ioCook,
    Alembic::Abc::ICompoundProperty & iParent,
    FnAttribute::GroupBuilder & oStaticGb,
    const std::string & iAttrPath,
    const NameMap* iRenameMap,
    Alembic::Abc::v10::IUInt64ArrayProperty * iIdsProperty)
{
    if (!iParent.valid())
    {
        return;
    }

    for (size_t i = 0; i < iParent.getNumProperties(); ++i)
    {
        const Alembic::AbcCoreAbstract::PropertyHeader &propHeader =
            iParent.getPropertyHeader(i);

        // Check meta. We need only Arnold official data.
        const auto& meta = propHeader.getMetaData();
        const std::string arnoldUserData = meta.get("userData");
        if (arnoldUserData == "yes")
        {
            continue;
        }


        std::string propName = iAttrPath + safeAttrName(propHeader.getName());

        // Rename attribute if we have a map for it.
        if (iRenameMap)
        {
            NameMap::const_iterator it = iRenameMap->find(propName);
            if (it != iRenameMap->end())
            {
                propName = it->second;
            }
        }

        //recurse if it's a compound
        if (propHeader.isCompound())
        {
            Alembic::Abc::ICompoundProperty childCompound(iParent,
                propHeader.getName());

            if (childCompound.valid())
            {
                processUserProperties(ioCook, childCompound, oStaticGb,
                    propName+".", nullptr, iIdsProperty);
            }
        }
        // It may be important to test if it's a Compound first in case
        // visibility has been activated or deactivated in a more precise way
        // than just "visibility=<bool>".
        else if (propName == "visibility")
        {
            setArnoldVisibility(ioCook, iParent, propHeader,
                propName, oStaticGb);
        }
        else if (propHeader.isScalar())
        {
            scalarPropertyToAttr(iParent, propHeader, propName,
                                 ioCook, oStaticGb);
        }
        else if (propHeader.isArray())
        {
            arrayPropertyToAttr(iParent, propHeader, propName,
                                FnAttribute::NullAttribute::getKatAttributeType(),
                                ioCook, oStaticGb, iIdsProperty);
        }
    }
}

void setArnoldVisibility(
    AbcCookPtr ioCook,
    Alembic::Abc::ICompoundProperty & iParent,
    const Alembic::AbcCoreAbstract::PropertyHeader & iPropHeader,
    const std::string & iPropName,
    FnAttribute::GroupBuilder & oStaticGb)
{
    // All those attributes are different ways to enable/disable visibility.
    // If visibility change, we need to reflect it on each of those.
    static std::array<std::string, 7> visibilityAttributes{
        "AI_RAY_CAMERA",
        "AI_RAY_SHADOW",
        "AI_RAY_DIFFUSE_TRANSMIT",
        "AI_RAY_SPECULAR_TRANSMIT",
        "AI_RAY_VOLUME",
        "AI_RAY_DIFFUSE_REFLECT",
        "AI_RAY_SPECULAR_REFLECT"
    };

    for (auto attribute: visibilityAttributes)
    {
        std::string attributeName = iPropName + "." + attribute;
        scalarPropertyToAttr(iParent, iPropHeader, attributeName,
            ioCook, oStaticGb);
    }
}

void processArnoldUserProperties(
        AbcCookPtr ioCook,
        Alembic::Abc::ICompoundProperty & iParent,
        FnAttribute::GroupBuilder & oStaticGb,
        const std::string & iAttrPath)
{

    if (!iParent.valid())
    {
        return;
    }

    for (size_t i = 0; i < iParent.getNumProperties(); ++i)
    {
        const Alembic::AbcCoreAbstract::PropertyHeader &propHeader =
            iParent.getPropertyHeader(i);

        if (!propHeader.isScalar())
        {
            continue;
        }

        // Check meta. We need only Arnold UserData.
        const auto& meta = propHeader.getMetaData();
        const std::string arnoldUserData = meta.get("userData");
        if (arnoldUserData != "yes")
        {
            continue;
        }

        // Check type. We only support color, int and float.
        std::string type;
        if (Alembic::AbcGeom::IFloatProperty::matches(propHeader))
        {
            type = "float";
        }
        else if (Alembic::AbcGeom::IInt32Property::matches(propHeader))
        {
            type = "int";
        }
        else if (Alembic::AbcGeom::IV3fProperty::matches(propHeader))
        {
            type = "color3";
        }
        else if (Alembic::AbcGeom::IStringProperty::matches(propHeader))
        {
            type = "string";
        }
        else
        {
            continue;
        }

        std::string propName =
            iAttrPath + safeAttrName(propHeader.getName());

        scalarPropertyToAttr(
                iParent,
                propHeader,
                propName + ".value",
                ioCook,
                oStaticGb);

        oStaticGb.set(
                propName + ".scope", FnAttribute::StringAttribute("primitive"));
        oStaticGb.set(
                propName + ".inputType",
                FnAttribute::StringAttribute(type));
        oStaticGb.set(
                propName + ".outputType",
                FnAttribute::StringAttribute(type));
    }
}

Alembic::Util::PlainOldDataType FnAttrTypeToPODType(FnKatAttributeType iType)
{
    if (iType == kFnKatAttributeTypeInt)
    {
        return Alembic::Util::kInt32POD;
    }
    else if (iType == kFnKatAttributeTypeFloat)
    {
        return Alembic::Util::kFloat32POD;
    }
    else if (iType == kFnKatAttributeTypeDouble)
    {
        return Alembic::Util::kFloat64POD;
    }
    else if (iType == kFnKatAttributeTypeString)
    {
        return Alembic::Util::kStringPOD;
    }

    return Alembic::Util::kUnknownPOD;
}

} //end of namespace WalterIn

