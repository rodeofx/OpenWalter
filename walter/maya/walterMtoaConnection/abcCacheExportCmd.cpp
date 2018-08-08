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


#include "mayaUtils.h"
#include "abcCacheExportCmd.h"
#include "abcExporterUtils.h"
#include "mtoaScene.h"
#include "rdoArnold.h"
#include "mtoaVersion.h"

#include <Alembic/AbcCoreLayer/Util.h>

#include <boost/algorithm/string.hpp>
#include <unordered_set>

namespace Abc =  Alembic::Abc;
namespace Mat = Alembic::AbcMaterial;

typedef std::pair<MPlug, std::string> TargetPlug;

// MPlug hasher to be able to create an unordered_set of MPlugs.
struct TargetPlugHash {
    size_t operator()(const TargetPlug& targetPlug) const
    {
        std::hash<std::string> hasher;
        return hasher(targetPlug.second + targetPlug.first.name().asChar());
    }
};

MStatus abcCacheExportCmd::doIt( const MArgList &args)
{
  MArgDatabase argData( syntax(), args);

  if(!argData.isFlagSet( "-f"))
  {
      MGlobal::displayError("no output file!");
      return MStatus::kFailure;

  }
  MString filename("");
  argData.getFlagArgument( "-f", 0, filename);

  // Used to mark that an Alembic object or property are meant to be replaced
  // when reading via AbcCoreLayer. Replacing an object or compound property
  // will also replace all of the children encountered and shading network so
  // far.
  Alembic::Abc::MetaData replace;
  Alembic::AbcCoreLayer::SetReplace(replace, true);

  Abc::OArchive archive(Alembic::AbcCoreOgawa::WriteArchive(), filename.asChar() );
  Abc::OObject root(archive, Abc::kTop);
  Abc::OObject materials(root, "materials");

    MSelectionList list;
    if(argData.isFlagSet( "-sl"))
    {
        MGlobal::getActiveSelectionList (list);
    }
    else if (argData.isFlagSet( "-node"))
    {
        MString node("");
        argData.getFlagArgument( "-node", 0, node);
        if (list.add(node) != MS::kSuccess)
        {
            MString warn = node;
            warn += " could not be select, skipping.";
            MGlobal::displayWarning(warn);
            return MStatus::kFailure;
        }
    }
    else
    {
         MItDependencyNodes nodeIt;
         for (; !nodeIt.isDone(); nodeIt.next())
         {
            MObject node = nodeIt.item();
            if (node.isNull())
                continue;
            list.add (node);
         }
    }

    MTOAScene scene;

    // Set because we shouldn't export materials twice.
    std::unordered_set<TargetPlug, TargetPlugHash> shaderToExport;

    static const char* exportKeys[][2] = {
        {"shader", "surface"},
        {"displacement", "displacement"} };

    MItSelectionList iter(list, MFn::kPluginShape);
     for (; !iter.isDone(); iter.next())
     {
         MObject dependNode;
         iter.getDependNode(dependNode);
         MFnDagNode dagNode(dependNode);

         if (dagNode.typeId() == WALTER_MAYA_SHAPE_ID)
         {
             // TODO: Implement a walter iterator to iterate this stuff.
             const MPlug layersAssignation =
                 dagNode.findPlug("layersAssignation");
             if (layersAssignation.isNull())
             {
                 continue;
             }

             if (layersAssignation.numElements() <= 0)
             {
                 continue;
             }

             for (unsigned i=0; i<layersAssignation.numElements(); i++)
             {
                 // Get walterStandin.layersAssignation[i]
                 const MPlug currentLayerCompound =
                     layersAssignation.elementByPhysicalIndex(i);
                 // Get walterStandin.layersAssignation[i].layer
                 MPlug layerPlug;
                 if (!GetChildMPlug(currentLayerCompound, "layer", layerPlug))
                 {
                     continue;
                 }

                 MPlugArray connections;
                 if (!layerPlug.connectedTo(connections, true, false) ||
                     !connections.length())
                 {
                     continue;
                 }

                 // The layer is the first connected node. We consider we have
                 // only one connection.
                 const MFnDependencyNode layer(connections[0].node());
                 const std::string layerName = layer.name().asChar();

                 MPlug shadersPlug;
                 if (!GetChildMPlug(
                             currentLayerCompound,
                             "shaderConnections",
                             shadersPlug))
                 {
                     continue;
                 }

                 for (unsigned j=0; j<shadersPlug.numElements(); j++)
                 {
                     // Get
                     //     walterStandin.
                     //     layersAssignation[i].
                     //     shaderConnections[j]
                     const MPlug shadersCompound =
                         shadersPlug.elementByPhysicalIndex(j);
                     // Get
                     //     walterStandin.
                     //     layersAssignation[].
                     //     shaderConnections[].
                     //     abcnode
                     MPlug abcnodePlug;
                     if (!GetChildMPlug(
                                 shadersCompound, "abcnode", abcnodePlug))
                     {
                         continue;
                     }

                     const MString abcnode = abcnodePlug.asString().asChar();
                     if (!abcnode.length()) {
                         continue;
                     }

                     BOOST_FOREACH (auto key, exportKeys) {
                         // Get
                         //     walterStandin.
                         //     layersAssignation[].
                         //     shaderConnections[].
                         //     shader
                         MPlug shaderPlug;
                         if (GetChildMPlug(shadersCompound, key[0], shaderPlug))
                         {
                             // Get the connected shader name
                             MPlugArray connections;
                             if (!shaderPlug.connectedTo(connections, 1, 0) ||
                                 !connections.length())
                             {
                                 continue;
                             }

                             shaderToExport.insert(
                                     TargetPlug(connections[0], key[1]));
                         }
                     }
                 }
             }
         }
         else
         {
             MPlug shaders = dagNode.findPlug("shaders");

             for (unsigned int i=0;i<shaders.numElements();++i)
             {
                 MPlug plug = shaders.elementByPhysicalIndex(i);
                 MPlugArray connections;
                 plug.connectedTo(connections, true, false);
                 for (unsigned int k=0; k<connections.length(); ++k)
                 {
                     MPlug sgPlug = connections[k];
                     if (sgPlug.node().apiType() == MFn::kShadingEngine ||
                         sgPlug.node().apiType() == MFn::kDisplacementShader)
                     {
                         shaderToExport.insert(TargetPlug(sgPlug, "surface"));
                     }
                 }
             }
         }


        BOOST_FOREACH(const TargetPlug& toExport, shaderToExport)
        {
            // TODO: WTF is this? It looks like it's a set that contains all the
            // nodes that are in the Alembic container. But it's filled, then
            // it's iterated, then filled, then iterated again. Thus, they
            // output nodes to alembic in the middle of filling exportedNodes.
            // And when they iterate the second time, Alembic crashes because
            // Alembic already has some nodes stored. It's a mess, and I don't
            // refactor it right now just because I have a fear to break it
            // just before my vacation. A couple notes how to refactor it
            // properly.  We don't need two nested loops. It would be better to
            // pull the loop "for(;sit!=send;++sit)" out from "BOOST_FOREACH".
            // Also, detecting the root node is wrong, there can be problems and
            // in some case the material can look differently.
            AtNodeSet* exportedNodes = new AtNodeSet;

            // Ugly fix. We keep the exported nodes here because they output
            // nodes to alembic in the middle of filling exportedNodes. So we
            // need an alternative set to don't mix everything up.
            AtNodeSet nodesInContainer;

            // create the material
            MFnDependencyNode container(toExport.first.node());

            AiMsgInfo("[EXPORT] Creating container : %s", container.name().asChar());
            AiMsgTab(+2);
            // TODO: crashes when container.name() is already exported
            Mat::OMaterial matObj(
                    materials, container.name().asChar(), replace);

            // Export nodes to Arnold using MTOA
            AtNodeSet roots;
            CNodeTranslator* translator = scene.exportNode(toExport.first, &roots);
#if WALTER_MTOA_VERSION >= 10400
            // exportNode() from the latest MTOA doesn't fill roots. The only
            // way to get the node is using the translator.
            AtNode* root = translator->GetArnoldNode();
#else
            BOOST_FOREACH(AtNode* root, roots)
#endif
             {
                 exportedNodes->insert(root);
                 // We need to traverse the tree again...
                 getAllArnoldNodes(root, exportedNodes);

                 AtNodeSet::const_iterator sit (exportedNodes->begin()), send(exportedNodes->end());
                 for(;sit!=send;++sit)
                 {
                     // adding the node to the network
                     MString nodeName(container.name());
                     nodeName = nodeName + ":" + MString(AiNodeGetName(*sit));

                     nodeName = MString(boost::replace_all_copy(boost::replace_all_copy(std::string(nodeName.asChar()), ".message", ""), ".", "_").c_str());


                     if (!nodesInContainer.insert(*sit).second)
                     {
                         // See comment near definition of nodesInContainer.
                         // With this we are isolating the Alembic crash and
                         // leave everything else unchanged.
                         continue;
                     }

                     AiMsgInfo("[EXPORTING %s] Added node : %s", container.name().asChar(), nodeName.asChar());
                     matObj.getSchema().addNetworkNode(nodeName.asChar(), "arnold", AiNodeEntryGetName(AiNodeGetNodeEntry(*sit)));

                     if(root == *sit)
                     {
                         AiMsgInfo("[EXPORTING %s] Root node is : %s", container.name().asChar(), nodeName.asChar());
                         //TODO : get if it's a volume, eventually
                        matObj.getSchema().setNetworkTerminal(
                        "arnold",
                        toExport.second,
                        nodeName.asChar(),
                        "out");
                     }

                     //export parameters


                     AtParamIterator* nodeParam = AiNodeEntryGetParamIterator(AiNodeGetNodeEntry(*sit));
                     int outputType = AiNodeEntryGetOutputType(AiNodeGetNodeEntry(*sit));

                     while (!AiParamIteratorFinished(nodeParam))
                     {
                         const AtParamEntry *paramEntry = AiParamIteratorGetNext(nodeParam);
                         const char* paramName = AiParamGetName(paramEntry);

                         if(AiParamGetType(paramEntry) == AI_TYPE_ARRAY)
                         {

                            AtArray* paramArray = AiNodeGetArray(*sit, paramName);

                            processArrayValues(*sit, paramName, paramArray, outputType, matObj, nodeName, container.name());
                            for(unsigned int i=0; i < RdoAiArrayGetNumElements(paramArray); i++)
                            {
                                processArrayParam(*sit, paramName, paramArray, i, outputType, matObj, nodeName, container.name());
                            }


                         }
                         else
                         {
                            processLinkedParam(*sit, AiParamGetType(paramEntry), outputType, matObj, nodeName, paramName, container.name(), true);
                         }
                     }
                     AiParamIteratorDestroy(nodeParam);
                 }
            }

            AiMsgTab(-2);

            delete exportedNodes;
        }

    }
    AiMsgInfo("[EXPORT] Success!");
    return MStatus::kSuccess;
}


//  There is never anything to undo.
//////////////////////////////////////////////////////////////////////
MStatus abcCacheExportCmd::undoIt(){
  return MStatus::kSuccess;
}

//
//  There is never really anything to redo.
//////////////////////////////////////////////////////////////////////
MStatus abcCacheExportCmd::redoIt(){
  return MStatus::kSuccess;
}

//
//
//////////////////////////////////////////////////////////////////////
bool abcCacheExportCmd::isUndoable() const{
  return false;
}

//
//
//////////////////////////////////////////////////////////////////////
bool abcCacheExportCmd::hasSyntax() const {
  return true;
}

//
//
//////////////////////////////////////////////////////////////////////
MSyntax abcCacheExportCmd::mySyntax() {
  MSyntax syntax;

  syntax.addFlag( "-sl", "-selection", MSyntax::kString);
  syntax.addFlag( "-f", "-file", MSyntax::kString);
  syntax.addFlag( "-n", "-node", MSyntax::kString);

  return syntax;
}

//
//
//////////////////////////////////////////////////////////////////////
bool abcCacheExportCmd::isHistoryOn() const {
  //what is this supposed to do?
  return false;
}

MString abcCacheExportCmd::commandString() const {
  return MString();
}


MStatus abcCacheExportCmd::setHistoryOn( bool state ){
  //ignore it for now
  return MStatus::kSuccess;
}

MStatus abcCacheExportCmd::setCommandString( const MString &str) {
  //ignore it for now
  return MStatus::kSuccess;
}
