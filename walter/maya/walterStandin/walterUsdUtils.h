// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERUSDUTILS_H_
#define __WALTERUSDUTILS_H_

#ifdef MFB_ALT_PACKAGE_NAME

#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MTime.h>
#include <string>
#include <unordered_map>

namespace WalterMaya
{
class ShapeNode;
}

// Several utilities to Import/Export data to USD.

/**
 * @brief Construct and return the session layer from the given object.
 *
 * @param obj Walter Standin node.
 *
 * @return The session layer as a text.
 */
std::string constructUSDSessionLayer(MObject obj);

/**
 * @brief Construct and return the variants layer from the given object.
 *
 * @param obj Walter Standin node.
 *
 * @return The variants layer as a text.
 */
std::string getVariantsLayerAsText(MObject obj);


/**
 * @brief Construct and return the purpose layer from the given object.
 *
 * @param obj Walter Standin node.
 *
 * @return The purpose layer as a text.
 */
std::string getPurposeLayerAsText(MObject obj);

/**
 * @brief Construct and return the visibility layer from the given object.
 *
 * @param obj Walter Standin node.
 *
 * @return The visibility layer as a text.
 */
std::string getVisibilityLayerAsText(MObject obj);

/**
 * @brief Puts a new session layer to the object.
 *
 * @param obj Walter Standin node.
 * @param session The session layer as a text.
 */
void setUSDSessionLayer(MObject obj, const std::string& session);

/**
 * @brief Puts a new variants layer to the object.
 *
 * @param obj Walter Standin node.
 * @param variants The variants layer as a text.
 */
void setUSDVariantsLayer(MObject obj, const std::string& variants);

/**
 * @brief Puts a new purpose layer to the object.
 *
 * @param obj Walter Standin node.
 * @param purpose The purpose layer as a text.
 */
void setUSDPurposeLayer(MObject obj, const std::string& purpose);

// Gets the transform of an USD object. Returns true if success.
bool getUSDTransform(
    MObject obj,
    const std::string& object,
    double time,
    double matrix[4][4]);

/**
 * @brief resets all the transforms at any time.
 *
 * @param node A ShapeNode.
 * @param subNodeName The subnode name (path).
 */
void resetUSDTransforms(
    const WalterMaya::ShapeNode* node,
    const MString& subNodeName);

/**
 * @brief Fills result with names of children of the subNode `subNodeName`.
 *
 * @param node A ShapeNode.
 * @param subNodeName The subnode name (path).
 * @param result The children names (as string array).
 *
 * @return True if succeded, false otherwise.
 */
bool dirUSD(
    const WalterMaya::ShapeNode* node,
    const MString& subNodeName,
    MStringArray& result);

/**
 * @brief Puts a new visibility layer to the object.
 *
 * @param obj Walter Standin node.
 * @param visibility The visibility layer as a text.
 */
void setUSDVisibilityLayer(MObject obj, const std::string& visibility);

/**
 * @brief Fills result with names/values of properties (USD attributes and/or
 * relationships) of the subNode `subNodeName`.
 *
 * @param node A ShapeNode.
 * @param subNodeName The subnode name (path).
 * @param result The children names (as an array of json).
 * @param attributeOnly If true, only attributes properties (no relation-ship)
 *
 * @return True if succeded, false otherwise.
 */
bool propertiesUSD(
    const WalterMaya::ShapeNode* node,
    const MString& subNodeName,
    MStringArray& result,
    bool attributeOnly = true);

/**
 * @brief Fills result with all the USD variants of the stage of the node.
 *
 * @param node A ShapeNode.
 * @param result The variants (as an array of json).
 *
 * @return True if succeded, false otherwise.
 */
bool getVariantsUSD(
    const WalterMaya::ShapeNode* node,
    const MString& subNodeName,
    bool recursively,
    MStringArray& result);

/**
 * @brief Sets the variants of the primitive at path primPath.
 *
 * @param node A ShapeNode.
 * @param subNodeName The subnode name (path).
 * @param variantName The variants name.
 * @param variantSetName The variant set name.
 *
 * @return True if succeded, false otherwise.
 */
bool setVariantUSD(
    WalterMaya::ShapeNode* node,
    const MString& subNodeName,
    const MString& variantName,
    const MString& variantSetName);

// Fills result with names/values of walter assigned overrides of subNodeName.
bool assignedOverridesUSD(
    const WalterMaya::ShapeNode* node,
    const MString& subNodeName,
    MStringArray& result);

/**
 * @brief Fills result with names/values of USD primVars attributes of
 * subNodeName.
 *
 * @param stage The USD stage.
 * @param subNodeName The subnode name (path).
 * @param result The properties (as an array of json).
 *
 */
bool primVarUSD(
    const WalterMaya::ShapeNode* node,
    const MString& subNodeName,
    MStringArray& result);

// Sets the transformations of several of objects to the USD stage.
bool updateUSDTransforms(
    const WalterMaya::ShapeNode* node,
    const std::unordered_map<std::string, MMatrix>& matrices);

/**
 * @brief Export the transformations to the external file.
 *
 * @param objectName The Walter shape name.
 * @param fileName The USD file path.
 * @param start The start frame.
 * @param end The end frame.
 * @param step The time step.
 * @param resetSessionLayer Clear the session layer before export.
 */
bool saveUSDTransforms(
    const MString& objectName,
    const MString& fileName,
    const MTime& start,
    const MTime& end,
    const MTime& step,
    bool resetSessionLayer = true);

// Merge all the objects togeteger based on the connected transformations.
bool freezeUSDTransforms(const WalterMaya::ShapeNode* userNode);
bool unfreezeUSDTransforms(const WalterMaya::ShapeNode* userNode);

/**
 * @brief Attach she GLSL material to the objects so it can be rendered with
 * Hydra.
 *
 * @param userNode The walterStandin node
 */
void processGLSLShaders(
    const WalterMaya::ShapeNode* userNode,
    bool logErrorIfTokenNotFound = false);

/**
 * @brief Mute/unmute layer with Hydra shaders, to it's possible to
 * disable/enable textures.
 *
 * @param userNode The walterStandin node
 * @param iSetVisible True for UnmuteLayer.
 */
void muteGLSLShaders(const WalterMaya::ShapeNode* userNode, bool iSetVisible);

/**
 * @brief Returns USD layer exported to the string.
 *
 * @param userNode The standin object
 * @param layerName The name of the requested layer. If the name is empty, the
 * flatten stage will be returned. To get the session layer, it's necessary to
 * specify "session".
 *
 * @return USD layer as string.
 */
MString getLayerAsString(
    const WalterMaya::ShapeNode* userNode,
    const MString& layerName);

/**
 * @brief Construct and return the layer with in-scene modification of the
 * object like assignments, attributes, transforms.
 *
 * @param obj Walter Standin node.
 *
 * @return The layer as a string.
 */
std::string getMayaStateAsUSDLayer(MObject obj);

bool extractAssignments(WalterMaya::ShapeNode* iUserNode);

/**
 * @brief Gets all the Hydra plugins.
 *
 * @param result The list of Hydra plugins.
 */
void getHydraPlugins(MStringArray& result);

/**
 * @brief Checks if the object is an USD pseudo-root.
 *
 * @param node Walter Standin node.
 * @param subNodeName The subnode name (path).
 *
 * @param result True if the object is a USD pseudo-root, false otherwise.
 */
bool isPseudoRoot(
    const WalterMaya::ShapeNode* node,
    const MString& subNodeName);

/**
 * @brief Checks if the object is visible.
 *
 * @param node Walter Standin node.
 * @param subNodeName The subnode name (path).
 *
 * @param result True if the object is a visible, false otherwise.
 */
bool isVisible(
    const WalterMaya::ShapeNode* node,
    const MString& subNodeName);

/**
 * @brief Toggles the visibility state (show, hidden) of the prim.
 *
 * @param node Walter Standin node.
 * @param subNodeName The subnode name (path).
 *
 * @return True if the prim at path is visible, false otherwise.
 */
bool toggleVisibility(
    const WalterMaya::ShapeNode* node,
    const MString& subNodeName);

/**
 * @brief Sets the visibility state (show, hidden) of the prim.
 *
 * @param node Walter Standin node.
 * @param subNodeName The subnode name (path).
 *
 * @return True if the prim at path is visible, false otherwise.
 */
bool setVisibility(
    const WalterMaya::ShapeNode* node,
    const MString& subNodeName,
    bool visible);

/**
 * @brief Hides all the prims stage execpted the primitive at path.
 *
 * @param node Walter Standin node.
 * @param subNodeName The subnode name (path).
 *
 * @param result True if the object is a visible, false otherwise.
 */
bool hideAllExceptedThis(
    const WalterMaya::ShapeNode* node,
    const MString& subNodeName);

/**
 * @brief Gets the list of primitives path matching the expression.
 *
 * @param node Walter Standin node.
 * @param expression A regex expression.
 *
 * @return An array of USD prims path.
 */
MStringArray primPathsMatchingExpression(
    const WalterMaya::ShapeNode* node,
    const MString& expression);

/**
 * @brief Sets the prim purpose token (default, render, proxy, guide).
 *
 * @param node Walter Standin node.
 * @param subNodeName The subnode name (path).
 * @param purpose The purpose token.
 *
 * @return The purpose token (as string)
 */
MString setPurpose(
    WalterMaya::ShapeNode* node,
    const MString& subNodeName,
    const MString& purpose);

/**
 * @brief Gets the prim purpose token.
 *
 * @param node Walter Standin node.
 * @param subNodeName The subnode name (path).
 *
 * @return The purpose token (as string)
 */
MString getPurpose(
    const WalterMaya::ShapeNode* node,
    const MString& subNodeName);

/**
 * @brief Sets the render engine purposes list, i.e the prim
 * purposes that the engine can render.
 *
 * @param node Walter Standin node.
 * @param purposes Array of purpose token.
 */
void setRenderPurpose(
    WalterMaya::ShapeNode* node,
    const MStringArray& purposes);

/**
 * @brief Gets the render engine purposes list, i.e the prim
 * purposes that the engine can render.
 *
 * @param node Walter Standin node.
 *
 * @return Array of purpose token.
 */
MStringArray getRenderPurpose(
    const WalterMaya::ShapeNode* node);

/**
 * @brief Export the purposes layer content to the external file.
 *
 * @param objectName The Walter shape name.
 * @param fileName The USD file path.
 */
bool savePurposes(
    const MString& objectName,
    const MString& fileName);

/**
 * @brief Export the variants layer content to the external file.
 *
 * @param objectName The Walter shape name.
 * @param fileName The USD file path.
 */
bool saveVariantsLayer(
    const MString& objectName,
    const MString& fileName);

#endif  // MFB_ALT_PACKAGE_NAME

#endif  // __WALTERUSDUTILS_H_
