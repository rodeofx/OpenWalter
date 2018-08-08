// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __UsdToGtPrim_h__
#define __UsdToGtPrim_h__

#include <GT/GT_Handles.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>

#include <gusd/purpose.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace WalterHoudini
{

/**
 * @brief Copy data from a Usd Prim to a GT_Primitive.
 *
 * We put this function in a seperated compilation unit to clarify
 * that it is the single point of contact with the Pixar Usd plugin for Houdini
 * (as we are using its GusdPrimWrapper objects to extract Usd data).
 *
 * @param prim The source prim.
 * @param time The time to get the data.
 * @param purpose From which purpose you want to get the data.
 */
GT_PrimitiveHandle getGtPrimFromUsd(
    const UsdPrim& prim,
    const UsdTimeCode& time,
    const GusdPurposeSet purpose = GUSD_PURPOSE_DEFAULT);
};

#endif
