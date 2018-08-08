// Copyright 2017 Rodeo FX.  All rights reserved.

#include "UsdToGtPrim.h"
#include <gusd/curvesWrapper.h>
#include <gusd/meshWrapper.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace WalterHoudini
{
GT_PrimitiveHandle getGtPrimFromUsd(
    const UsdPrim& prim,
    const UsdTimeCode& time,
    const GusdPurposeSet purpose)
{
    GT_PrimitiveHandle gtPrimHandle;
    UsdGeomImageable imageable(prim);

    if (prim.IsA<UsdGeomMesh>())
    {
        gtPrimHandle = GusdMeshWrapper::defineForRead(
            imageable, time, purpose);
    }
    else if (prim.IsA<UsdGeomCurves>())
    {
        gtPrimHandle = GusdCurvesWrapper::defineForRead(
            imageable, time, purpose);
    }
    return gtPrimHandle;
}
}; // end namespace WalterHoudini
