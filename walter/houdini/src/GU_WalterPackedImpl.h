// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __GU_WalterPackedImpl__
#define __GU_WalterPackedImpl__

#include <GU/GU_PackedImpl.h>
#include <SYS/SYS_Version.h>

class GU_PrimPacked;

namespace WalterHoudini
{

class GU_WalterPackedImpl : public GU_PackedImpl
{
public:
    GU_WalterPackedImpl();
    GU_WalterPackedImpl(const GU_WalterPackedImpl& src);
    virtual ~GU_WalterPackedImpl();

    static void install(GA_PrimitiveFactory* factory);

    // Get the type ID for the GU_WalterPackedImpl primitive type.
    static GA_PrimitiveTypeId typeId()
    {
        return theTypeId;
    }

    // Virtual interface from GU_PackedImpl interface
    virtual GU_PackedFactory* getFactory() const;
    virtual GU_PackedImpl* copy() const;
    virtual void clearData();

    virtual bool isValid() const;
    virtual bool supportsJSONLoad() const
    {
        return true;
    }
    virtual bool loadFromJSON(GU_PrimPacked *prim, const UT_JSONValueMap& options, const GA_LoadMap&)
    {
        updateFrom(options);
        return true;
    }
    virtual bool save(UT_Options& options, const GA_SaveMap& map) const;
    virtual bool getBounds(UT_BoundingBox& box) const;
    virtual bool getRenderingBounds(UT_BoundingBox& box) const;
    virtual void getVelocityRange(
        UT_Vector3& minVelocity,
        UT_Vector3& maxVelocity) const;
    virtual void getWidthRange(fpreal& min, fpreal& max) const;
    // Unpack the procedural into a GU_Detail. By default, this calls
    // getGTFull() and converts the GT geometry to a GU_Detail.
    virtual bool unpack(GU_Detail& destgdp) const;
    virtual bool unpackWithContext(
        GU_Detail& destgdp,
        GU_PackedContext& context) const;

    // Report memory usage (includes all shared memory)
    virtual int64 getMemoryUsage(bool inclusive) const;

    // Count memory usage using a UT_MemoryCounter in order to count
    // shared memory correctly.
    virtual void countMemory(
        UT_MemoryCounter& memoryCounter,
        bool inclusiveUsage) const;

    // Member data accessors for intrinsics
    const GU_ConstDetailHandle& detail() const
    {
        return mDetail;
    }
    void setDetail(const GU_ConstDetailHandle& h)
    {
        mDetail = h;
    }

    void setFileName(const UT_StringHolder& v);
    UT_StringHolder intrinsicFileName() const
    {
        return mFileName;
    }

    UT_StringHolder intrinsicFileName(const GU_PrimPacked* prim) const
    {
        return intrinsicFileName();
    }
    void setFileName(GU_PrimPacked* prim, const UT_StringHolder& fileName)
    {
        setFileName(fileName);
    }

    void setPrimPath(const UT_StringHolder& v);
    UT_StringHolder intrinsicPrimPath() const
    {
        return mPrimPath;
    }

    UT_StringHolder intrinsicPrimPath(const GU_PrimPacked* prim) const
    {
        return intrinsicPrimPath();
    }
    void setPrimPath(GU_PrimPacked* prim, const UT_StringHolder& p)
    {
        setPrimPath(p);
    }

    void setFrame(fpreal frame);
    fpreal intrinsicFrame() const
    {
        return mFrame;
    }

    fpreal intrinsicFrame(const GU_PrimPacked* prim) const
    {
        return intrinsicFrame();
    }
    void setFrame(GU_PrimPacked* prim, fpreal frame) { setFrame(frame); }

    void setRendererName(const UT_StringHolder& v);
    UT_StringHolder intrinsicRendererName() const
    {
        return mRendererName;
    }

    UT_StringHolder intrinsicRendererName(const GU_PrimPacked* prim) const
    {
        return intrinsicRendererName();
    }
    void setRendererName(GU_PrimPacked* prim, const UT_StringHolder& p)
    {
        setRendererName(p);
    }

    // Note: The caller is responsible for setting the vertex/point for the
    // primitive.
    // In non-Walter packed primitive code, you probably just want to call
    //   GU_PrimPacked::build(gdp, "AlembicRef")
    // which handles the point creation automatically.
    static GU_PrimPacked* build(
        GU_Detail& gdp,
        const std::string& fileName,
        const std::string& primPath,
        const std::string& rendererName,
        fpreal frame);

    void update(const UT_Options& options);
    virtual bool load(
        GU_PrimPacked* prim,
        const UT_Options& options,
        const GA_LoadMap& map) override;

    virtual void update(GU_PrimPacked* prim, const UT_Options& options) override
    {
        update(options);
    }

    static const UT_StringRef usdFileName;
    static const UT_StringRef usdFrame;
    static const UT_StringRef usdPrimPath;
    static const UT_StringRef rendererName;


protected:
    /// updateFrom() will update from either UT_Options or UT_JSONValueMap
    template <typename T> void updateFrom(const T& options);

private:
    void clearWalter();

    GU_ConstDetailHandle mDetail;

    // Intrinsics
    UT_StringHolder mFileName;
    UT_StringHolder mPrimPath;
    fpreal mFrame;
    UT_StringHolder mRendererName;

    static GA_PrimitiveTypeId theTypeId;
};
} // end namespace WalterHoudini

#endif
