// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERUSDCOMMONUTILS_H__
#define __WALTERUSDCOMMONUTILS_H__

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stage.h"
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace WalterUSDCommonUtils
{
/**
 * @brief Compose and open several usd layers.
 *
 * @param path One or several USD layers separated with ":" symbol.
 *
 * @return The pointer to the new layer.
 */
SdfLayerRefPtr getUSDLayer(const std::string& path);

/**
 * @brief Compose and open several usd layers.
 *
 * @param paths One or several USD layes as a vector.
 *
 * @return The pointer to the new layer.
 */
SdfLayerRefPtr getUSDLayer(const std::vector<std::string>& paths);

/**
 * @brief Creates or returns an anonymous layer that can be used to modify the
 * stage.
 *
 * @param stage The stage to modify. The layer will be created on the front of
 * this stage.
 * @param name The name of the layer. If the stage already contains the layer
 * with requeted name, this layer will be returned.
 *
 * @return The created or existed USD layer.
 */
SdfLayerRefPtr getAnonymousLayer(UsdStageRefPtr stage, const std::string& name);

/**
 * @brief Gets or creates the variants layer and sets its content from external
 * references (e.g Maya scene).
 *
 * @param stage The USD stage.
 * @param variants The variants list (in a flat string).
 *
 * @return True if succeded, false otherwise.
 */
bool setUSDVariantsLayer(UsdStageRefPtr stage, const char* variants);

/**
 * @brief Gets or creates the purpose layer and sets its content from external
 * references (e.g Maya scene).
 *
 * @param stage The USD stage.
 * @param purpose The purpose list (in a flat string).
 *
 * @return True if succeded, false otherwise.
 */
bool setUSDPurposeLayer(UsdStageRefPtr stage, const char* purpose);

/**
 * @brief Gets or creates the visibility layer and sets its content from external
 * references (e.g Maya scene).
 *
 * @param stage The USD stage.
 * @param visibility The visibility list (in a flat string).
 *
 * @return True if succeded, false otherwise.
 */
bool setUSDVisibilityLayer(UsdStageRefPtr stage, const char* visibility);

/**
 * @brief Constructs the variant layer.
 *
 * @param stage The USD stage.
 *
 * @return The variants list in a flat string.
 */
std::string getVariantsLayerAsText(UsdStageRefPtr stage);

/**
 * @brief Constructs the visibility layer.
 *
 * @param stage The USD stage.
 *
 * @return The visibility list in a flat string.
 */
std::string getVisibilityLayerAsText(UsdStageRefPtr stage);

/**
 * @brief Constructs the purpose layer.
 *
 * @param stage The USD stage.
 *
 * @return The purpose list in a flat string.
 */
std::string getPurposeLayerAsText(UsdStageRefPtr stage);

/**
 * @brief Fills result with the names of the primitive children.
 *
 * @param stage The USD stage.
 * @param primPath The primitive path (as string).
 *
 * @return The children names (as string array).
 */
std::vector<std::string> dirUSD(UsdStageRefPtr stage, const char* primPath);

/**
 * @brief Fills result array with names/values of properties (USD attributes
 * and/or relationships) of the primitive at path primPath.
 *
 * @param stage The USD stage.
 * @param primPath The primitive path (as string).
 * @param time The time.
 * @param attributeOnly If true, only attributes properties (no relation-ship)
 * will be extracted.
 *
 * @return The properties (as an array of json).
 */
std::vector<std::string> propertiesUSD(
    UsdStageRefPtr stage,
    const char* primPath,
    double time,
    bool attributeOnly = true);

/**
 * @brief Fills result with all the USD variants of the stage.
 *
 * @param stage The USD stage.
 *
 * @return The variants list (as an array of json).
 */
std::vector<std::string> getVariantsUSD(UsdStageRefPtr stage);

/**
 * @brief Sets the variants of the primitive at path primPath.
 *
 * @param stage The USD stage.
 * @param primPath The primitive path (as string).
 * @param variantName The variants name.
 * @param variantSetName The variant set name.
 *
 */
void setVariantUSD(
    UsdStageRefPtr stage,
    const char* primPath,
    const char* variantName,
    const char* variantSetName);

/**
 * @brief Sets the variants of the primitive at path primPath.
 *
 * @param stage The USD stage.
 * @param variantDefStr flat representation
 * (primPath{variantSetName=variantName}).
 *
 */
void setVariantUSD(UsdStageRefPtr stage, const char* variantDefStr);

/**
 * @brief Gets the names/values of USD primVars attributes
 * of the primitive at path primPath.
 *
 * @param stage The USD stage.
 * @param primPath The primitive path (as an array of json).
 * @param time The time.
 *
 * @return The properties (as string array).
 */
std::vector<std::string> primVarUSD(
    UsdStageRefPtr stage,
    const char* primPath,
    double time);

/**
 * @brief Checks that the prim at path is a USD pseudo root.
 *
 * @param stage The USD stage.
 * @param primPath The primitive path.
 *
 * @return True if the prim at path is a pseudo-root, false otherwise.
 */
bool isPseudoRoot(
    UsdStageRefPtr stage,
    const char* primPath);

/**
 * @brief Toggles the visibility state (show, hidden) of the prim.
 *
 * @param stage The USD stage.
 * @param primPath The primitive path.
 *
 * @return True if the prim at path is visible, false otherwise.
 */
bool toggleVisibility(
    UsdStageRefPtr stage,
    const char* primPath);

/**
 * @brief Sets the visibility state (show, hidden) of the prim.
 *
 * @param stage The USD stage.
 * @param primPath The primitive path.
 *
 * @return True if the prim at path is visible, false otherwise.
 */
bool setVisibility(
    UsdStageRefPtr stage,
    const char* primPath,
    bool visible);

/**
 * @brief Checks if the prim at path is visible
 *
 * @param stage The USD stage.
 * @param primPath The primitive path.
 *
 * @return True if the prim is visible, false otherwise.
 */
bool isVisible(
    UsdStageRefPtr stage,
    const char* primPath);

/**
 * @brief Hides all the prims stage execpted the primitive at path.
 *
 * @param stage The USD stage.
 * @param primPath The primitive path.
 *
 * @return True if the prim at path is visible, false otherwise.
 */
bool hideAllExceptedThis(
    UsdStageRefPtr stage,
    const char* primPath);

/**
 * @brief Check that the regex expression matchs a least one USD prim path
 * primitive at path primPath.
 *
 * @param stage The USD stage.
 * @param expression The primitive path (as an array of json).
 *
 * @return True if there is a match, false otherwise.
 */
bool expressionIsMatching(
    UsdStageRefPtr stage,
    const char* expression);

/**
 * @brief Gets the list of primitives matching the expression.
 *
 * @param stage The USD stage.
 * @param expression A regex expression.
 *
 * @return A vector of USD prims.
 */
std::vector<UsdPrim> primsMatchingExpression(
    UsdStageRefPtr stage,
    const char* expression);

/**
 * @brief Gets the list of primitives path matching the expression.
 *
 * @param stage The USD stage.
 * @param expression A regex expression.
 *
 * @return A vector of USD prims path (string).
 */
std::vector<std::string> primPathsMatchingExpression(
    UsdStageRefPtr stage,
    const char* expression);

/**
 * @brief Sets the prim purpose token (default, render, proxy, guide).
 *
 * @param stage The USD stage.
 * @param primPath The primitive path.
 * @param purpose The purpose token.
 *
 * @return The purpose token (as string)
 */
std::string setPurpose(
    UsdStageRefPtr stage,
    const char* primPath,
    const char* purpose);

/**
 * @brief Gets the prim purpose token.
 *
 * @param stage The USD stage.
 * @param primPath The primitive path.
 *
 * @return The purpose token (as string)
 */
std::string purpose(
    UsdStageRefPtr stage,
    const char* primPath);

/**
 * @brief Store information for a master primitive, like the name of the
 * primitive it describe and the list of variant selection of it.
 */
struct MasterPrimInfo
{
    std::string realName;
    std::vector<std::string> variantsSelection;
    bool isEmpty() { return realName == ""; }
};

typedef std::unordered_map<std::string, MasterPrimInfo> MastersInfoMap;

/**
 * @brief Gets the binding between master primitive name and their
 * information such that the name of the primitive it represents and the
 * variants selection used to build this master prim.
 *
 * @param stage The USD stage.
 *
 * @return A map of master name/master information.
 */
MastersInfoMap getMastersInfo(UsdStageRefPtr stage);

/**
 * @brief Get the list of variants selection for a given path.
 *
 * @param path The SdfPath on which to extract the list of variant selection.
 *
 * @return A list of variant selections (strings).
 */
std::vector<std::string> getVariantsSelection(const SdfPath& path);

} // namespace WalterUSDCommonUtils

#endif // __WALTERUSDCOMMONUTILS_H__
