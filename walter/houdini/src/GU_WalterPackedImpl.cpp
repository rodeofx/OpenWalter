// Copyright 2017 Rodeo FX.  All rights reserved.

#include "GU_WalterPackedImpl.h"

#include "walterHoudiniUtils.h"
#include "UsdToGtPrim.h"
#include <FS/UT_DSO.h>
#include <GT/GT_Primitive.h>
#include <GT/GT_RefineParms.h>
#include <GT/GT_Util.h>
#include <GU/GU_PackedFactory.h>
#include <GU/GU_PrimPacked.h>
#include <UT/UT_MemoryCounter.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/curves.h>
#include <pxr/usd/usdGeom/imageable.h>
#include <pxr/usd/usdGeom/mesh.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace WalterHoudini
{

const UT_StringRef GU_WalterPackedImpl::usdFileName = "usdFileName";
const UT_StringRef GU_WalterPackedImpl::usdFrame = "usdFrame";
const UT_StringRef GU_WalterPackedImpl::usdPrimPath = "usdPrimPath";
const UT_StringRef GU_WalterPackedImpl::rendererName = "hdRendererName";

static bool IsSupportedPrim(const UsdPrim& prim)
{
    if (!prim || !prim.IsA<UsdGeomImageable>())
    {
        return false;
    }

    return prim.IsA<UsdGeomMesh>() || prim.IsA<UsdGeomCurves>();
}

class WalterFactory : public GU_PackedFactory
{
public:
    WalterFactory() : GU_PackedFactory("PackedWalter", "Packed Walter")
    {
        registerIntrinsic(
            GU_WalterPackedImpl::usdFileName,
            StringHolderGetterCast(&GU_WalterPackedImpl::intrinsicFileName),
            StringHolderSetterCast(&GU_WalterPackedImpl::setFileName));
        registerIntrinsic(
            GU_WalterPackedImpl::usdPrimPath,
            StringHolderGetterCast(&GU_WalterPackedImpl::intrinsicPrimPath),
            StringHolderSetterCast(&GU_WalterPackedImpl::setPrimPath));
        registerIntrinsic(
            GU_WalterPackedImpl::usdFrame,
            FloatGetterCast(&GU_WalterPackedImpl::intrinsicFrame),
            FloatSetterCast(&GU_WalterPackedImpl::setFrame));
        registerIntrinsic(
            GU_WalterPackedImpl::rendererName,
            StringHolderGetterCast(&GU_WalterPackedImpl::intrinsicRendererName),
            StringHolderSetterCast(&GU_WalterPackedImpl::setRendererName));

    }
    virtual ~WalterFactory()
    {}

    virtual GU_PackedImpl* create() const
    {
        return new GU_WalterPackedImpl();
    }
};

static WalterFactory* theFactory = NULL;

static UT_Lock theLock;

GA_PrimitiveTypeId GU_WalterPackedImpl::theTypeId(-1);

GU_WalterPackedImpl::GU_WalterPackedImpl() : GU_PackedImpl(), mDetail()
{}

GU_WalterPackedImpl::GU_WalterPackedImpl(const GU_WalterPackedImpl& src) :
        GU_PackedImpl(src),
        mDetail()
{
    setFileName(src.intrinsicFileName());
    setPrimPath(src.intrinsicPrimPath());
    setFrame(src.intrinsicFrame());
    setRendererName(src.intrinsicRendererName());
}

GU_WalterPackedImpl::~GU_WalterPackedImpl()
{
    clearWalter();
}

void GU_WalterPackedImpl::install(GA_PrimitiveFactory* gafactory)
{
    UT_ASSERT(!theFactory);
    if (theFactory)
        return;

    theFactory = new WalterFactory();
    GU_PrimPacked::registerPacked(gafactory, theFactory);
    if (theFactory->isRegistered())
    {
        theTypeId = theFactory->typeDef().getId();
    }
    else
    {
        fprintf(
            stderr,
            "[WALTER]: Unable to register packed Walter from %s\n",
            UT_DSO::getRunningFile());
    }
}

void GU_WalterPackedImpl::clearWalter()
{
    mDetail = GU_ConstDetailHandle();
}

GU_PackedFactory* GU_WalterPackedImpl::getFactory() const
{
    return theFactory;
}

GU_PackedImpl* GU_WalterPackedImpl::copy() const
{
    return new GU_WalterPackedImpl(*this);
}

void GU_WalterPackedImpl::clearData()
{
    // This method is called when primitives are "stashed" during the cooking
    // process. However, primitives are typically immediately "unstashed" or
    // they are deleted if the primitives aren't recreated after the fact. We
    // can just leave our data.
}

bool GU_WalterPackedImpl::isValid() const
{
    return detail().isValid();
}

template <typename T> void GU_WalterPackedImpl::updateFrom(const T& options)
{
    UT_StringHolder sval;
    if (import(options, GU_WalterPackedImpl::usdFileName, sval))
    {
        setFileName(sval);
    }

    if (import(options, GU_WalterPackedImpl::usdPrimPath, sval))
    {
        setPrimPath(sval);
    }

    fpreal fval;
    if (import(options, GU_WalterPackedImpl::usdFrame, fval))
    {
        setFrame(fval);
    }

    if (import(options, GU_WalterPackedImpl::rendererName, sval))
    {
        setRendererName(sval);
    }
}

bool GU_WalterPackedImpl::save(UT_Options& options, const GA_SaveMap& map) const
{
    options.setOptionS(GU_WalterPackedImpl::usdFileName, intrinsicFileName());
    options.setOptionF(GU_WalterPackedImpl::usdFrame, intrinsicFrame());
    options.setOptionS(GU_WalterPackedImpl::usdPrimPath, intrinsicPrimPath());
    options.setOptionS(GU_WalterPackedImpl::rendererName, intrinsicRendererName());
    return true;
}

bool GU_WalterPackedImpl::getBounds(UT_BoundingBox& box) const
{
    // All spheres are unit spheres with transforms applied
    box.initBounds(-1, -1, -1);
    box.enlargeBounds(1, 1, 1);

    // TODO: If computing the bounding box is expensive, you may want to cache
    // the box by calling SYSconst_cast(this)->setBoxCache(box);
    return true;
}

bool GU_WalterPackedImpl::getRenderingBounds(UT_BoundingBox& box) const
{
    // When geometry contains points or curves, the width attributes need to be
    // taken into account when computing the rendering bounds.
    return getBounds(box);
}

void GU_WalterPackedImpl::getVelocityRange(
    UT_Vector3& minVelocity,
    UT_Vector3& maxVelocity) const
{
    // No velocity attribute on geometry
    minVelocity = 0;
    maxVelocity = 0;
}

void GU_WalterPackedImpl::getWidthRange(fpreal& min, fpreal& max) const
{
    min = max = 0;  // Width is only important for curves/points.
}

bool GU_WalterPackedImpl::unpack(GU_Detail& destgdp) const
{
    // This may allocate geometry for the primitive
    GU_DetailHandleAutoReadLock rlock(getPackedDetail());
    if (!rlock.getGdp())
    {
        return false;
    }

    return unpackToDetail(destgdp, rlock.getGdp());
}

bool GU_WalterPackedImpl::unpackWithContext(
    GU_Detail& destgdp,
    GU_PackedContext& context) const
{
    UsdStageRefPtr stage =
        WalterHoudiniUtils::getStage(intrinsicFileName().toStdString());
    if (!stage)
    {
        return GU_PackedImpl::unpackWithContext(destgdp, context);
    }

    UsdTimeCode time(intrinsicFrame());

    UsdPrim root;
    if (intrinsicPrimPath().length() > 0)
    {
        root = stage->GetPrimAtPath(SdfPath(intrinsicPrimPath().toStdString()));
    }
    else
    {
        root = stage->GetPseudoRoot();
    }

    UsdPrimRange range(root);
    for (const UsdPrim& prim : range)
    {
        if (!IsSupportedPrim(prim))
        {
            continue;
        }

        // Create a list of geometry details for a single GT primitive. Each
        // refined primitive is stored in a separate detail.
        UT_Array<GU_Detail*> details;
        GT_PrimitiveHandle gtPrim =
            WalterHoudini::getGtPrimFromUsd(prim, time);

        UsdGeomImageable imageable(prim);
        GfMatrix4d gfMatrix = imageable.ComputeLocalToWorldTransform(time);
        UT_Matrix4D utMatrix =
            WalterHoudiniUtils::MatrixConvert<UT_Matrix4D>(gfMatrix);
        // New because it requres GT_TransformHandle. So no need to delete.
        gtPrim->setPrimitiveTransform(new GT_Transform(&utMatrix, 1));

        GT_RefineParms rparms;
        // Need to manually force polysoup to be turned off.
        rparms.setAllowPolySoup(false);
        rparms.set("usd:primvarPattern", "*");

        GA_Size startIndex = destgdp.getNumPrimitives();
        GT_Util::makeGEO(details, gtPrim, &rparms);

        for (GU_Detail* detail : details)
        {
            unpackToDetail(destgdp, detail, true);
            // These details must be deleted by the caller.
            delete detail;
        }
        // Add usdpath and usdprimpath attributes to unpacked geometry.
        GA_Size endIndex = destgdp.getNumPrimitives();

        const char* path = prim.GetPrim().GetPath().GetString().c_str();

        if (endIndex > startIndex)
        {
            GA_RWHandleS primPathAttr(destgdp.addStringTuple(
                GA_ATTRIB_PRIMITIVE, "usdprimpath", 1));
            GA_RWHandleS pathAttr(
                destgdp.addStringTuple(GA_ATTRIB_PRIMITIVE, "usdpath", 1));

            for (GA_Size i = startIndex; i < endIndex; ++i)
            {
                primPathAttr.set(destgdp.primitiveOffset(i), 0, path);
                pathAttr.set(
                    destgdp.primitiveOffset(i), 0, intrinsicFileName().c_str());
            }
        }
    }

    return true;
}

int64 GU_WalterPackedImpl::getMemoryUsage(bool inclusive) const
{
    int64 mem = inclusive ? sizeof(*this) : 0;

    // Don't count the (shared) GU_Detail, since that will greatly
    // over-estimate the overall memory usage.
    mem += detail().getMemoryUsage(false);

    return mem;
}

void GU_WalterPackedImpl::countMemory(
    UT_MemoryCounter& memoryCounter,
    bool inclusiveUsage) const
{
    if (memoryCounter.mustCountUnshared())
    {
        size_t mem = inclusiveUsage ? sizeof(*this) : 0;
        mem += detail().getMemoryUsage(false);
        UT_MEMORY_DEBUG_LOG("GU_WalterPackedImpl", int64(mem));
        memoryCounter.countUnshared(mem);
    }
}

GU_PrimPacked* GU_WalterPackedImpl::build(
    GU_Detail& gdp,
    const std::string& fileName,
    const std::string& primPath,
    const std::string& rendererName,
    fpreal frame)
{
    GA_Primitive* prim = gdp.appendPrimitive(theFactory->typeDef().getId());
    // Note:  The primitive is invalid until you do something like
    //   prim->setVertexPoint(gdp.appendPointOffset());
    GU_PrimPacked* pack = UTverify_cast<GU_PrimPacked*>(prim);

    // Cast it to Walter.
    GU_WalterPackedImpl* walter =
        UTverify_cast<GU_WalterPackedImpl*>(pack->implementation());
    walter->setFileName(fileName);
    walter->setPrimPath(primPath);
    walter->setRendererName(rendererName);
    walter->setFrame(frame);
    return pack;
}

void GU_WalterPackedImpl::setFileName(const UT_StringHolder& v)
{
    if (mFileName != v)
    {
        mFileName = v;
        // TODO: Clear cache, make dirty.
    }
}

void GU_WalterPackedImpl::setPrimPath(const UT_StringHolder& v)
{
    if (mPrimPath != v)
    {
        mPrimPath = v;
        // TODO: Clear cache, make dirty.
    }
}

void GU_WalterPackedImpl::setFrame(fpreal frame)
{
    if (frame != mFrame)
    {
        mFrame = frame;
        // TODO: Reset cache, make dirty, update transforms.
    }
}

void GU_WalterPackedImpl::setRendererName(const UT_StringHolder& v)
{
    if (mRendererName != v)
    {
        mRendererName = v;
        // TODO: Clear cache, make dirty.
    }
}

void GU_WalterPackedImpl::update(const UT_Options& options)
{
    updateFrom(options);
}

bool GU_WalterPackedImpl::load(
    GU_PrimPacked* prim,
    const UT_Options& options,
    const GA_LoadMap&)
{
    // Loading Walter is simple, we can just update the parameters.
    updateFrom(options);
    return true;
}

} // end namespace WalterHoudini
