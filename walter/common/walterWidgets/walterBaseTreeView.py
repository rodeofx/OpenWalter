"""
Attribute Editor tamplate for walterStandin node. It's written on Python to be
able to use complex UI and PySide.
"""

# Copyright 2017 Rodeo FX. All rights reserved.

from . import toPyObject
from .Qt import QtCore
from .Qt import QtGui
from .Qt import QtWidgets
from .itemStyle import ItemStyle
from .utils import dpiScale
from copy import deepcopy
import weakref

NODE_BAR_COLOUR = QtCore.Qt.UserRole + 1
ACTIONS = QtCore.Qt.UserRole + 2
ICON = QtCore.Qt.UserRole + 3
TEXT_INDENT = QtCore.Qt.UserRole + 4





class BaseTreeView(QtWidgets.QTreeView):
    """The main view, used to show the Alembic files."""

    def __init__(self, parent=None):
        """Called after the instance has been created."""
        super(BaseTreeView, self).__init__(parent)

        # Hide the header
        self.header().hide()

        # Set the custom tree model
        model = BaseModel(self)
        self.setModel(model)

        # Custom style
        delegate = BaseDelegate(self)
        self.setItemDelegate(delegate)
        self.setStyle(ItemStyle(self.style()))
        self.setRootIsDecorated(False)

        self.setAnimated(True)

        self.setExpandsOnDoubleClick(False)

        # We need this custom flag because if setDropIndicatorShown is set to
        # false, dropIndicatorPosition returns wrong data. We use this flag to
        # skip drawing of the indicator.
        self.customIndicatorShown = True

    def mousePressEvent(self, event):
        """Receive mouse press events for the widget."""

        index = self.indexAt(event.pos())

        # RND-495: Show the current object properties when middle click on it.
        # We don't want `QtGui.QTreeView.mousePressEvent` to be called since we
        # don't want to select the object when displaying its props.
        if QtCore.Qt.MidButton == event.buttons():
            self.showProperties(event)
            return

        super(BaseTreeView, self).mousePressEvent(event)

        # Redraw the item
        self.redraw(index)

    def mouseReleaseEvent(self, event):
        """Trigger actions based on mouse presses."""
        super(BaseTreeView, self).mouseReleaseEvent(event)
        index = self.indexAt(event.pos())

        if not index.isValid():
            return

        action = self.itemDelegate().getLastAction()

        # Redraw the item
        self.redraw(index)

        button = event.button()

        # TODO: Signal/slot is better
        if button == QtCore.Qt.LeftButton:
            if action:
                self.model().executeAction(action, index)
        elif button == QtCore.Qt.RightButton:
            # We need to open context menu even if there is no action
            self.model().executeContextAction(action, index, event.pos())

    def leaveEvent(self, *args, **kwargs):
        """Clear the action tracking."""
        self.itemDelegate().clearLastAction()

    def redraw(self, index):
        """Regraw given index."""
        rect = self.visualRect(index)
        self.setDirtyRegion(QtGui.QRegion(rect))

    def showProperties(self, event):
        """Show the properties of the item at index `index`."""
        pass


class BaseModel(QtCore.QAbstractItemModel):
    """Data model for our QTreeView."""

    def __init__(self, treeView, parent=None):
        """Called after the instance has been created."""
        super(BaseModel, self).__init__(parent)
        self.treeView = weakref.ref(treeView)

        self.rootItem = None
        self.refresh()

    def flags(self, index):
        """The item's flags for the given index."""
        defaultFlags = super(BaseModel, self).flags(index)

        if index.isValid():
            item = index.internalPointer()
            return defaultFlags | item.flags()

        return defaultFlags | QtCore.Qt.ItemIsDropEnabled

    def refresh(self):
        """Read the attributes and recreate the layers."""
        pass

    def rowCount(self, parent=QtCore.QModelIndex()):
        """The number of rows under the given parent."""
        if parent.column() > 0:
            return 0
        if not parent.isValid():
            item = self.rootItem
        else:
            item = parent.internalPointer()

        if not item:
            return 0

        return item.childCount()

    def columnCount(self, parent=None):
        """Return the number of columns for the children of the given parent."""
        return 1

    def data(self, index, role=QtCore.Qt.DisplayRole):
        """
        Return the data stored under the given role for the item referred to by
        the index.
        """
        if not index.isValid():
            return
        item = index.internalPointer()
        if (role == QtCore.Qt.DisplayRole or
                role == QtCore.Qt.EditRole or role == QtCore.Qt.ToolTipRole):
            return item.getName()
        elif role == QtCore.Qt.SizeHintRole:
            return QtCore.QSize(dpiScale(250), dpiScale(20))
        elif role == QtCore.Qt.BackgroundRole:
            return QtGui.QColor(71, 71, 71)
        elif role == NODE_BAR_COLOUR:
            return QtGui.QColor(113, 142, 164)
        elif role == ACTIONS:
            return item.getActions()
        elif role == ICON:
            return item.getIcon()
        elif role == TEXT_INDENT:
            return item.getIndent()

    def setData(self, index, value, role=QtCore.Qt.EditRole):
        """Set the role data for the item at index to value."""
        if not index.isValid():
            return False

        if role == QtCore.Qt.EditRole and value:
            item = index.internalPointer()

            # TODO: Don't assign directly
            item.name = value
            self.dataChanged.emit(index, index)

            return True

        return False

    def indexFromItem(self, node):
        """Create the index that represents the given item in the model."""
        if not node or not node.parent():
            return QtCore.QModelIndex()

        for i in range(node.parent().childCount()):
            if node.parent().child(i) is node:
                return self.createIndex(i, 0, node)

        return QtCore.QModelIndex()

    def index(self, row, column, parent):
        """
        The index of the item in the model specified by the given row, column
        and parent index.
        """
        if not self.hasIndex(row, column, parent):
            return QtCore.QModelIndex()

        if not parent.isValid():
            parentItem = self.rootItem
        else:
            parentItem = parent.internalPointer()

        childItem = parentItem.child(row)
        if childItem:
            return self.createIndex(row, column, childItem)
        else:
            return QtCore.QModelIndex()

    def parent(self, index):
        """The parent of the model item with the given index."""
        if not index.isValid():
            return QtCore.QModelIndex()

        childItem = index.internalPointer()
        if not childItem or not isinstance(childItem, BaseItem):
            return QtCore.QModelIndex()

        parentItem = childItem.parent()

        if not parentItem or parentItem == self.rootItem:
            return QtCore.QModelIndex()

        return self.createIndex(parentItem.row(), 0, parentItem)

    def executeAction(self, action, index):
        """User pressed by one of the actions."""
        pass

    def executeContextAction(self, action, index, point):
        """User pressed with right button by one of the actions."""
        pass


class BaseDelegate(QtWidgets.QItemDelegate):
    """Custom display delegate for the tree items."""

    ICON_PADDING = dpiScale(10)
    COLOR_BAR_WIDTH = dpiScale(6)
    ICON_WIDTH = dpiScale(20)
    ACTION_BORDER = dpiScale(2)
    ICON_HIGHLIGHT = QtGui.QColor(113, 142, 164)

    def __init__(self, treeView):
        """Called after the instance has been created."""
        super(BaseDelegate, self).__init__()
        self.treeView = weakref.ref(treeView)
        self.lastHitAction = None
        self.textColor = self.treeView().palette().text().color()

    def paint(self, painter, option, index):
        """Main entry point of drawing the cell."""
        if not index.isValid():
            return

        # sizeHint = toPyObject(index.data(QtCore.Qt.SizeHintRole))
        # if sizeHint.height() <= 1:
        #     return

        rect = deepcopy(option.rect)
        highlightedColor = option.palette.color(QtGui.QPalette.Highlight)
        isHighlighted = \
            option.showDecorationSelected and \
            option.state & QtWidgets.QStyle.State_Selected

        self.drawBackground(
            painter, rect, index, isHighlighted, highlightedColor)
        self.drawColorBar(painter, rect, index)
        self.drawFill(painter, rect)
        actionIconRect = self.drawActionIcons(rect, painter, option, index)
        textRect = self.drawText(rect, actionIconRect, painter, index)
        self.drawIcon(textRect, painter, option, index)

    def drawBackground(
            self, painter, rect, index, isHighlighted, highlightColor):
        """Draw the cell bacground color / image."""
        if isHighlighted:
            # Draw the highlight color
            painter.fillRect(rect, highlightColor)
        else:
            # Otherwise draw our background color
            background = toPyObject(index.data(QtCore.Qt.BackgroundRole))
            painter.fillRect(rect, background)

    def drawColorBar(self, painter, rect, index):
        """Draw the label color bar."""
        color = toPyObject(index.data(NODE_BAR_COLOUR))
        rect2 = deepcopy(rect)
        rect2.setRight(rect2.left() + self.COLOR_BAR_WIDTH)
        painter.fillRect(rect2, color)

    def drawFill(self, painter, rect):
        """Draw the border of the cell."""
        painter.save()

        painter.setRenderHint(QtGui.QPainter.Antialiasing)
        painter.setRenderHint(QtGui.QPainter.HighQualityAntialiasing)

        # Draw a 2 pixel border around the box
        painter.setPen(QtGui.QPen(QtGui.QColor(43, 43, 43), 2))
        painter.drawRoundedRect(rect, 3, 3)

        painter.restore()

    def drawIcon(self, rect, painter, option, index):
        """Draw the item's icon."""
        painter.save()

        if (index.column() == 0):
            icon = toPyObject(index.data(ICON))
            if icon:
                newRect = QtCore.QRect()
                padding = dpiScale(4)

                center = \
                    toPyObject(index.data(QtCore.Qt.SizeHintRole)).height() / 2
                newRect.setRight(rect.left() - padding)
                newRect.setLeft(newRect.right() - self.ICON_WIDTH + dpiScale(1))
                newRect.setTop(rect.top() + center - self.ICON_WIDTH / 2)
                newRect.setBottom(newRect.top() + self.ICON_WIDTH - dpiScale(1))
                painter.drawPixmap(newRect, icon)

        painter.restore()

    def drawText(self, rect, actionIconRect, painter, index):
        """Draw the node name."""
        # Text color
        # TODO: If the item is disabed, draw a lighter text
        painter.setPen(
            QtGui.QPen(self.textColor, 1))

        # Text font
        painter.setFont(QtWidgets.QApplication.font())

        # Draw the text for the node
        textRect = deepcopy(rect)
        textRect.setBottom(textRect.bottom() + dpiScale(2))
        textRect.setLeft(
            textRect.left() +
            toPyObject(index.data(TEXT_INDENT)) +
            self.ICON_PADDING)

        right = textRect.right() - dpiScale(11)
        if actionIconRect:
            right = actionIconRect.left() - dpiScale(11)
        textRect.setRight(right)

        painter.drawText(
            textRect,
            QtCore.Qt.AlignLeft | QtCore.Qt.AlignVCenter,
            str(toPyObject(index.data(QtCore.Qt.DisplayRole))))

        return textRect

    def drawActionIcons(self, rect, painter, option, index):
        """Draw the icons and buttons on the right side of the item."""
        painter.save()

        actions = toPyObject(index.data(ACTIONS))

        buttonPressed = \
            QtWidgets.QApplication.mouseButtons() == QtCore.Qt.LeftButton or \
            QtWidgets.QApplication.mouseButtons() == QtCore.Qt.RightButton

        # Position
        center = toPyObject(index.data(QtCore.Qt.SizeHintRole)).height() / 2
        start = self.ACTION_BORDER

        iconRectCumul = None
        for pixmap, opacity, action in actions:

            if not pixmap or not opacity:
                continue
            # Draw icon
            start += self.ICON_WIDTH + self.ACTION_BORDER

            width = min(self.ICON_WIDTH, pixmap.rect().width())
            height = min(self.ICON_WIDTH, pixmap.rect().height())

            iconRect = QtCore.QRect(
                rect.right() - start + (self.ICON_WIDTH - width) / 2,
                rect.top() + center - height / 2,
                width,
                height)
            iconRectCumul = iconRect

            painter.setRenderHint(QtGui.QPainter.SmoothPixmapTransform)
            painter.setOpacity(opacity)
            painter.drawPixmap(iconRect, pixmap)

            # Highlight the icon depending on the mouse over.
            cursorPosition = self.treeView().mapFromGlobal(QtGui.QCursor.pos())
            if buttonPressed and iconRect.contains(cursorPosition):
                painter.setOpacity(0.75)
                painter.setRenderHint(QtGui.QPainter.Antialiasing)
                painter.setRenderHint(QtGui.QPainter.HighQualityAntialiasing)

                # Set the clip region to the given rectangle
                painter.setClipping(True)
                painter.setClipRect(iconRect)

                # Form a new rectangle to draw a circle
                centerRect = iconRect.center()
                tick = 1.0

                iconRect.setCoords(
                    centerRect.x() - tick * self.ICON_WIDTH,
                    centerRect.y() - tick * self.ICON_WIDTH,
                    centerRect.x() + tick * self.ICON_WIDTH,
                    centerRect.y() + tick * self.ICON_WIDTH)

                painter.setBrush(QtGui.QBrush(self.ICON_HIGHLIGHT))
                painter.setPen(QtCore.Qt.NoPen)

                painter.drawEllipse(iconRect)

                # Save the action to know
                self.lastHitAction = action

                painter.setClipping(False)

        painter.restore()
        return iconRectCumul

    def getLastAction(self):
        """Track action to process clicks on the icons."""
        action = self.lastHitAction
        self.clearLastAction()
        return action

    def clearLastAction(self):
        """Clear the action tracking."""
        self.lastHitAction = None

    def updateEditorGeometry(self, editor, option, index):
        """Update the editor's rectangle for the item specified by index."""
        rect = deepcopy(option.rect)

        rigthCumul = 0
        start = self.ACTION_BORDER
        actions = toPyObject(index.data(ACTIONS))
        for pixmap, opacity, action in actions:
            if not pixmap or not opacity:
                continue

            start += self.ICON_WIDTH + self.ACTION_BORDER
            width = min(self.ICON_WIDTH, pixmap.rect().width())
            rigthCumul = rect.right() - start + (self.ICON_WIDTH - width) / 2

        if rigthCumul:
            right = rigthCumul - dpiScale(11)
        else:
            right = rect.right() - dpiScale(11)

        # Offset the rectangle because we need to see all the decorations.
        leftOffset = \
            toPyObject(index.data(TEXT_INDENT)) + \
            self.ICON_PADDING - \
            2 * self.ACTION_BORDER
        topOffset = self.ACTION_BORDER
        bottomOffset = self.ACTION_BORDER

        rect.setCoords(
            rect.left() + leftOffset,
            rect.top() + topOffset,
            right,
            rect.bottom() - bottomOffset)

        editor.setGeometry(rect)


class BaseItem(object):
    """
    A Python object used to store an item's data, and keep note of it's parents
    and/or children. It's a tree data structure with parent and children.
    """

    def __init__(self, parentItem, name):
        """Called after the instance has been created."""
        self.name = name
        self.childItems = []
        self.setParent(parentItem)

    def getName(self):
        """The label of the item."""
        return self.name

    def appendChild(self, child):
        """Add a child to the data structure."""
        child.parentItem = self
        self.childItems.append(child)

    def insertChild(self, child, rowNumber):
        """Insert a child item."""
        child.parentItem = self
        self.childItems.insert(rowNumber, child)

    def removeChild(self, child):
        for child_ in child.childItems:
            child.removeChild(child_)
        child.parentItem = None
        self.childItems.remove(child)

    def setParent(self, parent, position=-1):
        """Set the parent item."""
        if parent:
            if position >= 0 and position < len(parent.childItems):
                parent.insertChild(self, position)
            else:
                parent.appendChild(self)
        else:
            self.parentItem = None

    def child(self, rowNumber):
        """Return the child if exists."""
        if rowNumber < len(self.childItems):
            return self.childItems[rowNumber]
        else:
            return None

    def childCount(self):
        """The number of the children."""
        return len(self.childItems)

    def rowCount(self, parentIndex=QtCore.QModelIndex()):
        """Return the number of rows under the given parent."""
        parent = self.parentItem
        if not parent:
            return 0
        return parent.childCount()

    def parent(self):
        """Return the parent TreeItem of the current item."""
        return self.parentItem

    def row(self):
        """The index of current node in the list of children of the parent."""
        if self.parent():
            return self.parent().childItems.index(self)
        return 0

    def getActions(self):
        """
        The list of actions to draw on the right side of the item in the tree
        view. It should return the data in the following format:
        [(icon1, opacity1, action1), (icon2, opacity2, action2), ...]
        """
        return []

    def flags(self):
        """The item's flags."""
        return QtCore.Qt.ItemIsDropEnabled

    def getIcon(self):
        """
        The icon of current node. It can be different depending on the item
        type.
        """
        pass

    def getIndent(self):
        """The text indent."""
        return dpiScale(20)

    @staticmethod
    def dpiScaledIcon(path):
        """Creates QPixmap and scales it for hi-dpi mode"""
        icon = QtGui.QPixmap(path)

        scale = dpiScale(1.0)
        if scale > 1.0:
            icon = icon.scaledToHeight(
                icon.height() * scale,
                QtCore.Qt.SmoothTransformation)

        return icon

    @staticmethod
    def coloredIcon(fileName, color=QtGui.QColor(255, 255, 255, 255)):
        """Load an icon and multiply it by specified color"""
        rgba = color.rgba()
        image = BaseItem.dpiScaledIcon(fileName).toImage()
        for x in range(image.width()):
            for y in range(image.height()):
                currentColor = image.pixel(x, y)
                image.setPixel(x, y, currentColor & rgba)

        return QtGui.QPixmap.fromImage(image)
