// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERUSDOPDELEGATE_H_
#define __WALTERUSDOPDELEGATE_H_

#include <pxr/usd/usd/prim.h>
#include "walterUSDOpIndex.h"

PXR_NAMESPACE_USING_DIRECTIVE

/** @brief It's a set of utilities to fill the index. It's a class to be able to
 * specify the methods as friends of OpIndex. */
class OpDelegate
{
public:
    /**
     * @brief populates the OpIndex with initial data. Usually it's a list of
     * expressions.
     *
     * @param iRoot The root prim.
     * @param oIndex The index to fill.
     */
    static void populate(const UsdPrim& iRoot, OpIndex& oIndex);
};

#endif
