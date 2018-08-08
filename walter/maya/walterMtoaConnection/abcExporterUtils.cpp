/*Abc Shader Exporter
Copyright (c) 2014, Gaël Honorez, All rights reserved.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3.0 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with this library.*/

#include "abcExporterUtils.h"
#include "rdoArnold.h"

#include <boost/algorithm/string.hpp>


namespace // <anonymous>
{
const MStringArray INVALID_COMPONENTS;
const char* rgbComp[3] = {"r", "g", "b"};
const MStringArray RGB_COMPONENTS(rgbComp, 3);
const char* rgbaComp[4] = {"r", "g", "b", "a"};
const MStringArray RGBA_COMPONENTS(rgbaComp, 4);
const char* point2Comp[2] = {"x", "y"};
const MStringArray POINT2_COMPONENTS(point2Comp, 2);
const char* vectorComp[3] = {"x", "y", "z"};
const MStringArray VECTOR_COMPONENTS(vectorComp, 3);
}




const MStringArray& GetComponentNames(int arnoldParamType)
{
   MStringArray componentNames;
   switch (arnoldParamType)
   {
   case AI_TYPE_RGB:
      return RGB_COMPONENTS;
   case AI_TYPE_RGBA:
      return RGBA_COMPONENTS;
   
   #if AI_VERSION_ARCH_NUM==4
   case AI_TYPE_VECTOR:
   #endif
   case RDO_AI_TYPE_VECTOR:
      return VECTOR_COMPONENTS;
   case RDO_AI_TYPE_VECTOR2:
      return POINT2_COMPONENTS;
   default:
      return INVALID_COMPONENTS;
   }
}

AtNode* renameAndCloneNodeByParent(AtNode* node, AtNode* parent)
{
    MString newName = MString(AiNodeGetName(parent)) + MString("|") + MString(AiNodeGetName(node));
    AtNode* existingNode = AiNodeLookUpByName(newName.asChar());

    if(existingNode == NULL)
    {
        AtNode* copy = AiNodeClone(node);
        AiNodeSetStr(copy, "name", newName.asChar());
        AiMsgDebug("renaming %s to %s", AiNodeGetName(node), AiNodeGetName(copy));
        return copy;
    }
    else
    {
        AiMsgDebug("%s already exists", AiNodeGetName(existingNode));
        return existingNode;
    }
}

bool relink(AtNode* src, AtNode* dest, const char* input, int comp)
{
    if(comp == -1)
        return AiNodeLinkOutput(src, "", dest, input);
    else
    {
        int outputType = AiNodeEntryGetType(AiNodeGetNodeEntry (src));
        if(outputType == AI_TYPE_RGB || outputType == AI_TYPE_RGBA)
        {
            if(comp == 0)
                return AiNodeLinkOutput(src, "r", dest, input);
            else if(comp == 1)
                return AiNodeLinkOutput(src, "g", dest, input);
            else if(comp == 2)
                return AiNodeLinkOutput(src, "b", dest, input);
            else if(comp == 3)
                return AiNodeLinkOutput(src, "a", dest, input);
        }
        else if(outputType == AI_TYPE_VECTOR ||
                outputType == RDO_AI_TYPE_VECTOR ||
                outputType == RDO_AI_TYPE_VECTOR2)
        {
            if(comp == 0)
                return AiNodeLinkOutput(src, "x", dest, input);
            else if(comp == 1)
                return AiNodeLinkOutput(src, "y", dest, input);
            else if(comp == 2)
                return AiNodeLinkOutput(src, "z", dest, input);
        }

    }
    return false;
}

void getAllArnoldNodes(AtNode* node, AtNodeSet* nodes)
{
    AtParamIterator* iter = AiNodeEntryGetParamIterator(AiNodeGetNodeEntry(node));
    while (!AiParamIteratorFinished(iter))
    {
        const AtParamEntry *pentry = AiParamIteratorGetNext(iter);
        const char* paramName = AiParamGetName(pentry);


        int inputType = AiParamGetType(pentry);
        if(inputType == AI_TYPE_ARRAY)
        {
            AtArray* paramArray = AiNodeGetArray(node, paramName);
            if(RdoAiArrayGetType(paramArray) == AI_TYPE_NODE ||
               RdoAiArrayGetType(paramArray) == AI_TYPE_POINTER)
            {
                for(unsigned int i=0; i < RdoAiArrayGetNumElements(paramArray); i++)
                {
                    AtNode* linkedNode = (AtNode*)AiArrayGetPtr(paramArray, i);
                    if(linkedNode != NULL)
                    {
                        //AiMsgDebug("we have a link to %s for %s", AiNodeGetName(linkedNode), AiNodeGetName(node));
                        //linkedNode = renameAndCloneNodeByParent(linkedNode, node);
                        //AiArraySetPtr(paramArray, i, linkedNode);
                        nodes->insert(linkedNode);
                        getAllArnoldNodes(linkedNode, nodes);
                    }
                }

            }
            else
            {
                if(AiNodeIsLinked(node, paramName))
                {
                    int comp;
                    for(unsigned int i=0; i < RdoAiArrayGetNumElements(paramArray); i++)
                    {

                        MString paramNameArray = MString(paramName) + "[" + MString(to_string(i).c_str()) +"]";
                        if (AiNodeIsLinked(node, paramNameArray.asChar()))
                        {
                            //AiMsgDebug("%s has a link", paramNameArray.asChar());
                            AtNode* linkedNode = NULL;
                            linkedNode = AiNodeGetLink(node, paramNameArray.asChar(), &comp);
                            if(linkedNode)
                            {
                                //AiMsgDebug("we have a link to %s on %s", AiNodeGetName(linkedNode), paramNameArray.asChar());
                                //linkedNode = renameAndCloneNodeByParent(linkedNode, node);
                                //relink(linkedNode, node, paramNameArray.asChar(), comp);
                                nodes->insert(linkedNode);
                                getAllArnoldNodes(linkedNode, nodes);
                            }
                            else
                            {
                                const MStringArray componentNames = GetComponentNames(inputType);

                                unsigned int numComponents = componentNames.length();
                                for (unsigned int i=0; i < numComponents; i++)
                                {
                                    MString compAttrName = paramNameArray + "." + componentNames[i];

                                    if(AiNodeIsLinked(node, compAttrName.asChar()))
                                    {
                                        linkedNode = AiNodeGetLink(node, compAttrName.asChar(), &comp);
                                        if(linkedNode)
                                        {
                                            //linkedNode = renameAndCloneNodeByParent(linkedNode, node);
                                            //relink(linkedNode, node, compAttrName.asChar(), comp);
                                            nodes->insert(linkedNode);
                                            getAllArnoldNodes(linkedNode, nodes);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        else if(AiNodeIsLinked(node, paramName))
        {

            {
                AtNode* linkedNode = NULL;
                int comp;
                linkedNode = AiNodeGetLink(node, paramName, &comp);
                if(linkedNode)
                {
                    // CRASHING HERE
                    //linkedNode = renameAndCloneNodeByParent(linkedNode, node);
                    //relink(linkedNode, node, paramName, comp);
                    nodes->insert(linkedNode);
                    getAllArnoldNodes(linkedNode, nodes);
                }
                else
                {
                    const MStringArray componentNames = GetComponentNames(inputType);

                    unsigned int numComponents = componentNames.length();
                    for (unsigned int i=0; i < numComponents; i++)
                    {
                        MString compAttrName = MString(paramName) + "." + componentNames[i];

                        if(AiNodeIsLinked(node, compAttrName.asChar()))
                        {
                            linkedNode = AiNodeGetLink(node, compAttrName.asChar(), &comp);
                            if(linkedNode)
                            {
                                //linkedNode = renameAndCloneNodeByParent(linkedNode, node);
                                //relink(linkedNode, node, compAttrName.asChar(), comp);
                                nodes->insert(linkedNode);
                                getAllArnoldNodes(linkedNode, nodes);
                            }
                        }
                    }
                }
            }
        }
    }

    AiParamIteratorDestroy(iter);

}


void processArrayValues(AtNode* sit, const char *paramName, AtArray* paramArray, int outputType, Mat::OMaterial matObj, MString nodeName, MString containerName)
{
    int typeArray = RdoAiArrayGetType(paramArray);
    if (typeArray == AI_TYPE_INT ||  typeArray == AI_TYPE_ENUM)
    {
        //type int
        Abc::OInt32ArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        std::vector<int> vals;
        for(unsigned int i=0; i < RdoAiArrayGetNumElements(paramArray); i++)
            vals.push_back(AiArrayGetInt(paramArray, i));

        prop.set(vals);

    }
    else if (typeArray == AI_TYPE_UINT)
    {
        //type int
        Abc::OUInt32ArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        std::vector<unsigned int> vals;
        for(unsigned int i=0; i < RdoAiArrayGetNumElements(paramArray); i++)
            vals.push_back(AiArrayGetUInt(paramArray, i));

        prop.set(vals);

    }
    else if (typeArray == AI_TYPE_BYTE)
    {
        Alembic::AbcCoreAbstract::MetaData md;
        md.set("type", boost::lexical_cast<std::string>("byte"));
        //type int
        Abc::OUInt32ArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName, md);
        std::vector<unsigned int> vals;
        for(unsigned int i=0; i < RdoAiArrayGetNumElements(paramArray); i++)
            vals.push_back(AiArrayGetByte(paramArray, i));

        prop.set(vals);

    }
    else if (typeArray == AI_TYPE_FLOAT)
    {
        // type float
        Abc::OFloatArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        std::vector<float> vals;
        for(unsigned int i=0; i < RdoAiArrayGetNumElements(paramArray); i++)
            vals.push_back(AiArrayGetFlt(paramArray, i));
        prop.set(vals);

    }
    else if (typeArray == AI_TYPE_BOOLEAN)
    {
        // type bool
        Abc::OBoolArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        std::vector<Abc::bool_t> vals;
        for(unsigned int i=0; i < RdoAiArrayGetNumElements(paramArray); i++)
            vals.push_back(AiArrayGetBool(paramArray, i));

        prop.set(vals);

    }
    else if (typeArray == AI_TYPE_RGB)
    {
        // type rgb
        Abc::OC3fArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        std::vector<Imath::C3f> vals;
        for(unsigned int i=0; i < RdoAiArrayGetNumElements(paramArray); i++)
        {
            RdoAtRGB a_val = AiArrayGetRGB(paramArray, i);
            Imath::C3f color_val( a_val.r, a_val.g, a_val.b );
            vals.push_back(color_val);
        }
        prop.set(vals);
    }
    else if (typeArray == AI_TYPE_RGBA)
    {
        // type rgba
        Abc::OC4fArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        std::vector<Imath::C4f> vals;
        for(unsigned int i=0; i < RdoAiArrayGetNumElements(paramArray); i++)
        {
            AtRGBA a_val = AiArrayGetRGBA(paramArray, i);
            Imath::C4f color_val( a_val.r, a_val.g, a_val.b, a_val.a );
            vals.push_back(color_val);
        }
        prop.set(vals);
    }
    else if (typeArray == RDO_AI_TYPE_VECTOR2)
    {
        // type point2
        Abc::OP2fArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        std::vector<Imath::V2f> vals;
        for(unsigned int i=0; i < RdoAiArrayGetNumElements(paramArray); i++)
        {
            RdoAtVector2 a_val = RdoAiArrayGetVec2(paramArray, i);
            Imath::V2f vec_val( a_val.x, a_val.y );
            vals.push_back(vec_val);
        }
        prop.set(vals);
    }


    else if (typeArray == RDO_AI_TYPE_VECTOR)
    {
        // type point
        Abc::OP3fArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        std::vector<Imath::V3f> vals;
        for(unsigned int i=0; i < RdoAiArrayGetNumElements(paramArray); i++)
        {
            RdoAtVector a_val = RdoAiArrayGetVec(paramArray, i);
            Imath::V3f vec_val( a_val.x, a_val.y, a_val.z );
            vals.push_back(vec_val);
        }
        prop.set(vals);
    }

    #if AI_VERSION_ARCH_NUM==4
    else if (typeArray == AI_TYPE_VECTOR)
    {
        Abc::OV3fArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        std::vector<Imath::V3f> vals;
        for(unsigned int i=0; i < RdoAiArrayGetNumElements(paramArray); i++)
        {
            AtVector a_val = AiArrayGetVec(paramArray, i);
            Imath::V3f vec_val( a_val.x, a_val.y, a_val.z );
            vals.push_back(vec_val);
        }
        prop.set(vals);
    }
    #endif

    else if (typeArray == AI_TYPE_STRING)
    {
        // type string
        Abc::OStringArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        std::vector<std::string> vals;
        for(unsigned int i=0; i < RdoAiArrayGetNumElements(paramArray); i++)
            vals.push_back(RdoAiArrayGetStr(paramArray, i));
        prop.set(vals);
    }

    cout << "exported " << RdoAiArrayGetNumElements(paramArray) << " array values of type " << typeArray << " for " << nodeName.asChar() << "." << paramName << endl;
}



void processArrayParam(AtNode* sit, const char *paramName, AtArray* paramArray, int index, int outputType, Mat::OMaterial matObj, MString nodeName, MString containerName)
{
    //first, test if the entry is linked
    //Build the paramname...

    int typeArray = RdoAiArrayGetType(paramArray);

    if(typeArray == AI_TYPE_NODE || typeArray == AI_TYPE_POINTER)
    {

        for(unsigned int i=0; i < RdoAiArrayGetNumElements(paramArray); i++)
        {
            AtNode* linkedNode = (AtNode*)AiArrayGetPtr(paramArray, i);
            if(linkedNode != NULL)
            {
                MString nodeNameLinked(containerName);
                nodeNameLinked = nodeNameLinked + ":" + MString(AiNodeGetName(linkedNode));

                MString paramNameArray = MString(paramName) + "[" + MString(to_string(i).c_str()) +"]";

                AiMsgDebug("%s.%s is linked", nodeName.asChar(), paramName);
                AiMsgDebug("Exporting link from %s.%s to %s.%s", nodeNameLinked.asChar(), paramNameArray.asChar(), nodeName.asChar(), paramName);
                matObj.getSchema().setNetworkNodeConnection(nodeName.asChar(), paramNameArray.asChar(), nodeNameLinked.asChar(), "");

            }
        }

    }
    else
    {
        MString paramNameArray = MString(paramName) + "[" + MString(to_string(index).c_str()) +"]";
        if (AiNodeGetLink(sit, paramNameArray.asChar()))
            processLinkedParam(sit, typeArray, outputType, matObj, nodeName, paramNameArray.asChar(), containerName);
    }
}

void processLinkedParam(AtNode* sit, int inputType, int outputType,  Mat::OMaterial matObj, MString nodeName, const char* paramName, MString containerName, bool interfacing)
{
    AiMsgTab (+2);
    if (AiNodeIsLinked(sit, paramName))
    {
        // check what is linked exactly

        if(AiNodeGetLink(sit, paramName))
        {
            AiMsgDebug("%s.%s is linked", nodeName.asChar(), paramName);
            exportLink(sit, matObj, nodeName, paramName, containerName);
        }
        else
        {
            const MStringArray componentNames = GetComponentNames(inputType);
            unsigned int numComponents = componentNames.length();
            for (unsigned int i=0; i < numComponents; i++)
            {
                MString compAttrName = MString(paramName) + "." + componentNames[i];
                if(AiNodeIsLinked(sit, compAttrName.asChar()))
                {
                    //cout << "exporting link : " << nodeName << "." << compAttrName.asChar() << endl;
                    exportLink(sit, matObj, nodeName, compAttrName.asChar(), containerName);

                }
            }

        }



    }
    else
        exportParameter(sit, matObj, inputType, nodeName, paramName, interfacing);

    AiMsgTab (-2);
}

void exportLink(AtNode* sit, Mat::OMaterial matObj, MString nodeName, const char* paramName, MString containerName)
{
    AiMsgTab (+2);
    int comp;
    AiMsgDebug("Checking link %s.%s", nodeName.asChar(), paramName);

    AtNode* linked = AiNodeGetLink(sit, paramName, &comp);
    int outputType = AiNodeEntryGetOutputType(AiNodeGetNodeEntry(linked));

    MString nodeNameLinked(containerName);

    nodeNameLinked = nodeNameLinked + ":" + MString(AiNodeGetName(linked));

    //We need to replace the "." stuff from the name as we does it from the exporter.
    nodeNameLinked = MString(boost::replace_all_copy(boost::replace_all_copy(std::string(nodeNameLinked.asChar()), ".message", ""), ".", "_").c_str());

    std::string outPlug;

    if(comp == -1)
        outPlug = "";
    else
    {
        if(outputType == AI_TYPE_RGB || outputType == AI_TYPE_RGBA)
        {
            if(comp == 0)
                outPlug = "r";
            else if(comp == 1)
                outPlug = "g";
            else if(comp == 2)
                outPlug = "b";
            else if(comp == 3)
                outPlug = "a";
        }
        else if(outputType == AI_TYPE_VECTOR ||
                outputType == RDO_AI_TYPE_VECTOR ||
                outputType == RDO_AI_TYPE_VECTOR2)
        {
            if(comp == 0)
                outPlug = "x";
            else if(comp == 1)
                outPlug = "y";
            else if(comp == 2)
                outPlug = "z";
        }

    }
    AiMsgDebug("Exporting link from %s.%s to %s.%s", nodeNameLinked.asChar(), outPlug.c_str(), nodeName.asChar(), paramName);
    matObj.getSchema().setNetworkNodeConnection(nodeName.asChar(), paramName, nodeNameLinked.asChar(), outPlug);

    AiMsgTab (-2);
}


void exportParameterFromArray(AtNode* sit, Mat::OMaterial matObj, AtArray* paramArray, int index, MString nodeName, const char* paramName)
{
    int type = RdoAiArrayGetType(paramArray);

    cout << "Array type " << type << " for " << nodeName.asChar() << "." << paramName << endl;
    if (type == AI_TYPE_INT || type == AI_TYPE_ENUM )
    {
        //type int
        Abc::OInt32Property prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        prop.set(AiArrayGetInt(paramArray, index));
    }
    else if (type == AI_TYPE_UINT || type == AI_TYPE_ENUM)
    {
        //type int
        Abc::OUInt32Property prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        prop.set(AiArrayGetUInt(paramArray, index));
    }
    else if (type == AI_TYPE_BYTE)
    {
        //type int
        Alembic::AbcCoreAbstract::MetaData md;
        md.set("type", boost::lexical_cast<std::string>("byte"));
        Abc::OUInt32Property prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName, md);
        prop.set(AiArrayGetByte(paramArray, index));
    }
    else if (type == AI_TYPE_FLOAT)
    {
        // type float
        Abc::OFloatProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        prop.set(AiArrayGetFlt(paramArray, index));

    }
    else if (type == AI_TYPE_BOOLEAN)
    {
        // type bool
        Abc::OBoolProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        prop.set(AiArrayGetBool(paramArray, index));
    }
    else if (type == AI_TYPE_RGB)
    {
        // type rgb
        Abc::OC3fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        RdoAtRGB a_val = AiArrayGetRGB(paramArray, index);
        Imath::C3f color_val( a_val.r, a_val.g, a_val.b );
        prop.set(color_val);
        cout << "exporting RGB value of " <<  a_val.r << " " << a_val.g << " " << a_val.b << " for " << nodeName.asChar() <<"."<<paramName << endl;
    }
    else if (type == AI_TYPE_RGBA)
    {
        // type rgba
        Abc::OC4fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        AtRGBA a_val = AiArrayGetRGBA(paramArray, index);
        Imath::C4f color_val( a_val.r, a_val.g, a_val.b, a_val.a );
        prop.set(color_val);
        cout << "exporting RGBA value of " <<  a_val.r << " " << a_val.g << " " << a_val.b << " " << a_val.a << " for " << nodeName.asChar() <<"."<<paramName << endl;
    }
    else if (type == RDO_AI_TYPE_VECTOR2)
    {
        // type point2
        Abc::OP2fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        RdoAtVector2 a_val = RdoAiArrayGetVec2(paramArray, index);
        Imath::V2f vec_val( a_val.x, a_val.y );
        prop.set(vec_val);
    }
    else if (type == RDO_AI_TYPE_VECTOR)
    {
        // type point
        Abc::OP3fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        RdoAtVector a_val = RdoAiArrayGetVec(paramArray, index);
        Imath::V3f vec_val( a_val.x, a_val.y, a_val.z );
        prop.set(vec_val);
    }

    #if AI_VERSION_ARCH_NUM==4
    else if (type == AI_TYPE_VECTOR)
    {
        // type vector
        Abc::OV3fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        AtVector a_val = AiArrayGetVec(paramArray, index);
        Imath::V3f vec_val( a_val.x, a_val.y, a_val.z );
        prop.set(vec_val);
    }
    #endif

    else if (type == AI_TYPE_STRING)
    {
        // type string
        Abc::OStringProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        prop.set(RdoAiArrayGetStr(paramArray, index));
    }

}


bool isDefaultValue(const AtNode* node, const char* paramName)
{
    const AtParamEntry* pentry = AiNodeEntryLookUpParameter (AiNodeGetNodeEntry(node), paramName);
    if(pentry == NULL)
        return false;

    const AtParamValue * pdefaultvalue = AiParamGetDefault (pentry);

    switch(AiParamGetType(pentry))
    {
        case AI_TYPE_BYTE:
            if (AiNodeGetByte(node, paramName) == RdoBYTE(pdefaultvalue))
                return true;
            break;
        case AI_TYPE_INT:
            if (AiNodeGetInt(node, paramName) == RdoINT(pdefaultvalue))
                return true;
            break;
        case AI_TYPE_UINT:
            if (AiNodeGetUInt(node, paramName) == RdoUINT(pdefaultvalue))
                return true;
            break;
        case AI_TYPE_BOOLEAN:
            if (AiNodeGetBool (node, paramName) == RdoBOOL(pdefaultvalue))
                return true;
            break;
        case AI_TYPE_FLOAT:
            if (AiNodeGetFlt (node, paramName) == RdoFLT(pdefaultvalue))
                return true;
            break;
        case AI_TYPE_RGB:
            if (AiNodeGetRGB (node, paramName) == RdoRGB(pdefaultvalue))
                return true;
            break;
        case AI_TYPE_RGBA:
            if (AiNodeGetRGBA (node, paramName) == RdoRGBA(pdefaultvalue))
                return true;
            break;
        #if AI_VERSION_ARCH_NUM==4
        case AI_TYPE_VECTOR:
            if (AiNodeGetVec (node, paramName) == pdefaultvalue->VEC)
                return true;
            break;
        #endif
        case RDO_AI_TYPE_VECTOR:
            if (RdoAiNodeGetVec (node, paramName) == RdoVEC(pdefaultvalue))
                return true;
            break;
        case RDO_AI_TYPE_VECTOR2:
            if (RdoAiNodeGetVec2 (node, paramName) == RdoVEC2(pdefaultvalue))
                return true;
            break;
        case AI_TYPE_STRING:
            if (strcmp(AiNodeGetStr(node, paramName), RdoSTR(pdefaultvalue)) == 0)
                return true;
            break;
        default:
            return false;
    }


    return false;
}

void exportParameter(AtNode* sit, Mat::OMaterial matObj, int type, MString nodeName, const char* paramName, bool interfacing)
{
    //header->setMetaData(md);
    if (type == AI_TYPE_INT || type == AI_TYPE_ENUM)
    {
        //type int
        Abc::OInt32Property prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        prop.set(AiNodeGetInt(sit, paramName));
    }
    else if (type == AI_TYPE_UINT)
    {
        //type int
        Abc::OUInt32Property prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        prop.set(AiNodeGetUInt(sit, paramName));

    }
    else if (type == AI_TYPE_BYTE)
    {
        Alembic::AbcCoreAbstract::MetaData md;
        md.set("type", boost::lexical_cast<std::string>("byte"));

        //type Byte
        Abc::OUInt32Property prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName, md);
        prop.set(AiNodeGetByte(sit, paramName));

    }
    else if (type == AI_TYPE_FLOAT)
    {
        // type float

        Alembic::AbcCoreAbstract::MetaData md;
        const AtNodeEntry* arnoldNodeEntry = AiNodeGetNodeEntry(sit);
        float val;
        bool success = AiMetaDataGetFlt(arnoldNodeEntry, paramName, "min", &val);
        if(success)
            md.set("min", boost::lexical_cast<std::string>(val));

        success = AiMetaDataGetFlt(arnoldNodeEntry, paramName, "max", &val);
        if(success)
            md.set("max", boost::lexical_cast<std::string>(val));

        success = AiMetaDataGetFlt(arnoldNodeEntry, paramName, "softmin", &val);
        if(success)
            md.set("softmin", boost::lexical_cast<std::string>(val));

        success = AiMetaDataGetFlt(arnoldNodeEntry, paramName, "softmax", &val);
        if(success)
            md.set("softmax", boost::lexical_cast<std::string>(val));

        Abc::OFloatProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName, md);
        prop.set(AiNodeGetFlt(sit, paramName));

    }
    else if (type == AI_TYPE_BOOLEAN)
    {
        // type bool
        Abc::OBoolProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        prop.set(AiNodeGetBool(sit, paramName));
    }
    else if (type == AI_TYPE_RGB)
    {
        // type rgb
        Abc::OC3fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        RdoAtRGB a_val = AiNodeGetRGB(sit, paramName);
        Imath::C3f color_val( a_val.r, a_val.g, a_val.b );
        prop.set(color_val);
        //cout << "exporting RGB value of " <<  a_val.r << " " << a_val.g << " " << a_val.b << " for " << nodeName.asChar() <<"."<<paramName << endl;
    }
    else if (type == AI_TYPE_RGBA)
    {
        // type rgb
        Abc::OC4fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        AtRGBA a_val = AiNodeGetRGBA(sit, paramName);
        Imath::C4f color_val( a_val.r, a_val.g, a_val.b, a_val.a );
        prop.set(color_val);
        //cout << "exporting RGB value of " <<  a_val.r << " " << a_val.g << " " << a_val.b << " for " << nodeName.asChar() <<"."<<paramName << endl;
    }
    else if (type == RDO_AI_TYPE_VECTOR2)
    {
        // type point
        Abc::OP2fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        RdoAtVector2 a_val = RdoAiNodeGetVec2(sit, paramName);
        Imath::V2f vec_val( a_val.x, a_val.y);
        prop.set(vec_val);
    }
    else if (type == AI_TYPE_VECTOR)
    {
        // type point
        Abc::OP3fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        RdoAtVector a_val = RdoAiNodeGetVec(sit, paramName);
        Imath::V3f vec_val( a_val.x, a_val.y, a_val.z );
        prop.set(vec_val);
    }
    #if AI_VERSION_ARCH_NUM==4
    else if (type == AI_TYPE_VECTOR)
    {
        // type vector
        Abc::OV3fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        AtVector a_val = AiNodeGetVec(sit, paramName);
        Imath::V3f vec_val( a_val.x, a_val.y, a_val.z );
        prop.set(vec_val);
    }
    #endif
    else if (type == AI_TYPE_STRING)
    {
        // type string
        Abc::OStringProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        prop.set(RdoAiNodeGetStr(sit, paramName));
    }
    else if (type == AI_TYPE_MATRIX)
    {
        // type matrix
        Abc::OM44fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName);
        AtMatrix a_val = AiNodeGetMatrix(sit, paramName);
        Imath::M44f mat_val(a_val.data);
        prop.set(mat_val);
    }

    if(interfacing && strcmp(paramName,"name") != 0 && isDefaultValue(sit, paramName) == false)
        if (type == AI_TYPE_BYTE || type == AI_TYPE_INT || type == AI_TYPE_UINT || type == AI_TYPE_ENUM || type == AI_TYPE_FLOAT || type == AI_TYPE_BOOLEAN || type == AI_TYPE_RGB ||type == AI_TYPE_STRING)
        {

            MString interfaceName = MString(AiNodeGetName(sit)) + ":" + MString(paramName);
            matObj.getSchema().setNetworkInterfaceParameterMapping(interfaceName.asChar(), nodeName.asChar(), paramName);
        }

}
