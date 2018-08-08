#include "AbcCook.h"
#include "ArrayPropUtils.h"
#include "ScalarPropUtils.h"
#include "ArbitraryGeomParamUtils.h"

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnDataBuilder.h>

namespace WalterIn
{

    void cookFaceset(AbcCookPtr ioCook, FnAttribute::GroupBuilder & oStaticGb)
    {
        Alembic::AbcGeom::IFaceSetPtr objPtr(
            new Alembic::AbcGeom::IFaceSet(*(ioCook->objPtr),
                                           Alembic::AbcGeom::kWrapExisting));

        Alembic::AbcGeom::IFaceSetSchema schema = objPtr->getSchema();

        Alembic::Abc::ICompoundProperty userProp = schema.getUserProperties();
        Alembic::Abc::ICompoundProperty arbGeom = schema.getArbGeomParams();
        std::string abcUser = "abcUser.";
        processUserProperties(ioCook, userProp, oStaticGb, abcUser);
        processArbGeomParams(ioCook, arbGeom, oStaticGb);

        oStaticGb.set("type", FnAttribute::StringAttribute("faceset"));

        // IFaceSetSchema doesn't expose this yet
        const Alembic::AbcCoreAbstract::PropertyHeader * propHeader =
            schema.getPropertyHeader(".faces");

        if (propHeader != NULL)
        {
            arrayPropertyToAttr(schema, *propHeader, "geometry.faces",
                                kFnKatAttributeTypeInt, ioCook, oStaticGb);
        }
    }

}

