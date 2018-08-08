#include "AbcCook.h"
#include "ArrayPropUtils.h"
#include "ScalarPropUtils.h"
#include "ArbitraryGeomParamUtils.h"

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnDataBuilder.h>

namespace WalterIn
{

///////////////////////////////////////////////////////////////////////////////
void evalCamera(Alembic::AbcGeom::ICameraSchema & iSchema,
                const OpArgs & iArgs,
                FnAttribute::GroupBuilder & oGb)
{
    Alembic::Abc::TimeSamplingPtr ts = iSchema.getTimeSampling();
    SampleTimes sampleTimes;
    iArgs.getRelevantSampleTimes(ts, iSchema.getNumSamples(), sampleTimes);

    bool multiSample = sampleTimes.size() > 1;

    oGb.set("geometry.projection", FnAttribute::StringAttribute("perspective"));

    //geometry attributes which are meaningful to katana
    FnAttribute::DoubleBuilder fovBuilder;
    FnAttribute::DoubleBuilder nearBuilder;
    FnAttribute::DoubleBuilder farBuilder;
    FnAttribute::DoubleBuilder leftBuilder;
    FnAttribute::DoubleBuilder rightBuilder;
    FnAttribute::DoubleBuilder bottomBuilder;
    FnAttribute::DoubleBuilder topBuilder;

    //info attributes just for tracking purposes
    FnAttribute::DoubleBuilder focalLengthBuilder;
    FnAttribute::DoubleBuilder horizontalApertureBuilder;
    FnAttribute::DoubleBuilder horizontalFilmOffsetBuilder;
    FnAttribute::DoubleBuilder verticalApertureBuilder;
    FnAttribute::DoubleBuilder verticalFilmOffsetBuilder;
    FnAttribute::DoubleBuilder lensSqueezeRatioBuilder;
    FnAttribute::DoubleBuilder overscanLeftBuilder;
    FnAttribute::DoubleBuilder overscanRightBuilder;
    FnAttribute::DoubleBuilder overscanTopBuilder;
    FnAttribute::DoubleBuilder overscanBottomBuilder;
    FnAttribute::DoubleBuilder fStopBuilder;
    FnAttribute::DoubleBuilder focusDistanceBuilder;
    FnAttribute::DoubleBuilder shutterOpenBuilder;
    FnAttribute::DoubleBuilder shutterCloseBuilder;

    std::map<std::string, FnAttribute::DoubleBuilder> filmBackBuilders;

    for (SampleTimes::iterator I = sampleTimes.begin();
            I != sampleTimes.end(); ++I)
    {
        Alembic::Abc::chrono_t inputSampleTime = (*I);
        Alembic::Abc::ISampleSelector sampleSelector(inputSampleTime);
        float relativeSampleTime = multiSample ?
                iArgs.getRelativeSampleTime(inputSampleTime) : 0;

        Alembic::AbcGeom::CameraSample sample =
            iSchema.getValue(sampleSelector);

        fovBuilder.get(relativeSampleTime).push_back(
            sample.getFieldOfView());

        nearBuilder.get(relativeSampleTime).push_back(
            sample.getNearClippingPlane());

        farBuilder.get(relativeSampleTime).push_back(
            sample.getFarClippingPlane());

        double top, bottom, left, right;
        sample.getScreenWindow(top, bottom, left, right);

        topBuilder.get(relativeSampleTime).push_back(top);
        bottomBuilder.get(relativeSampleTime).push_back(bottom);
        leftBuilder.get(relativeSampleTime).push_back(left);
        rightBuilder.get(relativeSampleTime).push_back(right);

        focalLengthBuilder.get(relativeSampleTime).push_back(
                sample.getFocalLength());
        horizontalApertureBuilder.get(relativeSampleTime).push_back(
                sample.getHorizontalAperture());
        horizontalFilmOffsetBuilder.get(relativeSampleTime).push_back(
                sample.getHorizontalFilmOffset());
        verticalApertureBuilder.get(relativeSampleTime).push_back(
                sample.getVerticalAperture());
        verticalFilmOffsetBuilder.get(relativeSampleTime).push_back(
                sample.getVerticalFilmOffset());
        lensSqueezeRatioBuilder.get(relativeSampleTime).push_back(
                sample.getLensSqueezeRatio());
        overscanLeftBuilder.get(relativeSampleTime).push_back(
                sample.getOverScanLeft());
        overscanRightBuilder.get(relativeSampleTime).push_back(
                sample.getOverScanRight());
        overscanTopBuilder.get(relativeSampleTime).push_back(
                sample.getOverScanTop());
        overscanBottomBuilder.get(relativeSampleTime).push_back(
                sample.getOverScanBottom());
        fStopBuilder.get(relativeSampleTime).push_back(
                sample.getFStop());
        focusDistanceBuilder.get(relativeSampleTime).push_back(
                sample.getFocusDistance());
        shutterOpenBuilder.get(relativeSampleTime).push_back(
                sample.getShutterOpen() * iArgs.fps);
        shutterCloseBuilder.get(relativeSampleTime).push_back(
                sample.getShutterClose() * iArgs.fps);

        for (std::size_t j = 0; j < sample.getNumOps(); ++j)
        {
            Alembic::AbcGeom::FilmBackXformOp fbop = sample.getOp(j);

            // on the first sample, make sure our data builder is set up with
            // the correct tuple
            if (I == sampleTimes.begin())
            {
                int64_t tupleSize = fbop.getNumChannels();

                // display the matrix as 3x3 instead of 1x9
                if (tupleSize == 9)
                {
                    tupleSize = 3;
                }
                filmBackBuilders[fbop.getHint()] =
                    FnAttribute::DoubleBuilder(tupleSize);
            }

            std::vector<double> & vals =
                filmBackBuilders[fbop.getHint()].get(relativeSampleTime);
            for (std::size_t k = 0; k < fbop.getNumChannels(); ++k)
            {
                vals.push_back(fbop.getChannelValue(k));
            }
        }
    }

    oGb.set("geometry.fov", fovBuilder.build());
    oGb.set("geometry.near", nearBuilder.build());
    oGb.set("geometry.far", farBuilder.build());

    oGb.set("geometry.left", leftBuilder.build());
    oGb.set("geometry.right", rightBuilder.build());
    oGb.set("geometry.bottom", bottomBuilder.build());
    oGb.set("geometry.top", topBuilder.build());


    oGb.set("info.abcCamera.focalLength",focalLengthBuilder.build());
    oGb.set("info.abcCamera.horizontalAperture",
            horizontalApertureBuilder.build());
    oGb.set("info.abcCamera.horizontalFilmOffset",
            horizontalFilmOffsetBuilder.build());
    oGb.set("info.abcCamera.verticalAperture",
            verticalApertureBuilder.build());
    oGb.set("info.abcCamera.verticalFilmOffset",
            verticalFilmOffsetBuilder.build());
    oGb.set("info.abcCamera.lensSqueezeRatio",
            lensSqueezeRatioBuilder.build());
    oGb.set("info.abcCamera.overscanLeft", overscanLeftBuilder.build());
    oGb.set("info.abcCamera.overscanRight", overscanRightBuilder.build());
    oGb.set("info.abcCamera.overscanTop", overscanTopBuilder.build());
    oGb.set("info.abcCamera.overscanBottom", overscanBottomBuilder.build());
    oGb.set("info.abcCamera.fStop", fStopBuilder.build());
    oGb.set("info.abcCamera.focusDistance", focusDistanceBuilder.build());
    oGb.set("info.abcCamera.shutterOpen", shutterOpenBuilder.build());
    oGb.set("info.abcCamera.shutterClose", shutterCloseBuilder.build());

    std::map<std::string, FnAttribute::DoubleBuilder>::iterator dbit;
    for (dbit = filmBackBuilders.begin(); dbit != filmBackBuilders.end();
        ++dbit)
    {
        oGb.set("info.abcCamera." + dbit->first, dbit->second.build());
    }
}

void cookCamera(AbcCookPtr ioCook, FnAttribute::GroupBuilder & oStaticGb)
{
    Alembic::AbcGeom::ICameraPtr objPtr(
        new Alembic::AbcGeom::ICamera(*(ioCook->objPtr),
                                      Alembic::AbcGeom::kWrapExisting));

    oStaticGb.set("type", FnAttribute::StringAttribute("camera"));

    Alembic::AbcGeom::ICameraSchema schema = objPtr->getSchema();

    Alembic::Abc::ICompoundProperty userProp = schema.getUserProperties();
    Alembic::Abc::ICompoundProperty arbGeom = schema.getArbGeomParams();
    std::string abcUser = "abcUser.";
    processUserProperties(ioCook, userProp, oStaticGb, abcUser);
    processArbGeomParams(ioCook, arbGeom, oStaticGb);

    if (schema.isConstant())
    {
        OpArgs defaultArgs;
        evalCamera(schema, defaultArgs, oStaticGb);
    }
    else
    {
        ioCook->objPtr = objPtr;
        ioCook->animatedSchema = true;
    }
}

}
