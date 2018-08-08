#include "AbcCook.h"
#include "ArrayPropUtils.h"
#include "ScalarPropUtils.h"
#include "ArbitraryGeomParamUtils.h"

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnDataBuilder.h>

namespace WalterIn
{

    void cookSubd(AbcCookPtr ioCook, FnAttribute::GroupBuilder & oStaticGb)
    {
        Alembic::AbcGeom::ISubDPtr meshPtr(
            new Alembic::AbcGeom::ISubD(*(ioCook->objPtr),
                                        Alembic::AbcGeom::kWrapExisting));

        Alembic::AbcGeom::ISubDSchema schema = meshPtr->getSchema();

        Alembic::Abc::ICompoundProperty userProp = schema.getUserProperties();
        Alembic::Abc::ICompoundProperty arbGeom = schema.getArbGeomParams();
        std::string abcUser = "abcUser.";
        processUserProperties(ioCook, userProp, oStaticGb, abcUser);
        processArbGeomParams(ioCook, arbGeom, oStaticGb);

        oStaticGb.set("type", FnAttribute::StringAttribute("subdmesh"));

        Alembic::AbcGeom::IV2fGeomParam uvsProp = schema.getUVsParam();
        if (uvsProp.valid())
        {
            processArbitraryGeomParam(ioCook, schema, uvsProp.getHeader(),
                oStaticGb, "geometry.arbitrary.st");
        }

        Alembic::Abc::IInt32ArrayProperty faceProp =
            schema.getFaceCountsProperty();
        if (faceProp.valid())
        {
            arrayPropertyToAttr(schema, faceProp.getHeader(),
                "geometry.poly.startIndex", kFnKatAttributeTypeInt,
                ioCook, oStaticGb);
        }

        Alembic::Abc::IInt32ArrayProperty indexProp =
            schema.getFaceIndicesProperty();
        if (indexProp.valid())
        {
            arrayPropertyToAttr(schema, indexProp.getHeader(),
                "geometry.poly.vertexList", kFnKatAttributeTypeInt,
                ioCook, oStaticGb);
        }

        Alembic::Abc::IP3fArrayProperty pointsProp =
            schema.getPositionsProperty();
        if (pointsProp.valid())
        {
            arrayPropertyToAttr(schema, pointsProp.getHeader(),
                "geometry.point.P", kFnKatAttributeTypeFloat,
                ioCook, oStaticGb);
        }

        Alembic::Abc::IV3fArrayProperty velocProp =
            schema.getVelocitiesProperty();
        if (velocProp.valid())
        {
            arrayPropertyToAttr(schema, velocProp.getHeader(),
                "geometry.point.v", kFnKatAttributeTypeFloat,
                ioCook, oStaticGb);
        }

        Alembic::Abc::IBox3dProperty boundsProp =
            schema.getSelfBoundsProperty();
        if (boundsProp.valid())
        {
            scalarPropertyToAttr(schema, boundsProp.getHeader(),
                                 "bound", ioCook, oStaticGb);
        }

        Alembic::Abc::IInt32Property fvibProp =
            schema.getFaceVaryingInterpolateBoundaryProperty();
        if (fvibProp.valid())
        {
            scalarPropertyToAttr(schema, fvibProp.getHeader(),
                "geometry.facevaryinginterpolateboundary", ioCook, oStaticGb);
        }

        Alembic::Abc::IInt32Property fvpcProp =
            schema.getFaceVaryingPropagateCornersProperty();
        if (fvpcProp.valid())
        {
            scalarPropertyToAttr(schema, fvpcProp.getHeader(),
                "geometry.facevaryingpropagatecorners", ioCook, oStaticGb);
        }

        Alembic::Abc::IInt32Property ibProp =
            schema.getInterpolateBoundaryProperty();
        if (ibProp.valid())
        {
            scalarPropertyToAttr(schema, ibProp.getHeader(),
                "geometry.interpolateBoundary", ioCook, oStaticGb);
        }

        Alembic::Abc::IInt32ArrayProperty holesProp = schema.getHolesProperty();
        if (holesProp.valid())
        {
            arrayPropertyToAttr(schema, holesProp.getHeader(),
                "geometry.holePolyIndices", kFnKatAttributeTypeInt,
                ioCook, oStaticGb);
        }

        Alembic::Abc::IInt32ArrayProperty crlProp =
            schema.getCreaseLengthsProperty();
        if (crlProp.valid())
        {
            arrayPropertyToAttr(schema, crlProp.getHeader(),
                "geometry.creaseLengths", kFnKatAttributeTypeInt,
                ioCook, oStaticGb);
        }

        Alembic::Abc::IInt32ArrayProperty criProp =
            schema.getCreaseIndicesProperty();
        if (criProp.valid())
        {
            arrayPropertyToAttr(schema, criProp.getHeader(),
                "geometry.creaseIndices", kFnKatAttributeTypeInt,
                ioCook, oStaticGb);
        }

        Alembic::Abc::IFloatArrayProperty crsProp =
            schema.getCreaseSharpnessesProperty();
        if (crsProp.valid())
        {
            arrayPropertyToAttr(schema, crsProp.getHeader(),
                "geometry.creaseSharpness", kFnKatAttributeTypeFloat,
                ioCook, oStaticGb);
        }

        Alembic::Abc::IInt32ArrayProperty coiProp =
            schema.getCornerIndicesProperty();
        if (coiProp.valid())
        {
            arrayPropertyToAttr(schema, coiProp.getHeader(),
                "geometry.cornerIndices", kFnKatAttributeTypeInt,
                ioCook, oStaticGb);
        }

        Alembic::Abc::IFloatArrayProperty cosProp =
            schema.getCornerSharpnessesProperty();
        if (cosProp.valid())
        {
            arrayPropertyToAttr(schema, cosProp.getHeader(),
                "geometry.cornerSharpness", kFnKatAttributeTypeFloat,
                ioCook, oStaticGb);
        }
    }

} //end of namespace WalterIn
