#include "AbcCook.h"
#include "ArrayPropUtils.h"
#include "ScalarPropUtils.h"
#include "ArbitraryGeomParamUtils.h"

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnDataBuilder.h>

namespace WalterIn
{
    void cookPolyMesh(AbcCookPtr ioCook, FnAttribute::GroupBuilder & oStaticGb)
    {
        Alembic::AbcGeom::IPolyMeshPtr meshPtr(
            new Alembic::AbcGeom::IPolyMesh(*(ioCook->objPtr),
                                            Alembic::AbcGeom::kWrapExisting));

        Alembic::AbcGeom::IPolyMeshSchema schema = meshPtr->getSchema();
        oStaticGb.set("type", FnAttribute::StringAttribute("polymesh"));

        Alembic::Abc::ICompoundProperty userProp = schema.getUserProperties();
        Alembic::Abc::ICompoundProperty arbGeom = schema.getArbGeomParams();
        std::string abcUser = "abcUser.";
        processUserProperties(ioCook, userProp, oStaticGb, abcUser);
        processArbGeomParams(ioCook, arbGeom, oStaticGb);

        Alembic::AbcGeom::IV2fGeomParam uvsProp = schema.getUVsParam();
        if (uvsProp.valid())
        {
            processArbitraryGeomParam(ioCook, schema, uvsProp.getHeader(),
                oStaticGb, "geometry.arbitrary.st");
        }

        Alembic::AbcGeom::IN3fGeomParam normProp = schema.getNormalsParam();
        if (normProp.valid())
        {
            Alembic::AbcGeom::GeometryScope scope = normProp.getScope();

            if (scope == Alembic::AbcGeom::kVertexScope ||
                scope == Alembic::AbcGeom::kVaryingScope)
            {
                processArbitraryGeomParam(ioCook, schema, normProp.getHeader(),
                    oStaticGb, "geometry.point.N");
            }
            else if (scope == Alembic::AbcGeom::kFacevaryingScope)
            {
                processArbitraryGeomParam(ioCook, schema, normProp.getHeader(),
                    oStaticGb, "geometry.vertex.N");
            }
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
            scalarPropertyToAttr(schema, boundsProp.getHeader(), "bound",
                                 ioCook, oStaticGb);
        }
    }

} //end of namespace WalterIn
