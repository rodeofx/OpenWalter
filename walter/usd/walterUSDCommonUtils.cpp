// Copyright 2017 Rodeo FX.  All rights reserved.

#include "PathUtil.h"
#include "walterUSDCommonUtils.h"
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usdGeom/imageable.h>

#include "schemas/expression.h"
#include "walterUsdConversion.h"
#include <boost/algorithm/string.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

/**
 * @brief Converts the primPath (Path of the primitive in the stage) to the
 * corresponding SdfPath.
 *
 * @param primPath The primitive path as string.
 *
 * @return The SdfPath path.
 */
SdfPath GetSdfPathFromPrimPath(const char* primPath)
{
    return (primPath != nullptr) && (primPath[0] == '\0') ?
        SdfPath::AbsoluteRootPath() :
        SdfPath(primPath);
}

SdfLayerRefPtr WalterUSDCommonUtils::getUSDLayer(const std::string& path)
{
    std::vector<std::string> archives;
    boost::algorithm::split(archives, path, boost::is_any_of(":"));
    return getUSDLayer(archives);
}

SdfLayerRefPtr WalterUSDCommonUtils::getUSDLayer(
    const std::vector<std::string>& paths)
{
    ArResolver& resolver = ArGetResolver();

    // Open and compose several layers. Since we can compose several different
    // formats, we need to be sure that alembics are composed with AbcCoreLayer.
    // To do it, it's necessary to provide the list of alembics as a single
    // layer. This trick doesn't work with .usd files, thus, we need to group
    // all the layers to contain one usd or list of abc files.
    std::vector<std::string> allArchives;
    std::vector<std::string> alembicArchives;
    for (const std::string archive : paths)
    {
        if (archive.empty())
        {
            continue;
        }

        if (boost::iends_with(archive, ".abc"))
        {
            // We need to compose the alembic files with AbcCoreLayer and we
            // need to keep all the alembics in the one single string.
            alembicArchives.push_back(archive);
        }
        else
        {
            if (!alembicArchives.empty())
            {
                allArchives.push_back(
                    boost::algorithm::join(alembicArchives, ":"));

                alembicArchives.clear();
            }

            allArchives.push_back(archive);
        }
    }

    if (!alembicArchives.empty())
    {
        allArchives.push_back(boost::algorithm::join(alembicArchives, ":"));
    }

    // We can check if we don't have archives or we have only one and skip the
    // anonymous layer generation like this:
    // if (allArchives.empty())
    //     return SdfLayerRefPtr();
    // else if(allArchives.size() == 1)
    //     return SdfLayer::FindOrOpen(allArchives[0]);
    // But we need anonymous layer to store internal sub-layers. For example to
    // freeze all the transforms.

    // In USD the first layer wins. In Walter the last layer wins. We just need
    // to reverse it.
    std::reverse(allArchives.begin(), allArchives.end());

    // Create a layer that refers to all the layers.
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    if (!allArchives.empty())
    {
        layer->SetSubLayerPaths(allArchives);
    }

    return layer;
}

SdfLayerRefPtr WalterUSDCommonUtils::getAnonymousLayer(
    UsdStageRefPtr stage,
    const std::string& name)
{
    const std::string fullName = name + ".usd";
    const std::string colonName = ":" + fullName;

    SdfLayerRefPtr rootLayer = stage->GetRootLayer();

    // Looking for the requested layer.
    SdfLayerRefPtr anonymousLayer;
    SdfSubLayerProxy subLayers = rootLayer->GetSubLayerPaths();
    for (size_t i = 0, s = subLayers.size(); i < s; i++)
    {
        // Iteration with index to be able to remove the layer from the sublayer
        // list.
        const std::string& identifier = subLayers[i];
        // TODO: Is there something better to determine if the layer is
        // anonimous and to get its name?
        if (boost::starts_with(identifier, "anon:") &&
            boost::ends_with(identifier, colonName))
        {
            anonymousLayer = SdfLayer::FindOrOpen(identifier);

            // USD destroys anonymous layer when calling UsdStage::MuteLayer. To
            // avoid it, it's necessary to keep it somethere. But if it happens,
            // we don't need to have its identifier in sub-layers, so it should
            // be removed.
            if (!anonymousLayer)
            {
                rootLayer->RemoveSubLayerPath(i);
            }

            break;
        }
    }

    // Create it if it's not there.
    if (!anonymousLayer)
    {
        anonymousLayer = SdfLayer::CreateAnonymous(fullName);

        // Put it to the front
        rootLayer->InsertSubLayerPath(anonymousLayer->GetIdentifier(), 0);
    }

    return anonymousLayer;
}

bool WalterUSDCommonUtils::setUSDVariantsLayer(
    UsdStageRefPtr stage,
    const char* variants)
{
    SdfLayerRefPtr variantLayer = getAnonymousLayer(stage, "variantLayer");

    if ((variants != nullptr) && (variants[0] == '\0'))
    {
        variantLayer->Clear();
    }

    else if (!variantLayer->ImportFromString(variants))
    {
        variantLayer->Clear();
        return false;
    }

    return true;
}

bool WalterUSDCommonUtils::setUSDPurposeLayer(
    UsdStageRefPtr stage,
    const char* purpose)
{
    SdfLayerRefPtr purposeLayer = getAnonymousLayer(stage, "purposeLayer");

    if ((purpose != nullptr) && (purpose[0] == '\0'))
    {
        purposeLayer->Clear();
    }

    else if (!purposeLayer->ImportFromString(purpose))
    {
        purposeLayer->Clear();
        return false;
    }

    return true;
}

std::string WalterUSDCommonUtils::getVariantsLayerAsText(UsdStageRefPtr stage)
{
    std::string variants;
    SdfLayerRefPtr variantLayer = getAnonymousLayer(stage, "variantLayer");
    variantLayer->ExportToString(&variants);
    return variants;
}

std::string WalterUSDCommonUtils::getPurposeLayerAsText(UsdStageRefPtr stage)
{
    std::string purpose;
    SdfLayerRefPtr purposeLayer = getAnonymousLayer(stage, "purposeLayer");
    purposeLayer->ExportToString(&purpose);
    return purpose;
}

bool WalterUSDCommonUtils::setUSDVisibilityLayer(
    UsdStageRefPtr stage,
    const char* visibility)
{
    SdfLayerRefPtr visibilityLayer = getAnonymousLayer(stage, "visibilityLayer");

    if ((visibility != nullptr) && (visibility[0] == '\0'))
    {
        visibilityLayer->Clear();
    }

    else if (!visibilityLayer->ImportFromString(visibility))
    {
        visibilityLayer->Clear();
        return false;
    }

    return true;
}

std::vector<std::string> WalterUSDCommonUtils::dirUSD(
    UsdStageRefPtr stage,
    const char* primPath)
{
    std::vector<std::string> result;
    SdfPath path = GetSdfPathFromPrimPath(primPath);

    UsdPrim prim = stage->GetPrimAtPath(path);
    if (!prim)
    {
        return {};
    }

    UsdPrimSiblingRange children = prim.GetChildren();
    for (const UsdPrim& prim : children)
        result.push_back(prim.GetPath().GetText());

    return result;
}

std::vector<std::string> WalterUSDCommonUtils::propertiesUSD(
    UsdStageRefPtr stage,
    const char* primPath,
    double time,
    bool attributeOnly)
{
    std::vector<std::string> result;
    SdfPath path = GetSdfPathFromPrimPath(primPath);

    UsdPrim prim = stage->GetPrimAtPath(path);
    if (!prim)
    {
        return {};
    }

    UsdTimeCode tc(time);
    const std::vector<UsdProperty>& properties = prim.GetProperties();

    for (unsigned i = 0; i < properties.size(); i++)
    {
        UsdProperty const& prop = properties[i];

        int arraySize = -1;
        std::string type, value;
        if (WalterUsdConversion::getPropertyValueAsString(
                prop, tc, type, value, arraySize, attributeOnly))
        {
            std::string description =
                WalterUsdConversion::constuctStringRepresentation(
                    prop.GetBaseName().GetText(),
                    prop.Is<UsdAttribute>() ? "Attribute" : "Relationship",
                    type,
                    value,
                    arraySize);

            result.push_back(description);
        }
    }
    return result;
}

std::string WalterUSDCommonUtils::getVisibilityLayerAsText(UsdStageRefPtr stage)
{
    std::string visibility;
    SdfLayerRefPtr visibilityLayer = getAnonymousLayer(stage, "visibilityLayer");
    visibilityLayer->ExportToString(&visibility);
    return visibility;
}

/**
 * @brief Traverses recursively the USD hierarchy to get the variants.
 *
 * @param prim The USD primitive the get the variants from.
 * @param result The variants list (as an array of json).
 *
 */
void GetPrimVariants(
    UsdPrim const& prim,
    bool recursively,
    std::vector<std::string>& result)
{
    const UsdVariantSets& variantsSets = prim.GetVariantSets();
    std::vector<std::string> variantsSetsName = variantsSets.GetNames();

    std::string jsonStr = "{ ";
    if (variantsSetsName.size() > 0)
    {
        jsonStr += "\"prim\": \"" + prim.GetPath().GetString() + "\"";
        jsonStr += ", \"variants\": [";
    }

    for (unsigned i = 0; i < variantsSetsName.size(); ++i)
    {
        const UsdVariantSet& variantsSet =
            variantsSets.GetVariantSet(variantsSetsName[i]);

        jsonStr += "{ ";
        jsonStr += "\"set\": \"" + variantsSetsName[i] + "\", \"names\": [";

        std::vector<std::string> variantsName = variantsSet.GetVariantNames();
        for (unsigned j = 0; j < variantsName.size(); ++j)
        {
            jsonStr += "\"" + variantsName[j] + "\"";
            if(j < (variantsName.size() - 1))
            {
                jsonStr += ", ";
            }
        }
        jsonStr += "], \"selection\": \"" + variantsSet.GetVariantSelection() + "\"";
        jsonStr += " } ";
        if (i < variantsSetsName.size() - 1)
        {
            jsonStr += ", ";
        }
        else
        {
            jsonStr += "] ";
        }
    }
    jsonStr += " }";

    if (variantsSetsName.size() > 0)
    {
        result.push_back(jsonStr);
    }

    if(recursively)
    {
        for (const UsdPrim& child : prim.GetChildren())
        {
            GetPrimVariants(child, recursively, result);
        }
    }
}

std::vector<std::string> WalterUSDCommonUtils::getVariantsUSD(
    UsdStageRefPtr stage,
    const char* primPath,
    bool recursively)
{
    std::vector<std::string> result;
    SdfPath path = GetSdfPathFromPrimPath(primPath);
    UsdPrim prim = stage->GetPrimAtPath(path);
    if (!prim)
    {
        return {};
    }

    GetPrimVariants(
        prim,
        recursively,
        result);

    return result;
}

/**
 * @brief Traverses recursively the USD hierarchy to set the selected variant.
 *
 * @param prim The USD primitive.
 * @param variantName const char* The variant to set.
 * @param variantSetName const char* The variantSet.
 */
void SetVariantsRecursively(
    UsdPrim const& prim,
    const char* variantName,
    const char* variantSetName)
{
    UsdVariantSets variantsSets = prim.GetVariantSets();
    if (variantsSets.HasVariantSet(variantSetName))
    {
        variantsSets.SetSelection(variantSetName, variantName);
    }

    for (const UsdPrim& child : prim.GetChildren())
    {
        SetVariantsRecursively(child, variantName, variantSetName);
    }
}

void WalterUSDCommonUtils::setVariantUSD(
    UsdStageRefPtr stage,
    const char* primPath,
    const char* variantName,
    const char* variantSetName)
{
    SdfPath path = GetSdfPathFromPrimPath(primPath);

    UsdPrim prim = stage->GetPrimAtPath(path);
    if (!prim)
    {
        return;
    }

    SetVariantsRecursively(prim, variantName, variantSetName);
}

void WalterUSDCommonUtils::setVariantUSD(
    UsdStageRefPtr stage,
    const char* variantDefStr)
{
    std::string str = variantDefStr;
    if (str.empty())
    {
        return;
    }

    std::vector<std::string> tmp;
    boost::algorithm::split(tmp, str, boost::is_any_of("{"));
    std::string nodePath = tmp[0];

    boost::algorithm::split(tmp, tmp[1], boost::is_any_of("="));
    std::string variantSetName = tmp[0];
    std::string variantName = tmp[1];
    variantName = variantName.substr(0, variantName.size() - 1);

    setVariantUSD(
        stage, nodePath.c_str(), variantName.c_str(), variantSetName.c_str());
}

std::vector<std::string> WalterUSDCommonUtils::primVarUSD(
    UsdStageRefPtr stage,
    const char* primPath,
    double time)
{
    std::vector<std::string> result;
    SdfPath path = GetSdfPathFromPrimPath(primPath);

    UsdPrim prim = stage->GetPrimAtPath(path);
    if (!prim)
    {
        return {};
    }

    if (!prim.IsA<UsdGeomImageable>())
    {
        return {};
    }

    UsdGeomImageable imageable(prim);

    const std::vector<UsdGeomPrimvar>& primVars = imageable.GetPrimvars();

    UsdTimeCode tc(time);
    for (unsigned i = 0; i < primVars.size(); ++i)
    {
        UsdGeomPrimvar const& primVar = primVars[i];
        UsdAttribute const& attr = primVar.GetAttr();

        int arraySize = -1;
        std::string type, value;
        if (WalterUsdConversion::getAttributeValueAsString(
                attr, tc, type, value, arraySize))
        {
            std::string description =
                WalterUsdConversion::constuctStringRepresentation(
                    attr.GetBaseName().GetText(),
                    "PrimVar",
                    type,
                    value,
                    arraySize);

            result.push_back(description);
        }
    }
    return result;
}

bool WalterUSDCommonUtils::isPseudoRoot(
    UsdStageRefPtr stage,
    const char* primPath)
{
    std::string pathStr = primPath;
    SdfPath path = pathStr.empty()
        ? SdfPath::AbsoluteRootPath()
        : SdfPath(pathStr);

    UsdPrim prim = stage->GetPrimAtPath(path);
    return prim
        ? prim.IsPseudoRoot()
        : false;
}

bool WalterUSDCommonUtils::isVisible(
    UsdStageRefPtr stage,
    const char* primPath)
{
    std::string pathStr = primPath;
    SdfPath path = pathStr.empty()
        ? SdfPath::AbsoluteRootPath()
        : SdfPath(pathStr);

    UsdPrim prim = stage->GetPrimAtPath(path);
    if (prim && prim.IsA<UsdGeomImageable>())
    {
        UsdGeomImageable imageable(prim);
        return imageable.ComputeVisibility() != TfToken("invisible");
    }

    return true;
}

bool WalterUSDCommonUtils::toggleVisibility(
    UsdStageRefPtr stage,
    const char* primPath)
{
    std::string pathStr = primPath;
    SdfPath path = pathStr.empty()
        ? SdfPath::AbsoluteRootPath()
        : SdfPath(pathStr);

    bool isVisible = true;
    UsdPrim prim = stage->GetPrimAtPath(path);

    if (prim && prim.IsA<UsdGeomImageable>())
    {
        UsdGeomImageable imageable(prim);
        isVisible = imageable.ComputeVisibility() != TfToken("invisible");
        isVisible = setVisibility(stage, primPath, !isVisible);
    }

    return !isVisible;
}

bool WalterUSDCommonUtils::setVisibility(
    UsdStageRefPtr stage,
    const char* primPath,
    bool visible)
{
    std::string pathStr = primPath;

    SdfPath path = pathStr.empty()
        ? SdfPath::AbsoluteRootPath()
        : SdfPath(pathStr);

    UsdPrim parentPrim = stage->GetPrimAtPath(path);

    // When making a prim visible, we fist must make its
    // parent visible because this properties is inherited.
    if(visible)
    {
        std::vector<UsdPrim> tmp;
        UsdPrim prim = parentPrim;

        while(prim && prim.IsValid()) {
            tmp.push_back(prim);
            prim = prim.GetParent();
        }

        for(uint32_t i=tmp.size(); i--;)
        {
            const UsdPrim& prim = tmp[i];
            if(prim && prim.IsValid() && prim.IsA<UsdGeomImageable>()) 
            {
                UsdGeomImageable imageable(prim);

                if(visible)
                {
                    imageable.MakeVisible();
                }
                else
                {
                    imageable.MakeInvisible();
                }
            }
        }
    }

    UsdPrimRange range(parentPrim);
    for (const UsdPrim& prim : range)
    {
        if( prim.IsA<UsdGeomImageable>()) 
        {
            UsdGeomImageable imageable(prim);

            if(visible)
            {
                imageable.MakeVisible();
            }
            else
            {
                imageable.MakeInvisible();
            }
        }
    }

    bool isVisible = false;
    if( parentPrim.IsA<UsdGeomImageable>()) {
        UsdGeomImageable imageable(parentPrim);
        isVisible = imageable.ComputeVisibility() != TfToken("invisible");
    }

    return isVisible;
}

bool WalterUSDCommonUtils::hideAllExceptedThis(
    UsdStageRefPtr stage,
    const char* primPath)
{
    if (!stage)
    {
        return false;
    }

    // Hide all the prims
    setVisibility(stage, "", false);
    // Make visible the one we want
    return setVisibility(stage, primPath, true);
}

bool WalterUSDCommonUtils::expressionIsMatching(
    UsdStageRefPtr stage,
    const char* expression)
{
    if (!stage)
    {
        return false;
    }

    bool expressionIsValid = false;

    // Generate a regexp object.
    void* regexp = WalterCommon::createRegex(expression);

    UsdPrimRange range(stage->GetPrimAtPath(SdfPath::AbsoluteRootPath()));
    for (const UsdPrim& prim : range)
    {
        const SdfPath& path = prim.GetPath();
        std::string current = path.GetString();

        // TODO: What if we have '/cube' and '/cubeBig/shape' objects? If we
        // try to select the first one, both of them will be selected.
        if (boost::starts_with(current, expression) ||
            WalterCommon::searchRegex(regexp, current))
        {
            expressionIsValid = true;
            break;
        }
    }
    // Delete a regexp object.
    WalterCommon::clearRegex(regexp);

    return expressionIsValid;
}

std::vector<UsdPrim> WalterUSDCommonUtils::primsMatchingExpression(
    UsdStageRefPtr stage,
    const char* expression)
{
    std::vector<UsdPrim> res;
    if (!stage)
    {
        return res;
    }

    // Generate a regexp object.
    void* regexp = WalterCommon::createRegex(expression);

    UsdPrimRange range(stage->GetPrimAtPath(SdfPath::AbsoluteRootPath()));
    for (const UsdPrim& prim : range)
    {
        const SdfPath& path = prim.GetPath();
        std::string current = path.GetString();

        // TODO: What if we have '/cube' and '/cubeBig/shape' objects? If we
        // try to select the first one, both of them will be selected.
        if (boost::starts_with(current, expression) ||
            WalterCommon::searchRegex(regexp, current))
        {
            res.push_back(prim);
        }
    }
    // Delete a regexp object.
    WalterCommon::clearRegex(regexp);
    return res;
}

std::vector<std::string> WalterUSDCommonUtils::primPathsMatchingExpression(
    UsdStageRefPtr stage,
    const char* expression)
{
    std::vector<UsdPrim> prims = primsMatchingExpression(
        stage, expression);

    std::vector<std::string> res;
    for(auto prim : prims)
    {
        res.push_back(prim.GetPath().GetText());
    }

    return res;
}

std::string WalterUSDCommonUtils::setPurpose(
    UsdStageRefPtr stage,
    const char* primPath,
    const char* purposeStr)
{
    std::string pathStr = primPath;

    SdfPath path = pathStr.empty()
        ? SdfPath::AbsoluteRootPath()
        : SdfPath(pathStr);

    UsdPrim parentPrim = stage->GetPrimAtPath(path);
    std::vector<UsdPrim> tmp;
    UsdPrim prim = parentPrim;
    while(prim && prim.IsValid()) 
    {
        tmp.push_back(prim);
        prim = prim.GetParent();
    }

    for(uint32_t i=tmp.size(); i--;)
    {
        const UsdPrim& prim = tmp[i];
        if(prim && prim.IsValid() && prim.IsA<UsdGeomImageable>()) 
        {
            UsdGeomImageable imageable(prim);
            TfToken token("default");
            imageable.CreatePurposeAttr(VtValue(token));
        }
    }

    UsdPrimRange range(parentPrim);
    for (const UsdPrim& prim : range)
    {
        if( prim.IsA<UsdGeomImageable>()) 
        {
            UsdGeomImageable imageable(prim);
            TfToken token(purposeStr);
            imageable.CreatePurposeAttr(VtValue(token));
        }
    }

    return purposeStr;
}

std::string WalterUSDCommonUtils::purpose(
    UsdStageRefPtr stage,
    const char* primPath)
{
    std::string pathStr = primPath;

    SdfPath path = pathStr.empty()
        ? SdfPath::AbsoluteRootPath()
        : SdfPath(pathStr);

    UsdPrim prim = stage->GetPrimAtPath(path);
    if (prim.IsA<UsdGeomImageable>())
    {
        UsdGeomImageable imageable(prim);
        TfToken token = imageable.ComputePurpose();
        return token.GetText();
    }

    return "default";
}

WalterUSDCommonUtils::MastersInfoMap WalterUSDCommonUtils::getMastersInfo(
    UsdStageRefPtr stage
)
{
    WalterUSDCommonUtils::MastersInfoMap mastersInfo;

    if (stage->GetMasters().empty())
    {
        return mastersInfo;
    }

    for (UsdPrim prim: stage->Traverse())
    {
        // If the primitive is not an instance there is no master prim
        // information to look for...
        if (!prim.IsInstance())
        {
            continue;
        }

        // ... Otherwise get the master primitive and look for more information
        // if it's the first time we encounter it.
        UsdPrim masterPrim = prim.GetMaster();

        if (mastersInfo.find(masterPrim.GetName()) ==
            mastersInfo.end())
        {
            const auto& primStack = prim.GetPrimStack();

            // This Prim is the 'real' one represented by the masterPrim, we
            // need it's name for material assignment and better naming in
            // Katana.
            SdfPrimSpec referencedPrim = primStack.back().GetSpec();

            // Also get the eventual list of variant selection (just the variant
            // name) to help construct a meaningful katana location for the
            // master location (instance source).
            SdfHandle<SdfPrimSpec> handle = *(primStack.rbegin() + 1);
            SdfPrimSpec variantsPrim = handle.GetSpec();

            std::vector<std::string> variantsSelection =
                WalterUSDCommonUtils::getVariantsSelection(
                    variantsPrim.GetPath()
                );

            // Build a MasterPrimInfo object with the two previous information
            // and mapped it with the USD master primitive name.
            WalterUSDCommonUtils::MasterPrimInfo masterInfo = {
                referencedPrim.GetName(),
                variantsSelection
            };

            mastersInfo.emplace(
                masterPrim.GetName(),
                masterInfo
            );
        }
    }

    return mastersInfo;
}

void getVariantsSelectionRecursively(
    const SdfPath& path,
    std::vector<std::string>& variantSelections)
{
    if (path.IsPrimVariantSelectionPath())
    {
        variantSelections.emplace_back(path.GetVariantSelection().second);

        SdfPath parentPath = path.GetParentPath();
        getVariantsSelectionRecursively(parentPath, variantSelections);
    }
}

std::vector<std::string> WalterUSDCommonUtils::getVariantsSelection(const SdfPath& path)
{
    std::vector<std::string> variantsSelection;
    getVariantsSelectionRecursively(path, variantsSelection);
    return variantsSelection;
}
