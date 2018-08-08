// Copyright 2017 Rodeo FX.  All rights reserved.

#include "OWalterExpressionsSchema.h"
#include "OWalterLayersSchema.h"

#include <Alembic/AbcCoreLayer/Util.h>
#include <boost/algorithm/string.hpp>
#include "PathUtil.h"

OWalterExpressionsSchema::OWalterExpressionsSchema(
        Alembic::AbcCoreAbstract::CompoundPropertyWriterPtr iParent,
        const std::string &iName,
        const Alembic::Abc::Argument &iArg0,
        const Alembic::Abc::Argument &iArg1,
        const Alembic::Abc::Argument &iArg2,
        const Alembic::Abc::Argument &iArg3 ) :
    Alembic::Abc::OSchema<WalterExpressionsSchemaInfo>(
            iParent, iName, iArg0, iArg1, iArg2, iArg3 )
{
}

OWalterExpressionsSchema::OWalterExpressionsSchema(
        Alembic::Abc::OCompoundProperty iParent,
        const std::string &iName,
        const Alembic::Abc::Argument &iArg0,
        const Alembic::Abc::Argument &iArg1,
        const Alembic::Abc::Argument &iArg2 ) :
    Alembic::Abc::OSchema<WalterExpressionsSchemaInfo>(
            iParent.getPtr(),
            iName,
            GetErrorHandlerPolicy( iParent ),
            iArg0,
            iArg1,
            iArg2 )
{
}

OWalterExpressionsSchema::~OWalterExpressionsSchema()
{
    // Put everything we have to Alembic.
    if (mExpressions.empty()) {
        // Nothing to do
        return;
    }

    // The shaders should be with "replace" metadata.
    Alembic::Abc::MetaData md;
    Alembic::AbcCoreLayer::SetReplace( md, true );

    for (auto eit=mExpressions.cbegin(); eit!=mExpressions.cend(); eit++) {
        std::string expression = eit->first;
        const ExpressionData& expressionData = eit->second;
        const LayerMap& layers = expressionData.layerMap;

        // Save layers
        if (layers.empty()) {
            // Nothing to do
            continue;
        }

        expression = WalterCommon::mangleString(
            WalterCommon::convertRegex(expression));

        Alembic::Abc::OCompoundProperty expProperty(this->getPtr(), expression);
        OWalterLayersSchema layersSchema = addLayers(expProperty);

        for (auto it=layers.cbegin(); it!=layers.cend(); it++) {
            const LayerKey& key = it->first;
            const std::string& value = it->second;

            layersSchema.setShader(key.first, key.second, value, md);
        }

        // Save groups
        if (expressionData.grouped)
        {
            Alembic::Abc::OCompoundProperty group(
                    expProperty, EXPRESSIONS_GROUP, md);
            Alembic::Abc::OStringProperty groupProp(
                    group, EXPRESSIONS_GROUPNAME);
            groupProp.set(expressionData.groupname);
        }
    }
}

void OWalterExpressionsSchema::setExpressionShader(
            const std::string & iExpression,
            const std::string & iLayer,
            const std::string & iShaderType,
            const std::string & iShaderName)
{
    // Save it to the internal map.
    mExpressions[iExpression].layerMap[LayerKey(iLayer, iShaderType)] =
        iShaderName;
}

void OWalterExpressionsSchema::setExpressionGroup(
            const std::string & iExpression,
            const std::string & iGroup)
{
    mExpressions[iExpression].grouped = true;
    mExpressions[iExpression].groupname = iGroup;
}

OWalterExpressionsSchema addExpressions(
        Alembic::Abc::OCompoundProperty iProp,
        const std::string& iPropName )
{
    return OWalterExpressionsSchema( iProp.getPtr(), iPropName );
}

OWalterExpressionsSchema addExpressions(
        Alembic::Abc::OObject iObject,
        const std::string& iPropName )
{
    return addExpressions( iObject.getProperties(), iPropName );
}

