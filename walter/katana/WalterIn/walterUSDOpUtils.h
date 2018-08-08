// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERUSDOPUTILS_H_
#define __WALTERUSDOPUTILS_H_

#include <pxr/base/tf/debug.h>
#include <pxr/usd/sdf/path.h>
#include <sys/syscall.h>
#include <sys/types.h>

#ifndef NDEBUG
#define W_DEBUG(MSG, ...) \
    TF_DEBUG(WALTER_KATANA) \
        .Msg( \
            "[WALTER d 0x%lx %s]: " MSG "\n", \
            syscall(SYS_gettid), \
            __FUNCTION__, \
            __VA_ARGS__)
#else
#define W_DEBUG(MSG, ...)
#endif

PXR_NAMESPACE_OPEN_SCOPE
TF_DEBUG_CODES(WALTER_KATANA);
PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

class OpEngine;

namespace OpUtils
{
/** @brief Represents the time and all the samples that it has. We should
 * use this object everywhere it possible. */
class Time : private boost::noncopyable
{
public:
    typedef double TimeType;
    typedef std::vector<TimeType> TimeRange;

    Time(
        TimeType iCurrent,
        TimeType iOpen,
        TimeType iClose,
        size_t iSamples = 1);

    TimeType current() const { return mCurrent; }
    size_t size() const { return mSamples.size(); }
    const TimeRange& samples() const { return mSamples; }

private:
    TimeRange mSamples;
    TimeType mCurrent;
};

typedef std::shared_ptr<Time> TimePtr;

/** @brief The class that should be created for each single Katana node and
 * contains the information for this node that it's not possible to put into
 * index. */
class PrivateData
{
public:
    PrivateData(
        OpEngine& iEngine,
        const std::string iKatanaRoot,
        const SdfPath& iCurrentPath,
        TimePtr iTime);

    OpEngine& engine() const { return *mEngine; }
    const SdfPath& path() const { return mCurrentPath; }
    const std::string& root() const { return mKatanaRoot; }
    TimePtr time() const { return mTime; }

    /**
     * @brief Delete this object.
     *
     * @param data It should always be PrivateData.
     */
    static void kill(void* data) { delete (PrivateData*)data; }

private:
    OpEngine* mEngine;

    // The Katana path to the WalterIn object. We can't put it to engine/index
    // because it's possible to have several nodes with the same engine.
    std::string mKatanaRoot;
    SdfPath mCurrentPath;
    TimePtr mTime;
};

/**
 * @brief The attribute name in USD and in Katana are different. It converts USD
 * name to full Katana name. "disp_height" -> "arnoldStatements.disp_height".
 *
 * @param iName "disp_height"
 *
 * @return "arnoldStatements.disp_height"
 */
std::string getClientAttributeName(const std::string& iName);
}

#endif
