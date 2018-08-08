#include "ArrayPropUtils.h"
#include <FnAttribute/FnDataBuilder.h>

#define POINT_ATTR_EXTRAPOLATION 1

namespace WalterIn
{

namespace {

// Converts the specified Alembic property array to an FnAttribute array type.
template <typename attrT, typename podT>
void convertAlembicTypesToKatanaTypes(
    ArrayProp& iProp,
    Alembic::Abc::ISampleSelector sampleSelector,
    typename attrT::value_type* sampleBuffer,
    size_t numPodValues)
{
    try
    {
        // Perform a conversion from the stored POD type to the proper
        // POD type for the Katana attribute.
        Alembic::Util::PlainOldDataType alembicType =
            FnAttrTypeToPODType(attrT::getKatAttributeType());
        iProp.prop.getAs(sampleBuffer, alembicType, sampleSelector);
    }
    catch (Alembic::Abc::Exception&)
    {
        // Because certain types cannot be automatically converted by
        // certain backends (HDF5 does not do well converting float16 to
        // float32, for example), catch the exception and perform a direct
        // cast below.  We don't do this for everything, because the
        // above is faster and works for the vast majority of cases.

        Alembic::Abc::ArraySamplePtr valuePtr;
        iProp.prop.get(valuePtr, sampleSelector);
        if (valuePtr && valuePtr->valid())
        {
            const podT* typedData =
                static_cast<const podT*>(valuePtr->getData());
            typename attrT::value_type* buffer =
                (typename attrT::value_type*)sampleBuffer;
            for (size_t i = 0; i < numPodValues; ++i)
            {
                buffer[i] =
                    static_cast<typename attrT::value_type>(typedData[i]);
            }
        }
    }
}

template<typename attrT>
attrT makeAttribute(float* sampleTimes,
                    int64_t numSampleTimes,
                    const typename attrT::value_type** samplePtrs,
                    int64_t numValues,
                    int64_t tupleSize)
{
    return attrT(sampleTimes, numSampleTimes, samplePtrs, numValues, tupleSize);
}

// Unused, but satisfies the compiler
template<>
FnAttribute::StringAttribute makeAttribute<FnAttribute::StringAttribute>(float*, int64_t, const std::string**, int64_t, int64_t)
{
    return FnAttribute::StringAttribute();
}

template<typename attrT>
typename attrT::value_type extrapolateAttr(const std::vector<typename attrT::value_type>& values,
                                           int64_t tupleSize,
                                           int64_t index,
                                           double t0,
                                           double tDelta,
                                           double targetTime)
{
    typename attrT::value_type vel = (tDelta != 0.0f) ? ((values[index + tupleSize] - values[index]) / tDelta) : 0.0f;
    return values[index] + vel * (targetTime - t0);
}

// Unused, but satisfies the compiler
template<>
std::string extrapolateAttr<FnAttribute::StringAttribute>(const std::vector<std::string>&,
                                                          int64_t,
                                                          int64_t,
                                                          double,
                                                          double,
                                                          double)
{
    return std::string();
}

} // namespace

// read iProp.prop into oGb
template <typename attrT, typename builderT, typename podT>
void readArrayProp(
        ArrayProp & iProp,
        const OpArgs & iArgs,
        FnAttribute::GroupBuilder & oGb,
        Alembic::Abc::IUInt64ArrayProperty * iIdsProperty)
{
    size_t extent = iProp.prop.getDataType().getExtent();
    int64_t tupleSize = extent;

    builderT b(tupleSize);

    Alembic::Abc::TimeSamplingPtr ts = iProp.prop.getTimeSampling();
    SampleTimes sampleTimes;
    iArgs.getRelevantSampleTimes(ts, iProp.prop.getNumSamples(), sampleTimes);

//    printf("Read array property: %s has Ids: %s\n", iProp.name.c_str(), iIdsProperty ? "yes" : "no");

    if (iIdsProperty && iProp.name != "geometry.poly.startIndex" && sampleTimes.size() > 1)
    {
//        printf("Attempting to read property with ID correspondence: %s\n", iProp.name.c_str());

        // Motion blur with varying topology, we correlate array elements between
        // motion samples by ID so we keep as much motion data as possible

        Alembic::Abc::TimeSamplingPtr idTs = iIdsProperty->getTimeSampling();
        SampleTimes idSampleTimes;
        iArgs.getRelevantSampleTimes(idTs, iIdsProperty->getNumSamples(), idSampleTimes);

        std::map<uint64_t, std::vector<typename attrT::value_type> > idToValueMap;
        std::map<uint64_t, std::vector<double> > idToSampleTimeMap;

        bool foundCorrespondence = idSampleTimes.size() == sampleTimes.size();
        if (foundCorrespondence)
        {
//            printf("Sample times correspond with IDs: %s\n", iProp.name.c_str());

            ArrayProp iIdProp;
            iIdProp.name = iIdsProperty->getName();
            iIdProp.prop = *iIdsProperty;
            iIdProp.asPod = iIdsProperty->getHeader().getDataType().getPod();

            Alembic::Util::Dimensions dims;
            std::vector<double>::iterator stIter = sampleTimes.begin();
            std::vector<double>::iterator idStIter = idSampleTimes.begin();
            for ( ; stIter != sampleTimes.end() && idStIter != idSampleTimes.end(); ++stIter, ++idStIter)
            {
                // Make sure sample times and sizes match between IDs and the array
                double sampleTime = *stIter;
                double idSampleTime = *idStIter;
                if (idSampleTime != sampleTime)
                {
//                    printf("Sample time does not correspond with IDs: %s: %f vs %f\n", iProp.name.c_str(), sampleTime, idSampleTime);
                    foundCorrespondence = false;
                    break;
                }
                Alembic::Abc::ISampleSelector sampleSelector(sampleTime);
                iProp.prop.getDimensions(dims, sampleSelector);
                const size_t numVals = dims.numPoints();
                const size_t numPodValues = numVals * tupleSize;
                iIdsProperty->getDimensions(dims, sampleSelector);
                const size_t numIdPodValues = dims.numPoints();
                if (numVals != numIdPodValues)
                {
//                    printf("Array size does not correspond with IDs: %s: %zu vs %zu\n", iProp.name.c_str(), numVals, numIdPodValues);
                    foundCorrespondence = false;
                    break;
                }
                if (numPodValues == 0)
                {
                    continue;
                }

                std::vector<typename attrT::value_type> sampleValues;
                sampleValues.resize(numPodValues);
                convertAlembicTypesToKatanaTypes<attrT, podT>(iProp, sampleSelector, &sampleValues[0], numPodValues);

                std::vector<FnAttribute::IntAttribute::value_type> idValues;
                idValues.resize(numIdPodValues);
                convertAlembicTypesToKatanaTypes<FnAttribute::IntAttribute, Alembic::Util::uint64_t>(iIdProp, sampleSelector, &idValues[0], numIdPodValues);

                // Map each value to its ID-based sequence of samples
                for (size_t i = 0; i < numVals; ++i)
                {
                    std::vector<typename attrT::value_type>& values = idToValueMap[idValues[i]];
                    std::vector<double>& times = idToSampleTimeMap[idValues[i]];
                    times.push_back(sampleTime);
                    for (int64_t tupleIndex = 0; tupleIndex < tupleSize; ++tupleIndex)
                    {
                        values.push_back(sampleValues[i * tupleSize + tupleIndex]);
                    }
                }
            }
        }
        if (foundCorrespondence)
        {
            // Build time-sampled array with all found IDs, and extrapolate
            // samples from nearby times when they don't exist
            const size_t numPodValues = idToValueMap.size() * tupleSize;
            typename attrT::value_type *finalValues = new typename attrT::value_type[sampleTimes.size() * numPodValues];
            size_t idIndex = 0;
            for (auto idValueMapIter = idToValueMap.begin(); idValueMapIter != idToValueMap.end(); ++idValueMapIter, ++idIndex)
            {
                uint64_t id = idValueMapIter->first;
                const std::vector<typename attrT::value_type>& values = idValueMapIter->second;
                const std::vector<double>& times = idToSampleTimeMap[id];
                if (times.size() == sampleTimes.size())
                {
                    for (size_t i = 0; i < sampleTimes.size(); ++i)
                    {
                        ::memcpy(&finalValues[i * numPodValues + idIndex * tupleSize],
                                 &values[i * tupleSize],
                                 tupleSize * sizeof(typename attrT::value_type));
                    }
                }
                else
                {
                    size_t idSampleTimeIdx = 0;
                    for (size_t i = 0; i < sampleTimes.size(); ++i)
                    {
                        if (sampleTimes[i] < times[idSampleTimeIdx])
                        {
#if !POINT_ATTR_EXTRAPOLATION
                            // This element had no sample at this early time, so copy the first available
                            ::memcpy(&finalValues[i * numPodValues + idIndex * tupleSize], &values[0], tupleSize * sizeof(typename attrT::value_type));
#else
                            if (idSampleTimeIdx + 1 < times.size())
                            {
                                // This element had no sample at this early time, so extrapolate from the first two available if possible
                                float tDelta = times[idSampleTimeIdx + 1] - times[idSampleTimeIdx];
                                for (int64_t j = 0; j < tupleSize; ++j)
                                {
                                    finalValues[i * numPodValues + idIndex * tupleSize + j] =
                                            extrapolateAttr<attrT>(values,
                                                                   tupleSize,
                                                                   j,
                                                                   times[idSampleTimeIdx],
                                                                   tDelta,
                                                                   sampleTimes[i]);
                                }
                            }
                            else
                            {
                                // This element had no sample at this early time, so copy the only available
                                ::memcpy(&finalValues[i * numPodValues + idIndex * tupleSize],
                                         &values[0],
                                         tupleSize * sizeof(typename attrT::value_type));
                            }
#endif
                        }
                        else
                        {
#if !POINT_ATTR_EXTRAPOLATION
                            // Copy the last available sample
                            ::memcpy(&finalValues[i * numPodValues + idIndex * tupleSize],
                                     &values[idSampleTimeIdx * tupleSize],
                                     tupleSize * sizeof(typename attrT::value_type));
#else
                            if (sampleTimes[i] > times[idSampleTimeIdx] && idSampleTimeIdx > 0)
                            {
                                // This element had no sample at this early time, so extrapolate from the first two available if possible
                                float tDelta = times[idSampleTimeIdx] - times[idSampleTimeIdx - 1];
                                for (int64_t j = 0; j < tupleSize; ++j)
                                {
                                    finalValues[i * numPodValues + idIndex * tupleSize + j] =
                                            extrapolateAttr<attrT>(values,
                                                                   tupleSize,
                                                                   (idSampleTimeIdx - 1) * tupleSize + j,
                                                                   times[idSampleTimeIdx - 1],
                                                                   tDelta,
                                                                   sampleTimes[i]);
                                }
                            }
                            else
                            {
                                // Copy the only available sample
                                ::memcpy(&finalValues[i * numPodValues + idIndex * tupleSize],
                                         &values[idSampleTimeIdx * tupleSize],
                                         tupleSize * sizeof(typename attrT::value_type));
                            }
#endif
                            // If the sample times match, advance the source sample time if we can
                            if (sampleTimes[i] == times[idSampleTimeIdx] && idSampleTimeIdx + 1 < times.size())
                                idSampleTimeIdx++;
                        }
                    }
                }
            }

            float* katanaSampleTimes = new float[sampleTimes.size()];
            const typename attrT::value_type** sampleDataPtrs = new const typename attrT::value_type*[sampleTimes.size()];
            for (size_t i = 0; i < sampleTimes.size(); ++i)
            {
                // Store the samples (with frame-relative times)
                katanaSampleTimes[i] = iArgs.getRelativeSampleTime(sampleTimes[i]);
                sampleDataPtrs[i] = &finalValues[i * numPodValues];
            }

            oGb.set(iProp.name,
                    makeAttribute<attrT>(katanaSampleTimes, sampleTimes.size(),
                                         sampleDataPtrs, numPodValues, tupleSize));

            delete[] katanaSampleTimes;
            delete[] sampleDataPtrs;
        }
        else
        {
            // IDs not usable? Re-try without IDs.
//            printf("Could not generate correspondence via IDs: %s\n", iProp.name.c_str());
            readArrayProp<attrT, builderT, podT>(iProp, iArgs, oGb, NULL);
        }
    }
    else
    {
        size_t numVals = 0;
        Alembic::Util::Dimensions dims;
        if (!sampleTimes.empty())
        {
            SampleTimes::iterator it = sampleTimes.begin();
            iProp.prop.getDimensions(dims, Alembic::Abc::ISampleSelector(*it));
            numVals = dims.numPoints();
            ++it;

            // make sure every sample we are using is the same size
            bool sameSize = true;
            for (; it != sampleTimes.end(); ++it)
            {
                iProp.prop.getDimensions(dims, Alembic::Abc::ISampleSelector(*it));
                if (numVals != dims.numPoints())
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
                iProp.prop.getDimensions(dims, ss);
                numVals = dims.numPoints();
            }
        }

        for (SampleTimes::iterator it = sampleTimes.begin();
             it != sampleTimes.end(); ++it)
        {
            Alembic::Abc::ISampleSelector ss(*it);

            const size_t numPodValues = extent * numVals;
            std::vector<typename attrT::value_type> value;
            size_t bufOffset = 0;

            if (iProp.name == "geometry.poly.startIndex" && numVals != 0)
            {
                bufOffset = 1;
            }

            value.resize(numPodValues + bufOffset);
            try
            {
                // Perform a conversion from the stored POD type to the proper
                // POD type for the Katana attribute.
                iProp.prop.getAs(&value[bufOffset], FnAttrTypeToPODType(attrT::getKatAttributeType()), ss);
            }
            catch(Alembic::Util::Exception&)
            {
                // Because certain types cannot be automatically converted by
                // certain backends (HDF5 does not do well converting float16 to
                // float32, for example), catch the exception and perform a direct
                // cast below.  We don't do this for everything, because the
                // above is faster and works for the vast majority of cases.

                Alembic::Abc::ArraySamplePtr valuePtr;
                iProp.prop.get(valuePtr, ss);
                if (valuePtr && valuePtr->valid())
                {
                    const podT* typedData = static_cast<const podT*>(valuePtr->getData());
                    for (size_t i = 0; i < numPodValues; ++i)
                    {
                        value[i + bufOffset] = static_cast<typename attrT::value_type> (typedData[i]);
                    }
                }
            }

            if (iProp.name == "geometry.poly.startIndex" && !value.empty())
            {
                for (size_t i = 2; i < value.size(); ++i)
                {
                    value[i] += value[i-1];
                }
            }

            if (sampleTimes.size() == 1)
            {
                // hopefully this will use a fancy attr
                oGb.set(iProp.name,
                    attrT(&(value.front()), value.size(), tupleSize));
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
}

void arrayPropertyToAttr(ArrayProp & iProp,
    const OpArgs & iArgs,
    FnAttribute::GroupBuilder & oGb,
    Alembic::Abc::IUInt64ArrayProperty * iIdsProperty)
{
    switch(iProp.asPod)
    {
        case Alembic::Util::kBooleanPOD:
            readArrayProp<FnAttribute::IntAttribute,
                          FnAttribute::IntBuilder,
                          Alembic::Util::bool_t>(
                iProp, iArgs, oGb, iIdsProperty);
        break;

        case Alembic::Util::kUint8POD:
            readArrayProp<FnAttribute::IntAttribute,
                          FnAttribute::IntBuilder,
                          Alembic::Util::uint8_t>(
                iProp, iArgs, oGb, iIdsProperty);
        break;

        case Alembic::Util::kInt8POD:
            readArrayProp<FnAttribute::IntAttribute,
                          FnAttribute::IntBuilder,
                          Alembic::Util::int8_t>(
                iProp, iArgs, oGb, iIdsProperty);
        break;

        case Alembic::Util::kUint16POD:
            readArrayProp<FnAttribute::IntAttribute,
                          FnAttribute::IntBuilder,
                          Alembic::Util::uint16_t>(
                iProp, iArgs, oGb, iIdsProperty);
        break;

        case Alembic::Util::kInt16POD:
            readArrayProp<FnAttribute::IntAttribute,
                          FnAttribute::IntBuilder,
                          Alembic::Util::int16_t>(
                iProp, iArgs, oGb, iIdsProperty);
        break;

        case Alembic::Util::kUint32POD:
            readArrayProp<FnAttribute::IntAttribute,
                          FnAttribute::IntBuilder,
                          Alembic::Util::uint32_t>(
                iProp, iArgs, oGb, iIdsProperty);
        break;

        case Alembic::Util::kInt32POD:
            readArrayProp<FnAttribute::IntAttribute,
                          FnAttribute::IntBuilder,
                          Alembic::Util::int32_t>(
                iProp, iArgs, oGb, iIdsProperty);
        break;

        case Alembic::Util::kUint64POD:
            readArrayProp<FnAttribute::IntAttribute,
                          FnAttribute::IntBuilder,
                          Alembic::Util::uint64_t>(
                iProp, iArgs, oGb, iIdsProperty);
        break;

        case Alembic::Util::kInt64POD:
            readArrayProp<FnAttribute::IntAttribute,
                          FnAttribute::IntBuilder,
                          Alembic::Util::int64_t>(
                iProp, iArgs, oGb, iIdsProperty);
        break;

        case Alembic::Util::kFloat16POD:
            readArrayProp<FnAttribute::FloatAttribute,
                          FnAttribute::FloatBuilder,
                          Alembic::Util::float16_t>(
                iProp, iArgs, oGb, iIdsProperty);
        break;

        case Alembic::Util::kFloat32POD:
            readArrayProp<FnAttribute::FloatAttribute,
                          FnAttribute::FloatBuilder,
                          Alembic::Util::float32_t>(
                iProp, iArgs, oGb, iIdsProperty);
        break;

        case Alembic::Util::kFloat64POD:
            readArrayProp<FnAttribute::DoubleAttribute,
                          FnAttribute::DoubleBuilder,
                          Alembic::Util::float64_t>(
                iProp, iArgs, oGb, iIdsProperty);
        break;

        case Alembic::Util::kStringPOD:
            readArrayProp<FnAttribute::StringAttribute,
                          FnAttribute::StringBuilder,
                          std::string>(
                iProp, iArgs, oGb, NULL); // NOTE: no support for strings correlated by IDs (varying topologies)
        break;

        default:
        break;
    }
}

void arrayPropertyToAttr(Alembic::Abc::ICompoundProperty & iParent,
    const Alembic::Abc::PropertyHeader & iPropHeader,
    const std::string & iPropName,
    FnKatAttributeType iType,
    AbcCookPtr ioCook,
    FnAttribute::GroupBuilder & oStaticGb,
    Alembic::Abc::v10::IUInt64ArrayProperty * iIdsProperty)
{
    Alembic::Abc::IArrayProperty prop(iParent, iPropHeader.getName());

    // bad prop don't bother with it
    if (!prop.valid() || prop.getNumSamples() == 0)
    {
        return;
    }

    ArrayProp item;
    item.name = iPropName;
    item.prop = prop;
    item.asPod = iPropHeader.getDataType().getPod();

    if (!prop.isConstant() && item.asPod != Alembic::Util::kUnknownPOD)
    {
        ioCook->arrayProps.push_back(item);
        return;
    }

    OpArgs defaultArgs;
    arrayPropertyToAttr(item, defaultArgs, oStaticGb, iIdsProperty);
}

}
