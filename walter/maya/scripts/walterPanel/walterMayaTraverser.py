"""The implementation of Walter for Maya."""

# Copyright 2017 Rodeo FX. All rights reserved.

from .walterOutliner import ABC_PREFIX
from walter import OVERRIDE_ATTRIBUTE
from walter import OVERRIDE_DISPLACEMENT
from walter import OVERRIDE_SHADER
from walter import OVERRIDE_TRANSFORM
import maya.api.OpenMaya as om
import maya.mel as mel
import pymel.core as pm


# The option names that are used to store the data of custom panels in the file
# dialog. They are outside of the class because they are used to generate the
# static constant inside the class.
FILEDIALOGOPTIONEND = 'walterFileDialogOptionEndFrame'
FILEDIALOGOPTIONSTART = 'walterFileDialogOptionStartFrame'
FILEDIALOGOPTIONSTEP = 'walterFileDialogOptionStep'


class WalterMayaImplementation(object):
    """The traversal through the alembic tree structure."""

    modifier = om.MDGModifier()
    EXPRESSIONGROUPNAME = 'egn'
    EXPRESSIONNAME = 'en'
    EXPRESSIONWEIGHT = 'ew'
    NAME = "Walter Maya Traverser %s" % pm.walterStandin(q=True, v=True)

    FILEDIALOGOPTIONSMEL = """
    global proc walterFileDialogOptionsUISetup(string $parent)
    {{
        setParent $parent;
        $parent = `scrollLayout -childResizable true`;

        // generate some UI which will show up in the options tab of the Maya
        // file dialog
        columnLayout -adj true;
        intFieldGrp
            -label "Start/End"
            -numberOfFields 2
            walterFileDialogOptionFrames;
        floatFieldGrp -label "Step" walterFileDialogOptionStep;
    }}

    global proc walterFileDialogOptionsUIInitValues(
        string $parent, string $filterType)
    {{
        int $start = `playbackOptions -q -minTime`;
        int $end = `playbackOptions -q -maxTime`;
        intFieldGrp -e -value1 $start -value2 $end walterFileDialogOptionFrames;
        floatFieldGrp -e -value1 1.0 walterFileDialogOptionStep;
    }}

    global proc walterFileDialogOptionsUICommit(string $parent)
    {{
        optionVar
            -iv {start}
            `intFieldGrp -q -value1 walterFileDialogOptionFrames`;
        optionVar
            -iv {end}
            `intFieldGrp -q -value2 walterFileDialogOptionFrames`;
        optionVar
            -fv {step}
            `floatFieldGrp -q -value1 walterFileDialogOptionStep`;
    }}
    """.format(
        start=FILEDIALOGOPTIONSTART,
        end=FILEDIALOGOPTIONEND,
        step=FILEDIALOGOPTIONSTEP)

    def origins(self):
        """Return the list of all the origin items to display in the tree."""
        # TODO: cmds
        return [n.name() for n in pm.ls(type="walterStandin")]

    def dir(self, origin, path):
        """
        Return the list of child objects of the path.

        Args:
            origin: The origin object that contains the tree.
            path: The path.
        """
        objects = pm.walterStandin(da=(origin, path))

        # Exclude /materials from the list
        if path == "/":
            objects[:] = [o for o in objects if o != "/materials"]

        return objects

    def props(self, origin, path):
        """
        Return the list of properties of the path.

        Args:
            origin: The origin object that contains the tree.
            path: The path.
        """
        return pm.walterStandin(pa=(origin, path))

    def setOverride(self, origin, layer, path, material, overrideType):
        """
        Save the material from the Maya nodes. It's looking for the connections
        of the Walter Standin object and creates/replaces the connection to the
        material.

        Args:
            origin: The Walter Standin object that contains the tree.
            layer: Render layer to save the material.
            path: The Alembic path to the node.
            material: The material object
            overrideType: The type of the override. Example: shader/displacement
        """
        # Material
        materialNode = self.getDependNode(material)
        if not materialNode:
            om.MGlobal.displayError(
                "%s: Material %s doesn't exist." % (self.NAME, material))
            return

        materialMessage = materialNode.findPlug("message", False)

        # Get walterStandin.layersAssignation
        layersAssignation = self.getLayersAssignationPlug(origin)
        # Get walterStandin.layersAssignation[i].shaderConnections
        shadersPlug = self.findShadersPlug(layersAssignation, layer)

        # Create if not found
        if not shadersPlug:
            # Get walterStandin.layersAssignation[i]
            layersAssignationPlug = self.addElementToPlug(layersAssignation)
            # Get walterStandin.layersAssignation[i].layer
            layerPlug = self.getChildMPlug(layersAssignationPlug, 'layer')
            # Get walterStandin.layersAssignation[i].shaderConnections
            shadersPlug = self.getChildMPlug(
                layersAssignationPlug, 'shaderConnections')

            # Connect the layer to the layerPlug
            layerDepend = self.getDependNode(layer)
            layerMessage = layerDepend.findPlug("message", False)

            self.modifier.connect(layerMessage, layerPlug)
            self.modifier.doIt()

        firstAvailableCompound = None

        # Find if there is already an assignment with this path
        for j in range(shadersPlug.numElements()):
            # Get walterStandin.layersAssignation[i].shaderConnections[j]
            shadersCompound = shadersPlug.elementByPhysicalIndex(j)
            # Get walterStandin.layersAssignation[].shaderConnections[j].abcnode
            abcnodePlug = self.getChildMPlug(shadersCompound, 'abcnode')

            abcnode = abcnodePlug.asString()

            if not firstAvailableCompound and not abcnode:
                # Save it to reuse in future and avoid creating new elements
                firstAvailableCompound = shadersCompound

            if abcnode != path:
                continue

            # We are here because we found requested assignment. We need to
            # change the connection with the shader
            # Get walterStandin.layersAssignation[i].shaderConnections[j].shader
            shaderPlug = self.getChildMPlug(shadersCompound, overrideType)
            self.disconnectPlug(shaderPlug)
            self.modifier.connect(materialMessage, shaderPlug)
            self.modifier.doIt()

            return

        # We are here because were unable to find the path.
        # Add an element to walterStandin.layersAssignation[i].shaderConnections
        # or use available if found
        shadersCompound = \
            firstAvailableCompound or self.addElementToPlug(shadersPlug)

        # Get walterStandin.layersAssignation[i].shaderConnections[j].abcnode
        abcnodePlug = self.getChildMPlug(shadersCompound, 'abcnode')
        abcnodePlug.setString(path)

        # Get walterStandin.layersAssignation[i].shaderConnections[j].shader
        shaderPlug = self.getChildMPlug(shadersCompound, overrideType)
        self.disconnectPlug(shaderPlug)
        self.modifier.connect(materialMessage, shaderPlug)
        self.modifier.doIt()

    def detachOverride(self, origin, layer, path, overrideType, overrideName):
        """
        Detach override from the path. It clears the connections of the given
        Alembic path. It doesn't delete the actual array item because this
        operation is very expensive. setOverride() can reuse such item.

        Args:
            origin: The Walter Standin object that contains the tree.
            layer: Render layer to save the material.
            path: The Alembic path to the node.
            overrideType: The type of the override. Example: shader/displacement
        """
        # For transforms we need to querry a specific parameter that is not
        # related to the layers.
        if overrideType == OVERRIDE_TRANSFORM:
            pm.walterStandin(detachTransform=(origin, path))
            return

        # Get walterStandin.layersAssignation
        layersAssignation = self.getLayersAssignationPlug(origin)
        # Get walterStandin.layersAssignation[i].shaderConnections
        shadersPlug = self.findShadersPlug(layersAssignation, layer)

        if not shadersPlug:
            om.MGlobal.displayWarning(
                "%s: Can't find layer %s in the node %s connections." % (
                    self.NAME, layer, origin))
            return

        # Find if there is already an assignment with this path
        for j in range(shadersPlug.numElements()):
            # Get walterStandin.layersAssignation[i].shaderConnections[j]
            shadersCompound = shadersPlug.elementByPhysicalIndex(j)
            # Get walterStandin.layersAssignation[].shaderConnections[j].abcnode
            abcnodePlug = self.getChildMPlug(shadersCompound, 'abcnode')

            if not abcnodePlug or abcnodePlug.asString() != path:
                continue

            # Disconnect all
            shaderPlug = self.getChildMPlug(shadersCompound, overrideType)
            self.disconnectPlug(shaderPlug)

            # Delete the name to be able to reuse this array item in future. To
            # erase name we need to be sure that nothing is connected.
            needEraseName = True
            for i in range(shadersCompound.numChildren()):
                child = shadersCompound.child(i)
                connection = child.connectedTo(True, False)
                if connection:
                    needEraseName = False
                    break

            # Erase the name
            if needEraseName:
                abcnodePlug.setString("")

            break

        # Don't delete HyperShade nodes
        if overrideType != OVERRIDE_SHADER and \
           overrideType != OVERRIDE_DISPLACEMENT:
            node = self.getNode(overrideName)
            self.modifier.deleteNode(node)
            self.modifier.doIt()

    def overrides(self, origin, layer, overrideType):
        """Return override assignment structure for given origin."""
        materialAssignment = {}

        # Check if it's Alembic overrideType
        if overrideType.startswith(ABC_PREFIX):
            # Get the override from Alembic
            abcType = overrideType[len(ABC_PREFIX):]
            # Get the list of all the objects of the given type and layer.
            # ao is assignmentObjects.
            objects = pm.walterStandin(ao=(origin, layer, abcType)) or []
            for abcnode in objects:
                shader = pm.walterStandin(
                    assignment=(origin, abcnode, layer, abcType))

                if shader:
                    if shader in materialAssignment:
                        materialAssignment[shader].append(abcnode)
                    else:
                        materialAssignment[shader] = [abcnode]

            return materialAssignment

        # For transforms we need to querry a specific parameter that is not
        # related to the layers.
        if overrideType == OVERRIDE_TRANSFORM:
            dependNode = self.getDependNode(origin)

            # Get walterStandin.transforms
            transformsPlug = dependNode.findPlug("transforms", False)

            for i in range(transformsPlug.numElements()):
                element = transformsPlug.elementByPhysicalIndex(i)
                objectPlug = self.getChildMPlug(element, 'tro')
                matrixPlug = self.getChildMPlug(element, 'trm')

                subObjectName = objectPlug.asString()
                if not subObjectName:
                    continue

                # Get the connected transform name
                connection = matrixPlug.connectedTo(True, False)
                if not connection:
                    continue
                # TODO: This operation to get the node name can be expensive
                transform = om.MFnDependencyNode(connection[0].node()).name()

                if transform in materialAssignment:
                    materialAssignment[transform].append(subObjectName)
                else:
                    materialAssignment[transform] = [subObjectName]

            return materialAssignment

        # Get walterStandin.layersAssignation
        layersAssignation = self.getLayersAssignationPlug(origin)
        # Get walterStandin.layersAssignation[i].shaderConnections
        shadersPlug = self.findShadersPlug(layersAssignation, layer)

        if not shadersPlug:
            # This object doesn't have an assignation for specified layer
            return

        for j in range(shadersPlug.numElements()):
            # Get walterStandin.layersAssignation[i].shaderConnections[j]
            shadersCompound = shadersPlug.elementByPhysicalIndex(j)
            # Get walterStandin.layersAssignation[].shaderConnections[j].abcnode
            abcnodePlug = self.getChildMPlug(shadersCompound, 'abcnode')
            # Get walterStandin.layersAssignation[i].shaderConnections[j].shader
            shaderPlug = self.getChildMPlug(shadersCompound, overrideType)
            if not abcnodePlug or not shaderPlug:
                continue

            # Get the connected shader name
            connection = shaderPlug.connectedTo(True, False)
            if not connection:
                continue
            # TODO: This operation to get the node name can be expensive
            shader = om.MFnDependencyNode(connection[0].node()).name()
            abcnode = abcnodePlug.asString()

            if abcnode:
                if shader in materialAssignment:
                    materialAssignment[shader].append(abcnode)
                else:
                    materialAssignment[shader] = [abcnode]

        return materialAssignment

    def action(self, obj):
        """Perform some action with the given object."""
        if not obj or not pm.objExists(obj):
            # It happens when the object is in the Alembic file, not in the
            # scene.
            om.MGlobal.displayWarning(
                "%s: Can't select %s. This object is not in the Maya scene." % (
                    self.NAME, obj))
            return

        # TODO: Get rid of the node name. Select by connection.
        pm.select(obj)

    def layers(self, origin):
        """Return all the layers of the origin item."""
        # Walter Standin
        dependNode = self.getDependNode(origin)

        if not dependNode:
            om.MGlobal.displayError(
                "%s: Can't find object %s in the scene." % (self.NAME, origin))
            return

        # Get walterStandin.layersAssignation
        layersAssignation = dependNode.findPlug("layersAssignation", False)

        layers = []
        for i in range(layersAssignation.numElements()):
            # Get walterStandin.layersAssignation[i]
            currentLayerCompound = layersAssignation.elementByPhysicalIndex(i)
            # Get walterStandin.layersAssignation[i].layer
            layerPlug = self.getChildMPlug(currentLayerCompound, 'layer')
            if not layerPlug or layerPlug.isNull:
                continue

            # Get connections of walterStandin.layersAssignation[i].layer
            connection = layerPlug.connectedTo(True, False)
            if not connection:
                continue

            # Get the name of the first connected node. We consider we have only
            # one connection.
            # TODO: better node name
            depNode = om.MFnDependencyNode(connection[0].node())
            layers.append(depNode.name())

        # Get the layers from Alembic
        abcLayers = pm.walterStandin(assignmentLayers=origin) or []

        return list(set(layers + abcLayers))

    def currentLayer(self):
        """Return the currently editable layer."""
        return pm.editRenderLayerGlobals(query=True, currentRenderLayer=True)

    def produceOverrides(self, object):
        """Check what overrides it's possible to get from this object."""
        nodeType = pm.nodeType(object)
        if nodeType == 'displacementShader':
            return [(object, OVERRIDE_DISPLACEMENT)]
        elif nodeType == 'walterOverride':
            return [(object, OVERRIDE_ATTRIBUTE)]
        elif nodeType == 'shadingEngine':
            # Get the shaders of this shading group.
            result = []
            types = [
                ("surfaceShader", OVERRIDE_SHADER),
                ("displacementShader", OVERRIDE_DISPLACEMENT)]
            for a, t in types:
                child = pm.connectionInfo(
                    "%s.%s" % (object, a), sourceFromDestination=True)
                if not child:
                    continue

                result.append((child.split('.')[0], t))

            return result

        return [(object, OVERRIDE_SHADER)]

    def overrideType(self, object):
        """
        Deprecated. Use produceOverrides() instead.

        Check the type of the object and return which override we can use.
        """
        om.MGlobal.displayWarning(
                "%s: Walter.overrideType is deprecated. "
                "Use Walter.produceOverrides instead." % (
                    self.NAME))

        nodeType = pm.nodeType(object)
        if nodeType == 'displacementShader':
            return OVERRIDE_DISPLACEMENT
        elif nodeType == 'walterOverride':
            return OVERRIDE_ATTRIBUTE

        return OVERRIDE_SHADER

    def rename(self, origin, oldName, newName):
        """Rename object in all the layers."""
        # Walter Standin
        dependNode = self.getDependNode(origin)

        if not dependNode:
            om.MGlobal.displayError(
                "%s: Can't find object %s in the scene." % (self.NAME, origin))
            return

        # Get walterStandin.layersAssignation
        layersAssignation = dependNode.findPlug("layersAssignation", False)

        for i in range(layersAssignation.numElements()):
            # Get walterStandin.layersAssignation[i]
            currentLayerCompound = layersAssignation.elementByPhysicalIndex(i)

            # Iterate walterStandin.layersAssignation[i].shaderConnections
            shadersPlug = self.getChildMPlug(
                currentLayerCompound, 'shaderConnections')

            for j in range(shadersPlug.numElements()):
                # Get walterStandin.layersAssignation[i].shaderConnections[j]
                shadersCompound = shadersPlug.elementByPhysicalIndex(j)
                # walterStandin.layersAssignation[].shaderConnections[j].abcnode
                abcnodePlug = self.getChildMPlug(shadersCompound, 'abcnode')

                # Rename
                if abcnodePlug.asString() == oldName:
                    abcnodePlug.setString(newName)
                    # We consider we have only one unique node per layer
                    break

        # Rename expressions in the groups and rename the groups.
        for expressionsCompound in self.iterateExpressionGroupPlugs(origin):
            # Get walterStandin.expressions[i].expressionname
            expressionNamePlug = \
                self.getChildMPlug(expressionsCompound, self.EXPRESSIONNAME)

            if expressionNamePlug.asString() == oldName:
                expressionNamePlug.setString(newName)

            # Get walterStandin.expressions[i].expressiongroupname
            expressionGroupNamePlug = \
                self.getChildMPlug(
                    expressionsCompound,
                    self.EXPRESSIONGROUPNAME)

            if expressionGroupNamePlug.asString() == oldName:
                expressionGroupNamePlug.setString(newName)

    def remove(self, origin, path):
        """Removed object in all the layers."""
        # Walter Standin
        dependNode = self.getDependNode(origin)

        if not dependNode:
            om.MGlobal.displayError(
                "%s: Can't find object %s in the scene." % (self.NAME, origin))
            return

        # Get walterStandin.layersAssignation
        layersAssignation = dependNode.findPlug("layersAssignation", False)

        for i in range(layersAssignation.numElements()):
            # Get walterStandin.layersAssignation[i]
            currentLayerCompound = layersAssignation.elementByPhysicalIndex(i)

            # Iterate walterStandin.layersAssignation[i].shaderConnections
            shadersPlug = self.getChildMPlug(
                currentLayerCompound, 'shaderConnections')

            for j in range(shadersPlug.numElements()):
                # Get walterStandin.layersAssignation[i].shaderConnections[j]
                shadersCompound = shadersPlug.elementByPhysicalIndex(j)
                # walterStandin.layersAssignation[].shaderConnections[j].abcnode
                abcnodePlug = self.getChildMPlug(shadersCompound, 'abcnode')

                # Remove
                if abcnodePlug.asString() == path:
                    # We consider we have only one unique node per layer
                    abcnodePlug.setString("")

                    # TODO: Using setString in this loop should be faster
                    for i in range(shadersCompound.numChildren()):
                        child = shadersCompound.child(i)
                        self.disconnectPlug(child)

                    break

        # Remove from the groups. Since we can't really remove it, we just
        # delete the fields and reuse them in the future.
        for expressionsCompound in self.iterateExpressionGroupPlugs(origin):
            # Get walterStandin.expressions[i].expressionname
            expressionNamePlug = \
                self.getChildMPlug(expressionsCompound, self.EXPRESSIONNAME)

            # Get walterStandin.expressions[i].expressiongroupname
            expressionGroupNamePlug = \
                self.getChildMPlug(
                    expressionsCompound, self.EXPRESSIONGROUPNAME)

            expressionName = expressionNamePlug.asString()
            if expressionNamePlug.asString() == path:
                expressionNamePlug.setString("")
                expressionGroupNamePlug.setString("")
                break

            expressionGroupName = expressionGroupNamePlug.asString()
            if expressionGroupName == path:
                # Recursively remove the expression. We need it to disconnect it
                # properly from all the attributes.
                self.remove(origin, expressionName)

    def create(self, overrideType):
        """Create an override object and return its name."""
        if overrideType == OVERRIDE_ATTRIBUTE:
            # pm.createNode doesn't work for unknown reason. It creates a node
            # but it's not immidiatley available.
            obj = self.modifier.createNode("walterOverride")
            self.modifier.doIt()
            return om.MFnDependencyNode(obj).name()

    def saveAssignment(self, origin, fileName=None):
        """Save the shader assignment to external file."""
        fileName = self.getFileDialog(fileName)

        if not fileName:
            return

        return pm.walterStandin(saveAssignment=(origin, fileName))

    def saveAttributes(self, origin, fileName=None):
        """Save the walterOverride nodes to the external file."""
        fileName = self.getFileDialog(fileName)

        if not fileName:
            return

        return pm.walterStandin(saveAttributes=(origin, fileName))

    def saveMaterials(self, origin, fileName=None):
        """Save the materials to the external file."""
        fileName = self.getFileDialog(fileName)

        if not fileName:
            return

        if not pm.pluginInfo('walterMtoaConnection', query=True, loaded=True):
            pm.loadPlugin('walterMtoaConnection', quiet=True)

        return pm.walterSaveShaders(node=origin, file=fileName)

    def saveTransforms(self, origin, fileName=None):
        """Save the shader assignment to external file."""
        fileName, start, end, step = self.getFileDialog(
            fileName, options=True, fileType="USD (*.usd *.usda)")

        if not fileName:
            return

        # Form the arguments of walterStandin. We form it as a dict bacause if
        # the time doesn't exist we shouldn't specify it as an argument.
        kwargs = {'saveTransforms': (origin, fileName)}
        if start is not None:
            kwargs['startTime'] = start
        if end is not None:
            kwargs['endTime'] = end
        if step is not None:
            kwargs['simulationRate'] = step

        return pm.walterStandin(**kwargs)

    def savePurposes(self, origin, fileName=None):
        """Save the shader assignment to external file."""
        fileName = self.getFileDialog(
            fileName, options=False, fileType="USD (*.usd *.usda)")

        if not fileName:
            return

        return pm.walterStandin(savePurposes=(origin, fileName))

    def saveVariantsLayer(self, origin, fileName=None):
        """Save the shader assignment to external file."""
        fileName = self.getFileDialog(
            fileName, options=False, fileType="USD (*.usd *.usda)")

        if not fileName:
            return

        return pm.walterStandin(saveVariantsLayer=(origin, fileName))

    def setGroup(self, origin, path, group):
        """Set the group of the given expression."""
        # Walter Standin
        dependNode = self.getDependNode(origin)

        if not dependNode:
            om.MGlobal.displayError(
                "%s: Can't find object %s in the scene." % (self.NAME, origin))
            return

        firstAvailableCompound = None

        # Get walterStandin.expressions
        expressionsPlug = dependNode.findPlug("expressions", False)

        for i in range(expressionsPlug.numElements()):
            # Get walterStandin.expressions[i]
            expressionsCompound = expressionsPlug.elementByPhysicalIndex(i)

            # Get walterStandin.expressions[i].expressionname
            expressionNamePlug = \
                self.getChildMPlug(expressionsCompound, self.EXPRESSIONNAME)

            # Get walterStandin.expressions[i].expressiongroupname
            expressionGroupNamePlug = \
                self.getChildMPlug(
                    expressionsCompound,
                    self.EXPRESSIONGROUPNAME)

            expressionName = expressionNamePlug.asString()

            # Save it on the case we need to create a new one
            if not firstAvailableCompound:
                if not expressionName:
                    firstAvailableCompound = expressionsCompound

            if path != expressionName:
                continue

            # We are here because we found requested expression. We need to
            # change the group name
            expressionGroupNamePlug.setString(group)

            return

        # We are here because were unable to find the requested expression. We
        # need to add a new item or reuse an empty one.
        expressionsCompound = \
            firstAvailableCompound or self.addElementToPlug(expressionsPlug)
        # Get walterStandin.expressions[i].expressionname
        expressionNamePlug = \
            self.getChildMPlug(expressionsCompound, self.EXPRESSIONNAME)
        # Get walterStandin.expressions[i].expressiongroupname
        expressionGroupNamePlug = \
            self.getChildMPlug(expressionsCompound, self.EXPRESSIONGROUPNAME)
        expressionNamePlug.setString(path)
        expressionGroupNamePlug.setString(group)

    def expressionGroups(self, origin):
        """Get all available expression groups."""
        alembicGroups = pm.walterStandin(alembicAllGroups=origin)
        if alembicGroups:
            groupSet = set(alembicGroups)
        else:
            groupSet = set()

        for exp, grp, _ in self.iterateExpressionGroups(origin):
            if exp and grp:
                groupSet.add(grp)

        return sorted(groupSet)

    def expressions(self, origin, group=None):
        """Get all the expressions of the specified group."""
        # The list of expressions of the specified group coming from the DCC.
        standinExpressions = set()
        # The list of expressions of other groups
        otherExpressions = set()
        for exp, grp, _ in self.iterateExpressionGroups(origin):
            if exp and group in [None, grp]:
                standinExpressions.add(exp)
            else:
                otherExpressions.add(exp)

        # The list of expressions coming from file, for example alembic
        if group is None:
            alembicExpressions = \
                pm.walterStandin(alembicExpressions=origin)
        else:
            alembicExpressions = \
                pm.walterStandin(alembicGroupExpressions=(origin, group))

        if alembicExpressions:
            # If the Alembic's expression is in another group, we shouldn't
            # display it because the expressions of the standin override
            # everything.
            alembicExpressions = set(alembicExpressions) - otherExpressions
        else:
            alembicExpressions = set()
        expr = standinExpressions | alembicExpressions
        # TODO: Optimize the sorting in the method.
        # Return a sorted list of expressions according to weight key value.
        # This method has a large cost of time complexity starting O(n log n) and up.
        # expressionWeight calls iterateExpressionGroups which is also called in this method.
        return sorted(expr, key=lambda x: self.expressionWeight(origin, x))

    def expressionWeight(self, origin, expression):
        """Returns the attribute weight stored which represent the row index order of the expression."""
        for exprWeight in self.iterateExpressionGroups(origin):
            if exprWeight[0] == expression:
                return exprWeight[-1]

    def setExpressionWeight(self, origin, expression, weight):
        """Stores the weight of an expression."""
        # Get origin's expressions
        dependNode = self.getDependNode(origin)
        expressionsPlug = dependNode.findPlug("expressions", False)

        for i in range(expressionsPlug.numElements()):
            # loop through expressions => set expression and name
            expressionsCompound = expressionsPlug.elementByPhysicalIndex(i)
            expressionNamePlug = self.getChildMPlug(expressionsCompound, self.EXPRESSIONNAME)
            # Test if it's the one we are querying, if so, set weight
            if expressionNamePlug.asString() == expression:
                expressionWeightPlug = self.getChildMPlug(expressionsCompound, self.EXPRESSIONWEIGHT)
                expressionWeightPlug.setInt(weight)
                break

    # Routine utilities.
    def getNode(self, nodeName):
        """Looking for MFnDependencyNode object in the scene."""
        selectionList = om.MSelectionList()
        try:
            selectionList.add(nodeName)
        except:
            return
        obj = selectionList.getDependNode(0)
        return obj

    def getDependNode(self, nodeName):
        """Looking for MFnDependencyNode object in the scene."""
        return om.MFnDependencyNode(self.getNode(nodeName))

    def getChildMPlug(self, plug, plugName):
        """Search child plug with given name in the given plug's children."""
        for i in range(plug.numChildren()):
            child = plug.child(i)
            # MPlug.partialName returns "la[0].layer"
            if child.partialName().split('.')[-1] == plugName:
                return child

    def getLayersAssignationPlug(self, nodeName):
        """Return MPlug node.layersAssignation."""
        # Walter Standin
        dependNode = self.getDependNode(nodeName)

        if not dependNode:
            return

        # Get walterStandin.layersAssignation
        return dependNode.findPlug("layersAssignation", False)

    def findShadersPlug(self, layersAssignation, layerName):
        """
        Return MPlug node.layersAssignation[i].shaderConnections for requested
        layer.
        """
        # Requested render layer
        layerDepend = self.getDependNode(layerName)
        if not layerDepend:
            return

        # Get MObject of the requested render layer to be able to comare it with
        # connected layers
        layerObject = layerDepend.object()

        requestedAssignation = None
        for i in range(layersAssignation.numElements()):
            # Get walterStandin.layersAssignation[i]
            currentLayerCompound = layersAssignation.elementByPhysicalIndex(i)
            # Get walterStandin.layersAssignation[i].layer
            layerPlug = self.getChildMPlug(currentLayerCompound, 'layer')
            if not layerPlug or layerPlug.isNull:
                continue

            connection = layerPlug.connectedTo(True, False)
            if not connection:
                continue

            # Compare the saved MObject with the first connected node. We
            # consider we have only one connection.
            if layerObject == connection[0].node():
                # Save walterStandin.layersAssignation[i]
                requestedAssignation = currentLayerCompound
                break

        if not requestedAssignation:
            return

        # Get walterStandin.layersAssignation[i].shaderConnections
        return self.getChildMPlug(requestedAssignation, 'shaderConnections')

    def addElementToPlug(self, plug):
        """Maya routine to add a new element to the compound plug array."""
        # MPlug.setNumElements works only if MPlug is not allocated. Maya, why
        # is it so compicated?
        data = plug.asMDataHandle()
        arrayData = om.MArrayDataHandle(data)
        builder = arrayData.builder()
        builder.addLastArray()
        arrayData.set(builder)
        plug.setMDataHandle(data)
        return plug.elementByPhysicalIndex(plug.numElements() - 1)

    def disconnectPlug(self, plug):
        """Maya routine to disconnect every source from the destination plug."""
        connections = plug.connectedTo(True, False)
        if connections:
            for c in connections:
                self.modifier.disconnect(c, plug)

            self.modifier.doIt()

    def iterateExpressionGroupPlugs(self, origin):
        """Iterate the expressions plug of the origin."""
        # Walter Standin
        dependNode = self.getDependNode(origin)

        if not dependNode:
            om.MGlobal.displayError(
                "%s: Can't find object %s in the scene." % (self.NAME, origin))
            return

        # Get walterStandin.expressions
        expressionsPlug = dependNode.findPlug("expressions", False)

        for i in range(expressionsPlug.numElements()):
            # Get walterStandin.expressions[i]
            yield expressionsPlug.elementByPhysicalIndex(i)

    def iterateExpressionGroups(self, origin):
        """Iterate the expression names and groups of the origin."""
        for expressionsCompound in self.iterateExpressionGroupPlugs(origin):
            # Get walterStandin.expressions[i].expressionname
            expressionName = \
                self.getChildMPlug(expressionsCompound, self.EXPRESSIONNAME)

            # Get walterStandin.expressions[i].expressiongroupname
            expressionGroupName = \
                self.getChildMPlug(
                    expressionsCompound, self.EXPRESSIONGROUPNAME)

            # Get WalterStandin.expressions[i].expressionWeight
            expressionWeight = \
                self.getChildMPlug(
                    expressionsCompound, self.EXPRESSIONWEIGHT)

            yield (expressionName.asString(), expressionGroupName.asString(), expressionWeight.asInt())

    def getFileDialog(
            self, fileName, options=False, fileType="Alembic Archives (*.abc)"):
        # Maya uses options to pass additional data from the panels of the
        # file dialogs. We need to clean the data to be sure that this data
        # didn't come from the prefious file dialog.
        pm.optionVar(remove=FILEDIALOGOPTIONSTART)
        pm.optionVar(remove=FILEDIALOGOPTIONEND)
        pm.optionVar(remove=FILEDIALOGOPTIONSTEP)

        # 0 - Any file, whether it exists or not.
        # 2 - Use a custom file dialog with a style that is consistent across
        #     platforms.
        if not fileName:
            kwargs = {
                "fileFilter": fileType,
                "fileMode": 0,
                "dialogStyle": 2}

            if options:
                # fileDialog2 supports options only with MEL. So we implemented
                # options in MEL.
                mel.eval(self.FILEDIALOGOPTIONSMEL)

                kwargs.update({
                    "optionsUIInit": "walterFileDialogOptionsUIInitValues",
                    "optionsUICreate": "walterFileDialogOptionsUISetup",
                    "optionsUICommit": "walterFileDialogOptionsUICommit"})

            fileName = pm.fileDialog2(**kwargs)

        # Maya returns a list
        if fileName and isinstance(fileName, list):
            fileName = fileName[0]

        if options:
            # Set default values if these options don't exist
            if pm.optionVar(exists=FILEDIALOGOPTIONSTART):
                start = pm.optionVar(q=FILEDIALOGOPTIONSTART)
            else:
                start = None

            if pm.optionVar(exists=FILEDIALOGOPTIONEND):
                end = pm.optionVar(q=FILEDIALOGOPTIONEND)
            else:
                end = None

            if pm.optionVar(exists=FILEDIALOGOPTIONSTEP):
                step = pm.optionVar(q=FILEDIALOGOPTIONSTEP)
            else:
                step = None

            return (fileName, start, end, step)

        return fileName

    def exposeTransforms(self, origin, alembicObject):
        """
        Create the locations objects for the object and its parents in the maya
        scene and connect them to the walter standin so the transforms of
        created locations will be passed to the USD session layer.

        :param str origin: The stand-in object.
        :param str alembicObject: Sub-object to connect the created locator.
        """
        result = pm.walterStandin(ext=(origin, alembicObject)) or []
        return dict(zip(result[0::2], result[1::2]))

    def invertFrozen(self, origin):
        """Inverse the freeze flag of the origin node."""
        frozen = self.getFrozen(origin)
        attribute = "{origin}.frozenTransforms".format(origin=origin)
        pm.setAttr(attribute, 1 - frozen)

    def getFrozen(self, origin):
        """Get the freeze flag of the origin node."""
        if not origin or not pm.objExists(origin):
            # It happens when the object is in the Alembic file, not in the
            # scene.
            om.MGlobal.displayWarning(
                "%s: object %s is not in the scene." % (self.NAME, origin))
            return 0

        attribute = "{origin}.frozenTransforms".format(origin=origin)
        return pm.getAttr(attribute)

    def refresh(self, origin):
        """Clear the USD playback cache."""
        pm.walterStandin(setCachedFramesDirty=origin)

    def setPreviewColorAttrib(self, shader, color):
        # TODO: it's very slow. OpenMaya is more preferable
        attrName = 'rdo_walter_previewColor'
        if not pm.attributeQuery(attrName, node=shader, exists=True):
            pm.addAttr(shader, longName=attrName, dt='float3')

        pm.setAttr(shader + '.' + attrName, color, type='float3')

    def getPreviewColorAttrib(self, shader):
        # TODO: it's very slow. OpenMaya is more preferable
        attrName = 'rdo_walter_previewColor'
        if not pm.attributeQuery(attrName, node=shader, exists=True):
            return None
        return pm.getAttr(shader + '.' + attrName)
