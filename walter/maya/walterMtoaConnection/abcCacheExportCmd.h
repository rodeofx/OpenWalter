/*Abc Shader Exporter
Copyright (c) 2014, Gaël Honorez, All rights reserved.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3.0 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with this library.*/


#ifndef ABC_CACHE_EXPORT_CMD_H
#define ABC_CACHE_EXPORT_CMD_H

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MString.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MPlugArray.h>

class abcCacheExportCmd : public MPxCommand {
  public:
    abcCacheExportCmd() {};
    ~abcCacheExportCmd() {};

    virtual MStatus doIt( const MArgList &args );
    virtual MStatus undoIt();

    virtual MStatus redoIt();

    virtual bool isUndoable() const;

    virtual bool hasSyntax() const;

    static MSyntax mySyntax();

    static void* creator() {return new abcCacheExportCmd;};

    bool isHistoryOn() const;

    MString commandString() const;

    MStatus setHistoryOn( bool state );

    MStatus setCommandString( const MString & );

};

#endif //FX_MANAGER_CMD_H
