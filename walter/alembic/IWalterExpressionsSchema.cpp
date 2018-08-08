// Copyright 2017 Rodeo FX. All rights reserved.

#include "IWalterExpressionsSchema.h"
#include "OWalterLayersSchema.h"

#include <boost/algorithm/string.hpp>
#include "PathUtil.h"

void IWalterExpressionsSchema::init()
{
    unsigned int numProps = getNumProperties();

    // Pre-allocate the vectors.
    mExpressionKeys.reserve(numProps);
    mExpressions.reserve(numProps);
    mGrouped.reserve(numProps);
    mGroupNames.reserve(numProps);

    for (size_t i=0; i<numProps; i++)
    {
        std::string expressionKey = getPropertyHeader(i).getName();
        mExpressionKeys.push_back(expressionKey);

        Alembic::Abc::ICompoundProperty expression(
                this->getPtr(), expressionKey);

        // We have mangled expressions. It's necessary to demangle them.
        // TODO: Save something like ".name" field
        mExpressions.push_back(WalterCommon::demangleString(expressionKey));

        // Get the name of the group
        if (expression.getPropertyHeader(EXPRESSIONS_GROUP))
        {
            Alembic::Abc::ICompoundProperty group(
                    expression, EXPRESSIONS_GROUP);
            Alembic::Abc::IStringProperty sprop(
                    group, EXPRESSIONS_GROUPNAME);
            mGrouped.push_back(true);
            mGroupNames.push_back(sprop.getValue());
        }
        else
        {
            mGrouped.push_back(false);
            mGroupNames.push_back(std::string());
        }
    }
}

const std::string& IWalterExpressionsSchema::getExpression(size_t n) const
{
    return mExpressions[n];
}

bool IWalterExpressionsSchema::getHasGroup(size_t n) const
{
    return mGrouped[n];
}

const std::string& IWalterExpressionsSchema::getExpressionGroup(size_t n) const
{
    return mGroupNames[n];
}

IWalterLayersSchema IWalterExpressionsSchema::getLayerSchema(size_t n) const
{
    Alembic::Abc::ICompoundProperty expression(
            this->getPtr(), mExpressionKeys[n]);
    return IWalterLayersSchema(expression, LAYERS_PROPNAME);
}

bool hasExpressions(
        Alembic::Abc::ICompoundProperty iCompound,
        IWalterExpressionsSchema& oResult,
        const std::string& iPropName)
{
    if (!iCompound.valid())
    {
        return false;
    }

    if (const Alembic::AbcCoreAbstract::PropertyHeader * header =
            iCompound.getPropertyHeader(iPropName))
    {
        if (IWalterExpressionsSchema::matches(*header))
        {
            oResult = IWalterExpressionsSchema(iCompound, iPropName);
            return true;
        }
    }

    return false;
}

bool hasExpressions(
        Alembic::Abc::IObject iObject,
        IWalterExpressionsSchema& oResult,
        const std::string& iPropName)
{
    //don't indicate has-a for matching Material objects
    if (iObject.valid() && iPropName == EXPRESSIONS_PROPNAME)
    {
        if (IWalterExpressions::matches(iObject.getHeader()))
        {
            return false;
        }
    }

    return hasExpressions(iObject.getProperties(), oResult, iPropName);
}
