// Copyright 2018 Rodeo FX.  All rights reserved.

#include <FnAPI/FnAPI.h>

// The logging is refactored in Katana 3. And it's not compiling anymore. This
// file brings Katana2 logging back.

#if KATANA_VERSION_MAJOR == 3

#include <string>

namespace Foundry
{
namespace Katana
{
namespace FnLogging
{
class FnLog
{
public:
    FnLog(std::string const& module = "");
    ~FnLog();
    void log(std::string const& message, unsigned int severity) const;
};
}
}
}

#endif
