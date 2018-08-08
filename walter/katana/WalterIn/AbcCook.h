#ifndef FnGeolibOp_WalterIn_AbcCook_H
#define FnGeolibOp_WalterIn_AbcCook_H

#include <Alembic/Abc/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcMaterial/All.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <unordered_map>

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnGroupBuilder.h>

namespace WalterIn
{

bool isBoundBox(const Alembic::AbcCoreAbstract::PropertyHeader & iPropHeader);

Alembic::Util::PlainOldDataType FnAttrTypeToPODType(FnKatAttributeType iType);

int64_t getTupleSize(const Alembic::AbcCoreAbstract::PropertyHeader & iHeader);

class AbcCook;
typedef boost::shared_ptr< AbcCook > AbcCookPtr;

typedef std::unordered_map<std::string, std::string> NameMap;

struct IndexedGeomParamPair
{
    std::string name;
    Alembic::Abc::IArrayProperty prop;
    Alembic::Abc::IUInt32ArrayProperty indexed;
    Alembic::Util::PlainOldDataType asPod;
};

struct ArrayProp
{
    std::string name;
    Alembic::Abc::IArrayProperty prop;
    Alembic::Util::PlainOldDataType asPod;
};

struct ScalarProp
{
    std::string name;
    Alembic::Abc::IScalarProperty prop;
};

class AbcCook
{
public:
    AbcCook() { animatedSchema = false; }

    Foundry::Katana::GroupAttribute staticGroup;

    std::vector< ArrayProp > arrayProps;
    std::vector< ScalarProp > scalarProps;
    std::vector< IndexedGeomParamPair > forcedExpandProps;

    Alembic::AbcGeom::IVisibilityProperty visProp;

    Alembic::Abc::IObjectPtr objPtr;
    boost::mutex mutex;

    bool animatedSchema;
};

typedef std::vector<double> SampleTimes;

struct OpArgs
{
    double currentTime;
    double shutterOpen;
    double shutterClose;
    double fps;
    int numSamples;

    enum ExtraFrameRangeBehavior
    {
        kError = 0,
        kHold = 1,
        kSkip = 2
    };

    ExtraFrameRangeBehavior behavior;
    bool useOnlyShutterOpenCloseTimes;

    OpArgs()
    : currentTime(1.0)
    , shutterOpen(0.0)
    , shutterClose(0.0)
    , fps(24.0)
    , numSamples(1)
    , behavior(kHold)
    , useOnlyShutterOpenCloseTimes(false)
    {}

    double getAbcFrameTime() const { return currentTime / fps; }

    void getRelevantSampleTimes(
        Alembic::AbcCoreAbstract::TimeSamplingPtr iTimeSampling,
        size_t iPropertyNumSamples,
        SampleTimes & oTimes) const
    {
        double frameTime = currentTime / fps;

        if (numSamples < 2)
        {
            oTimes.push_back(frameTime);
            return;
        }

        if (iPropertyNumSamples < 2)
        {
            oTimes.push_back(0.0);
            return;
        }

        //TODO, what's a reasonable epsilon?
        static const double epsilon = 1.0/10000.0;

        double shutterOpenTime =
            (currentTime + shutterOpen) / fps;
        double shutterCloseTime =
            (currentTime + shutterClose) / fps;

        //
        // We don't want to grab samples from far outside the shutter window just
        // because the samples immediately near the shutter open and shutter
        // close times are a small epsilon outside the shutter we specify.  The
        // tolerance is already used for the upper bound below, but we'll use it
        // to grab a more expected lower and upper bound.
        //

        std::pair<Alembic::Abc::index_t, Alembic::Abc::chrono_t> shutterOpenFloor =
            iTimeSampling->getNearIndex(shutterOpenTime, iPropertyNumSamples);
        if (fabs(shutterOpenFloor.second - shutterOpenTime) > epsilon)
        {
            shutterOpenFloor =
                iTimeSampling->getFloorIndex(shutterOpenTime, iPropertyNumSamples);
        }

        std::pair<Alembic::Abc::index_t, Alembic::Abc::chrono_t> shutterCloseCeil =
            iTimeSampling->getNearIndex(shutterCloseTime, iPropertyNumSamples);
        if (fabs(shutterCloseCeil.second - shutterCloseTime) > epsilon)
        {
            shutterCloseCeil =
                iTimeSampling->getCeilIndex(shutterCloseTime, iPropertyNumSamples);
        }

        // Use shutter open/close times only if the number of samples is 2 and
        // we explicitly requested them to be used
        if (numSamples == 2 && useOnlyShutterOpenCloseTimes)
        {
            oTimes.push_back(shutterOpenFloor.second);
            oTimes.push_back(shutterCloseCeil.second);
            return;
        }

        for (Alembic::Abc::index_t i = shutterOpenFloor.first;
             i < shutterCloseCeil.first; ++i)
        {
            oTimes.push_back(iTimeSampling->getSampleTime(i));
        }

        //no samples above? put frame time in there and get out
        if (oTimes.empty())
        {
            oTimes.push_back(frameTime);
            return;
        }

        double lastSample = *(oTimes.rbegin());

        //determine whether we need the extra sample at the end
        if ((fabs(lastSample-shutterCloseTime) > epsilon)
                && lastSample<shutterCloseTime)
        {
            oTimes.push_back(shutterCloseCeil.second);
        }
    }

    float getRelativeSampleTime(Alembic::Abc::chrono_t iTime) const
    {
        double frameTime = currentTime / fps;

        double result = (iTime - frameTime) * fps;

        const double tolerance = 0.000165;

        if (Imath::equalWithAbsError(result, 0.0, tolerance))
        {
            result = 0;
        }
        else if (Imath::equalWithAbsError(result, shutterOpen, tolerance))
        {
            result = shutterOpen;
        }
        else if (Imath::equalWithAbsError(result, shutterClose, tolerance))
        {
            result = shutterClose;
        }

        return (float) result;
    }

};

/*
 * Note: This function construct the GroupBuilder with every
 *       parameters, users and arnold parameters. That's the one set
 *       under arnoldStatements in Katana.
 */
void processUserProperties(AbcCookPtr ioCook,
    Alembic::Abc::ICompoundProperty & iParent,
    Foundry::Katana::GroupBuilder & oStaticGb,
    const std::string & iAttrPath,
    const NameMap* iRenameMap=nullptr,
    Alembic::Abc::v10::IUInt64ArrayProperty * iIdsProperty = NULL);

void setArnoldVisibility(
    AbcCookPtr ioCook,
    Alembic::Abc::ICompoundProperty & iParent,
    const Alembic::AbcCoreAbstract::PropertyHeader & iPropHeader,
    const std::string & iPropName,
    FnAttribute::GroupBuilder & oStaticGb);

/*
 * Note: This function construct the GroupBuilder with just user parameters.
 *       That's the one set under geometry.arbitrary in Katana.
 */
void processArnoldUserProperties(
        AbcCookPtr ioCook,
        Alembic::Abc::ICompoundProperty & iParent,
        Foundry::Katana::GroupBuilder & oStaticGb,
        const std::string & iAttrPath);

void initAbcCook(AbcCookPtr ioCookPtr,
                 Foundry::Katana::GroupBuilder & oStaticGb);

void evalObject(AbcCookPtr ioCookPtr, const OpArgs & iArgs,
                Foundry::Katana::GroupBuilder & oGb,
                Alembic::Abc::v10::IUInt64ArrayProperty * iIdsProperty = NULL);

void cookCamera(AbcCookPtr ioCook, Foundry::Katana::GroupBuilder & oStaticGb);

void evalCamera(Alembic::AbcGeom::ICameraSchema & iSchema,
                const OpArgs & iArgs,
                Foundry::Katana::GroupBuilder & oGb);

void cookCurves(AbcCookPtr ioCook, Foundry::Katana::GroupBuilder & oStaticGb);

void evalCurves(Alembic::AbcGeom::ICurvesSchema & iSchema,
                const OpArgs & iArgs,
                Foundry::Katana::GroupBuilder & oGb);

void cookFaceset(AbcCookPtr ioCook, Foundry::Katana::GroupBuilder & oStaticGb);

void cookNuPatch(AbcCookPtr ioCook, Foundry::Katana::GroupBuilder & oStaticGb);
void evalNuPatch(Alembic::AbcGeom::INuPatchSchema & iSchema,
                 const OpArgs & iArgs,
                 bool iIgnoreConstant,
                 Foundry::Katana::GroupBuilder & oGb);

void cookPoints(AbcCookPtr ioCook, Foundry::Katana::GroupBuilder & oStaticGb);

void cookPolyMesh(AbcCookPtr ioCook, Foundry::Katana::GroupBuilder & oStaticGb);

void cookSubd(AbcCookPtr ioCook, Foundry::Katana::GroupBuilder & oStaticGb);

void cookXform(AbcCookPtr ioCook, Foundry::Katana::GroupBuilder & oStaticGb);

void evalXform(Alembic::AbcGeom::IXformSchema & iSchema,
               const OpArgs & iArgs,
               Foundry::Katana::GroupBuilder & oGb);

void cookMaterial(AbcCookPtr ioCook, Foundry::Katana::GroupBuilder & oStaticGb);

void cookLight(AbcCookPtr ioCook, Foundry::Katana::GroupBuilder & oStaticGb);

}

#endif
