// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __OWALTEREXPRESSIONSSCHEMA_H_
#define __OWALTEREXPRESSIONSSCHEMA_H_

#include "SchemaInfoDeclarations.h"

#define EXPRESSIONS_PROPNAME ".expressions"
#define EXPRESSIONS_GROUP ".group"
#define EXPRESSIONS_GROUPNAME ".name"

// Schema for writing shader assignments as either an object or a compound
// property.
class ALEMBIC_EXPORT OWalterExpressionsSchema :
    public Alembic::Abc::OSchema<WalterExpressionsSchemaInfo>
{
public:
    typedef OWalterExpressionsSchema this_type;

    OWalterExpressionsSchema(
        Alembic::AbcCoreAbstract::CompoundPropertyWriterPtr iParent,
        const std::string &iName,
        const Alembic::Abc::Argument &iArg0 = Alembic::Abc::Argument(),
        const Alembic::Abc::Argument &iArg1 = Alembic::Abc::Argument(),
        const Alembic::Abc::Argument &iArg2 = Alembic::Abc::Argument(),
        const Alembic::Abc::Argument &iArg3 = Alembic::Abc::Argument() );

    OWalterExpressionsSchema(
            Alembic::Abc::OCompoundProperty iParent,
            const std::string &iName,
            const Alembic::Abc::Argument &iArg0 = Alembic::Abc::Argument(),
            const Alembic::Abc::Argument &iArg1 = Alembic::Abc::Argument(),
            const Alembic::Abc::Argument &iArg2 = Alembic::Abc::Argument() );

    // Put everything to Alembic when the object is destroyed.
    ~OWalterExpressionsSchema();

    // Copy constructor.
    OWalterExpressionsSchema( const OWalterExpressionsSchema& iCopy ) :
        Alembic::Abc::OSchema<WalterExpressionsSchemaInfo>()
    {
        *this = iCopy;
    }

    // Declare shader for a given expression and shaderType. It can be called
    // many times per expression/layer/shader type. The real information will go
    // to Alembic when the object is destroyed.
    void setExpressionShader(
            const std::string & iExpression,
            const std::string & iLayer,
            const std::string & iShaderType,
            const std::string & iShaderName);

    // Declare a group for a given expression. It can be called many times. The
    // real information will go to Alembic when the object is destroyed.
    void setExpressionGroup(
            const std::string & iExpression,
            const std::string & iGroup);

private:
    // Layer name, layer target (shader, displacement etc.)
    typedef std::pair<std::string, std::string> LayerKey;
    // Layer, shader
    typedef std::map<LayerKey, std::string> LayerMap;

    struct ExpressionData
    {
        bool grouped;
        std::string groupname;
        LayerMap layerMap;

        // Default constructor
        ExpressionData() : grouped(false) {}
    };

    // Expression, data
    typedef std::map<std::string, ExpressionData> ExpMap;

    ExpMap mExpressions;
};

// Adds a local material schema within any compound.
OWalterExpressionsSchema addExpressions(
        Alembic::Abc::OCompoundProperty iProp,
        const std::string& iPropName = EXPRESSIONS_PROPNAME );

// Adds a local expression schema to any object.
OWalterExpressionsSchema addExpressions(
        Alembic::Abc::OObject iObject,
        const std::string& iPropName = EXPRESSIONS_PROPNAME );

#endif
