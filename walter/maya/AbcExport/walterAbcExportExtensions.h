// Copyright 2017 Rodeo FX. All rights reserved.

#ifndef __WALTERABCEXPORTEXTENSIONS_H__
#define __WALTERABCEXPORTEXTENSIONS_H__

#include "MayaLightWriter.h"

/** @brief This is a helper object that exports lights to Alembic performing
 * conversion to Arnold. */
class ArnoldLightExporter
{
public:
    ArnoldLightExporter();

    /**
     * @brief Adds a light to the internal list.
     *
     * @param iLight Given light.
     */
    void add(MayaLightWriterPtr iLight) { mLightList.push_back(iLight); }

    /**
     * @brief Export all the saved lights to Alembic.
     *
     * @param isFirst true if it's the first frame.
     */
    void eval(bool isFirst);

    /**
     * @brief Checks if the given object is a light.
     *
     * @param iObject The given object.
     *
     * @return True if it's a light.
     */
    static bool isLight(const MObject& iObject);

private:
    bool mValid;
    std::vector<Alembic::Util::shared_ptr<MayaLightWriter>> mLightList;
};

#endif
