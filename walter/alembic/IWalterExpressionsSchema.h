// Copyright 2017 Rodeo FX. All rights reserved.

#ifndef __IWALTEREXPRESSIONSSCHEMA_H_
#define __IWALTEREXPRESSIONSSCHEMA_H_

#include "IWalterLayersSchema.h"
#include "OWalterExpressionsSchema.h"
#include "SchemaInfoDeclarations.h"

#include <Alembic/Abc/ISchemaObject.h>

// Schema for reading and querying shader assignments from either an object or
// compound property.
class ALEMBIC_EXPORT IWalterExpressionsSchema :
    public Alembic::Abc::ISchema<WalterExpressionsSchemaInfo>
{
public:
    typedef IWalterExpressionsSchema this_type;

    IWalterExpressionsSchema() {}

    // This constructor creates a new assignments reader.
    // The first argument is the parent ICompoundProperty, from which the error
    // handler policy for is derived. The second argument is the name of the
    // ICompoundProperty that contains this schemas properties. The remaining
    // optional arguments can be used to override the ErrorHandlerPolicy and to
    // specify schema interpretation matching.
    IWalterExpressionsSchema(
            const ICompoundProperty &iParent,
            const std::string &iName,
            const Alembic::Abc::Argument &iArg0 = Alembic::Abc::Argument(),
            const Alembic::Abc::Argument &iArg1 = Alembic::Abc::Argument() ) :
        Alembic::Abc::ISchema<WalterExpressionsSchemaInfo>(
                iParent, iName, iArg0, iArg1)
    {
        init();
    }

    // This constructor wraps an existing ICompoundProperty as the assignments
    // reader, and the error handler policy is derived from it. The remaining
    // optional arguments can be used to override the ErrorHandlerPolicy and to
    // specify schema interpretation matching.
    IWalterExpressionsSchema(
            const ICompoundProperty &iProp,
            const Alembic::Abc::Argument &iArg0 = Alembic::Abc::Argument(),
            const Alembic::Abc::Argument &iArg1 = Alembic::Abc::Argument() ) :
        Alembic::Abc::ISchema<WalterExpressionsSchemaInfo>(iProp, iArg0, iArg1)
    {
        init();
    }

    // Copy constructor.
    IWalterExpressionsSchema( const IWalterExpressionsSchema& iCopy ) :
        Alembic::Abc::ISchema<WalterExpressionsSchemaInfo>()
    {
        *this = iCopy;
    }

    const std::string& getExpression(size_t n) const;
    bool getHasGroup(size_t n) const;
    const std::string& getExpressionGroup(size_t n) const;
    IWalterLayersSchema getLayerSchema(size_t n) const;

private:
    void init();

    // It's vector for accessing by index.
    std::vector<std::string> mExpressionKeys;
    std::vector<std::string> mExpressions;
    std::vector<bool> mGrouped;
    std::vector<std::string> mGroupNames;
};

typedef Alembic::Abc::ISchemaObject<IWalterExpressionsSchema>
    IWalterExpressions;

// Returns true and fills result within the given compound property.
bool hasExpressions(
        Alembic::Abc::ICompoundProperty iCompound,
        IWalterExpressionsSchema& oResult,
        const std::string& iPropName = EXPRESSIONS_PROPNAME);

// Returns true and fills result within the given compound property.
bool hasExpressions(
        Alembic::Abc::IObject iObject,
        IWalterExpressionsSchema& oResult,
        const std::string& iPropName = EXPRESSIONS_PROPNAME);

#endif
