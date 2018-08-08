// Copyright 2017 Rodeo FX.  All rights reserved.

#include "GU_WalterPackedImpl.h"
#include "SOP_WalterNode.h"

#include <SYS/SYS_Version.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_ScopedPtr.h>

SYS_VISIBILITY_EXPORT void newSopOperator(OP_OperatorTable *operators)
{
    WalterHoudini::SOP_WalterNode::install(operators);
}

SYS_VISIBILITY_EXPORT void newGeometryPrim(GA_PrimitiveFactory *f)
{
    WalterHoudini::GU_WalterPackedImpl::install(f);
}
