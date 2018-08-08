// Copyright 2017 Rodeo FX. All rights reserved.

#include "IWalterLayersSchema.h"

#include <Alembic/AbcMaterial/All.h>
#include <boost/algorithm/string.hpp>

size_t IWalterLayersSchema::getNumTargets() const
{
    return mTargets.size();
}

const std::string& IWalterLayersSchema::getTargetName(size_t n) const
{
    return mTargets[n];
}

std::string IWalterLayersSchema::getShaderName(
        const std::string& iTarget,
        const std::string& iShaderType) const
{
    TargetKey key(iTarget, iShaderType);
    TargetMap::const_iterator found = mShaders.find(key);
    if (found != mShaders.end()) {
        return found->second;
    }

    return std::string();
}

void IWalterLayersSchema::init()
{
    for (size_t i=0; i<getNumProperties(); i++) {
        std::string layerAndType = getPropertyHeader(i).getName();
        std::vector<std::string> split;

        boost::split(split, layerAndType, boost::is_any_of("."));
        if (split.size()<3)
            continue;

        Alembic::Abc::ICompoundProperty layerProp(this->getPtr(), layerAndType);

        std::string shader;
        if (!Alembic::AbcMaterial::getMaterialAssignmentPath(layerProp, shader))
            continue;

        const std::string& target = split[1];
        const std::string& type = split[2];

        if (std::find(mTargets.begin(),mTargets.end(),target) == mTargets.end())
            mTargets.push_back(target);

        mShaders[TargetKey(target, type)] = shader;
    }
}
