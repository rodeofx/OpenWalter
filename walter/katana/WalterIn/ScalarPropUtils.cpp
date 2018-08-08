#include "ScalarPropUtils.h"
#include <FnAttribute/FnDataBuilder.h>



namespace WalterIn
{

///////////////////////////////////////////////////////////////////////////////

template <typename attrT, typename builderT, typename podT>
void readScalarProp(
    ScalarProp & iProp,
    const OpArgs & iArgs,
    FnAttribute::GroupBuilder & oGb)
{

    bool isBound = isBoundBox(iProp.prop.getHeader()) && (iProp.name == "bound");
    size_t extent = iProp.prop.getDataType().getExtent();
    size_t tupleSize = extent;
    if (isBound)
        tupleSize = 2;

    builderT b(tupleSize);

    Alembic::Abc::TimeSamplingPtr ts = iProp.prop.getTimeSampling();
    SampleTimes sampleTimes;
    iArgs.getRelevantSampleTimes(ts, iProp.prop.getNumSamples(), sampleTimes);
    for (SampleTimes::iterator it = sampleTimes.begin();
         it != sampleTimes.end(); ++it)
    {
        Alembic::Abc::ISampleSelector ss(*it);
        std::vector<podT> scalarValue(extent);
        iProp.prop.get(&scalarValue.front(), ss);

        std::vector<typename attrT::value_type> value(extent);

        for (size_t i = 0; i < extent; ++i)
        {
            value[i] = static_cast<typename attrT::value_type> (scalarValue[i]);
        }

        if (isBound)
        {
            // from min X,Y,Z max X,Y,Z
            if (value.size() == 6)
            {
                typename attrT::value_type
                minX = value[0], minY = value[1], minZ = value[2],
                maxX = value[3], maxY = value[4], maxZ = value[5];

                // to min/max X, Y, Z
                value[0] = minX; value[1] = maxX;
                value[2] = minY; value[3] = maxY;
                value[4] = minZ; value[5] = maxZ;
            }
        }

        if (sampleTimes.size() == 1)
        {
            // hopefully this will use a fancy attr
            oGb.set(iProp.name, attrT(&value.front(), value.size(), tupleSize));
        }
        else
        {
            b.set(value, iArgs.getRelativeSampleTime(*it));
        }
    }

    if (sampleTimes.size() > 1)
    {
        oGb.set(iProp.name, b.build());
    }
}

void scalarPropertyToAttr(
    ScalarProp & iProp,
    const OpArgs & iArgs,
    FnAttribute::GroupBuilder & oGb)
{
    Alembic::Util::PlainOldDataType pod = iProp.prop.getDataType().getPod();

    switch(pod)
    {
        case Alembic::Util::kBooleanPOD:
            readScalarProp<FnAttribute::IntAttribute,
                           FnAttribute::IntBuilder,
                           Alembic::Util::bool_t>(
                iProp, iArgs, oGb);
        break;

        case Alembic::Util::kUint8POD:
            readScalarProp<FnAttribute::IntAttribute,
                           FnAttribute::IntBuilder,
                           Alembic::Util::uint8_t>(
                iProp, iArgs, oGb);
        break;

        case Alembic::Util::kInt8POD:
            readScalarProp<FnAttribute::IntAttribute,
                           FnAttribute::IntBuilder,
                           Alembic::Util::int8_t>(
                iProp, iArgs, oGb);
        break;

        case Alembic::Util::kUint16POD:
            readScalarProp<FnAttribute::IntAttribute,
                           FnAttribute::IntBuilder,
                           Alembic::Util::uint16_t>(
                iProp, iArgs, oGb);
        break;

        case Alembic::Util::kInt16POD:
            readScalarProp<FnAttribute::IntAttribute,
                           FnAttribute::IntBuilder,
                           Alembic::Util::int16_t>(
                iProp, iArgs, oGb);
        break;

        case Alembic::Util::kUint32POD:
            readScalarProp<FnAttribute::IntAttribute,
                           FnAttribute::IntBuilder,
                           Alembic::Util::uint32_t>(
                iProp, iArgs, oGb);
        break;

        case Alembic::Util::kInt32POD:
            readScalarProp<FnAttribute::IntAttribute,
                           FnAttribute::IntBuilder,
                           Alembic::Util::int32_t>(
                iProp, iArgs, oGb);
        break;

        case Alembic::Util::kFloat16POD:
            readScalarProp<FnAttribute::FloatAttribute,
                           FnAttribute::FloatBuilder,
                           Alembic::Util::float16_t>(
                iProp, iArgs, oGb);
        break;

        case Alembic::Util::kFloat32POD:
            readScalarProp<FnAttribute::FloatAttribute,
                           FnAttribute::FloatBuilder,
                           Alembic::Util::float32_t>(
                iProp, iArgs, oGb);
        break;

        case Alembic::Util::kFloat64POD:
            readScalarProp<FnAttribute::DoubleAttribute,
                           FnAttribute::DoubleBuilder,
                           Alembic::Util::float64_t>(
                iProp, iArgs, oGb);
        break;

        case Alembic::Util::kStringPOD:
            readScalarProp<FnAttribute::StringAttribute,
                           FnAttribute::StringBuilder,
                           std::string>(
                iProp, iArgs, oGb);
        break;

        default:
        break;
    }

}

void scalarPropertyToAttr(
    Alembic::Abc::ICompoundProperty & iParent,
    const Alembic::Abc::PropertyHeader & iHeader,
    const std::string & iPropName,
    AbcCookPtr ioCook,
    FnAttribute::GroupBuilder & oStaticGb)
{
    Alembic::Abc::IScalarProperty prop(iParent, iHeader.getName());

    // bad prop don't bother with it
    if (!prop.valid() || prop.getNumSamples() == 0)
    {
        return;
    }

    ScalarProp item;
    item.name = iPropName;
    item.prop = prop;

    if (!prop.isConstant())
    {

        ioCook->scalarProps.push_back(item);
        return;
    }

    OpArgs defaultArgs;
    scalarPropertyToAttr(item, defaultArgs, oStaticGb);
}

}
