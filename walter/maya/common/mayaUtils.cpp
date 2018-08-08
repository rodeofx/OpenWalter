// Copyright 2017 Rodeo FX.  All rights reserved.

#include "mayaUtils.h"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <string.h>

// namespace walter {
// namespace maya {


// Search child plug with given name in the given plug's children.
bool GetChildMPlug(const MPlug& i_plug, const MString& name, MPlug& o_plug)
{
    const MString dotName = "." + name;
    unsigned dotNameLen = dotName.length();

    for (unsigned i=0; i<i_plug.numChildren(); i++)
    {
        o_plug = i_plug.child(i);
        if (o_plug.isNull())
        {
            continue;
        }

        // MPlug::partialName returns "la[0].layer"
        const MString partial = o_plug.partialName();
        unsigned partialLen = partial.length();
        if (partialLen <= dotNameLen)
        {
            continue;
        }

        if (partial.substring(partialLen-dotNameLen, partialLen-1) == dotName)
        {
            return true;
        }
    }

    return false;
}

std::string attributeNameDemangle(const std::string& name, bool* oIsUserData)
{
    static const char* walter = "walter";
    static const int walterLen = strlen(walter);

    if (name.substr(0, walterLen) != "walter")
    {
        return "";
    }

    // Form the correct json name.
    // Cut prefix. Prefix is walter. "walterSubdivType" -> "SubdivType"
    std::string arnoldName = name.substr(walterLen, name.size()-walterLen);

    if (arnoldName[0]=='_')
    {
        // It's a custom attribute. Just remove the first character.
        arnoldName = arnoldName.substr(1);

        if (oIsUserData)
        {
            *oIsUserData = true;
        }
    }
    else
    {
        // Camel case to snack case. "SubdivType" -> "Subdiv_Type"
        arnoldName = boost::regex_replace(
                arnoldName, boost::regex("([a-z0-9])([A-Z])"), "\\1_\\2");
        // Lower case. "Subdiv_Type" -> "subdiv_type"
        boost::algorithm::to_lower(arnoldName);

        if (oIsUserData)
        {
            *oIsUserData = false;
        }
    }

    return arnoldName;
}

// } // end namespace walter
// } // end namespace maya
