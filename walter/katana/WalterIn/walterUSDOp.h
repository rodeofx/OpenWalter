// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERUSDOP_H_
#define __WALTERUSDOP_H_

#include <FnGeolib/op/FnGeolibOp.h>

/**
 * @brief The entry point for USD scene graph generation.
 *
 * @param ioInterface A reference to a valid GeolibCookInterface object.
 * @param iArchives List of USD archives to load.
 */
void cookUSDRoot(
    Foundry::Katana::GeolibCookInterface& ioInterface,
    const std::vector<std::string>& iArchives);

/** @brief  */
void registerUSDPlugins();

#endif
