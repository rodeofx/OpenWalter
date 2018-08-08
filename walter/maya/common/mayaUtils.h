// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTER_MAYA_UTILS_H_
#define __WALTER_MAYA_UTILS_H_

#include <maya/MString.h>
#include <maya/MPlug.h>
#include <string>

// namespace walter {
// namespace maya {

static const unsigned int WALTER_MAYA_SHAPE_ID(0x07201);

// Search child plug with given name in the given plug's children.
bool GetChildMPlug(const MPlug& i_plug, const MString& name, MPlug& o_plug);

// Maya name to arnold name convert. "walterSubdivType" -> "subdiv_type".
// Set oIsUserData to true if it's a User Data.
std::string attributeNameDemangle(
        const std::string& name,
        bool* oIsUserData = nullptr);

// } // end namespace walter
// } // end namespace maya

#endif // end __WALTER_MAYA_UTILS_H_
