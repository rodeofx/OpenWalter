// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __SOP_WalterNode_h__
#define __SOP_WalterNode_h__

#include <SOP/SOP_Node.h>

namespace WalterHoudini
{

class SOP_WalterNode : public SOP_Node
{
    enum class PrimLoadingMode
    {
        single,
        children
    };

public:
    static void install(OP_OperatorTable* table);
    static OP_Node* constructor(OP_Network*, const char*, OP_Operator*);

    // Get all the file names combined in one string
    std::string evalSessionName(fpreal time) const;

    // Stores the description of the interface of the SOP in Houdini.
    // Each parm template refers to a parameter.
    static PRM_Template sTemplateList[];

protected:
    SOP_WalterNode(OP_Network* net, const char* name, OP_Operator* op);
    virtual ~SOP_WalterNode();

    // cookMySop does the actual work of the SOP computing, in this
    // case, a star shape.
    virtual OP_ERROR cookMySop(OP_Context& context);

private:
    struct Parms
    {
        Parms();
        Parms(const Parms& src);

        /// Compare this set of parameters with the other set of parameters to
        /// see if the update is needed (i.e. the filename has changed, etc.)
        bool needsUpdate(const Parms& parms);
        bool timeChanged(const Parms& parms);

        Parms& operator=(const Parms& src);

        std::string mFileName;
        std::string mPrimPath;
        fpreal mFrame;
        PrimLoadingMode mPrimMode;
        std::string mHdRendererName;
    };

    void evaluateParms(Parms& parms, OP_Context& context);

    Parms mLastParms;
};
} // end namespace WalterHoudini

#endif
