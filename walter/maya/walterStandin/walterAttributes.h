// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERATTRIBUTES_H_
#define __WALTERATTRIBUTES_H_

// Several utilities to save/load attributes from Alembic files.

#include <maya/MString.h>

// Save all the attributes from given walter node to a separate Alembic file.
bool saveAttributes(const MString& objectName, const MString& fileName);

#endif
