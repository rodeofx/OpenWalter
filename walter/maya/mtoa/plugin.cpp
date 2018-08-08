#include "walterTranslator.h"
#include "extension/Extension.h"

extern "C" {

DLLEXPORT void initializeExtension(CExtension& plugin)
{
    MStatus status;

    status = plugin.RegisterTranslator("walterStandin",
                                       "",
                                       CWalterStandinTranslator::creator,
                                       CWalterStandinTranslator::NodeInitializer);

}

DLLEXPORT void deinitializeExtension(CExtension& plugin)
{
}
}
