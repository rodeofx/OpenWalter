// Copyright 2017 Rodeo FX. All rights reserved.

#ifndef __CONVERT_TO_ARNOLD_H__
#define __CONVERT_TO_ARNOLD_H__

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>

/** @brief A MEL command that exports given objects to Arnold and returns the
 * description of the objects using Json. */
class ConvertToArnoldCmd : public MPxCommand
{
public:
    ConvertToArnoldCmd(){};
    static void* creator() { return new ConvertToArnoldCmd; }
    static MSyntax syntax();
    virtual MStatus doIt(const MArgList&);
};

#endif
