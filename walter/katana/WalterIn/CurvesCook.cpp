#include "AbcCook.h"
#include "ArrayPropUtils.h"
#include "ScalarPropUtils.h"
#include "ArbitraryGeomParamUtils.h"

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnDataBuilder.h>

namespace WalterIn
{
    void evalCurves(Alembic::AbcGeom::ICurvesSchema & iSchema,
               const OpArgs & iArgs,
               FnAttribute::GroupBuilder & oGb)
    {
        const Alembic::AbcCoreAbstract::PropertyHeader * header =
            iSchema.getPropertyHeader("curveBasisAndType");

        if (header != NULL)
        {
            Alembic::Abc::IScalarProperty basisAndTypeProp(iSchema,
                "curveBasisAndType");
            Alembic::Util::uint8_t data[4];
            Alembic::Abc::ISampleSelector ss(iArgs.getAbcFrameTime());
            basisAndTypeProp.get(data, ss);
            Alembic::AbcGeom::CurveType curveType =
                static_cast<Alembic::AbcGeom::CurveType>( data[0] );
            Alembic::AbcGeom::CurvePeriodicity wrap =
                static_cast<Alembic::AbcGeom::CurvePeriodicity>( data[1] );
            if (curveType == Alembic::AbcGeom::kLinear)
            {
                oGb.set("geometry.degree", FnAttribute::IntAttribute(1));
            }
            else if (curveType == Alembic::AbcGeom::kCubic)
            {
                oGb.set("geometry.degree", FnAttribute::IntAttribute(3));
            }
            else
            {
                Alembic::Abc::IUcharArrayProperty orderProp =
                    iSchema.getOrdersProperty();
                std::vector< FnAttribute::IntAttribute::value_type > degreeVec;

                if (orderProp.valid())
                {
                    Alembic::Util::Dimensions dims;
                    orderProp.getDimensions(dims, ss);
                    degreeVec.resize(dims.numPoints());
                    orderProp.getAs(degreeVec.data(), FnAttrTypeToPODType(
                        FnAttribute::IntAttribute::getKatAttributeType()), ss);
                }

                // it is stored as order so we need to subtract 1 to get degree
                for (std::size_t i = 0; i < degreeVec.size(); ++i)
                {
                    if (degreeVec[i] > 1)
                    {
                        degreeVec[i] -= 1;
                    }
                    else
                    {
                        degreeVec[i] = 1;
                    }
                }

                // fall back to linear, if there is no data
                if (degreeVec.size() == 0)
                {
                    degreeVec.push_back(1);
                }

                oGb.set("geometry.degree", FnAttribute::IntAttribute(
                    degreeVec.data(), (int64_t)degreeVec.size(), 1));
            }

            if (wrap == Alembic::AbcGeom::kPeriodic)
            {
                oGb.set("geometry.closed", FnAttribute::IntAttribute(1));
            }
            else
            {
                oGb.set("geometry.closed", FnAttribute::IntAttribute(0));
            }
        }
    }

    void cookCurves(AbcCookPtr ioCook, FnAttribute::GroupBuilder & oStaticGb)
    {
        Alembic::AbcGeom::ICurvesPtr objPtr(
            new Alembic::AbcGeom::ICurves(*(ioCook->objPtr),
                                          Alembic::AbcGeom::kWrapExisting));

        Alembic::AbcGeom::ICurvesSchema schema = objPtr->getSchema();

        oStaticGb.set("type", FnAttribute::StringAttribute("curves"));

        Alembic::Abc::ICompoundProperty userProp = schema.getUserProperties();
        Alembic::Abc::ICompoundProperty arbGeom = schema.getArbGeomParams();
        std::string abcUser = "abcUser.";
        processUserProperties(ioCook, userProp, oStaticGb, abcUser);
        processArbGeomParams(ioCook, arbGeom, oStaticGb);

        Alembic::AbcGeom::IV2fGeomParam uvsProp = schema.getUVsParam();
        if (uvsProp.valid())
        {
            processArbitraryGeomParam(ioCook, schema,
                uvsProp.getHeader(), oStaticGb,"geometry.arbitrary.st");
        }

        Alembic::AbcGeom::IN3fGeomParam normProp = schema.getNormalsParam();
        if (normProp.valid())
        {
            Alembic::AbcGeom::GeometryScope scope = normProp.getScope();

            //katana's first-class support for N for curves currently
            //only supports prman "vertex" mapping. Arnold doesn't have
            //meaningful normals support for curves at the moment so
            //it has to be arbitrary
            if (scope == Alembic::AbcGeom::kVertexScope)
            {
                processArbitraryGeomParam(ioCook, schema,
                    normProp.getHeader(), oStaticGb,"geometry.point.N");
            }
            else
            {
                processArbitraryGeomParam(ioCook, schema, normProp.getHeader(),
                    oStaticGb, "geometry.arbitrary.N");
            }
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

        Alembic::Abc::IInt32ArrayProperty numVertsProp =
            schema.getNumVerticesProperty();
        if (numVertsProp.valid())
        {
            arrayPropertyToAttr(schema, numVertsProp.getHeader(),
                "geometry.numVertices", kFnKatAttributeTypeInt,
                ioCook, oStaticGb);
        }

        Alembic::Abc::IFloatArrayProperty knotsProp = schema.getKnotsProperty();
        if (knotsProp.valid())
        {
            arrayPropertyToAttr(schema, knotsProp.getHeader(),
                "geometry.knots", kFnKatAttributeTypeFloat,
                ioCook, oStaticGb);
        }
        else
        {
            // katana's viewer has historically looked for knots
            oStaticGb.set("geometry.knots", FnAttribute::FloatAttribute(0.f));
        }

        Alembic::AbcGeom::IFloatGeomParam widthsProp = schema.getWidthsParam();
        if (widthsProp.valid())
        {
            Alembic::AbcGeom::GeometryScope scope = widthsProp.getScope();

            if (scope == Alembic::AbcGeom::kConstantScope)
            {
                Alembic::Abc::IFloatArrayProperty widthsValueProp =
                    widthsProp.getValueProperty();
                arrayPropertyToAttr(schema, widthsValueProp.getHeader(),
                    "geometry.constantWidth", kFnKatAttributeTypeFloat,
                    ioCook, oStaticGb);
            }
            else if (scope == Alembic::AbcGeom::kVertexScope)
            {
                Alembic::Abc::IFloatArrayProperty widthsValueProp =
                    widthsProp.getValueProperty();
                arrayPropertyToAttr(schema, widthsValueProp.getHeader(),
                    "geometry.point.width", kFnKatAttributeTypeFloat,
                    ioCook, oStaticGb);
            }
            else
            {
                processArbitraryGeomParam(ioCook, schema,
                    widthsProp.getHeader(), oStaticGb,
                    "geometry.arbitrary.width");
            }
        }
        // set a default constantWidth
        else
        {
            oStaticGb.set("geometry.constantWidth",
                FnAttribute::FloatAttribute(0.1f));
        }

        // let's hack our way to the degree and closed values until
        // ICurves has a convenience function not on the sample to get it
        Alembic::Abc::IScalarProperty basisAndTypeProp(schema,
                                                       "curveBasisAndType");
        if (basisAndTypeProp.valid() && basisAndTypeProp.isConstant())
        {
            OpArgs defaultArgs;
            evalCurves(schema, defaultArgs, oStaticGb);
        }
        else if (basisAndTypeProp.valid())
        {
            // it's animated, gotta keep it around for further evaluation
            // since it creates more than 1 attr
            ioCook->objPtr = objPtr;
            ioCook->animatedSchema = true;
        }
    }

}

