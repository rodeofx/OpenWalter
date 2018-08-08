// Copyright 2017 Rodeo FX.  All rights reserved.

#include "OWalterLayersSchema.h"

#include <Alembic/AbcMaterial/All.h>

OWalterLayersSchema::OWalterLayersSchema(
        Alembic::AbcCoreAbstract::CompoundPropertyWriterPtr iParent,
        const std::string &iName,
        const Alembic::Abc::Argument &iArg0,
        const Alembic::Abc::Argument &iArg1,
        const Alembic::Abc::Argument &iArg2,
        const Alembic::Abc::Argument &iArg3 ) :
    Alembic::Abc::OSchema<WalterLayersSchemaInfo>(
            iParent, iName, iArg0, iArg1, iArg2, iArg3 )
{
}

OWalterLayersSchema::OWalterLayersSchema(
        Alembic::Abc::OCompoundProperty iParent,
        const std::string &iName,
        const Alembic::Abc::Argument &iArg0,
        const Alembic::Abc::Argument &iArg1,
        const Alembic::Abc::Argument &iArg2 ) :
    Alembic::Abc::OSchema<WalterLayersSchemaInfo>(
            iParent.getPtr(),
            iName,
            GetErrorHandlerPolicy( iParent ),
            iArg0,
            iArg1,
            iArg2 )
{
}

void OWalterLayersSchema::setShader(
        const std::string & iTarget,
        const std::string & iShaderType,
        const std::string & iShaderName,
        const Alembic::Abc::MetaData& md)
{
    // Create a compound attribute with name ".layer1.displacement"
    // TODO: Check layer and type names.
    Alembic::Abc::OCompoundProperty layer(
            this->getPtr(),
            std::string(".") + iTarget + "." + iShaderType,
            md );

    Alembic::AbcMaterial::addMaterialAssignment(layer, iShaderName);
}

OWalterLayersSchema addLayers(
        Alembic::Abc::OCompoundProperty iProp,
        const std::string& iPropName)
{
    return OWalterLayersSchema( iProp.getPtr(), iPropName );
}

OWalterLayersSchema addLayers(
        Alembic::Abc::OObject iObject,
        const std::string& iPropName)
{
    return addLayers( iObject.getProperties(), iPropName );
}
