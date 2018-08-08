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


#ifndef _Abc_Exporter_Utils_h_
#define _Abc_Exporter_Utils_h_

#include "mtoaScene.h"
#include "mtoaVersion.h"

#include "ai.h"
// FIXME: private mtoa headers:
#include "translators/NodeTranslator.h"
#include <Alembic/Abc/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcMaterial/OMaterial.h>

#include <boost/lexical_cast.hpp>

#include <sstream>


namespace Abc =  Alembic::Abc;
namespace Mat = Alembic::AbcMaterial;

template <class T>
 inline std::string to_string (const T& t)
 {
 std::stringstream ss;
 ss << t;
 return ss.str();
 }

const MStringArray& GetComponentNames(int arnoldParamType);

void processLinkedParam(AtNode* sit, int inputType, int outputType,  Mat::OMaterial matObj, MString nodeName, const char* paramName, MString containerName, bool interfacing = false);
void processArrayParam(AtNode* sit, const char *paramName, AtArray* paramArray, int index, int outputType, Mat::OMaterial matObj, MString nodeName, MString containerName);

void processArrayValues(AtNode* sit, const char *paramName, AtArray* paramArray, int outputType, Mat::OMaterial matObj, MString nodeName, MString containerName);

void exportParameterFromArray(AtNode* sit, Mat::OMaterial matObj, AtArray* paramArray, int index, MString nodeName, const char* paramName);
void exportParameter(AtNode* sit, Mat::OMaterial matObj, int type, MString nodeName, const char* paramName, bool interfacing = false);
void exportLink(AtNode* sit, Mat::OMaterial matObj, MString nodeName, const char* paramName, MString containerName);

bool relink(AtNode* src, AtNode* dest, const char* input, int comp);
AtNode* renameAndCloneNodeByParent(AtNode* node, AtNode* parent);
void getAllArnoldNodes(AtNode* node, AtNodeSet* nodes);
bool isDefaultValue(const AtNode* node, const char* paramName);

#endif
