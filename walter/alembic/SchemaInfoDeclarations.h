// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __SCHEMAINFODECLARATIONS_H_
#define __SCHEMAINFODECLARATIONS_H_

#include <Alembic/Abc/ISchema.h>
#include <Alembic/Abc/OSchema.h>

ALEMBIC_ABC_DECLARE_SCHEMA_INFO(
        "RodeoWalter_Layers_v1",
        "",
        ".layers",
        false,
        WalterLayersSchemaInfo );

ALEMBIC_ABC_DECLARE_SCHEMA_INFO(
        "RodeoWalter_Expressions_v1",
        "",
        ".expressions",
        false,
        WalterExpressionsSchemaInfo );

#endif
