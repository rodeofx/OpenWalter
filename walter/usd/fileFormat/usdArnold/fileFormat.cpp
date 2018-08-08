#include "fileFormat.h"

#include <pxr/base/tf/fileUtils.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/usdaFileFormat.h>

#include <pxr/usd/usd/stage.h>

#include <string>

#include "arnoldTranslator.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(
    UsdArnoldFileFormatTokens,
    USDARNOLD_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(UsdArnoldFileFormat, SdfFileFormat);
}

UsdArnoldFileFormat::UsdArnoldFileFormat() :
        SdfFileFormat(
            UsdArnoldFileFormatTokens->Id,
            UsdArnoldFileFormatTokens->Version,
            UsdArnoldFileFormatTokens->Target,
            UsdArnoldFileFormatTokens->Id),
        mUsda(SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id))
{}

UsdArnoldFileFormat::~UsdArnoldFileFormat()
{}

bool UsdArnoldFileFormat::CanRead(const std::string& filePath) const
{
    auto extension = TfGetExtension(filePath);
    if (extension.empty())
    {
        return false;
    }

    return extension == this->GetFormatId();
}

bool UsdArnoldFileFormat::Read(
    const SdfLayerBasePtr& layerBase,
    const std::string& filePath,
    bool metadataOnly) const
{
    SdfLayerHandle layer = TfDynamic_cast<SdfLayerHandle>(layerBase);
    if (!TF_VERIFY(layer))
    {
        return false;
    }

    // Translate obj to usd schema.
    SdfLayerRefPtr objAsUsd = ArnoldUSDTranslator::arnoldToUsdLayer(filePath);
    if (!objAsUsd)
    {
        return false;
    }

    // Move generated content into final layer.
    layer->TransferContent(objAsUsd);

    return true;
}

bool UsdArnoldFileFormat::ReadFromString(
    const SdfLayerBasePtr& layerBase,
    const std::string& str) const
{
    return false;
}

bool UsdArnoldFileFormat::WriteToString(
    const SdfLayerBase* layerBase,
    std::string* str,
    const std::string& comment) const
{
    // It's super important to have it like this. Otherwise usdcat doesn't work.
    return mUsda->WriteToString(layerBase, str, comment);
}

bool UsdArnoldFileFormat::WriteToStream(
    const SdfSpecHandle& spec,
    std::ostream& out,
    size_t indent) const
{
    // It's super important to have it like this. Otherwise usdcat doesn't work.
    return mUsda->WriteToStream(spec, out, indent);
}

bool UsdArnoldFileFormat::_IsStreamingLayer(const SdfLayerBase& layer) const
{
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
