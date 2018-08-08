"""The Walter main panel."""

# Copyright 2017 Rodeo FX. All rights reserved.

from .walterOutliner import OutlinerTreeView
from walterWidgets.Qt import QtCore
from walterWidgets.Qt import QtGui
from walterWidgets.Qt import QtWidgets
from walterWidgets.Qt import shiboken
from walter import Walter
import maya.OpenMayaUI as OpenMayaUI
import maya.utils
import pymel.core as pm

PANEL_NAME = "Walter %s" % pm.walterStandin(query=True, version=True)
CALLBACK_OWNER = "walterPanel"
CALLBACK_SELECT = "walterPanelSelectionChanged"


class WalterWidget(QtWidgets.QFrame):
    """The Walter main panel."""

    def __init__(self, parent):
        """Called after the instance has been created."""
        super(WalterWidget, self).__init__(parent)

        # Add a red boredr if it's debug version. It allows visually detect the
        # wrong version.
        if "debug" in PANEL_NAME:
            widget_id = "panel_%s" % CALLBACK_OWNER
            self.setStyleSheet(
                "#%s { border: 3px solid rgb(68,34,34); }" % widget_id)

        layout = QtWidgets.QVBoxLayout(self)

        self.walterOutliner = OutlinerTreeView(traverser=Walter(), parent=self)
        layout.addWidget(self.walterOutliner)
        self.walterOutliner.itemSelected.connect(self.subSelectObjects)
        self.walterOutliner.expressionIsMatching.connect(
            self.onexpressionIsMatching)
        self.setLayout(layout)

        # Clear the previously created callbacks
        callbacks = pm.callbacks(
            listCallbacks=True,
            hook=CALLBACK_SELECT,
            owner=CALLBACK_OWNER)

        if callbacks:
            for c in callbacks:
                pm.callbacks(
                    removeCallback=c,
                    hook=CALLBACK_SELECT,
                    owner=CALLBACK_OWNER)

        # Create a new callback to have ability to celect items in the tree when
        # user selects sub-objects in the viewport
        pm.callbacks(
            addCallback=self.selectNode,
            hook=CALLBACK_SELECT,
            owner=CALLBACK_OWNER)

    def selectNode(self, obj, node):
        """
        Select an item is the tree view.

        Args:
            obj: The object that contains the item.
            node: The item to select.
        """
        # Prevent firing signals in Qt to avoid infinite loop.
        oldState = self.walterOutliner.blockSignals(True)

        # It's possible that we receive the full object name. We need to convert
        # it to the regular name.
        # TODO: It fails if obj doesn't exist.
        obj = pm.ls(obj)[0].name()

        self.walterOutliner.select(obj, node)

        self.walterOutliner.blockSignals(oldState)

    @QtCore.Slot()
    def doRefresh(self):
        """Refresh the UI."""
        self.walterOutliner.refresh()

    @QtCore.Slot(str, object)
    def subSelectObjects(self, node, paths):
        """
        Select specified sub-object.
        Args:
            node: The rdoCache node.
            paths: The list of sub-objects to select.
        """
        if paths:
            # Clear the selection first.
            pm.walterStandin(clearSubSelection=node)

            for path in sorted(paths):
                # Select each sub-object.
                pm.walterStandin(addSubSelection=(node, path))
            # Refresh the viewport
            pm.refresh()
        elif node:
            # Select object
            pm.select(node)

    @QtCore.Slot(str, object)
    def onexpressionIsMatching(self, index, path):
        """Check if the regex expression is valid."""
        item = index.internalPointer()
        item.isValidExpression = pm.walterStandin(
            expressionIsMatching=
                (item.getOriginObject(), path))
        self.walterOutliner.update(index)

def getDialogParent():
    """
    Get the Maya window.

    Returns:
        Maya window
    """
    # Find a parent for the dialog - this is the Maya mainWindow()
    ptr = OpenMayaUI.MQtUtil.mainWindow()
    parent = shiboken.wrapInstance(long(ptr), QtWidgets.QMainWindow)
    return parent


def dockPanel(panel):
    """
    Put the widget to the Maya's dock.

    Args:
        panel: the widget to put
    """
    # Retrieve the panel name.
    panel_name = panel.objectName()

    # Create a Maya panel name.
    maya_panel_name = "maya_%s" % panel_name

    # Create a new Maya window.
    maya_window = pm.window()

    # Add a layout to the Maya window.
    maya_layout = pm.formLayout(parent=maya_window)

    # Reparent the panel under the Maya window layout.
    pm.control(panel_name, edit=True, parent=maya_layout)

    attachForm = [
        (panel_name, 'top', 1),
        (panel_name, 'left', 1),
        (panel_name, 'bottom', 1),
        (panel_name, 'right', 1)]

    # Keep the panel sides aligned with the Maya window layout sides.
    pm.formLayout(
        maya_layout,
        edit=True,
        attachForm=attachForm)

    # Dock the Maya window into a new tab of Maya Channel Box dock area.
    pm.dockControl(
        maya_panel_name, area="left", content=maya_window, label=PANEL_NAME)

    # Once Maya will have completed its UI update and be idle,
    # raise (with "r=True") the new dock tab to the top.
    maya.utils.executeDeferred(
        "import maya.cmds as cmds\n"
        "cmds.dockControl('%s', edit=True, r=True)" % maya_panel_name)


def getPanel():
    """Gets the main panel."""
    # make a unique id for the app widget based off of the panel id
    widget_id = "panel_%s" % CALLBACK_OWNER

    if pm.control(widget_id, query=1, exists=1):
        # Find the panel widget for later use.
        for widget in QtWidgets.QApplication.allWidgets():
            if widget.objectName() == widget_id:
                return widget
    return None


def createPanel():
    """Create the main panel."""
    widget_instance = getPanel()

    if widget_instance:
        panel_name = widget_instance.objectName()
        maya_panel_name = "maya_%s" % panel_name
        pm.control(maya_panel_name,
                   edit=True,
                   visible=(not widget_instance.isVisible()))
        return

    # parent the UI to the main maya window
    parent = getDialogParent()
    widget_instance = WalterWidget(parent)
    widget_instance.setParent(parent)

    widget_id = "panel_%s" % CALLBACK_OWNER
    # set its name - this means that it can also be found via the maya API
    widget_instance.setObjectName(widget_id)

    # Dock the panel into a new Maya panel in the active Maya window.
    dockPanel(widget_instance)


def refreshPanel():
    """Refreshs the main panel."""
    widget_instance = getPanel()
    if widget_instance:
        widget_instance.doRefresh()


def hidePanel():
    """Refreshs the main panel."""
    widget_instance = getPanel()

    if widget_instance:
        panel_name = widget_instance.objectName()
        maya_panel_name = "maya_%s" % panel_name
        pm.control(
            maya_panel_name, edit=True, visible=False)
