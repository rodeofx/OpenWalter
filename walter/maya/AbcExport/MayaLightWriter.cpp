//-*****************************************************************************
//
// Copyright (c) 2009-2012,
//  Sony Pictures Imageworks Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic, nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//-*****************************************************************************
//
// Copyright 2017 Rodeo FX. All rights reserved.

#include "MayaLightWriter.h"

MayaLightWriter::MayaLightWriter(
    MDagPath& iDag,
    Alembic::Abc::OObject& iParent,
    Alembic::Util::uint32_t iTimeIndex,
    const JobArgs& iArgs) :
        mIsAnimated(false),
        mDagPath(iDag)
{
    MStatus status = MS::kSuccess;
    MFnDagNode mfnLight(iDag, &status);
    if (!status)
    {
        MGlobal::displayError("MFnLight() failed for MayaLightWriter");
    }

    MString name = mfnLight.name();
    name = util::stripNamespaces(name, iArgs.stripNamespace);

    Alembic::AbcCoreAbstract::MetaData md;
    util::setMeta(mfnLight, iArgs, md);
    Alembic::AbcGeom::OLight obj(iParent, name.asChar(), iTimeIndex, md);
    mSchema = obj.getSchema();

    MObject lightObj = iDag.node();
    if (iTimeIndex != 0 && util::isAnimated(lightObj))
    {
        mIsAnimated = true;
    }
    else
    {
        iTimeIndex = 0;
    }

    // Setup default properties.
    Alembic::Abc::OCompoundProperty light(mSchema.getPtr(), ".light");
    mType = Alembic::Abc::OStringProperty(light, ".type");
    mColor = Alembic::Abc::OC3fProperty(light, ".color", iTimeIndex);
    mIntensity = Alembic::Abc::OFloatProperty(light, ".intensity", iTimeIndex);
    mDecay = Alembic::Abc::OFloatProperty(light, ".decay", iTimeIndex);

    // The type of the light should never be animated, so we can setup it here.
    mType.set(mfnLight.typeName().asChar());

    Alembic::Abc::OCompoundProperty cp;
    Alembic::Abc::OCompoundProperty up;
    if (AttributesWriter::hasAnyAttr(mfnLight, iArgs))
    {
        cp = mSchema.getArbGeomParams();
        up = mSchema.getUserProperties();
    }

    mAttrs = AttributesWriterPtr(
        new AttributesWriter(cp, up, obj, mfnLight, iTimeIndex, iArgs, true));

    if (!mIsAnimated || iArgs.setFirstAnimShape)
    {
        write();
    }
}

void MayaLightWriter::write()
{
    MFnDagNode mfnLight(mDagPath);

    // Save color
    MPlug colorPlug = mfnLight.findPlug("color");
    if (!colorPlug.isNull())
    {
        Alembic::Abc::C3f colorVal(
            colorPlug.child(0).asFloat(),
            colorPlug.child(1).asFloat(),
            colorPlug.child(2).asFloat());
        mColor.set(colorVal);
    }

    // Save intensity
    MPlug intensityPlug = mfnLight.findPlug("intensity");
    if (!intensityPlug.isNull())
    {
        mIntensity.set(intensityPlug.asFloat());
    }

    // Get decay
    MPlug decayPlug = mfnLight.findPlug("decayRate");
    if (!decayPlug.isNull())
    {
        mDecay.set(decayPlug.asFloat());
    }
}

bool MayaLightWriter::isAnimated() const
{
    return mIsAnimated || (mAttrs != NULL && mAttrs->isAnimated());
}
