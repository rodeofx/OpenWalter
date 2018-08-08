#ifndef USDARNOLD_FILE_FORMAT_H
#define USDARNOLD_FILE_FORMAT_H

#include <pxr/base/tf/staticTokens.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <iosfwd>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

#define USDARNOLD_FILE_FORMAT_TOKENS \
    ((Id, "ass"))((Version, "1.0"))((Target, "usd"))

TF_DECLARE_PUBLIC_TOKENS(
    UsdArnoldFileFormatTokens,
    USDARNOLD_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(UsdArnoldFileFormat);
TF_DECLARE_WEAK_AND_REF_PTRS(SdfLayerBase);

/// \class UsdArnoldFileFormat

class UsdArnoldFileFormat : public SdfFileFormat
{
public:
    virtual bool CanRead(const std::string& file) const override;
    virtual bool Read(
        const SdfLayerBasePtr& layerBase,
        const std::string& filePath,
        bool metadataOnly) const override;
    virtual bool ReadFromString(
        const SdfLayerBasePtr& layerBase,
        const std::string& str) const override;

    // We override Write methods so SdfLayer::ExportToString() etc, work.  We
    // don't support writing general Usd data back to ass files.  So
    // SdfLayer::Save() doesn't work, for example.
    virtual bool WriteToString(
        const SdfLayerBase* layerBase,
        std::string* str,
        const std::string& comment = std::string()) const override;
    virtual bool WriteToStream(
        const SdfSpecHandle& spec,
        std::ostream& out,
        size_t indent) const override;

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    virtual ~UsdArnoldFileFormat();
    UsdArnoldFileFormat();

private:
    virtual bool _IsStreamingLayer(const SdfLayerBase& layer) const;

    SdfFileFormatConstPtr mUsda;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // USDARNOLD_FILE_FORMAT_H
