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


#include "abcCacheExportCmd.h"
#include "convertToArnold.h"

#include <maya/MFnPlugin.h>


//#include <maya/MSceneMessage.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>


#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else 
#undef DLLEXPORT
#define DLLEXPORT __attribute__ ((visibility("default")))
#endif

DLLEXPORT MStatus initializePlugin( MObject obj )
//
//    Description:
//        this method is called when the plug-in is loaded into Maya.  It
//        registers all of the services that this plug-in provides with
//        Maya.
//
//    Arguments:
//        obj - a handle to the plug-in object (use MFnPlugin to access it)
//
{
    MStatus   status;
    MFnPlugin plugin( obj, "RodeoFX", "2017", "Any");

    plugin.registerCommand( "walterSaveShaders", abcCacheExportCmd::creator, abcCacheExportCmd::mySyntax);

    plugin.registerCommand(
        "walterConvertToArnold",
        ConvertToArnoldCmd::creator,
        ConvertToArnoldCmd::syntax);

    return status;
}

DLLEXPORT MStatus uninitializePlugin( MObject obj)
//
//    Description:
//        this method is called when the plug-in is unloaded from Maya. It
//        deregisters all of the services that it was providing.
//
//    Arguments:
//        obj - a handle to the plug-in object (use MFnPlugin to access it)
//
{
    MStatus   status;
    MFnPlugin plugin( obj );

    status = plugin.deregisterCommand("walterSaveShaders");
    if (!status) {
        status.perror("deregisterCommand");
        return status;
    }

    status = plugin.deregisterCommand("walterConvertToArnold");
    if (!status)
    {
        status.perror("deregisterCommand walterConvertToArnold");
        return status;
    }

    return status;
}
