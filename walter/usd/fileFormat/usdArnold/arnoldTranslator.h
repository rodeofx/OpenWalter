// Copyright 2018 Rodeo FX.  All rights reserved.

#ifndef __ARNOLDTRANSLATOR_H__
#define __ARNOLDTRANSLATOR_H__

#include <pxr/usd/sdf/layer.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace ArnoldUSDTranslator
{
    /**
     * @brief Convert ass file to SdfLayer.
     *
     * @param iFile Full file name.
     *
     * @return The pointer to the created layer.
     */
    SdfLayerRefPtr arnoldToUsdLayer(const std::string& iFile);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
