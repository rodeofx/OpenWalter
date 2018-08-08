// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef _WALTEROVERRIDENODE_H_
#define _WALTEROVERRIDENODE_H_

#include <maya/MPxNode.h>
#include <maya/MTypeId.h>

class WalterOverride : public MPxNode {
public:
    WalterOverride();
    virtual ~WalterOverride();
    static void* creator();
    static MStatus initialize();

    virtual void postConstructor();

    static MTypeId mId;

private:
    static void nameChanged(MObject& node, void *data);
};

#endif
