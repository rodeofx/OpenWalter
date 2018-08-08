#include "AbcCook.h"
#include "ArrayPropUtils.h"
#include "ScalarPropUtils.h"
#include "ArbitraryGeomParamUtils.h"

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnDataBuilder.h>

namespace WalterIn
{

    void evalNuPatch(Alembic::AbcGeom::INuPatchSchema & iSchema,
                     const OpArgs & iArgs,
                     bool iIgnoreConstant,
                     FnAttribute::GroupBuilder & oGb)
    {

        Alembic::Abc::IP3fArrayProperty pointsProp =
            iSchema.getPositionsProperty();

        Alembic::Abc::IFloatArrayProperty weightsProp =
            iSchema.getPositionWeightsProperty();

        Alembic::Abc::TimeSamplingPtr ts = iSchema.getTimeSampling();
        SampleTimes sampleTimes;
        iArgs.getRelevantSampleTimes(ts, iSchema.getNumSamples(), sampleTimes);

        if (!iIgnoreConstant || !pointsProp.isConstant() ||
            !weightsProp.isConstant())
        {
            FnAttribute::FloatBuilder pwBuilder(4);

            for (SampleTimes::iterator it = sampleTimes.begin();
                it != sampleTimes.end(); ++it)
            {
                Alembic::Util::Dimensions dims;
                Alembic::Abc::ISampleSelector ss(*it);
                pointsProp.getDimensions(dims, ss);
                size_t numPoints = dims.numPoints();

                size_t numWeights = 0;
                if (weightsProp.valid())
                {
                    weightsProp.getDimensions(dims, ss);
                    numWeights = dims.numPoints();
                }

                std::vector< float > pointsVal;
                std::vector< float > weightsVal;

                // make it bigger than what we need
                pointsVal.resize(numPoints * 4);
                if (numPoints != 0)
                {
                    pointsProp.getAs(&(pointsVal.front()), ss);
                }

                if (numPoints == numWeights && numPoints != 0)
                {
                    weightsVal.resize(numPoints);
                    weightsProp.getAs(&(weightsVal.front()), ss);
                }

                for (std::size_t i = numPoints; i > 0; --i)
                {
                    if (weightsVal.empty())
                    {
                        pointsVal[(i - 1) * 4 + 3] = 1.0f;
                    }
                    else
                    {
                        pointsVal[(i - 1) * 4 + 3] = weightsVal[i - 1];
                    }
                    pointsVal[(i - 1) * 4 + 2] = pointsVal[(i - 1) * 3 + 2];
                    pointsVal[(i - 1) * 4 + 1] = pointsVal[(i - 1) * 3 + 1];
                    pointsVal[(i - 1) * 4    ] = pointsVal[(i - 1) * 3    ];
                }

                if (sampleTimes.size() == 1)
                {
                    // hopefully this will use a fancy attr
                    oGb.set("geometry.point.Pw",
                            FnAttribute::FloatAttribute(&(pointsVal.front()),
                                                        pointsVal.size(), 4));
                }
                else
                {
                    pwBuilder.set(pointsVal, iArgs.getRelativeSampleTime(*it));
                }
            }
            if (sampleTimes.size() > 1)
            {
                oGb.set("geometry.point.Pw", pwBuilder.build());
            }
        }

        // handle u.min, u.max, v.min, v.max and the knots themselves
        Alembic::Abc::IFloatArrayProperty uknotProp =
            iSchema.getUKnotsProperty();
        if (!iIgnoreConstant || !uknotProp.isConstant())
        {
            FnAttribute::FloatBuilder uBuilder;
            FnAttribute::FloatBuilder umaxBuilder;
            for (SampleTimes::iterator it = sampleTimes.begin();
                it != sampleTimes.end(); ++it)
            {
                Alembic::Abc::ISampleSelector ss(*it);
                Alembic::Util::Dimensions dims;
                uknotProp.getDimensions(dims, ss);
                size_t numU = dims.numPoints();
                std::vector< float > uVal(numU);
                if (numU != 0)
                {
                    uknotProp.getAs(&(uVal.front()), ss);
                    umaxBuilder.push_back(uVal[uVal.size() - 1],
                        iArgs.getRelativeSampleTime(*it));
                }
                uBuilder.set(uVal, iArgs.getRelativeSampleTime(*it));
            }
            oGb.set("geometry.u.knots", uBuilder.build());
            oGb.set("geometry.u.min", FnAttribute::FloatAttribute(0.0));
            oGb.set("geometry.u.max", umaxBuilder.build());
        }

        Alembic::Abc::IFloatArrayProperty vknotProp =
            iSchema.getVKnotsProperty();
        if (!iIgnoreConstant || !vknotProp.isConstant())
        {
            FnAttribute::FloatBuilder vBuilder;
            FnAttribute::FloatBuilder vmaxBuilder;
            for (SampleTimes::iterator it = sampleTimes.begin();
                it != sampleTimes.end(); ++it)
            {
                Alembic::Abc::ISampleSelector ss(*it);
                Alembic::Util::Dimensions dims;
                vknotProp.getDimensions(dims, ss);
                size_t numV = dims.numPoints();
                std::vector< float > vVal(numV);
                if (numV != 0)
                {
                    vknotProp.getAs(&(vVal.front()), ss);
                    vmaxBuilder.push_back(vVal[vVal.size() - 1],
                                    iArgs.getRelativeSampleTime(*it));
                }
                vBuilder.set(vVal, iArgs.getRelativeSampleTime(*it));
            }
            oGb.set("geometry.v.knots", vBuilder.build());
            oGb.set("geometry.v.min", FnAttribute::FloatAttribute(0.0));
            oGb.set("geometry.v.max", vmaxBuilder.build());
        }
    }

    void cookNuPatch(AbcCookPtr ioCook, FnAttribute::GroupBuilder & oStaticGb)
    {
        Alembic::AbcGeom::INuPatchPtr objPtr(
            new Alembic::AbcGeom::INuPatch(*(ioCook->objPtr),
                                           Alembic::AbcGeom::kWrapExisting));

        Alembic::AbcGeom::INuPatchSchema schema = objPtr->getSchema();

        oStaticGb.set("type", FnAttribute::StringAttribute("nurbspatch"));

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

            if (scope == Alembic::AbcGeom::kVertexScope)
            {
                processArbitraryGeomParam(ioCook, schema, normProp.getHeader(),
                    oStaticGb, "geometry.point.N");
            }
            else
            {
                processArbitraryGeomParam(ioCook, schema, normProp.getHeader(),
                    oStaticGb, "geometry.arbitrary.N");
            }
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

        const Alembic::AbcCoreAbstract::PropertyHeader * propHeader;

        // uOrder, vOrder, nu, nv don't have getters so we'll do it directly
        propHeader = schema.getPropertyHeader("uOrder");
        if (propHeader != NULL)
        {
            scalarPropertyToAttr(schema, *propHeader, "geometry.u.order",
                                 ioCook, oStaticGb);
        }

        propHeader = schema.getPropertyHeader("vOrder");
        if (propHeader != NULL)
        {
            scalarPropertyToAttr(schema, *propHeader, "geometry.v.order",
                                 ioCook, oStaticGb);
        }

        propHeader = schema.getPropertyHeader("nu");
        if (propHeader != NULL)
        {
            scalarPropertyToAttr(schema, *propHeader, "geometry.uSize",
                                 ioCook, oStaticGb);
        }

        propHeader = schema.getPropertyHeader("nv");
        if (propHeader != NULL)
        {
            scalarPropertyToAttr(schema, *propHeader, "geometry.vSize",
                                 ioCook, oStaticGb);
        }

        // trim stuff, INuPatch currently doesn't have getters for this, so
        // we will try to get it directly
        propHeader = schema.getPropertyHeader("trim_n");
        if (propHeader != NULL)
        {
            arrayPropertyToAttr(schema, *propHeader,
                "geometry.trimCurves.trim_n", kFnKatAttributeTypeInt,
                ioCook, oStaticGb);
        }

        propHeader = schema.getPropertyHeader("trim_ncurves");
        if (propHeader != NULL)
        {
            arrayPropertyToAttr(schema, *propHeader,
                "geometry.trimCurves.trim_ncurves", kFnKatAttributeTypeInt,
                ioCook, oStaticGb);
        }

        propHeader = schema.getPropertyHeader("trim_order");
        if (propHeader != NULL)
        {
            arrayPropertyToAttr(schema, *propHeader,
                "geometry.trimCurves.trim_order", kFnKatAttributeTypeInt,
                ioCook, oStaticGb);
        }

        // read all of these as doubles
        propHeader = schema.getPropertyHeader("trim_knot");
        if (propHeader != NULL)
        {
            arrayPropertyToAttr(schema, *propHeader,
                "geometry.trimCurves.trim_knot", kFnKatAttributeTypeDouble,
                ioCook, oStaticGb);
        }

        propHeader = schema.getPropertyHeader("trim_min");
        if (propHeader != NULL)
        {
            arrayPropertyToAttr(schema, *propHeader,
                "geometry.trimCurves.trim_min", kFnKatAttributeTypeDouble,
                ioCook, oStaticGb);
        }

        propHeader = schema.getPropertyHeader("trim_max");
        if (propHeader != NULL)
        {
            arrayPropertyToAttr(schema, *propHeader,
                "geometry.trimCurves.trim_max", kFnKatAttributeTypeDouble,
                ioCook, oStaticGb);
        }

        propHeader = schema.getPropertyHeader("trim_u");
        if (propHeader != NULL)
        {
            arrayPropertyToAttr(schema, *propHeader,
                "geometry.trimCurves.trim_u", kFnKatAttributeTypeDouble,
                ioCook, oStaticGb);
        }

        propHeader = schema.getPropertyHeader("trim_v");
        if (propHeader != NULL)
        {
            arrayPropertyToAttr(schema, *propHeader,
                "geometry.trimCurves.trim_v", kFnKatAttributeTypeDouble,
                ioCook, oStaticGb);
        }

        propHeader = schema.getPropertyHeader("trim_w");
        if (propHeader != NULL)
        {
            arrayPropertyToAttr(schema, *propHeader,
                "geometry.trimCurves.trim_w", kFnKatAttributeTypeDouble,
                ioCook, oStaticGb);
        }

        if (!schema.isConstant())
        {
            ioCook->objPtr = objPtr;
            ioCook->animatedSchema = true;
        }
        else
        {
            OpArgs defaultArgs;
            evalNuPatch(schema, defaultArgs, false, oStaticGb);
        }
    }
}

