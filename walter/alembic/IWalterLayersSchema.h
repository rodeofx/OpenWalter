// Copyright 2017 Rodeo FX. All rights reserved.

#ifndef __IWALTERLAYERSSCHEMA_H_
#define __IWALTERLAYERSSCHEMA_H_

#include "SchemaInfoDeclarations.h"

// Schema for reading and querying layers definitions from either an object or
// compound property.
class ALEMBIC_EXPORT IWalterLayersSchema :
    public Alembic::Abc::ISchema<WalterLayersSchemaInfo>
{
public:
    typedef IWalterLayersSchema this_type;

    // This constructor creates a new layers reader.
    // The first argument is the parent ICompoundProperty, from which the error
    // handler policy for is derived.  The second argument is the name of the
    // ICompoundProperty that contains this schemas properties. The remaining
    // optional arguments can be used to override the
    // ErrorHandlerPolicy and to specify schema interpretation matching.
    IWalterLayersSchema(
            const ICompoundProperty &iParent,
            const std::string &iName,
            const Alembic::Abc::Argument &iArg0 = Alembic::Abc::Argument(),
            const Alembic::Abc::Argument &iArg1 = Alembic::Abc::Argument() ) :
        Alembic::Abc::ISchema<WalterLayersSchemaInfo>(
                iParent, iName, iArg0, iArg1 )
    {
        init();
    }

    // This constructor wraps an existing ICompoundProperty as the layers
    // reader, and the error handler policy is derived from it. The remaining
    // optional arguments can be used to override the ErrorHandlerPolicy and to
    // specify schema interpretation matching.
    IWalterLayersSchema(
            const ICompoundProperty &iProp,
            const Alembic::Abc::Argument &iArg0 = Alembic::Abc::Argument(),
            const Alembic::Abc::Argument &iArg1 = Alembic::Abc::Argument() ) :
        Alembic::Abc::ISchema<WalterLayersSchemaInfo>( iProp, iArg0, iArg1 )
    {
        init();
    }

    // Copy constructor.
    IWalterLayersSchema( const IWalterLayersSchema& iCopy ) :
        Alembic::Abc::ISchema<WalterLayersSchemaInfo>()
    {
        *this = iCopy;
    }

    size_t getNumTargets() const;

    const std::string& getTargetName(size_t n) const;

    std::string getShaderName(
            const std::string& iTarget,
            const std::string& iShaderType) const;

private:
    typedef std::pair<std::string, std::string> TargetKey;
    typedef std::map<TargetKey, std::string> TargetMap;

    void init();

    std::vector<std::string> mTargets;
    TargetMap mShaders;
};

#endif
