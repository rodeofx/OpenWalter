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

#include "ai.h"

#include <Alembic/Abc/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcMaterial/OMaterial.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
#include <set>
//#include <pystring.h>

#include <sstream>

namespace Abc =  Alembic::Abc;
namespace Mat = Alembic::AbcMaterial;

typedef std::set<AtNode*> AtNodeSet;

const std::vector<std::string> GetComponentNames(int arnoldParamType);

void getAllArnoldNodes(AtNode* node, AtNodeSet* nodes);


void processArrayValues(AtNode* sit, const char *paramName, AtArray* paramArray, int outputType, Mat::OMaterial matObj, std::string nodeName);

void processArrayParam(AtNode* sit, const char *paramName, AtArray* paramArray, int index, int outputType, Mat::OMaterial matObj, std::string nodeName, std::string containerName);

void processLinkedParam(AtNode* sit, int inputType, int outputType,  Mat::OMaterial matObj,  std::string nodeName, std::string paramName, std::string containerName);

void exportLink(AtNode* sit, Mat::OMaterial matObj, std::string nodeName, std::string paramName, std::string containerName);

void exportParameter(AtNode* sit, Mat::OMaterial matObj, int type, std::string nodeName, std::string paramName);

bool isDefaultValue(AtNode* node, const char* paramName);

#endif