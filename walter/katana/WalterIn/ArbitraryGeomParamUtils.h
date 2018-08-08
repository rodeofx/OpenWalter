#ifndef FnGeolibOp_WalterIn_ArbitraryGeomParamUtils_H
#define FnGeolibOp_WalterIn_ArbitraryGeomParamUtils_H

#include <Alembic/Abc/All.h>
#include <FnAttribute/FnGroupBuilder.h>
#include "AbcCook.h"

namespace WalterIn
{

void indexedParamToAttr(IndexedGeomParamPair & iProp,
    const OpArgs & iArgs,
    Foundry::Katana::GroupBuilder & oGb);

void processArbitraryGeomParam(AbcCookPtr ioCook,
    Alembic::Abc::ICompoundProperty & iParent,
    const Alembic::AbcCoreAbstract::PropertyHeader & iPropHeader,
    Foundry::Katana::GroupBuilder & oStaticGb,
    const std::string & iAttrPath,
    Alembic::Abc::v10::IUInt64ArrayProperty * iIdsProperty = NULL);

void processArbGeomParams(AbcCookPtr ioCook,
    Alembic::Abc::ICompoundProperty & iParent,
    Foundry::Katana::GroupBuilder & oStaticGb,
    Alembic::Abc::IUInt64ArrayProperty * iIdsProperty = NULL);
}

#endif
