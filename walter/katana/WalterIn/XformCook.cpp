#include <sstream>
#include "AbcCook.h"
#include "ArrayPropUtils.h"
#include "ScalarPropUtils.h"
#include "ArbitraryGeomParamUtils.h"

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnDataBuilder.h>

namespace WalterIn
{

///////////////////////////////////////////////////////////////////////////////
void evalXform(Alembic::AbcGeom::IXformSchema & iSchema,
               const OpArgs & iArgs,
               FnAttribute::GroupBuilder & oGb)
{
    //this can't be multi-sampled in katana so we'll query it at the current
    //time and hope for the best.
    bool inheritsXform = iSchema.getInheritsXforms(
        Alembic::Abc::ISampleSelector(iArgs.getAbcFrameTime()));


    Alembic::Abc::TimeSamplingPtr ts = iSchema.getTimeSampling();
    SampleTimes sampleTimes;

    iArgs.getRelevantSampleTimes(ts, iSchema.getNumSamples(), sampleTimes);

    bool multiSample = sampleTimes.size() > 1;

    Alembic::AbcGeom::XformSample referenceOps;
    iSchema.get(referenceOps,
                Alembic::Abc::ISampleSelector(*sampleTimes.begin()));

    typedef std::vector<double> DoubleVector;
    typedef std::map<float, DoubleVector> DoubleAttrSampleMap;

    std::vector<FnAttribute::DoubleBuilder> builders;
    builders.reserve(referenceOps.getNumOps());

    for (SampleTimes::iterator I = sampleTimes.begin();
            I != sampleTimes.end(); ++I)
    {
        Alembic::Abc::chrono_t inputSampleTime = (*I);

        float relativeSampleTime = multiSample ?
            iArgs.getRelativeSampleTime(inputSampleTime) : 0.0f;

        Alembic::AbcGeom::XformSample ops;

        iSchema.get(ops, Alembic::Abc::ISampleSelector(inputSampleTime));

        for (size_t opindex = 0; opindex < ops.getNumOps(); ++opindex)
        {
            const Alembic::AbcGeom::XformOp & xformOp = ops[opindex];

            if (builders.size() <= opindex)
            {
                int tupleSize = (int) xformOp.getNumChannels();

                switch (xformOp.getType())
                {
                    case Alembic::AbcGeom::kRotateXOperation:
                    case Alembic::AbcGeom::kRotateYOperation:
                    case Alembic::AbcGeom::kRotateZOperation:
                        tupleSize = 4;
                    break;

                    case Alembic::AbcGeom::kMatrixOperation:
                        tupleSize = 4;
                    break;

                    default:
                    break;
                }

                builders.push_back(FnAttribute::DoubleBuilder(tupleSize));
            }

            DoubleVector & sampleVector =
                    builders[opindex].get(relativeSampleTime);

            //rotation is the only case in which our order differs
            //if (xformOp.getType() == AbcGeom::kRotateOperation)
            switch (xformOp.getType())
            {
                case Alembic::AbcGeom::kRotateOperation:
                case Alembic::AbcGeom::kRotateXOperation:
                case Alembic::AbcGeom::kRotateYOperation:
                case Alembic::AbcGeom::kRotateZOperation:
                {
                    sampleVector.resize(4);

                    sampleVector[0] = xformOp.getAngle();
                    Alembic::AbcGeom::V3d axis = xformOp.getAxis();

                    sampleVector[1] = axis[0];
                    sampleVector[2] = axis[1];
                    sampleVector[3] = axis[2];

                }
                break;

                default:
                    sampleVector.resize(xformOp.getNumChannels());
                    for (size_t i = 0; i < sampleVector.size(); ++i)
                    {
                        sampleVector[i] = xformOp.getChannelValue(i);
                    }
                break;
            }
        }
    }

    if (!builders.empty() || !inheritsXform)
    {
        if (!inheritsXform)
        {
            oGb.set("xform.origin", FnAttribute::DoubleAttribute(0.0), false);
        }

        size_t opindex = 0;

        size_t opTypeCount[4] = {0, 0, 0, 0};

        for (opindex = 0; opindex < referenceOps.getNumOps(); ++opindex)
        {
            const Alembic::AbcGeom::XformOp & xformOp = referenceOps[opindex];

            std::string attrName;

            size_t curCount = 0;

            switch (xformOp.getType())
            {
                case Alembic::AbcGeom::kScaleOperation:
                {
                    attrName = "xform.scale";
                    curCount = opTypeCount[0]++;
                }
                break;

                case Alembic::AbcGeom::kTranslateOperation:
                {
                    attrName = "xform.translate";
                    curCount = opTypeCount[1]++;
                }
                break;

                case Alembic::AbcGeom::kRotateOperation:
                case Alembic::AbcGeom::kRotateXOperation:
                case Alembic::AbcGeom::kRotateYOperation:
                case Alembic::AbcGeom::kRotateZOperation:
                {
                    attrName = "xform.rotate";
                    curCount = opTypeCount[2]++;
                }
                break;

                case Alembic::AbcGeom::kMatrixOperation:
                {
                    attrName = "xform.matrix";
                    curCount = opTypeCount[3]++;
                }
                break;
            }

            if (curCount > 0)
            {
                std::stringstream strm;
                strm << curCount;
                attrName += strm.str();
            }

            oGb.set(attrName, builders[opindex].build(), false);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void cookXform(AbcCookPtr ioCook, FnAttribute::GroupBuilder & oStaticGb)
{
    Alembic::AbcGeom::IXformPtr objPtr(
        new Alembic::AbcGeom::IXform(*(ioCook->objPtr),
                                     Alembic::AbcGeom::kWrapExisting));

    Alembic::AbcGeom::IXformSchema schema = objPtr->getSchema();

    oStaticGb.set("type", FnAttribute::StringAttribute("group"));

    Alembic::Abc::ICompoundProperty userProp = schema.getUserProperties();
    Alembic::Abc::ICompoundProperty arbGeom = schema.getArbGeomParams();
    std::string abcUser = "abcUser.";
    processUserProperties(ioCook, userProp, oStaticGb, abcUser);
    processArbGeomParams(ioCook, arbGeom, oStaticGb);

    //check for child bounds

    Alembic::Abc::IBox3dProperty boundsProp = schema.getChildBoundsProperty();
    if (boundsProp.valid())
    {
        scalarPropertyToAttr(schema, boundsProp.getHeader(), "bound",
                             ioCook, oStaticGb);
    }

    if (schema.isConstant())
    {
        OpArgs defaultArgs;
        evalXform(schema, defaultArgs, oStaticGb);
    }

    if (!schema.isConstant())
    {
        ioCook->objPtr = objPtr;
        ioCook->animatedSchema = true;
    }

}

} //end of namespace WalterIn
