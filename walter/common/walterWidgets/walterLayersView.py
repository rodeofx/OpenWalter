# Copyright 2017 Rodeo FX. All rights reserved.

from .Qt import QtCore
from .Qt import QtGui
from .Qt import QtWidgets
from .utils import toPyObject
from .walterBaseTreeView import BaseItem
from .walterBaseTreeView import BaseModel
from .walterBaseTreeView import BaseTreeView
import os


ICONS_PATH = os.path.join(os.path.dirname(os.path.realpath(__file__)), "icons")


class LayersView(BaseTreeView):
    """The main view, used to show the Alembic files."""

    def __init__(self, parent=None):
        """Called after the instance has been created."""
        super(LayersView, self).__init__(parent)

        model = LayersModel(self)
        self.setModel(model)

        # Context Menu
        self.contextMenu = QtWidgets.QMenu(self)
        self.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self.showContextMenu)

        # Accept drop events
        self.setAcceptDrops(True)
        self.setDragDropMode(QtWidgets.QAbstractItemView.DragDrop)
        self.setDragEnabled(True)
        self.setDropIndicatorShown(True)

    def setCurrentNode(self, node):
        """Clear the widget and generate the view of the new node."""
        model = self.model()
        model.setCurrentNode(node)
        model.refresh()

    def showContextMenu(self, point):
        """Rebuild the context menu from scratch."""
        self.contextMenu.clear()

        index = self.indexAt(point)

        if index.isValid():
            rmAction = self.contextMenu.addAction("Remove")
            # c in lambda is for compatibility with PySide/PyQt. PyQt puts the
            # checked state as a first argument of QAction::triggered slot,
            # PySide doesn't put anything. Although PySide is wrong according to
            # the Qt specification we have to support both calls (see
            # http://doc.qt.io/qt-4.8/qaction.html#triggered)
            rmAction.triggered.connect(
                lambda c=None, i=index, s=self: s.model().remove(i))
        else:
            layerAction = self.contextMenu.addAction("Add Empty Layer")
            layerAction.triggered.connect(self.model().add)

        # popup() is asynchronous, exec_() is synchronous.
        self.contextMenu.exec_(self.mapToGlobal(point))


class LayersModel(BaseModel):
    """Data model for our QTreeView."""

    MIME_TYPES = ['application/walter.layer.item', 'text/uri-list']

    def __init__(self, treeView):
        """Called after the instance has been created."""
        # Need to define it before super.init because it calls refresh
        self.currentNode = None

        super(LayersModel, self).__init__(treeView)

    def refresh(self):
        """Read the node and recreate the tree."""
        pass

    def updateCurrentNode(self, attribute=None):
        """Put the layers to the Maya object."""
        pass

    def executeAction(self, action, index):
        """We are here because user pressed by one of the actions."""
        pass

    def setData(self, index, value, role=QtCore.Qt.EditRole):
        """Set the role data for the item at index to value."""
        if not index.isValid():
            return False

        if role == QtCore.Qt.EditRole and value:
            item = index.internalPointer()

            # TODO: Don't assign directly
            item.name = str(toPyObject(value))
            self.dataChanged.emit(index, index)

            # Update the Maya object
            self.updateCurrentNode(attribute="cacheFileName")

            return True

        return super(LayersModel, self).setData(index, value, role)

    def setCurrentNode(self, node):
        """Save the node to use in future."""
        self.currentNode = node

    def add(self, name=None):
        """Add a layer to the end of the list."""
        # This method can be called from the QAction::triggered slot. PyQt puts
        # the checked state as a first argument, PySide doesn't put anything.
        # Although PySide is wrong according to the Qt specification (see
        # http://doc.qt.io/qt-4.8/qaction.html#triggered), we have to support
        # both calls.

        if isinstance(name, bool) or not name:
            itemName = ""
        else:
            itemName = name

        if self.rootItem:
            self.insert(self.rootItem.childCount(), itemName)

    def insert(self, row, name):
        """Add a layer to the list of items."""
        if self.rootItem:
            self.beginInsertRows(QtCore.QModelIndex(), row, row)
            self.rootItem.insertChild(LayersItem(None, name, True, True), row)
            self.endInsertRows()

            # Update the Maya object
            self.updateCurrentNode()

    def remove(self, index):
        """Remove the index."""
        if not index.isValid():
            return

        item = index.internalPointer()

        i = item.row()

        self.beginRemoveRows(QtCore.QModelIndex(), i, i)
        self.rootItem.childItems.pop(i).setParent(None)
        self.endRemoveRows()

        # Update the Maya object
        self.updateCurrentNode()

    def mimeData(self, indexes):
        """
        Return an object that contains serialized items of data corresponding
        to the list of indexes specified.
        """
        mimeData = QtCore.QMimeData()
        encodedData = QtCore.QByteArray()
        stream = QtCore.QDataStream(encodedData, QtCore.QIODevice.WriteOnly)

        for index in indexes:
            if index.isValid() and index.column() == 0:
                # For performance reasons we do not make a full copy
                # Instead we just serialize the current row
                stream.writeInt32(index.row())

        mimeData.setData("application/walter.layer.item", encodedData)

        return mimeData

    def mimeTypes(self):
        """Return the list of MIME types allowed for drop."""
        return self.MIME_TYPES

    def dropMimeData(self, data, action, row, column, parentIndex):
        """
        Handle the data supplied by a drag and drop operation that ended with
        the given action.
        """
        if action == QtCore.Qt.IgnoreAction:
            return True

        if not data.hasFormat("application/walter.layer.item"):
            # Probably it's a file. We need to add a new layer.
            if not data.hasText():
                return False

            # It's a file. We need to insert it to the list.
            text = str(data.text())

            # Remove prefix if necessary
            filePrefix = 'file://'
            if text.startswith(filePrefix):
                text = text[len(filePrefix):]

            # Use only the first line
            text = text.split('\n')[0].strip(chr(13))

            if row < 0:
                self.add(text)
            else:
                self.insert(row, text)

            return True

        if parentIndex.isValid():
            parent = parentIndex.internalPointer()
        else:
            parent = self.rootItem

        # -1 means user droped the item to the viewport. We need to put it to
        # the end of the list
        if row < 0:
            row = parent.childCount()

        # It's moving ot the items. We need to remove the old one and insert it
        # to the new position.
        encodedData = data.data("application/walter.layer.item")
        stream = QtCore.QDataStream(encodedData, QtCore.QIODevice.ReadOnly)

        while not stream.atEnd():
            srcRow = stream.readInt32()

            if srcRow == row or srcRow == row - 1:
                # Position has not been changed.
                continue

            self.beginMoveRows(
                QtCore.QModelIndex(),
                srcRow,
                srcRow,
                QtCore.QModelIndex(),
                row)

            parent.insertChild(
                parent.childItems.pop(srcRow),
                row - (0 if row < srcRow else 1))

            self.endMoveRows()

            # Update the Maya object.
            self.updateCurrentNode()

        return True


class LayersItem(BaseItem):
    """
    A Python object used to store an item's data, and keep note of it's parents
    and/or children. It's a tree data structure with parent and children.
    """

    ITEM_ICON = None
    OPEN_ICON = None
    VIS_ICON = None
    RND_ICON = None

    # Enum
    ACTION_NONE, ACTION_OPEN, ACTION_VISIBLE, ACTION_RENDERABLE = range(4)

    def __new__(cls, *args, **kwargs):
        # We cannot create a QPixmap when no GUI is being used. That's why it's
        # created here. The QPixmap will be created once.
        if not cls.ITEM_ICON:
            cls.ITEM_ICON = QtGui.QPixmap(os.path.join(ICONS_PATH, "layer.png"))
        if not cls.OPEN_ICON:
            cls.OPEN_ICON = \
                BaseItem.dpiScaledIcon(os.path.join(ICONS_PATH, "open.png"))
        if not cls.VIS_ICON:
            cls.VIS_ICON = \
                BaseItem.dpiScaledIcon(os.path.join(ICONS_PATH, "show.png"))
        if not cls.RND_ICON:
            cls.RND_ICON = BaseItem.dpiScaledIcon(":/renderable.png")

        return super(LayersItem, cls).__new__(cls, *args, **kwargs)

    def __init__(self, parentItem, name, visible, renderable):
        """Called after the instance has been created."""
        super(LayersItem, self).__init__(parentItem, name)
        self.visible = visible
        self.renderable = renderable

    def getActions(self):
        """
        The list of actions to draw on the right side of the item in the tree
        view. It should return the data in the following format:
        [(icon1, opacity1, action1), (icon2, opacity2, action2), ...]
        """
        visibleOpacity = 1.0 if self.visible else 0.2
        renderableOpacity = 1.0 if self.renderable else 0.2
        return [
            (self.OPEN_ICON, 1.0, self.ACTION_OPEN),
            (self.VIS_ICON, visibleOpacity, self.ACTION_VISIBLE),
            (self.RND_ICON, renderableOpacity, self.ACTION_RENDERABLE)]

    def flags(self):
        """The item's flags."""
        return QtCore.Qt.ItemIsDragEnabled | QtCore.Qt.ItemIsEditable

    def getIcon(self):
        """
        The icon of current node. It can be different depending on the item
        type.
        """
        return self.ITEM_ICON
