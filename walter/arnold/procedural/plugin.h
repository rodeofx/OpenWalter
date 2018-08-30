// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <pxr/base/tf/debug.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/bboxCache.h>

// Declare USD debugging messages.
PXR_NAMESPACE_OPEN_SCOPE
TF_DEBUG_CODES(WALTER_ARNOLD_PLUGIN);
PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

struct RendererPluginData
{
    std::string dso;
    std::string prefix;
    std::string filePaths;
    boost::optional<float> motionStart;
    boost::optional<float> motionEnd;
};

class AtNode;
class RendererAttribute;
class RendererIndex;
typedef std::unordered_map<std::string, RendererAttribute> NameToAttribute;

class RendererAttribute
{
public:
    typedef std::function<void(AtNode*)> ArnoldParameter;
    typedef std::shared_ptr<ArnoldParameter> ArnoldParameterPtr;

    RendererAttribute(ArnoldParameterPtr function) :
            mFunction(function),
            mVisibility(false),
            mVisibilityFlag(0)
    {}

    RendererAttribute(const RendererAttribute& src) :
            mFunction(src.mFunction),
            mVisibility(src.mVisibility),
            mVisibilityFlag(src.mVisibilityFlag)
    {}

    /**
     * @brief Constructs visibility attribute.
     *
     * @param visibilityFlag The visibility flag to save.
     */
    RendererAttribute(uint8_t visibilityFlag) :
            mFunction(nullptr),
            mVisibility(true),
            mVisibilityFlag(visibilityFlag)
    {}

    void evaluate(AtNode* node) const;

    bool valid() const;

    /**
     * @brief Returns true if it's a visibility attribute.
     */
    bool isVisibility() const;
    /**
     * @brief Returns the visibility flag if it's a visibility attribte.
     */
    uint8_t visibilityFlag() const;

private:
    ArnoldParameterPtr mFunction;

    // True if this attribute contains a visiblity flag.
    bool mVisibility;
    uint8_t mVisibilityFlag;
};

// Everything related to the conversion from USD to Arnold is here.
class RendererPlugin
{
public:
    RendererPlugin();

    // True if it's a supported privitive.
    bool isSupported(const UsdPrim& prim) const;

    // True it it's necessary to output it immediatly and avoid self-expanding
    // stuff.
    bool isImmediate(const UsdPrim& prim) const;

    // Output the procedural from any prim.
    void* output(
        const UsdPrim& prim,
        const std::vector<float>& times,
        const void* userData
    ) const;

    // Output the procedural with the primitive.
    void* outputBBox(
        const UsdPrim& prim,
        const std::vector<float>& times,
        const void* userData,
        RendererIndex& index) const;

    // Output the procedural from a point of the PointInstancer.
    void* outputBBoxFromPoint(
        const UsdPrim& prim,
        const int id,
        const std::vector<float>& times,
        const void* userData,
        RendererIndex& index) const;

    // Output an object and return AiNode.
    void* outputReference(
        const UsdPrim& prim,
        const std::vector<float>& times,
        const void* userData,
        RendererIndex& index) const;

    // Set the Arnold connections for the node specified with data. We need the
    // index to get the pointers to another nodes.
    void establishConnections(
        const UsdPrim& prim,
        void* data,
        RendererIndex& index) const;

    // Output polymesh. We need index to access the shaders.
    void* render(
        const UsdPrim& prim,
        const std::vector<float>& times,
        void* renderData,
        const void* userData,
        RendererIndex& index) const;

    // Return an object that it's possible use to output Arnold parameter.
    // Basically, it convers UsdAttribute to Arnold Parameter.
    RendererAttribute createRendererAttribute(
        const UsdAttribute& attr,
        UsdTimeCode time = UsdTimeCode::Default()) const;

private:
    AtNode* outputGeomMesh(
        const UsdPrim& prim,
        const std::vector<float>& times,
        const char* name,
        const void* userData) const;

    AtNode* outputGeomCurves(
        const UsdPrim& prim,
        const std::vector<float>& times,
        const char* name,
        const void* userData) const;

    AtNode* outputShader(
        const UsdPrim& prim,
        const std::vector<float>& times,
        const char* name) const;

    void outputPrimvars(
        const UsdPrim& prim,
        float times,
        AtNode* node,
        const VtIntArray* numVertsArray) const;

    /**
     * @brief Returns path to walterOverride object assigned to the given node
     * and computes the visibility of the given node. We need it as a separate
     * function to be able to skip the object if it's not visible.
     *
     * @param path USD path to the object.
     * @param layer "defaultRenderLayer"
     * @param userData The arnold user data.
     * @param index RendererIndex object.
     * @param oVisibility The computed visibility.
     *
     * @return Path to walterOverride object assigned to the given node.
     */
    const NameToAttribute* getAssignedAttributes(
        const SdfPath& path,
        const std::string& layer,
        const void* userData,
        RendererIndex& index,
        uint8_t* oVisibility) const;

    /**
     * @brief Ouput attributes and shaders to Arnold node.
     *
     * @param node Arnold node.
     * @param path USD path.
     * @param attributePath USD path to the walterOverride object.
     * @param index RendererIndex object.
     * @param layer "defaultRenderLayer"
     * @param userData The arnold user data.
     * @param forReference True if it's necessary to output attributes of the
     * reference.
     */
    void outputAttributes(
        AtNode* node,
        const SdfPath& path,
        const NameToAttribute* attributes,
        RendererIndex& index,
        const std::string& layer,
        const void* userData,
        bool forReference) const;

    /**
     * @brief Output shader or displacemet.
     *
     * @param node Arnold node.
     * @param objectName USD path. It's a string because we keep all the
     * assignments as strings and resolve them with boost::regex as std::string.
     * @param index RendererIndex object.
     * @param layer "defaultRenderLayer"
     * @param target "shader" or "displacement"
     */
    void outputShadingAttribute(
        AtNode* node,
        const std::string& objectName,
        RendererIndex& index,
        const std::string& layer,
        const std::string& target) const;

    /**
     * @brief It's a hack. We can't disable objects with Walter Override because
     * of Arnold bug. We filed the bug report to Solid Angle on Nov 7 and still
     * waiting them. The subject of email is "Issue with our custom procedural [
     * ref:_00DD0rHEv._500D01aInPJ:ref ]". So to disable the object we output a
     * very small point that is visible only for volume rays.
     *
     * @param iNodeName the arnold name of the node.
     *
     * @return The Arnold node.
     */
    void* outputEmptyNode(const std::string& iNodeName) const;
};

#endif
