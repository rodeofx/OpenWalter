// Copyright 2017 Rodeo FX.  All rights reserved.

/*
For static linking only:
We need to create plugInfo.json with descriptions of all the plugins and
convert it to an ELF object like this:
objcopy \
       --input binary \
       --output elf64-x86-64 \
       --binary-architecture i386 \
       plugInfo.json plugInfo.o
After that we will be able to read this file as an external symbol. Since we
compile static USD with weak _ReadPlugInfoObject, we are able to replace it
here. 
*/

#ifdef MFB_ALT_PACKAGE_NAME

#include <pxr/base/js/json.h>
#include <pxr/base/tf/diagnosticLite.h>

extern char _binary_plugInfo_json_start;
extern char _binary_plugInfo_json_end;

PXR_NAMESPACE_USING_DIRECTIVE

extern "C" bool _ReadPlugInfoObject(
        const std::string& pathname, JsObject* result)
{
    std::string json(
            &_binary_plugInfo_json_start,
            size_t(&_binary_plugInfo_json_end - &_binary_plugInfo_json_start));

    result->clear();

    // Read JSON.
    JsParseError error;
    JsValue plugInfo = JsParseString(json, &error);

    // Validate.
    if (plugInfo.IsNull())
    {
        TF_RUNTIME_ERROR("Plugin info file %s couldn't be read "
                         "(line %d, col %d): %s", pathname.c_str(),
                         error.line, error.column, error.reason.c_str());
    }
    else if (!plugInfo.IsObject())
    {
        // The contents didn't evaluate to a json object....
        TF_RUNTIME_ERROR("Plugin info file %s did not contain a JSON object",
                         pathname.c_str());
    }
    else
    {
        *result = plugInfo.GetJsObject();
    }
    return true;
}

#endif
