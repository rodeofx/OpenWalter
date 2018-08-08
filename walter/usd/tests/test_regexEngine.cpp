// Copyright 2017 Rodeo FX. All rights reserved.

#include "PathUtil.h"
#include <gtest/gtest.h>

TEST(regexEngine, matchPattern)
{
    EXPECT_EQ(WalterCommon::matchPattern("Boost Libraries", "\\w+\\s\\w+"), true);
}

TEST(regexEngine, mangleString)
{
    auto demangled = WalterCommon::mangleString("/Hello /World");
    EXPECT_EQ(demangled, "\\Hello \\World");
}

TEST(regexEngine, demangleString)
{
    auto demangled = WalterCommon::demangleString("\\Hello \\World");
    EXPECT_EQ(demangled, "/Hello /World");
}

TEST(regexEngine, convertRegex)
{
    auto converted = WalterCommon::convertRegex("\\d \\D \\w \\W");
    EXPECT_EQ(converted, "[0-9] [^0-9] [a-zA-Z0-9_] [^a-zA-Z0-9_]");
}