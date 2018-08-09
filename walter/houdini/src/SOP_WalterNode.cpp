// Copyright 2017 Rodeo FX.  All rights reserved.

#include "SOP_WalterNode.h"

#include "GU_WalterPackedImpl.h"
#include "walterHoudiniUtils.h"

#include <CH/CH_LocalVariable.h>
#include <CMD/CMD_Manager.h>
#include <GEO/GEO_PrimPoly.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimPacked.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>
#include <SYS/SYS_Math.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_StringStream.h>
#include <limits.h>

namespace WalterHoudini
{

int pickNodes(void* data, int index, fpreal t, const PRM_Template* tplate);

static PRM_Name fileName("fileName#", "Path");
static PRM_SpareData fileData(
    PRM_SpareArgs() << PRM_SpareToken(
                           PRM_SpareData::getFileChooserPatternToken(),
                           "*.abc,*.usd,*.usda,*.usdb,*.usdc")
                    << PRM_SpareToken(
                           PRM_SpareData::getFileChooserModeToken(),
                           PRM_SpareData::getFileChooserModeValRead()));

static PRM_Name hydraPlugins[] = {
    PRM_Name("HdStreamRendererPlugin", "Stream"),
    PRM_Name("HdEmbreeRendererPlugin", "Embree"),
    PRM_Name()};

static PRM_Name hydraPluginName("hdPlugin", "Hydra Plugin");
static PRM_Default hydraPluginDefault(0, "HdStreamRendererPlugin");
static PRM_ChoiceList hydraPluginChoice(PRM_CHOICELIST_SINGLE, hydraPlugins);


static PRM_Name layersName("layers", "Layers");
static PRM_Template layersTemplates[] = {
    PRM_Template(PRM_FILE, 1, &fileName, 0, 0, 0, 0, &fileData),
    PRM_Template()};

static PRM_Name primPathName("primPath", "Prim Path");
static PRM_Name pickObjectPathName("pickObjectPath", "Pick");
static PRM_SpareData pickObjectPathData(
    PRM_SpareArgs()
    << PRM_SpareToken(PRM_SpareData::getButtonIconToken(), "BUTTONS_tree"));

static PRM_Name primModeName("primMode", "Create Primitives For");
static PRM_Name primModeOptions[] = {
    PRM_Name("root", "Specified Primitive Only"),
    PRM_Name("children", "Children Primitives"),
    PRM_Name()};
static PRM_Default primModeDefault(0, "root");
static PRM_ChoiceList primModeChoice(PRM_CHOICELIST_SINGLE, primModeOptions);

static PRM_Name frameName("frame", "Frame");
static PRM_Default frameDef(0, "$FF");

PRM_Template SOP_WalterNode::sTemplateList[] = {
    PRM_Template(PRM_ORD, 1, &hydraPluginName, &hydraPluginDefault, &hydraPluginChoice),   
    PRM_Template(PRM_MULTITYPE_SCROLL, layersTemplates, 1, &layersName),
    PRM_Template(PRM_STRING, PRM_TYPE_JOIN_PAIR, 1, &primPathName),
    PRM_Template(
        PRM_CALLBACK,
        PRM_TYPE_NO_LABEL,
        1,
        &pickObjectPathName,
        0,
        0,
        0,
        pickNodes,
        &pickObjectPathData),
    PRM_Template(PRM_ORD, 1, &primModeName, &primModeDefault, &primModeChoice),
    PRM_Template(PRM_FLT, 1, &frameName, &frameDef),
    PRM_Template()};

// This is called when the user presses the tree button to see the tree of the
// loaded stage.
int pickNodes(void* data, int index, fpreal t, const PRM_Template* tplate)
{
    SOP_WalterNode* sop = (SOP_WalterNode*)(data);

    // The treechooser command presents the list of strings using a tree-style
    // graphical interface. If _-r_ option is not present, the command will
    // return multiple choices as a space-separated list.
    UT_WorkBuffer cmd;
    cmd.strcpy("treechooser -r");

    UT_String primPath;
    sop->evalString(primPath, primPathName.getToken(), 0, t);
    if (primPath.isstring())
    {
        // Specify initial selection of nodes in the tree.
        cmd.strcat(" -s ");
        cmd.protectedStrcat(primPath);
    }

    std::vector<std::string> objects;
    WalterHoudiniUtils::getObjectList(
        sop->evalSessionName(t), "", false, objects);
    for (auto object : objects)
    {
        cmd.strcat(" ");
        cmd.protectedStrcat(object.c_str());
    }

    CMD_Manager* mgr = CMDgetManager();
    UT_OStringStream os;
    mgr->execute(cmd.buffer(), 0, &os);
    UT_String result(os.str().buffer());
    result.trimBoundingSpace();
    sop->setChRefString(
        result, CH_STRING_LITERAL, primPathName.getToken(), 0, t);

    return 0;
}

// newSopOperator is the hook that Houdini grabs from this dll and invokes to
// register the SOP.  In this case we add ourselves to the specified operator
// table.
void SOP_WalterNode::install(OP_OperatorTable* table)
{
    OP_Operator* op = new OP_Operator(
        "walter_import",  // Internal name
        "Walter",  // UI name
        constructor,  // How to build the SOP
        sTemplateList,  // My parameters
        0,  // Min # of sources
        0,  // Max # of sources
        0,  // Local variables
        OP_FLAG_GENERATOR);  // Flag it as generator

    op->setIconName("Walter_round_logo_gold.png");
    table->addOperator(op);
}

OP_Node* SOP_WalterNode::constructor(
    OP_Network* net,
    const char* name,
    OP_Operator* op)
{
    return new SOP_WalterNode(net, name, op);
}

std::string SOP_WalterNode::evalSessionName(fpreal time) const
{
    // Get number of layers.
    int nLayers = evalInt(layersName.getToken(), 0, time);

    // We need to create a string that contains all the layers separated with
    // the semicolon.
    UT_String fullFileName;
    // Houdini iterates parameters from 1.
    for (int i = 1; i <= nLayers; i++)
    {
        // Form the name.
        char buffer[64];
        sprintf(buffer, "fileName%i", i);

        // Get the value.
        UT_String current;
        evalString(current, buffer, 0, time);

        if (current && current.length())
        {
            if (fullFileName)
            {
                fullFileName += ":";
            }

            fullFileName += current;
        }
    }

    return fullFileName.toStdString();
}

SOP_WalterNode::SOP_WalterNode(
    OP_Network* net,
    const char* name,
    OP_Operator* op) :
        SOP_Node(net, name, op)
{
    // This SOP always generates fresh geometry, so setting this flag is a bit
    // redundant, but one could change it to check for the old star and only
    // bump relevant data IDs, (P and the primitive list), depending on what
    // parameters changed.
    mySopFlags.setManagesDataIDs(true);
}

OP_ERROR SOP_WalterNode::cookMySop(OP_Context& context)
{
    if (lockInputs(context) >= UT_ERROR_ABORT)
    {
        return error();
    }

    Parms parms;
    evaluateParms(parms, context);

    if (mLastParms.needsUpdate(parms))
    {
        // Clear all.
        // TODO: It's possible to keep and reuse all the primitives.
        gdp->clearAndDestroy();

        std::vector<std::string> primPathVec;

        switch (parms.mPrimMode)
        {
            case PrimLoadingMode::single:
            default:
                primPathVec.push_back(parms.mPrimPath);
                break;
            case PrimLoadingMode::children:
                WalterHoudiniUtils::getObjectList(
                    parms.mFileName, parms.mPrimPath, true, primPathVec);
                break;
        }

        for (const std::string& primPath : primPathVec)
        {
            // Create a packed Walter primitive.
            GU_PrimPacked* packed = GU_WalterPackedImpl::build(
                *gdp,
                parms.mFileName,
                primPath,
                parms.mHdRendererName,
                parms.mFrame);
            if (!packed)
            {
                fprintf(stderr, "[WALTER]: Can't create packed Walter\n");
                // TODO: continue;
                return error();
            }

            // Assign it as a point.
            GA_Offset pt = gdp->appendPointOffset();
            packed->setVertexPoint(pt);

            // TODO: Set the position.
            // UT_Vector3 pivot(0, 0, 0);
            // gdp->setPos3(pt, pivot);
        }
    }
    else if (mLastParms.timeChanged(parms))
    {
        for (GA_Offset pt : gdp->getPrimitiveRange())
        {
            GA_Primitive* prm = gdp->getPrimitive(pt);
            GU_PrimPacked* pack = UTverify_cast<GU_PrimPacked*>(prm);

            // Cast it to Walter.
            GU_WalterPackedImpl* walter =
                UTverify_cast<GU_WalterPackedImpl*>(pack->implementation());
            walter->setFrame(parms.mFrame);
        }
    }

    mLastParms = parms;

    return error();
}

SOP_WalterNode::~SOP_WalterNode()
{}

SOP_WalterNode::Parms::Parms() :
        mFileName(),
        mFrame(std::numeric_limits<float>::min()),
        mPrimPath(),
        mPrimMode(PrimLoadingMode::single),
        mHdRendererName()
{}

SOP_WalterNode::Parms::Parms(const SOP_WalterNode::Parms& src)
{
    *this = src;
}

bool SOP_WalterNode::Parms::needsUpdate(const SOP_WalterNode::Parms& parms)
{
    return mFileName != parms.mFileName || mPrimPath != parms.mPrimPath ||
        mPrimMode != parms.mPrimMode || mHdRendererName != parms.mHdRendererName;
}

bool SOP_WalterNode::Parms::timeChanged(const SOP_WalterNode::Parms& parms)
{
    return mFrame != parms.mFrame;
}

SOP_WalterNode::Parms& SOP_WalterNode::Parms::operator=(
    const SOP_WalterNode::Parms& src)
{
    mFileName = src.mFileName;
    mPrimPath = src.mPrimPath;
    mFrame = src.mFrame;
    mPrimMode = src.mPrimMode;
    mHdRendererName = src.mHdRendererName;
    return *this;
}

void SOP_WalterNode::evaluateParms(Parms& parms, OP_Context& context)
{
    fpreal now = context.getTime();

    UT_String primPath;
    evalString(primPath, primPathName.getToken(), 0, now);

    parms.mFileName = evalSessionName(now);
    parms.mPrimPath = primPath.toStdString();
    parms.mFrame = evalFloat(frameName.getToken(), 0, now);
    parms.mPrimMode =
        static_cast<PrimLoadingMode>(evalInt(primModeName.getToken(), 0, now));

    UT_String hdName;
    evalString(hdName, hydraPluginName.getToken(), 0, now);
    parms.mHdRendererName = hdName.toStdString();
}

} // end namespace WalterHoudini