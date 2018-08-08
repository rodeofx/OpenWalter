// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERTHREADINGUTILS_H_
#define __WALTERTHREADINGUTILS_H_

#include <maya/MObject.h>

namespace WalterThreadingUtils
{
/**
 * @brief Starts loading stage in a background thread.
 *
 * @param iObj Walter standin to load.
 */
void loadStage(const MObject& iObj);

void joinAll();
}

#endif
