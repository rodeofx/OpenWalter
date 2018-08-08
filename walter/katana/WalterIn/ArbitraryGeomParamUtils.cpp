#include "ArbitraryGeomParamUtils.h"
#include "ArrayPropUtils.h"
#include <Alembic/AbcGeom/All.h>

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnDataBuilder.h>

namespace WalterIn
{

FnAttribute::StringAttribute getScopeAttribute(
    Alembic::AbcGeom::GeometryScope scope)
{
    switch (scope)
    {
    case Alembic::AbcGeom::kUniformScope:
        return FnAttribute::StringAttribute("face");
    case Alembic::AbcGeom::kVaryingScope:
        return FnAttribute::StringAttribute("point");
    case Alembic::AbcGeom::kVertexScope:
        return FnAttribute::StringAttribute("point");
    case Alembic::AbcGeom::kFacevaryingScope:
        return FnAttribute::StringAttribute("vertex");
    default:
        return FnAttribute::StringAttribute("primitive");
    }
}

std::string getInputType(const Alembic::Abc::PropertyHeader & iPropHeader)
{
    std::string interp = iPropHeader.getMetaData().get("interpretation");

    Alembic::Util::PlainOldDataType podType =
        iPropHeader.getDataType().getPod();

    Alembic::Util::uint8_t extent =  iPropHeader.getDataType().getExtent();

    if (podType == Alembic::Util::kBooleanPOD)
    {
        return "bool";
    }
    else if (podType == Alembic::Util::kFloat64POD ||
             podType == Alembic::Util::kFloat32POD)
    {
        if (interp == "point" && extent == 3)
        {
            return "point3";
        }

        if (interp == "point" && extent == 4)
        {
            return "point4";
        }

        if (interp == "vector" && extent == 3)
        {
            return "vector3";
        }

        if (interp == "vector" && extent == 4)
        {
            return "vector4";
        }

        // a VERY special case (point2 on purpose)
        if (interp == "vector" && extent == 2)
        {
            return "point2";
        }

        if (interp == "normal" && extent == 3)
        {
            return "normal3";
        }

        if (interp == "normal" && extent == 4)
        {
            return "normal4";
        }

        if (interp == "rgb" && extent == 3)
        {
            return "color3";
        }

        if (interp == "rgba" && extent == 4)
        {
            return "color4";
        }

        if (interp == "matrix" && extent == 9)
        {
            return "matrix9";
        }

        if (interp == "matrix" && extent == 16)
        {
            return "matrix16";
        }

        if (interp == "quat" && extent == 4)
        {
            return "point4";
        }
    }

    return "";
}

// read iProp.prop into oGb
template <typename attrT, typename builderT>
void readAsExpandedProp(
        IndexedGeomParamPair & iProp,
        const OpArgs & iArgs,
        FnAttribute::GroupBuilder & oGb)
{

    size_t extent = iProp.prop.getDataType().getExtent();
    int64_t tupleSize = extent;
    builderT b(tupleSize);

    Alembic::Abc::TimeSamplingPtr ts = iProp.prop.getTimeSampling();
    SampleTimes sampleTimes;
    iArgs.getRelevantSampleTimes(ts, iProp.prop.getNumSamples(), sampleTimes);
    size_t numIndexed = 0;
    Alembic::Util::Dimensions dims;
    if (!sampleTimes.empty())
    {
        SampleTimes::iterator it = sampleTimes.begin();

        iProp.indexed.getDimensions(dims, Alembic::Abc::ISampleSelector(*it));
        numIndexed = dims.numPoints();
        ++it;

        // make sure every sample we are using is the same size
        bool sameSize = true;
        for (; it != sampleTimes.end(); ++it)
        {
            iProp.indexed.getDimensions(dims,
                                        Alembic::Abc::ISampleSelector(*it));
            if (numIndexed != dims.numPoints())
            {
                sameSize = false;
                break;
            }
        }

        // not the same, use just a single time
        if (!sameSize)
        {
            sampleTimes.clear();
            sampleTimes.push_back(iArgs.getAbcFrameTime());
            Alembic::Abc::ISampleSelector ss(*sampleTimes.begin());
            iProp.indexed.getDimensions(dims, ss);
            numIndexed = dims.numPoints();
        }
    }

    for (SampleTimes::iterator it = sampleTimes.begin();
         it != sampleTimes.end(); ++it)
    {
        std::vector<typename attrT::value_type> packedVals;
        std::vector<Alembic::Util::uint32_t> indexedValue(numIndexed);

        Alembic::Abc::ISampleSelector ss(*it);
        iProp.prop.getDimensions(dims, ss);
        size_t numVals = dims.numPoints();
        packedVals.resize(extent * numVals);

        if (numVals > 0)
        {
            iProp.prop.getAs(&packedVals.front(), iProp.asPod, ss);
        }

        if (numIndexed > 0)
        {
            iProp.indexed.getAs(&indexedValue.front(), ss);
        }

        // unroll it ourselves
        std::vector<typename attrT::value_type> value;
        value.resize(extent * indexedValue.size());
        size_t packedSize = packedVals.size() / extent;
        for (size_t i = 0; i < indexedValue.size(); ++i)
        {
            Alembic::Util::uint32_t curIndex = indexedValue[i];
            if (indexedValue[i] < packedSize)
            {
                for (size_t j = 0; j < extent; ++j)
                {
                    value[i*extent + j] = packedVals[curIndex * extent + j];
                }
            }
        }

        if (sampleTimes.size() == 1)
        {
            // hopefully this will use a fancy attr
            if (value.empty())
            {
                typename attrT::value_type * nullPtr = NULL;
                oGb.set(iProp.name, attrT(nullPtr, 0, tupleSize));
            }
            else
            {
                oGb.set(iProp.name, attrT(&value.front(), value.size(),
                        tupleSize));
            }
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

template <typename attrT, typename geomParamT>
void processArbitraryGeomParam(
    Alembic::Abc::ICompoundProperty & iParent,
    const Alembic::Abc::PropertyHeader & iPropHeader,
    const std::string & iAttrName,
    AbcCookPtr ioCook,
    FnAttribute::GroupBuilder & oStaticGb,
    Alembic::Abc::IUInt64ArrayProperty * iIdsProperty)
{
    geomParamT param(iParent, iPropHeader.getName());

    attrT a;
    Alembic::Util::PlainOldDataType asPod = FnAttrTypeToPODType(
        a.getKatAttributeType());

    bool forceAsExpanded = (iAttrName.substr(0, 18) != "geometry.arbitrary");

    // if a param has no samples, don't even bother creating it
    if (param.getNumSamples() == 0)
    {
        return;
    }

    if (!forceAsExpanded)
    {
        oStaticGb.set(iAttrName + ".scope",
            getScopeAttribute(Alembic::AbcGeom::GetGeometryScope(
                              iPropHeader.getMetaData())));

        // Calculate the input type
        // Notice that if param is an indexed parameter the input type has to
        // be retrieved from the value property
        const std::string inputType =
            param.isIndexed()
                ? getInputType(param.getValueProperty().getHeader())
                : getInputType(iPropHeader);

        if (inputType != "")
        {
            oStaticGb.set(iAttrName + ".inputType",
                FnAttribute::StringAttribute(inputType));
        }

        if (param.getArrayExtent() > 1)
        {
            int arrayExtent = (int) param.getArrayExtent();
            oStaticGb.set(iAttrName + ".elementSize",
                FnAttribute::IntAttribute(arrayExtent));
        }
    }

    if (!param.isConstant())
    {
        if (param.isIndexed() && forceAsExpanded)
        {
            IndexedGeomParamPair valPair;
            valPair.name = iAttrName;
            valPair.prop = param.getValueProperty();
            valPair.indexed = param.getIndexProperty();
            valPair.asPod = asPod;
            ioCook->forcedExpandProps.push_back(valPair);

            // no reading to do, bail early
            return;
        }
        // don't force expansion
        else if (param.isIndexed())
        {
            if (!param.getValueProperty().isConstant())
            {
                ArrayProp item;
                item.name = iAttrName + ".indexedValue";
                item.prop = param.getValueProperty();
                item.asPod = asPod;
                ioCook->arrayProps.push_back(item);
            }

            if (!param.getIndexProperty().isConstant())
            {
                ArrayProp item;
                item.name = iAttrName + ".index";
                item.prop = param.getIndexProperty();
                item.asPod = Alembic::Util::kInt32POD;
                ioCook->arrayProps.push_back(item);
            }
        }
        // not indexed!
        else
        {
            ArrayProp item;
            item.name = iAttrName;
            if (!forceAsExpanded)
            {
                item.name += ".value";
            }
            item.prop = param.getValueProperty();
            item.asPod = asPod;
            ioCook->arrayProps.push_back(item);
        }
    }

    std::string valueName = iAttrName;
    OpArgs defaultArgs;

    Alembic::Abc::IUInt32ArrayProperty indexProp = param.getIndexProperty();
    if (indexProp.valid() && indexProp.isConstant() && !forceAsExpanded)
    {
        ArrayProp item;
        item.name = iAttrName + ".index";
        item.prop = indexProp;
        item.asPod = Alembic::Util::kInt32POD;
        arrayPropertyToAttr(item, defaultArgs, oStaticGb, iIdsProperty);
        valueName += ".indexedValue";
    }
    else if (!forceAsExpanded)
    {
        valueName += ".value";
    }

    typename geomParamT::prop_type valueProp;
    valueProp = param.getValueProperty();

    if ((valueProp.valid() && valueProp.isConstant()) &&
        (!forceAsExpanded || !param.isIndexed()))
    {
        ArrayProp item;
        item.name = valueName;
        item.prop = valueProp;
        item.asPod = asPod;
        arrayPropertyToAttr(item, defaultArgs, oStaticGb, iIdsProperty);
    }
    else if (forceAsExpanded && param.isConstant())
    {
        IndexedGeomParamPair valPair;
        valPair.name = iAttrName;
        valPair.prop = param.getValueProperty();
        valPair.indexed = param.getIndexProperty();
        valPair.asPod = asPod;
        // NOTE: we don't support indexed params correlated by IDs (e.g. differing topology)
        indexedParamToAttr(valPair, defaultArgs, oStaticGb);
    }
}


void indexedParamToAttr(IndexedGeomParamPair & iProp,
    const OpArgs & iArgs,
    FnAttribute::GroupBuilder & oGb)
{
    if (iProp.asPod == Alembic::Util::kFloat32POD)
    {
        readAsExpandedProp<FnAttribute::FloatAttribute,
                           FnAttribute::FloatBuilder>(
            iProp, iArgs, oGb);
    }
    else if (iProp.asPod == Alembic::Util::kInt32POD)
    {
        readAsExpandedProp<FnAttribute::IntAttribute,
                           FnAttribute::IntBuilder>(
            iProp, iArgs, oGb);
    }
    else if (iProp.asPod == Alembic::Util::kFloat64POD)
    {
        readAsExpandedProp<FnAttribute::DoubleAttribute,
                           FnAttribute::DoubleBuilder>(
            iProp, iArgs, oGb);
    }
    else if (iProp.asPod == Alembic::Util::kStringPOD)
    {
        readAsExpandedProp<FnAttribute::StringAttribute,
                           FnAttribute::StringBuilder>(
            iProp, iArgs, oGb);
    }
}

void processArbitraryGeomParam( AbcCookPtr ioCook,
    Alembic::Abc::ICompoundProperty & iParent,
    const Alembic::AbcCoreAbstract::PropertyHeader & iPropHeader,
    FnAttribute::GroupBuilder & oStaticGb,
    const std::string & iAttrPath,
    Alembic::Abc::IUInt64ArrayProperty * iIdsProperty )
{

    if (Alembic::AbcGeom::IFloatGeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::FloatAttribute, Alembic::AbcGeom::IFloatGeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IDoubleGeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::DoubleAttribute,
            Alembic::AbcGeom::IDoubleGeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IInt32GeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::IntAttribute, Alembic::AbcGeom::IInt32GeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IUInt32GeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::IntAttribute, Alembic::AbcGeom::IUInt32GeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IInt16GeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::IntAttribute, Alembic::AbcGeom::IInt16GeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IUInt16GeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::IntAttribute, Alembic::AbcGeom::IUInt16GeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::ICharGeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::IntAttribute, Alembic::AbcGeom::ICharGeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IUcharGeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::IntAttribute, Alembic::AbcGeom::IUcharGeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IBoolGeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::IntAttribute, Alembic::AbcGeom::IBoolGeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IStringGeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::StringAttribute,
            Alembic::AbcGeom::IStringGeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IV2fGeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::FloatAttribute, Alembic::AbcGeom::IV2fGeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IV3dGeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::DoubleAttribute, Alembic::AbcGeom::IV3dGeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IV3fGeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::FloatAttribute, Alembic::AbcGeom::IV3fGeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IP3fGeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::FloatAttribute, Alembic::AbcGeom::IP3fGeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IP3dGeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::DoubleAttribute, Alembic::AbcGeom::IP3dGeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IN3fGeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::FloatAttribute, Alembic::AbcGeom::IN3fGeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IC3fGeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::FloatAttribute, Alembic::AbcGeom::IC3fGeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IC4fGeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::FloatAttribute, Alembic::AbcGeom::IC4fGeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IM44fGeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::FloatAttribute, Alembic::AbcGeom::IM44fGeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
    else if (Alembic::AbcGeom::IQuatfGeomParam::matches(iPropHeader))
    {
        processArbitraryGeomParam<
            FnAttribute::FloatAttribute, Alembic::AbcGeom::IQuatfGeomParam>(
                iParent, iPropHeader, iAttrPath, ioCook, oStaticGb, iIdsProperty);
    }
}

void processArbGeomParams( AbcCookPtr ioCook,
    Alembic::Abc::ICompoundProperty & iParent,
    FnAttribute::GroupBuilder & oStaticGb,
    Alembic::Abc::v10::IUInt64ArrayProperty * iIdsProperty )
{
    if (!iParent.valid())
    {
        return;
    }

    std::string attrPath = "geometry.arbitrary.";
    for (size_t i = 0; i < iParent.getNumProperties(); ++i)
    {
        const Alembic::AbcCoreAbstract::PropertyHeader &propHeader =
            iParent.getPropertyHeader(i);
        processArbitraryGeomParam( ioCook, iParent, propHeader, oStaticGb,
            attrPath + propHeader.getName(), iIdsProperty );
    }
}

}
