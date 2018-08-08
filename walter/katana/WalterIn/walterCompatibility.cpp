// Copyright 2018 Rodeo FX.  All rights reserved.

#include "walterCompatibility.h"

#if KATANA_VERSION_MAJOR == 3

Foundry::Katana::FnLogging::FnLog::FnLog(std::string const& module)
{}

Foundry::Katana::FnLogging::FnLog::~FnLog()
{}

void Foundry::Katana::FnLogging::FnLog::log(
    std::string const& message,
    unsigned int severity) const
{}

#endif
