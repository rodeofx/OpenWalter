#ifndef ABCSHADERUTILS_H_
#define ABCSHADERUTILS_H_

#include <ai.h>
#include <Alembic/Abc/All.h>
#include <Alembic/AbcMaterial/IMaterial.h>

namespace Abc =  Alembic::Abc;
namespace Mat = Alembic::AbcMaterial;

namespace {
	std::map<std::string, std::string> emptyRemap;
}

void setUserParameter(AtNode* source, std::string interfaceName, Alembic::AbcCoreAbstract::PropertyHeader header, AtNode* node, std::map<std::string, std::string> remapping = emptyRemap);
void setArrayParameter(Alembic::Abc::ICompoundProperty props, Alembic::AbcCoreAbstract::PropertyHeader header, AtNode* node, std::map<std::string, std::string> remapping = emptyRemap);
void setParameter(Alembic::Abc::ICompoundProperty props, Alembic::AbcCoreAbstract::PropertyHeader header, AtNode* node, std::map<std::string, std::string> remapping = emptyRemap);

#endif
