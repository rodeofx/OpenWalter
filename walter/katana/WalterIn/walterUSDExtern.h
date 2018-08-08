// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERUSDEXTERN_H_
#define __WALTERUSDEXTERN_H_

#include "pxr/usd/usd/stage.h"
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

class WalterUSDExtern
{
public:
    /**
     * @brief Sets the USD identifier (url/filepath).
     *
     * @param identifier The USD identifier.
     *
     */
    void setIdentifier(const char* identifiers);

    /**
     * @brief Extracts the variants contained in the USD scene.
     *
     */
    void extractVariantsUsd();

    /**
     * @brief Gets the variants list (per primitive).
     *
     * @return A serie of json separate by `|`.
     */
    std::string& getVariantsUsd();

    /**
     * @brief Sets the variants of the primitive.
     *
     * @param primPath The primitive path (as string).
     * @param variantName The variants name.
     * @param variantSetName The variant set name.
     *
     * @return The variant lists as a flat string.
     */
    std::string setVariantUsd(
        const char* primPath,
        const char* variantSetName,
        const char* variantName);

    /**
     * @brief Sets the variants of the primitive.
     *
     * @param variants The variant (primPath{variantSet=variant}).
     *
     * @return A flatten representation of the operation.
     */
    std::string setVariantUsd(const char* variants);

    /**
     * @brief Gets the number of USD primitives.
     *
     * @return Number of USD primitives.
     */
    int getPrimsCount();

private:
    int mPrimsCount;
    std::string mFlattenVariants;
    std::vector<std::string> mIdentifiers;
};

#endif
