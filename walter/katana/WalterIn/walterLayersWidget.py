# Copyright 2017 Rodeo FX. All rights reserved.

from UI4.FormMaster import KatanaFactory
from UI4.Manifest import AssetBrowser
from UI4.Manifest import QtGui
from UI4.Manifest import QtCore
from UI4.Manifest import QT4FormWidgets
from UI4.Util import AssetWidgetDelegatePlugins
import walterWidgets


class WalterLayersModel(walterWidgets.LayersModel):
    """Data model for walterWidgets.LayersView."""

    def refresh(self):
        """Read the node and recreate the tree."""
        if not self.currentNode:
            return

        self.beginResetModel()

        # TODO: Resolve the circular references between objects to be able to
        # delete all the objects in the structure.
        self.rootItem = walterWidgets.LayersItem(None, None, None, None)

        archives = self.currentNode.getValue() or ""
        if archives:
            for i, archive in enumerate(archives.split(':')):
                if archive and archive[0] == '^':
                    visibility = False
                    archive = archive[1:]
                else:
                    visibility = True

                walterWidgets.LayersItem(self.rootItem, archive, visibility, 1)
        else:
            # We need to keep at least one layer to fast load file
            walterWidgets.LayersItem(self.rootItem, "", 1, 1)

        self.endResetModel()

    def updateCurrentNode(self, attribute=None):
        """Put the layers to the Maya object."""
        archives = [i.getName() for i in self.rootItem.childItems]
        visibilities = [i.visible for i in self.rootItem.childItems]

        archives = \
            [("" if vis else "^") + a for a, vis in zip(archives, visibilities)]

        self.currentNode.setValue(':'.join(archives))

    def executeAction(self, action, index):
        """We are here because user pressed by one of the actions."""
        if not index.isValid():
            return

        if action == walterWidgets.LayersItem.ACTION_OPEN:
            # Create a file open dialog
            dialog = AssetBrowser.Browser.BrowserDialog()
            dialog.setWindowTitle('Choose Alembic layer')
            dialog.setSaveMode(False)

            # Get Asset Widget
            pluginDelegate = \
                AssetWidgetDelegatePlugins.DefaultAssetWidgetDelegate(
                    self.currentNode)
            if pluginDelegate:
                pluginDelegate.configureAssetBrowser(dialog)

            # Show dialog
            if dialog.exec_() != QtGui.QDialog.Accepted:
                return

            # Get the file name
            archive = dialog.getResult()

            # Set it to the item
            self.setData(index, QtCore.QVariant(archive))

        elif action == walterWidgets.LayersItem.ACTION_VISIBLE:
            item = index.internalPointer()
            item.visible = not item.visible

        # Update the Maya object
        self.updateCurrentNode()


class WalterLayersWidget(QT4FormWidgets.BaseValueFormWidget):
    """The WalterLayersWidget offers a useful list view for a string policy."""

    def _buildControlWidget(self, layout):
        """Called when FormWidget needs a container for control widgets."""
        self.__control = walterWidgets.LayersView(self)

        model = WalterLayersModel(self.__control)
        self.__control.setModel(model)

        layout.addWidget(self.__control)

    def _updateControlWidget(self):
        """Update the control widget to the state of the policy."""
        policy = self.getValuePolicy()
        self.__control.setCurrentNode(policy)


KatanaFactory.RegisterPluginWidget("walterLayers", WalterLayersWidget)
