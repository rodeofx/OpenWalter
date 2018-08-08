// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef _walterCmd_h_
#define _walterCmd_h_


#include <maya/MPxCommand.h>

#include <maya/MArgDatabase.h>
#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MSelectionList.h>
#include <maya/MTime.h>

#include <cassert>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace WalterMaya
{
class Command : public MPxCommand
{
public:
    static const char* LOCATOR_ATTR_PRIM_PATH;
    
    static MSyntax cmdSyntax();
    static void* creator();

    Command();
    virtual ~Command();

    virtual MStatus doIt(const MArgList&);
    virtual bool isUndoable() const;
    virtual bool hasSyntax() const;

private:
    MStatus clearSubSelection(const MString& objectName) const;

    MStatus addSubSelection(
        const MString& objectName,
        const MString& subNodeName) const;

    MStatus dirAlembic(const MString& objectName, const MString& subNodeName)
        const;

    MStatus propsAlembic(const MString& objectName, const MString& subNodeName)
        const;

    MStatus getVariants(
        const MString& objectName) const;

    MStatus setVariant(
        const MString& objectName,
        const MString& subNodeName,
        const MString& variantName,
        const MString& variantSetName) const;

    // Write list of objects with given layer and target to the result of the
    // command.
    MStatus getAssignmentObjects(
        const MString& objectName,
        const std::string& layer,
        const std::string& target) const;

    // Write all the used layers to the result of the command.
    MStatus getAssignmentLayers(const MString& objectName) const;

    // Write the shader name of the specified object to the result of the
    // command.
    MStatus getAssignment(
        const MString& objectName,
        const MString& subNodeName,
        const MString& layer,
        const MString& type) const;

    // Write all the used layers to the result of the command.
    MStatus getAlembicAllGroups(const MString& objectName) const;

    // Write all the expressions of the specified group to the result of the
    // command.
    MStatus getAlembicExpressions(
        const MString& objectName,
        const MString* groupName) const;

    // Create a locator node(s) and pass their transforms to the walterStandin.
    MStatus exposeTransform(
        const MString& objectName,
        const MString& subObjectName) const;

    MStatus detachTransform(
        const MString& objectName,
        const MString& subObjectName) const;

    // Clear the USD playback cache.
    MStatus setCachedFramesDirty(const MString& objectName) const;

    /**
     * @brief Sets a text copy of a USD layer as a result of the script
     * execution.
     *
     * @param objectName The name of the standin shape.
     * @param layerName The name of the requested layer.
     *
     * @return The status.
     */
    MStatus getLayerAsString(
        const MString& objectName,
        const MString& layerName) const;

    /**
     * @brief Calls onLoaded if it was never called.
     */
    MStatus join(const MString& objectName) const;

    /**
     * @brief Checks that a regex represents a valid walter expression.
     *
     * @param objectName The name of the standin shape.
     * @param expression The regex expression to validate.
     *
     * @return The status.
     */
    MStatus expressionIsMatching(
        const MString& objectName,
        const MString& expression) const;

    /**
     * @brief Checks if the object is an USD pseudo-root.
     *
     * @param objectName The name of the standin shape.
     * @param subObjectName The subnode name (path).
     *
     * @return The status.
     */
    MStatus isPseudoRoot(
        const MString& objectName,
        const MString& subObjectName) const;

    /**
     * @brief Checks if the object is visible.
     *
     * @param objectName The name of the standin shape.
     * @param subObjectName The subnode name (path).
     *
     * @return The status.
     */
    MStatus isVisible(
        const MString& objectName,
        const MString& subObjectName) const;

    /**
     * @brief Toggles the visibility state (show, hidden) of the object.
     *
     * @param objectName The name of the standin shape.
     * @param subObjectName The subnode name (path).
     *
     * @return The status.
     */
    MStatus toggleVisibility(
        const MString& objectName,
        const MString& subObjectName) const;

    /**
     * @brief Sets the visibility state (show, hidden) of the object.
     *
     * @param objectName The name of the standin shape.
     * @param subObjectName The subnode name (path).
     *
     * @return The status.
     */
    MStatus setVisibility(
        const MString& objectName,
        const MString& subObjectName,
        bool visibility) const;

    /**
     * @brief Hides all the scene execpted the object at subObjectName.
     *
     * @param objectName The name of the standin shape.
     * @param subObjectName The subnode name (path).
     *
     * @return The status.
     */
    MStatus hideAllExceptedThis(
        const MString& objectName,
        const MString& subObjectName) const;

    /**
     * @brief Gets the list of objects path matching the expression.
     *
     * @param objectName The name of the standin shape.
     * @param expression A regex expression.
     *
     * @return The status.
     */
    MStatus primPathsMatchingExpression(
        const MString& objectName,
        const MString& expression) const;

    /**
     * @brief Sets the prim purpose token (default, render, proxy, guide).
     *
     * @param obj Walter Standin node.
     * @param subNodeName The subnode name (path).
     * @param purpose The purpose token.
     *
     * @return The status.
     */
    MStatus setPurpose(
        const MString& objectName,
        const MString& subObjectName,
        const MString& purpose) const;

    /**
     * @brief Gets the prim purpose token.
     *
     * @param obj Walter Standin node.
     * @param subNodeName The subnode name (path).
     *
     * @return The status.
     */
    MStatus getPurpose(
        const MString& objectName,
        const MString& subObjectName) const;

    /**
     * @brief Sets the render engine purposes list, i.e the prim
     * purposes that the engine can render.
     *
     * @param objectName The name of the standin shape.
     * @param purposes List of purpose tokens separated by ":".
     *
     * @return The status.
     */
    MStatus setRenderPurpose(
        const MString& objectName,
        const MString& purposeList) const;

    /**
     * @brief Gets the render engine purposes list, i.e the prim
     * purposes that the engine can render.
     *
     * @param objectName The name of the standin shape.
     *
     * @return The status.
     */
    MStatus getRenderPurpose(
        const MString& objectName) const;
 };

}  // namespace WalterMaya

#endif
