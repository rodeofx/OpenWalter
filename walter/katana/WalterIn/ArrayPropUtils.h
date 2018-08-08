#ifndef FnGeolibOp_WalterIn_ArrayPropUtils_H
#define FnGeolibOp_WalterIn_ArrayPropUtils_H

#include <Alembic/Abc/All.h>
#include <FnAttribute/FnGroupBuilder.h>
#include "AbcCook.h"

namespace WalterIn
{

void arrayPropertyToAttr(Alembic::Abc::ICompoundProperty & iParent,
    const Alembic::AbcCoreAbstract::PropertyHeader & iPropHeader,
    const std::string & iPropName,
    FnKatAttributeType iType,
    AbcCookPtr ioCook,
    Foundry::Katana::GroupBuilder & oStaticGb,
    Alembic::Abc::IUInt64ArrayProperty * iIdsProperty = NULL);

void arrayPropertyToAttr(ArrayProp & iProp,
    const OpArgs & iArgs,
    Foundry::Katana::GroupBuilder & oGb,
    Alembic::Abc::IUInt64ArrayProperty * iIdsProperty = NULL);

}

#endif
