"""
Attribute Editor tamplate for walterStandin node. It's written on Python to be
able to use complex UI and PySide.
"""

# Copyright 2017 Rodeo FX. All rights reserved.

from walterWidgets.Qt import QtCore
from walterWidgets.Qt import QtGui
from walterWidgets.Qt import QtWidgets
from walterWidgets.Qt import shiboken

from pymel.core.uitypes import AETemplate
import maya.OpenMayaUI as OpenMayaUI
import maya.cmds as cmds
import pymel.core as pm
import os
import walterWidgets
from walterVariantsMenu import VariantButton
from walterPurposeMenu import RenderPurposeButton

WALTER_SCENE_CHANGED_CALLBACK = "WALTER_SCENE_CHANGED"
WALTER_SCENE_CHANGED_CALLBACK_OWNER = "WalterLayersModel"


def splitPaths(path):
    """Split one-string paths separated with a semicolon to the list of paths"""
    # It's already enough for linux. But in windows a regular path is
    # c:\Temp\walter, we need to detect the drive
    parts = path.split(':')

    paths = []

    drive = None

    for part in parts:
        if len(part) == 1:
            # It's a drive. Just save it.
            drive = part
        else:
            if drive:
                # Recontruct the path.
                paths.append(
                    '{drive}:{path}'.format(drive=drive, path=part))
            else:
                paths.append(part)

            # TODO: do we need a path with one letter only? Right now it's
            # filtered out.
            drive = None

    return paths


class WalterLayersModel(walterWidgets.LayersModel):
    """Data model for walterWidgets.LayersView."""

    def __init__(self, treeView):
        super(WalterLayersModel, self).__init__(treeView)
        self.__addSceneChangedCallback()
        self.dataChanged.connect(self.__onDataChanged)

    def __addSceneChangedCallback(self):
        callbacks = pm.callbacks(listCallbacks=True,
                                 hook=WALTER_SCENE_CHANGED_CALLBACK,
                                 owner=WALTER_SCENE_CHANGED_CALLBACK_OWNER)

        if callbacks:
            for c in callbacks:
                pm.callbacks(removeCallback=c,
                             hook=WALTER_SCENE_CHANGED_CALLBACK,
                             owner=WALTER_SCENE_CHANGED_CALLBACK_OWNER)

        pm.callbacks(addCallback=self.__updateModel,
                     hook=WALTER_SCENE_CHANGED_CALLBACK,
                     owner=WALTER_SCENE_CHANGED_CALLBACK_OWNER)

    def __updateModel(self, action, walterStandinName,
                      prevWalterStandinName=None):
        self.refresh()

    def __onDataChanged(self, topLeft, bottomRight):
        self.updateCurrentNode(attribute="cacheFileName")

    def refresh(self):
        """Read the node and recreate the tree."""
        if not self.currentNode or not cmds.objExists(self.currentNode):
            return

        self.beginResetModel()

        # TODO: Resolve the circular references between objects to be able to
        # delete all the objects in the structure.
        self.rootItem = walterWidgets.LayersItem(None, None, None, None)

        archives = cmds.getAttr('.'.join([self.currentNode, "cacheFileName"]))
        if archives:
            plugVis = '.'.join([self.currentNode, "visibilities"])
            plugRnd = '.'.join([self.currentNode, "renderables"])
            for i, archive in enumerate(splitPaths(archives)):
                vis = cmds.getAttr("%s[%i]" % (plugVis, i))
                rnd = cmds.getAttr("%s[%i]" % (plugRnd, i))
                walterWidgets.LayersItem(self.rootItem, archive, vis, rnd)
        else:
            # We need to keep at least one layer to fast load file
            walterWidgets.LayersItem(self.rootItem, "", 1, 1)

        self.endResetModel()

    def updateCurrentNode(self, attribute=None):
        """Put the layers to the Maya object."""
        # Update the Maya object
        if not attribute or attribute == "cacheFileName":
            archives = [i.getName() for i in self.rootItem.childItems]
            plug = '.'.join([self.currentNode, "cacheFileName"])
            cmds.setAttr(plug, ':'.join(archives), type="string")

        if not attribute or attribute == "visibilities":
            visibilities = [i.visible for i in self.rootItem.childItems]
            plug = '.'.join([self.currentNode, "visibilities"])
            for i, v in enumerate(visibilities):
                arrayPlug = "%s[%i]" % (plug, i)
                cmds.setAttr(arrayPlug, v)

        if not attribute or attribute == "renderables":
            renderables = [i.renderable for i in self.rootItem.childItems]
            plug = '.'.join([self.currentNode, "renderables"])
            for i, r in enumerate(renderables):
                arrayPlug = "%s[%i]" % (plug, i)
                cmds.setAttr(arrayPlug, r)

    def executeAction(self, action, index):
        """We are here because user pressed by one of the actions."""
        if not index.isValid():
            return

        if action == walterWidgets.LayersItem.ACTION_OPEN:
            startingDirectory = \
                os.path.dirname(index.data(QtCore.Qt.DisplayRole)) or None

            # File modes:
            # 0 Any file, whether it exists or not.
            # 1 A single existing file.
            # 2 The name of a directory. Both directories and files are
            #   displayed in the dialog.
            # 3 The name of a directory. Only directories are displayed in the
            #   dialog.
            # 4 Then names of one or more existing files.
            # Display Styles:
            # 1 On Windows or Mac OS X will use a native style file dialog.
            # 2 Use a custom file dialog with a style that is consistent across
            #   platforms.
            archive = cmds.fileDialog2(
                fileFilter=
                    "Alembic/USD Files (*.abc *.usd *.usda *.usdb *.usdc)",
                startingDirectory=startingDirectory,
                fileMode=1,
                dialogStyle=2)

            if not archive:
                return

            # Get filename
            archive = archive[0]

            # Set it to the item
            self.setData(index, archive)

            # Update the Maya object
            self.updateCurrentNode(attribute="cacheFileName")

        elif action == walterWidgets.LayersItem.ACTION_VISIBLE:
            item = index.internalPointer()
            item.visible = not item.visible

            # Update the Maya object
            self.updateCurrentNode(attribute="visibilities")

        elif action == walterWidgets.LayersItem.ACTION_RENDERABLE:
            item = index.internalPointer()
            item.renderable = not item.renderable

            # Update the Maya object
            self.updateCurrentNode(attribute="renderables")


class WalterToolbar(QtWidgets.QToolBar):

    def __init__(self, parent, nodeName):
        super(WalterToolbar, self).__init__(parent)
        self._nodeName = nodeName

        # Walter Panel button.
        actPanel = self.addAction(
            QtGui.QIcon(":/perspOutlinerLayout.png"),
            'Walter Panel',
            self.__openPanel)
        actPanel.setStatusTip('Display Walter Panel')

        self.addSeparator()
        self.variantButton = VariantButton(self)
        self.renderPurposeButton = RenderPurposeButton(self)

        # Geometry button. Controls the attribute useBBox.
        actGeo = self.addAction(QtGui.QIcon(":/mesh.svg"), 'Geometry')
        actGeo.setStatusTip('Load Geometry')
        actGeo.setCheckable(True)
        actGeo.toggled.connect(self.__onGeo)

        # Bounding Box button. Controls the attributes overrideLevelOfDetail
        # and overrideEnabled.
        actTextured = \
            self.addAction(QtGui.QIcon(":/Textured.png"), 'Textured')
        actTextured.setStatusTip('Enable Textures')
        actTextured.setCheckable(True)
        actTextured.toggled.connect(self.__onTextured)

        self.hydraPluginsCombo = QtWidgets.QComboBox(self)
        for shortName, displayName in AEwalterStandinTemplate.hydraPlugins():
            self.hydraPluginsCombo.addItem(displayName, shortName)

        # The size shouldn't be too big.
        self.hydraPluginsCombo.setFixedHeight(self.iconSize().height())
        self.hydraPluginsCombo.setMaximumWidth(self.iconSize().width() * 4)
        self.addWidget(self.hydraPluginsCombo)

        self.__onHydraPluginIndexChanged(0)
        self.hydraPluginsCombo.currentIndexChanged.connect(
            self.__onHydraPluginIndexChanged)

        self.variantMenuScriptJobId = None

    def __onHydraPluginIndexChanged(self, index):
        cmds.setAttr(
            '%s.hydraPlugin' % self._nodeName,
            self.hydraPluginsCombo.itemData(index),
            type="string")

    def __openPanel(self):
        """Callback that opens Walter Panel."""
        import walterPanel
        walterPanel.createPanel()

    def __onGeo(self, triggered):
        """Callback when user pressed Geometry button."""
        cmds.setAttr('%s.useBBox' % self._nodeName, 1 - triggered)

    def __onTextured(self, triggered):
        """Callback when user pressed Bounding Box button."""
        cmds.setAttr('%s.overrideTexturing' % self._nodeName, triggered)
        cmds.setAttr('%s.overrideEnabled' % self._nodeName, not triggered)

    def __resetMenus(self):
        self.variantButton.resetMenu(self._nodeName)
        self.renderPurposeButton.resetMenu(self._nodeName)

    def setNodeName(self, nodeName):
        self._nodeName = nodeName

    def replace(self, nodeName, node):
        self.setNodeName(nodeName)

        # RND-537: Callback to update the variants
        # menu when a path to a walter layer changed.
        if self.variantMenuScriptJobId is not None:
            cmds.scriptJob(kill=self.variantMenuScriptJobId)

        self.variantMenuScriptJobId = cmds.scriptJob(
            attributeChange=[
                node + '.cacheFileName',
                lambda s=self: s.__resetMenus()])

        for action in self.actions():
            text = action.text()

            if text == 'Geometry':
                geometry = 1 - cmds.getAttr('%s.useBBox' % node)

                oldState = action.blockSignals(True)
                action.setChecked(geometry)
                action.blockSignals(oldState)
            elif text == 'Textured':
                texturing = cmds.getAttr('%s.overrideTexturing' % node)
                override = cmds.getAttr('%s.overrideEnabled' % node)

                oldState = action.blockSignals(True)
                action.setChecked(texturing or not override)
                action.blockSignals(oldState)

        hydraPluginExists = False
        renderList = AEwalterStandinTemplate.hydraPlugins()
        hydraPlugin = cmds.getAttr('%s.hydraPlugin' % self._nodeName)

        if hydraPlugin:
            for i, (name, _) in enumerate(renderList):
                if hydraPlugin == name:
                    hydraPluginExists = True
                    if self.hydraPluginsCombo:
                        self.hydraPluginsCombo.setCurrentIndex(i)
                    break

        if not hydraPluginExists:
            if self.hydraPluginsCombo:
                oldState = self.hydraPluginsCombo.blockSignals(True)
                self.hydraPluginsCombo.setCurrentIndex(0)
                self.hydraPluginsCombo.blockSignals(oldState)

        self.__resetMenus()


class AEwalterStandinTemplate(AETemplate):
    """Attribute Editor template using python."""

    def __init__(self, nodeName):
        """Generate the layout."""
        super(AEwalterStandinTemplate, self).__init__(nodeName)

        # self.hydraPluginsCombo = None

        self.beginScrollLayout()

        # We use "message" attribute to have access to the AE, but we don't
        # create an actual control. It should be before everything to save the
        # current node name before the controls access it.
        self.callCustom(self.__build, self.__replace, "message")

        # The main layout.
        self.beginLayout("Walter", collapse=False)

        # Walter toolbar
        self.callCustom(self.__toolbarNew, self.__toolbarReplace, "dummy")

        # Walter Layers widget
        self.callCustom(self.__layersNew,
                        self.__layersReplace, "cacheFileName")

        self.endLayout()

        # Ability to add custom layouts from external plugins.
        cmds.callbacks(
            nodeName,
            executeCallbacks=True,
            hook="AETemplateCustomContent")

        # Put all the rest to the separate layout.
        self.addExtraControls()

        self.endScrollLayout()

    def __setNodeName(self, nodeName):
        """Set the name of the current node."""
        self._nodeName = nodeName
        currentWidget = self.__currentWidget()
        item = currentWidget.layout().itemAt(0)
        if item:
            toolbar = item.widget()
            if toolbar:
                toolbar.setNodeName(nodeName)

    def __build(self, plug):
        """Called to create some UI objects when a new node type is edited."""
        self.__replace(plug)

    def __replace(self, plug):
        """
        Called if an attribute editor already exists and another node of the
        same type is now to be edited.
        """
        self.__setNodeName(plug.split('.')[0])

    def __currentWidget(self, pySideType=QtWidgets.QWidget):
        """Cast and return the current widget."""
        # Get the current widget Maya name.
        currentWidgetName = cmds.setParent(query=True)
        # Convert it to C++ ptr.
        ptr = OpenMayaUI.MQtUtil.findLayout(currentWidgetName)
        # Convert it to PySide object.
        return shiboken.wrapInstance(long(ptr), pySideType)

    def __toolbarNew(self, plug):
        """Create the Panel button."""
        currentWidget = self.__currentWidget()
        toolbar = WalterToolbar(currentWidget, self._nodeName)
        currentWidget.layout().addWidget(toolbar)
        toolbar.setObjectName("abcToolbarWidget")

        self.__toolbarReplace(plug)

    def __toolbarReplace(self, plug):
        """Replace a Panel button."""
        currentWidget = self.__currentWidget()
        node, _ = plug.split('.')
        toolbar = currentWidget.layout().itemAt(0).widget()
        toolbar.replace(self._nodeName, node)

    def __layersNew(self, plug):
        """Create the layers widget."""
        currentWidget = self.__currentWidget()

        layers = walterWidgets.LayersView()
        model = WalterLayersModel(layers)
        layers.setModel(model)
        currentWidget.layout().addWidget(layers)
        layers.setObjectName("abcLayersWidget")

        self.__layersReplace(plug)

    def __layersReplace(self, plug):
        """Replace the layers widget to show another node."""
        currentWidget = self.__currentWidget()

        layers = currentWidget.layout().itemAt(0).widget()
        layers.setCurrentNode(plug.split('.')[0])

    @staticmethod
    def hydraPlugins():
        """Return all the hydra plugins."""
        # Get the list of the plugins. Now it's ['name', 'b', 'nameA', 'c'].
        hydraPlugins = cmds.walterStandin(hydraPlugins=True)
        # Convert it to [('name', 'b'), ('nameA', 'c')]
        return zip(hydraPlugins[::2], hydraPlugins[1::2])
