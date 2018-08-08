// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __OWALTERLAYERSSCHEMA_H_
#define __OWALTERLAYERSSCHEMA_H_

#include <Alembic/Abc/All.h>

#include "SchemaInfoDeclarations.h"

#define LAYERS_PROPNAME ".layers"

// Schema for writing per layer shader assignments as either an object or a
// compound property.
class ALEMBIC_EXPORT OWalterLayersSchema :
    public Alembic::Abc::OSchema<WalterLayersSchemaInfo>
{
public:
    typedef OWalterLayersSchema this_type;

    OWalterLayersSchema(
        Alembic::AbcCoreAbstract::CompoundPropertyWriterPtr iParent,
        const std::string &iName,
        const Alembic::Abc::Argument &iArg0 = Alembic::Abc::Argument(),
        const Alembic::Abc::Argument &iArg1 = Alembic::Abc::Argument(),
        const Alembic::Abc::Argument &iArg2 = Alembic::Abc::Argument(),
        const Alembic::Abc::Argument &iArg3 = Alembic::Abc::Argument() );

    OWalterLayersSchema(
            Alembic::Abc::OCompoundProperty iParent,
            const std::string &iName,
            const Alembic::Abc::Argument &iArg0 = Alembic::Abc::Argument(),
            const Alembic::Abc::Argument &iArg1 = Alembic::Abc::Argument(),
            const Alembic::Abc::Argument &iArg2 = Alembic::Abc::Argument() );

    // Copy constructor.
    OWalterLayersSchema( const OWalterLayersSchema& iCopy ) :
        Alembic::Abc::OSchema<WalterLayersSchemaInfo>()
    {
        *this = iCopy;
    }

    // Declare shader for a given target and shaderType.
    // "Target's" value is an agreed upon convention for a render layer.
    // "ShaderType's" value is an agreed upon convention for shader terminals
    // such as "surface," "displacement," "light", "coshader_somename."
    // "ShaderName's" value is an identifier meaningful to the target
    // application (i.e. "paintedplastic," "fancy_spot_light").
    // It's only valid to call this once per target/shaderType combo.
    void setShader(
            const std::string & iTarget,
            const std::string & iShaderType,
            const std::string & iShaderName,
            const Alembic::Abc::MetaData& md);
};

// Adds a local layer schema within any compound.
OWalterLayersSchema addLayers(
        Alembic::Abc::OCompoundProperty iProp,
        const std::string& iPropName = LAYERS_PROPNAME );

// Adds a local layer schema to any object.
OWalterLayersSchema addLayers(
        Alembic::Abc::OObject iObject,
        const std::string& iPropName = LAYERS_PROPNAME );

#endif
