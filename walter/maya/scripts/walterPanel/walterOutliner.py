"""Tree View widget for the Walter panel."""

# Copyright 2017 Rodeo FX. All rights reserved.

from .mergeExpressions import MergeExpressions
from walterWidgets.Qt import QtCore
from walterWidgets.Qt import QtGui
from walterWidgets.Qt import QtWidgets

from copy import deepcopy
from walterWidgets import ACTIONS
from walterWidgets import BaseDelegate
from walterWidgets import BaseItem
from walterWidgets import BaseModel
from walterWidgets import BaseTreeView
from walterWidgets import NODE_BAR_COLOUR
from walterWidgets import dpiScale
import pymel.core as pm
from walter import OVERRIDE_ATTRIBUTE
from walter import OVERRIDE_DISPLACEMENT
from walter import OVERRIDE_SHADER
from walter import OVERRIDE_TRANSFORM
from walterPropertiesDialog import WalterPropertiesDialog
from walterVisibilityMenu import VisibilityMenu
from walterPurposeMenu import StagePurposeMenu
from walterVariantsMenu import VariantsMenu

# Global look variables.
ITEM_HEIGHT = dpiScale(22)
ITEM_INDENT = dpiScale(8)
EXPAND_SENSITIVITY = dpiScale(4)

CHILD_COUNT = QtCore.Qt.UserRole + 64
SHADER = QtCore.Qt.UserRole + 65
DISPLACEMENT = QtCore.Qt.UserRole + 66
ATTRIBUTE = QtCore.Qt.UserRole + 67
TRANSFORM = QtCore.Qt.UserRole + 68
SHADER_ICON = QtCore.Qt.UserRole + 69
VISIBILITY = QtCore.Qt.UserRole + 70

ABC_PREFIX = '__AbcWalter_'
OVERRIDE_SHADER_ABC = ABC_PREFIX + OVERRIDE_SHADER
OVERRIDE_DISPLACEMENT_ABC = ABC_PREFIX + OVERRIDE_DISPLACEMENT
OVERRIDE_ATTRIBUTE_ABC = ABC_PREFIX + OVERRIDE_ATTRIBUTE
OVERRIDES = [
    OVERRIDE_TRANSFORM,
    OVERRIDE_SHADER,
    OVERRIDE_DISPLACEMENT,
    OVERRIDE_ATTRIBUTE,
    OVERRIDE_SHADER_ABC,
    OVERRIDE_DISPLACEMENT_ABC,
    OVERRIDE_ATTRIBUTE_ABC]


class OutlinerTreeView(BaseTreeView):
    """The main view, used to show the Alembic networks."""

    SelectionModel_Select = 0x0002

    itemSelected = QtCore.Signal(str, object)
    expressionIsMatching = QtCore.Signal(object, object)

    def __init__(self, traverser, parent=None):
        """Called after the instance has been created."""
        super(OutlinerTreeView, self).__init__(parent)
        self.traverser = traverser

        self.layer = self.traverser.currentLayer()
        # Set the custom tree model
        model = TreeModel(self, self.traverser)
        self.setModel(model)

        # Custom style
        delegate = TreeViewDelegate(self)
        self.setItemDelegate(delegate)
        self.setIndentation(ITEM_INDENT)

        # Accept drop events
        self.setAcceptDrops(True)
        self.setDragDropMode(QtWidgets.QAbstractItemView.DragDrop)
        self.setDragEnabled(True)
        self.setDropIndicatorShown(True)

        # Edit item when user double clicks it
        self.setEditTriggers(QtWidgets.QAbstractItemView.DoubleClicked)

        # Context Menu
        self.contextMenu = QtWidgets.QMenu(self)

        self.dialog = WalterPropertiesDialog(self)

        # Multi Selection
        self.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)

        self.expanded.connect(self.onExpanded)

        self.highlightViewportCache = None

    def mousePressEvent(self, event):
        index = self.indexAt(event.pos())
        if event.button() == QtCore.Qt.RightButton and not index.isValid():
            self.contextMenu.clear()
            refreshAction = self.contextMenu.addAction("Refresh")
            refreshAction.triggered.connect(self.refresh)
            self.contextMenu.exec_(self.mapToGlobal(event.pos()))
        else:
            super(OutlinerTreeView, self).mousePressEvent(event)

    def showContextMenu(self, point, itemAction = None):
        """Rebuild the context menu from scratch."""
        self.contextMenu.clear()

        indices = self.selectedIndexes()
        if not indices:
            return

        isSingleSelection = len(indices) == 1
        currentLayer = self.getLayer()

        originIndices = []
        alembicIndices = []
        alembicObjects = []
        expressionIndices = []
        expGroupIndices = []
        hasOverrideIndices = {
            OVERRIDE_TRANSFORM: [],
            OVERRIDE_SHADER: [],
            OVERRIDE_DISPLACEMENT: [],
            OVERRIDE_ATTRIBUTE: []}
        noOverrideIndices = deepcopy(hasOverrideIndices)

        for index in indices:
            if not index.isValid():
                continue

            item = index.internalPointer()

            if item.getType() == TreeItem.TYPE_ORIGIN:
                originIndices.append(index)
            elif item.getType() == TreeItem.TYPE_ALEMBIC:
                alembicIndices.append(index)
                alembicObjects.append(item.alembicObject)
            elif item.getType() == TreeItem.TYPE_EXPRESSION:
                expressionIndices.append(index)
            elif item.getType() == TreeItem.TYPE_EXPGROUP:
                expGroupIndices.append(index)

            for override in hasOverrideIndices:
                if item.getOverride(currentLayer, override):
                    hasOverrideIndices[override].append(index)
                elif item.alembicObject or item.expression or item.expressionGroup:
                    noOverrideIndices[override].append(index)

        if itemAction == TreeItem.ACTION_SURFACE:

            allIndices = alembicIndices + expressionIndices + expGroupIndices

            colorDefaultAction = self.contextMenu.addAction("Default (Grey)")
            colorRedAction = self.contextMenu.addAction("Red")
            colorGreenAction = self.contextMenu.addAction("Green")
            colorBlueAction = self.contextMenu.addAction("Blue")
            colorYellowAction = self.contextMenu.addAction("Yellow")
            self.contextMenu.addSeparator()

            # expAndGroupindices = expressionIndices + expGroupIndices

            colorDefaultAction.triggered.connect(
                lambda a=allIndices, m=self.model():
                [m.setData(i, (240, 240, 240), SHADER_ICON) for i in a])
            colorRedAction.triggered.connect(
                lambda a=allIndices, m=self.model():
                [m.setData(i, (255, 0, 0), SHADER_ICON) for i in a])
            colorGreenAction.triggered.connect(
                lambda a=allIndices, m=self.model():
                [m.setData(i, (0, 255, 0), SHADER_ICON) for i in a])
            colorBlueAction.triggered.connect(
                lambda a=allIndices, m=self.model():
                [m.setData(i, (0, 0, 255), SHADER_ICON) for i in a])
            colorYellowAction.triggered.connect(
                lambda a=allIndices, m=self.model():
                [m.setData(i, (255, 255, 0), SHADER_ICON) for i in a])

        if alembicObjects:
            # Context menu Copy
            copyAction = self.contextMenu.addAction("Copy")

            text = alembicObjects[0]

            # Generate an expression that matches to all the selected objects.
            for i in range(1, len(alembicObjects)):
                expression = MergeExpressions(text, alembicObjects[i])
                text = expression()

            copyAction.triggered.connect(
                lambda p=text: QtWidgets.QApplication.clipboard().setText(p))

        if alembicIndices:
            # Context menu Expose Transforms
            # Don't set transform of a prim pseudo-root.
            # It's not supported by USD and leads to a SEGFAULT
            transfoIndices = []
            for index in alembicIndices:
                item = index.internalPointer()
                if not item.isPseudoRoot:
                    transfoIndices.append(index)

            if transfoIndices:
                exposeAction = self.contextMenu.addAction("Expose Transforms")
                exposeAction.triggered.connect(
                    lambda a=transfoIndices, m=self.model():
                    m.exposeTransforms(a))

        overridesAndActions = {
            OVERRIDE_TRANSFORM: "Detach Transform",
            OVERRIDE_SHADER: "Detach Shader",
            OVERRIDE_DISPLACEMENT: "Detach Displacement",
            OVERRIDE_ATTRIBUTE: "Detach Attribute"}
        for ovr, message in overridesAndActions.iteritems():
            if hasOverrideIndices[ovr]:
                action = self.contextMenu.addAction(message)
                action.triggered.connect(
                    lambda i=hasOverrideIndices[ovr], m=self.model(), o=ovr:
                    m.detachOverride(i, o))

        if noOverrideIndices[OVERRIDE_ATTRIBUTE]:
            createAction = self.contextMenu.addAction("Attribute Override")
            createAction.triggered.connect(
                lambda i=noOverrideIndices[OVERRIDE_ATTRIBUTE], m=self.model():
                m.createOverride(i, OVERRIDE_ATTRIBUTE))

        visIndices = []
        for index in indices:
            item = index.internalPointer()
            if not item.isPseudoRoot:
                visIndices.append(index)
        if visIndices:
            self.contextMenu.addMenu(VisibilityMenu(
                visIndices,
                self.contextMenu))

            self.contextMenu.addMenu(StagePurposeMenu(
                visIndices,
                self.contextMenu))

        if len(visIndices) == 1:
            item = visIndices[0].internalPointer()
            variantMenu = VariantsMenu()
            if variantMenu.reset(
                item.getOriginObject(),
                item.alembicObject,
                'Variants',
                False, False, False):
                self.contextMenu.addMenu(variantMenu)

        # Remove item
        removeActions = [
            (expressionIndices, "Remove Expression"),
            (expGroupIndices, "Remove Group")]
        for indices, message in removeActions:
            if not indices:
                continue

            removeAction = self.contextMenu.addAction(message)

            # Don't allow to remove expressions/groups from Alembic.
            locked = False
            for i in indices:
                if not i.isValid():
                    continue

                item = i.internalPointer()

                for o in TreeItem.OVERRIDES_THAT_LOCK:
                    if item.hasOverride(o):
                        locked = True
                        break

                if locked:
                    break

            # Gray the action
            removeAction.setEnabled(not locked)

            removeAction.triggered.connect(
                lambda idxs=indices, m=self.model():
                m.removeExpression(idxs))

        # TODO: a button will be better
        if isSingleSelection and originIndices:
            createActionAssignments = \
                self.contextMenu.addAction("Save Assignments...")
            createActionAssignments.triggered.connect(
                lambda i=originIndices[0], m=self.model(): m.saveAssignment(i))

            createActionMaterials = \
                self.contextMenu.addAction("Save Materials...")
            createActionMaterials.triggered.connect(
                lambda i=originIndices[0], m=self.model(): m.saveMaterials(i))

            createActionAttributes = \
                self.contextMenu.addAction("Save Attribute Overrides...")
            createActionAttributes.triggered.connect(
                lambda i=originIndices[0], m=self.model(): m.saveAttributes(i))

            createActionTransforms = \
                self.contextMenu.addAction("Save Transforms...")
            createActionTransforms.triggered.connect(
                lambda i=originIndices[0], m=self.model(): m.saveTransforms(i))

            createActionPurposes = \
                self.contextMenu.addAction("Save Purposes...")
            createActionPurposes.triggered.connect(
                lambda i=originIndices[0], m=self.model(): m.savePurposes(i))

            createActionVariants = \
                self.contextMenu.addAction("Save Variants...")
            createActionVariants.triggered.connect(
                lambda i=originIndices[0], m=self.model(): m.saveVariantsLayer(i))

        if expressionIndices and not originIndices and not alembicObjects:
            group = self.contextMenu.addAction("Group")
            group.triggered.connect(
                lambda idxs=expressionIndices, m=self.model():
                m.groupExpressions(idxs))

            # Add Action in Menu to remove from group only when expression is in Group
            item = expressionIndices[0].internalPointer()
            if item.parent().getType() == TreeItem.TYPE_EXPGROUP:
                ungroup = self.contextMenu.addAction("Ungroup")
                ungroup.triggered.connect(
                    lambda idxs=expressionIndices, m=self.model():
                    m.unGroupExpressions(idxs))

        if self.contextMenu.isEmpty():
            refreshAction = self.contextMenu.addAction("Refresh")
            refreshAction.triggered.connect(self.refresh)

        if not self.contextMenu.isEmpty():
            # popup() is asynchronous, exec_() is synchronous.
            self.contextMenu.exec_(self.mapToGlobal(point))

    def selectionChanged(self, selected, deselected):
        """
        Called when the selection is changed. The previous selection (which
        may be empty), is specified by deselected, and the new selection by
        selected.
        """
        # This will redraw selected items in the tree view.
        super(OutlinerTreeView, self).selectionChanged(selected, deselected)

        indices = self.selectedIndexes()
        if indices:
            paths = []

            for i in indices:
                if not i.isValid():
                    continue

                item = i.internalPointer()

                subObject = item.alembicObject or item.expression
                if subObject:
                    paths.append(subObject)

            # TODO: use highlightViewport()
            self.itemSelected.emit(item.getOriginObject(), paths)

    def highlightViewport(self, origin, path):
        """
        Highlight given object in the viewport.
        """
        if self.highlightViewportCache != path:
            self.highlightViewportCache = path
            self.itemSelected.emit(origin, [path])

        # RND-584. See walterPanel.expressionIsMatching
        indices = self.selectedIndexes()
        if indices and len(indices) > 0:
            # Get the first item since only a single 
            # expression can be edited at a time.
            index = indices[0]
            item = index.internalPointer()
            if item.expression:
                self.expressionIsMatching.emit(index, path)

    # TODO: put it to TreeModel
    def onExpanded(self, index):
        """It is called when the item specified by index is expanded."""
        if not index.isValid():
            return

        # Load the objects of the children items
        item = index.internalPointer()
        for i in range(item.childCount()):
            child = item.child(i)
            child.obtainChildren()

    def select(self, origin, path):
        """Select item by object name and path."""
        # TODO: Do it with TreeModel. Don't use the items directly.
        root = self.model().rootItem

        # Find the object.
        found = root.find(origin=origin, path=path)

        if not found:
            return

        # setCurrentIndex() expands items but doesn't emit "expanded" signal. We
        # should do it manually. Otherwise, newly expanded items don't contain
        # children.
        parent = found.parent()
        while parent:
            index = self.model().indexFromItem(parent)
            self.onExpanded(index)
            parent = parent.parent()

        # Deselect everything first.
        self.clearSelection()

        # We are ready to manipulate with the selection.
        # Get the index of the item.
        index = self.model().indexFromItem(found)
        # Select it.
        self.selectionModel().setCurrentIndex(
            index, OutlinerTreeView.SelectionModel_Select)

    def showProperties(self, event):
        """Implementation of BaseTreeView."""
        index = self.indexAt(event.pos())
        item = index.internalPointer()
        if item:
            self.dialog.showAtPosition(
                self.mapToGlobal(event.pos()),
                item.getProperties())

    def refresh(self):
        """Recreate the tree."""
        self.layer = self.traverser.currentLayer()
        self.model().refresh()

    def dropEventIndex(self, event):
        """Return the index the user droped something to."""
        # Get index under mouse cursor and check if it exists
        index = self.indexAt(event.pos())

        if not index.isValid():
            return

        return index

    def dragEnterEvent(self, event):
        """
        The event handler which is called when a drag is in progress and the
        mouse enters this widget.

        Args:
            event: The event is passed in this parameter.
        """
        super(OutlinerTreeView, self).dragEnterEvent(event)
        # In this step we should accept drop even if the tree view item is
        # wrong. Because once it's rejected here, Qt will not be trying to drop
        # on other items.
        data = event.mimeData()
        self.sourceIndex = self.indexAt(event.pos()).internalPointer()

        # With format 'x-maya-data' we can be sure that the source app is maya
        if data.hasFormat('application/x-maya-data') and data.hasText():
            # Maya objects
            event.accept()
        elif data.hasFormat(TreeModel.MIME_TYPE):
            event.accept()
        else:
            event.ignore()

    def dragMoveEvent(self, event):
        """
        The event handler which is called if a drag is in progress, and when any
        of the following conditions occur: the cursor enters this widget, the
        cursor moves within this widget, or a modifier key is pressed on the
        keyboard while this widget has the focus.

        Args:
            event: The event is passed in this parameter.
        """
        super(OutlinerTreeView, self).dragMoveEvent(event)
        # In this step we should check the type of event.
        data = event.mimeData()
        accept = False

        # The position of the indicator. Can be one of the following: OnItem,
        # AboveItem, BelowItem, OnViewport
        dropPosition = self.dropIndicatorPosition()

        # Get index under mouse cursor and check if it exists
        index = self.dropEventIndex(event)
        if index:
            self.dropIndex = index
            if data.hasFormat(TreeModel.MIME_TYPE):
                # It's expression item
                item = index.internalPointer()
                itemType = item.getType()

                if itemType == TreeItem.TYPE_EXPRESSION:
                    # We need to check if the user wants to ungroup item by drop
                    # the expression outside the group. Since the last child of
                    # the group is expression, we are here.
                    if dropPosition == QtWidgets.QAbstractItemView.BelowItem:

                        if index.row() == item.getOriginItem().childCount() - 1:
                            # There is no next index, it means the current one
                            # is the las one.
                            accept = True
                    if dropPosition == QtWidgets.QAbstractItemView.AboveItem:
                        accept = True

                elif itemType == TreeItem.TYPE_EXPGROUP:
                    # Only groups can acceps the expression items
                    if item.parent().getName() == self.sourceIndex.getOriginItem().getName():
                        # We need to test if we are exactly on the item, not on indicator line.
                        if dropPosition == QtWidgets.QAbstractItemView.OnItem:
                            # On drop we will add the expression to the group.
                            accept = True
                        # Check if we are not on an item but above.
                        elif dropPosition == QtWidgets.QAbstractItemView.AboveItem:
                            # The user is going to drop the item above to ungroup it.
                            accept = False
                        # Check if the indicator points below the item.
                        elif dropPosition == QtWidgets.QAbstractItemView.BelowItem:
                            # The user is going to drop the item below the
                            # group. If the group is collapsed, it means we need
                            # to ungroup it.
                            accept = self.isExpanded(index)

                elif itemType == TreeItem.TYPE_ORIGIN:
                    # expression items draged on WalterStandin to ungroup
                    if (dropPosition == QtWidgets.QAbstractItemView.OnItem and
                        item.getName() ==
                            self.sourceIndex.getOriginItem().getName()):
                        # limit the item drop to the originItem tree
                        accept = True
            else:
                # The hack. Sometimes Qt sets drop position as BelowItem or
                # AboveItem when it should set it to OnItem. We found out that
                # it's coming from QAbstractItemViewPrivate::position() of Qt.
                # It always returns BelowItem or AboveItem if the item doesn't
                # have ItemIsDropEnabled. Our item has it, and we don't know
                # what's going on because it's urgent and we don't have time to
                # continue the investigation. This hack works just perfectly,
                # because if the user drops the material to the object in the
                # panel, we don't care if it's above or below.
                # TODO: investigate it properly.
                dropPosition = QtWidgets.QAbstractItemView.OnItem
                # Maya objects
                accept = True

        if accept:
            # Deselect everything first.
            self.clearSelection()

            if dropPosition in [QtWidgets.QAbstractItemView.AboveItem,
                                QtWidgets.QAbstractItemView.BelowItem]:
                self.customIndicatorShown = True
            else:
                # Don't use indicator, use selection
                self.customIndicatorShown = False

                # Change selection
                self.selectionModel().setCurrentIndex(
                    index, OutlinerTreeView.SelectionModel_Select)

            event.accept()

        else:
            self.customIndicatorShown = False
            event.ignore()

    def mouseDoubleClickEvent(self, event):
        """Receive mouse double click events for the widget."""

        # RND-495: Show the current object properties when middle click on it's item.
        # Don't expand the tree-item on double middle clicks.
        if QtCore.Qt.MidButton == event.buttons():
            return

        super(OutlinerTreeView, self).mouseDoubleClickEvent(event)
        index = self.indexAt(event.pos())
        item = index.internalPointer()
        # TODO: Make sure that item is not None
        #       If double-clicking outiside any Item, it Throws an error.
        if item.getType() != TreeItem.TYPE_EXPGROUP:
            self.setExpandedChildren(index, not self.isExpanded(index))

    def getLayer(self):
        """Return the layer that is displayed by this widget."""
        return self.layer

    def setExpandedChildren(self, index, expanded):
        """
        Set the item referred to by index and all the children to either
        collapse or expanded, depending on the value of expanded.
        """
        if not index.isValid():
            return

        # Expand this
        self.setExpanded(index, expanded)

        # Expand children
        childCount = index.model().rowCount(index)
        for i in range(childCount):
            child = index.child(i, 0)
            self.setExpandedChildren(child, expanded)


WALTER_SCENE_CALLBACKS_OWNER = "OutlinerTreeView"
WALTER_SCENE_CHANGED_CALLBACK = "WALTER_SCENE_CHANGED"
WALTER_TRANFORM_LOCATOR_DELETED = \
    "WALTER_TRANFORM_LOCATOR_DELETED"

WALTER_UPDATE_ITEMS = 'UPDATE_ITEMS'
WALTER_ADD_ROOT_ITEM = 'ADD_ROOT_ITEM'
WALTER_REMOVE_ROOT_ITEM = 'REMOVE_ROOT_ITEM'
WALTER_RENAME_ROOT_ITEM = 'RENAME_ROOT_ITEM'
WALTER_ATTACH_TRANSFORM = 'ATTACH_TRANSFORM'
WALTER_DETACH_TRANSFORM = 'DETACH_TRANSFORM'


class TreeModel(BaseModel):
    """Data model for our QTreeView."""

    MIME_TYPE = 'application/walter.panel.expression'

    def __init__(self, treeView, traverser, parent=None):
        """Called after the instance has been created."""
        self.traverser = traverser
        super(TreeModel, self).__init__(treeView, parent)
        self.__addCallback(WALTER_SCENE_CHANGED_CALLBACK,
                           WALTER_SCENE_CALLBACKS_OWNER,
                           self.__updateModel)

        self.__addCallback(WALTER_TRANFORM_LOCATOR_DELETED,
                           WALTER_SCENE_CALLBACKS_OWNER,
                           self.__exposeOrDetachTransforms)

    def __addCallback(self, hook, owner, callback):
        callbacks = pm.callbacks(listCallbacks=True,
                                 hook=hook,
                                 owner=owner)

        if callbacks:
            for c in callbacks:
                pm.callbacks(removeCallback=c,
                             hook=hook,
                             owner=owner)

        pm.callbacks(addCallback=callback,
                     hook=hook,
                     owner=owner)

    def refresh(self):
        """Read the scene and recreate the tree."""
        self.beginResetModel()

        # TODO: Resolve the circular references between objects to be able to
        # delete all the objects in the structure.
        self.rootItem = TreeItem(None, self.traverser)
        self.rootItem.obtainChildren()

        # Assign overrides.
        for override in OVERRIDES:
            self.setOverrides(override)

        self.endResetModel()

    def __getWalterStandinRootItem(self, walterStandinName):
        walterStandinRootItem = None
        walterStandinRootItemIndex = 0
        for i in range(self.rootItem.childCount()):
            originItem = self.rootItem.child(i)
            if originItem.getName() == walterStandinName:
                walterStandinRootItem = originItem
                walterStandinRootItemIndex = i
                return walterStandinRootItem, walterStandinRootItemIndex
        return None, None

    def __updateModel(self, action, walterStandinName,
                      prevWalterStandinName=None):
        """Update dynamically the tree model."""

        # Gets the walter standin root item
        walterStandinRootItem, walterStandinRootItemIndex = \
            self.__getWalterStandinRootItem(walterStandinName)

        if walterStandinRootItem:
            if action == WALTER_UPDATE_ITEMS:
                self.__addOrRemoveItems(walterStandinRootItem)

            elif action == WALTER_REMOVE_ROOT_ITEM:
                self.__removeItem(self.rootItem, walterStandinRootItem,
                                  walterStandinRootItemIndex)
        else:
            if action == WALTER_ADD_ROOT_ITEM:
                self.__addItem(self.rootItem, walterStandinName)

            elif action == WALTER_RENAME_ROOT_ITEM and prevWalterStandinName:
                for i in range(self.rootItem.childCount()):
                    originItem = self.rootItem.child(i)
                    if originItem.getName() == prevWalterStandinName:
                        originItem.setName(walterStandinName)
                        break

        self.dataChanged.emit(
            QtCore.QModelIndex(), QtCore.QModelIndex())

    def __exposeOrDetachTransforms(self, action, walterStandinName,
                                    primPath=None):
        found = self.rootItem.find(origin=walterStandinName, path=primPath)
        index = self.indexFromItem(found)

        if index:
            if action == WALTER_DETACH_TRANSFORM:
                self.detachOverride([index], OVERRIDE_TRANSFORM)

            elif action == WALTER_ATTACH_TRANSFORM:
                self.exposeTransforms([index])

    def __addOrRemoveItems(self, walterStandinRootItem):
        walterRootItem = walterStandinRootItem.child(0)

        treeItems = []
        treeItemsName = []
        for i in range(walterRootItem.childCount()):
            item = walterRootItem.child(i)
            treeItems.append(item)
            treeItemsName.append('/' + item.getName())

        expItems = []
        expItemsName = []
        for i in range(walterStandinRootItem.childCount()):
            item = walterStandinRootItem.child(i)
            expItems.append(item)
            expItemsName.append(item.getName())

        expressions = self.traverser.expressions(
            walterStandinRootItem.getName())

        expressionGroups = self.traverser.expressionGroups(
            walterStandinRootItem.getName())

        sceneObjectNames = self.traverser.dir(
            walterStandinRootItem.getName(), '/')

        self.__addOrRemoveSceneItems(walterStandinRootItem,
                                     walterRootItem,
                                     treeItems,
                                     treeItemsName,
                                     sceneObjectNames)

        self.__addOrRemoveExpressionItems(walterStandinRootItem,
                                          expItems,
                                          expItemsName,
                                          expressions,
                                          expressionGroups)

    def __addOrRemoveSceneItems(self, rootItem, walterRootItem,
                                treeItems, treeItemsName, sceneObjectNames):

        removeIndices = []
        treeItemsToRemove = []
        for idx, item in enumerate(treeItems):
            if treeItemsName[idx] not in sceneObjectNames:
                treeItemsToRemove.append(item)
                removeIndices.append(idx)

        treeItemsToAdd = []
        for item in sceneObjectNames:
            if item not in treeItemsName:
                treeItemsToAdd.append(item)

        if len(treeItemsToRemove) > 0:
            self.__removeItems(walterRootItem,
                               treeItemsToRemove,
                               removeIndices)

        if len(treeItemsToAdd) > 0:
            self.__addItems(rootItem.getName(),
                            walterRootItem,
                            treeItemsToAdd,
                            len(treeItems))

            walterRootItem.childrenObtained = True

    def __addOrRemoveExpressionItems(self, walterStandinRootItem, treeItems,
                                     treeItemsName, expressions,
                                     expressionGroups):

        removeIndex = []
        treeItemToRemove = []
        for idx, treeItem in enumerate(treeItems):
            if treeItem.getType() == TreeItem.TYPE_EXPRESSION \
               and treeItem.getName() not in expressions:
                treeItemToRemove.append(treeItem)
                removeIndex.append(idx)

        for idx, treeItem in enumerate(treeItems):
            if treeItem.getType() == TreeItem.TYPE_EXPGROUP \
               and treeItem.getName() not in expressionGroups:
                treeItemToRemove.append(treeItem)
                removeIndex.append(idx)

        if treeItemToRemove:

            zipped = zip(removeIndex, treeItemToRemove)
            zipped = sorted(zipped, reverse=True)
            removeIndex, treeItemToRemove = zip(*zipped)

            for i in range(len(removeIndex)):
                self.__removeItem(walterStandinRootItem,
                                  treeItemToRemove[i],
                                  removeIndex[i])

        else:
            treeItemsToAdd = None
            for item in expressions:
                if item not in treeItemsName:
                    treeItemsToAdd = item
                    break

            if treeItemsToAdd:
                self.__addItem(walterStandinRootItem,
                               treeItemsToAdd,
                               isExpression=True)

                for override in OVERRIDES:
                    self.setOverrides(override)

    def __addItem(self, parentItem, name, isExpression=False):
        self.beginInsertRows(self.indexFromItem(parentItem),
                             parentItem.childCount(),
                             parentItem.childCount() + 1)

        walterRoot = None

        if isExpression:
            TreeItem(parentItem,
                     self.traverser,
                     expression=name)

        else:
            walterRoot = TreeItem(parentItem,
                                  self.traverser,
                                  originObject=name)

        self.endInsertRows()

        if walterRoot:
            walterRoot.obtainChildren()
            for override in OVERRIDES:
                self.setOverrides(override)

    def __removeItem(self, parentItem, item, index):
        self.beginRemoveRows(self.indexFromItem(parentItem),
                             index,
                             index)

        parentItem.removeChild(item)

        self.endRemoveRows()

    def __removeItems(self, parentItem, items, indices):
        if len(items) == 0:
            return

        self.beginRemoveRows(self.indexFromItem(parentItem),
                             indices[0],
                             indices[len(indices) - 1])

        for item in items:
            parentItem.removeChild(item)

        self.endRemoveRows()

    def __addItems(self, parentItemName, parentItem,
                   items, index):

        if len(items) == 0:
            return

        newParentsItem = []
        self.beginInsertRows(self.indexFromItem(parentItem),
                             index,
                             index + len(items))

        for item in items:
            newParentItem = TreeItem(parentItem,
                                     self.traverser,
                                     alembicObject=item)

            newParentsItem.append(newParentItem)
            newParentItem.childrenObtained = True

        self.endInsertRows()

        for newParentItem in newParentsItem:
            newItems = self.traverser.dir(parentItemName,
                                          newParentItem.alembicObject)
            self.__addItems(parentItemName,
                            newParentItem,
                            newItems,
                            0)

    def data(self, index, role=QtCore.Qt.DisplayRole):
        """
        Return the data stored under the given role for the item referred to by
        the index.
        """
        if not index.isValid():
            return
        item = index.internalPointer()
        if role == QtCore.Qt.DisplayRole or role == QtCore.Qt.EditRole:
            return item.getName()
        elif role == QtCore.Qt.SizeHintRole:
            return item.getHeight()
        elif role == QtCore.Qt.BackgroundRole:
            return item.getBackgroundColor()
        elif role == CHILD_COUNT:
            return item.childCount()
        elif role == ACTIONS:
            return item.getActions(self.treeView().getLayer())
        elif role == TRANSFORM:
            return item.getOverride(
                self.treeView().getLayer(), OVERRIDE_TRANSFORM)
        elif role == SHADER:
            return item.getOverride(self.treeView().getLayer(), OVERRIDE_SHADER)
        elif role == DISPLACEMENT:
            return item.getOverride(
                self.treeView().getLayer(), OVERRIDE_DISPLACEMENT)
        elif role == ATTRIBUTE:
            return item.getOverride(
                self.treeView().getLayer(), OVERRIDE_ATTRIBUTE)
        elif role == NODE_BAR_COLOUR:
            return item.getLabelColor()
        else:
            return super(TreeModel, self).data(index, role)

    def setData(self, index, value, role=QtCore.Qt.EditRole):
        """Set the role data for the item at index to value."""
        if not index.isValid():
            return False

        if role == QtCore.Qt.EditRole and value:
            item = index.internalPointer()
            originItem = item.getOriginItem()

            self.traverser.rename(originItem.getName(), item.getName(), value)

            item.setName(value)

            return True

        elif role == SHADER:
            item = index.internalPointer()
            item.setOverride(value, self.treeView().getLayer(), OVERRIDE_SHADER)
            self.dataChanged.emit(index, QtCore.QModelIndex())
            return True

        elif role == DISPLACEMENT:
            item = index.internalPointer()
            item.setOverride(
                value, self.treeView().getLayer(), OVERRIDE_DISPLACEMENT)
            self.dataChanged.emit(index, QtCore.QModelIndex())
            return True

        elif role == ATTRIBUTE:
            item = index.internalPointer()
            item.setOverride(
                value, self.treeView().getLayer(), OVERRIDE_ATTRIBUTE)
            self.dataChanged.emit(index, QtCore.QModelIndex())
            return True

        elif role == TRANSFORM:
            item = index.internalPointer()
            item.setOverride(
                value, self.treeView().getLayer(), OVERRIDE_TRANSFORM)
            self.dataChanged.emit(index, QtCore.QModelIndex())
            return True

        elif role == SHADER_ICON:
            item = index.internalPointer()
            item.setSurfaceIconColor(value)
            self.dataChanged.emit(index, QtCore.QModelIndex())
            return True

        return False

    def setOverrides(self, overrideType):
        """Initialize materials in the tree items."""
        for i in range(self.rootItem.childCount()):
            originItem = self.rootItem.child(i)
            originName = originItem.getName()

            # All the expressions of the node
            expressions = self.traverser.expressions(originName)

            # We need the default layer for the transform overrides.
            layers = self.traverser.layers(originName) or ['defaultRenderLayer']

            expressionsToCreate = {}

            for layer in layers:
                overrides = self.traverser.overrides(
                    originName, layer, overrideType)

                if not overrides:
                    continue

                for shader, objects in overrides.iteritems():
                    for path in objects:
                        item = originItem.find(
                            path=path,
                            skipAlembic=path in expressions)

                        if not item:
                            # If the item is not found, then the path is the
                            # expression, we add it to a dictionnary to set override later.
                            expressionsToCreate[path] = (shader, layer)
                            continue
                        item.setOverride(shader, layer, overrideType)

            for expr in expressions:
                if expr in expressionsToCreate:
                    pair = expressionsToCreate[expr]
                    item = TreeItem(
                                originItem, self.traverser, expression=expr)
                    item.setOverride(pair[0], pair[1], overrideType)

    def detachOverride(self, indices, overrideType):
        """Remove the shader from the index."""
        for index in indices:
            overrideName = self.data(index, TreeModel.dataType(overrideType))
            self.setData(index, None, TreeModel.dataType(overrideType))

            item = index.internalPointer()
            originItem = item.getOriginItem()

            if item.getType() == TreeItem.TYPE_EXPGROUP:
                # When group, detach override on every children.
                # No need to detach override on the group itself (as there is no real override set on it)
                childIndices = [self.indexFromItem(childItem) for childItem in item.childItems]
                self.detachOverride(childIndices, overrideType)

            else:
                # TODO: Don't use item.alembicObject directly
                self.traverser.detachOverride(
                    originItem.getName(),
                    self.treeView().getLayer(),
                    item.alembicObject or item.expression,
                    overrideType,
                    overrideName)

    def exposeTransforms(self, indices):
        """Create locators for each object and its parent."""
        root = self.rootItem


        for index in indices:
            item = index.internalPointer()
            originItem = item.getOriginItem()
            originName = originItem.getName()


            transforms = \
                self.traverser.exposeTransforms(originName, item.alembicObject)


            over = item.getOverride(self.treeView().getLayer(),
                                    OVERRIDE_TRANSFORM)


            for subObject, transform in transforms.iteritems():

                # Find the object.
                found = root.find(origin=originName, path=subObject)
                if not found:
                    continue

                # Get the index of the item.
                index = self.indexFromItem(found)

                self.setData(index, transform, TRANSFORM)

    def createOverride(self, indices, overrideType):
        """Create the override."""
        override = self.traverser.create(overrideType)
        if not override:
            return

        for index in indices:
            self.setData(index, override, TreeModel.dataType(overrideType))

            item = index.internalPointer()
            originItem = item.getOriginItem()

            # "Maya" part. Settin goverride is not automatic. If it's a group,
            # we need to set override to all the children of the group.
            if item.getType() == TreeItem.TYPE_EXPGROUP:
                for childItem in item.childItems:
                    self.traverser.setOverride(
                        originItem.getName(),
                        self.treeView().getLayer(),
                        childItem.expression,
                        override,
                        overrideType)
            else:
                self.traverser.setOverride(
                    originItem.getName(),
                    self.treeView().getLayer(),
                    item.alembicObject or item.expression,
                    override,
                    overrideType)

    def removeExpression(self, indices):
        """Removes an expression from the tree view."""

        if not indices:
            return

        item = indices[0].internalPointer()
        walterStandinName = item.getOriginItem().getName()

        rows = []
        items = []
        for index in indices:
            rows.append(index.row())
            item = index.internalPointer()
            items.append(item)
            self.traverser.remove(walterStandinName, item.getName())

        # Sort to remove the rows with higher index first.
        zipped = zip(rows, items)
        zipped = sorted(zipped, reverse=True)
        rows, items = zip(*zipped)

        for i in range(len(rows)):
            self.__removeItem(items[i].parent(), items[i], rows[i])

    @staticmethod
    def dataType(overrideType):
        """Convert override type (OVERRIDE_SHADER) to data type (SHADER)."""
        if overrideType == OVERRIDE_DISPLACEMENT:
            return DISPLACEMENT
        elif overrideType == OVERRIDE_ATTRIBUTE:
            return ATTRIBUTE
        elif overrideType == OVERRIDE_TRANSFORM:
            return TRANSFORM
        return SHADER

    def saveAssignment(self, index):
        """Save assignment of the origin object to the external file."""
        self.__saveLayer(index, self.traverser.saveAssignment)

    def saveAttributes(self, index):
        """Save assignment of the origin object to the external file."""
        self.__saveLayer(index, self.traverser.saveAttributes)

    def saveMaterials(self, index):
        """Save materials of the origin object to the external file."""
        self.__saveLayer(index, self.traverser.saveMaterials)

    def saveTransforms(self, index):
        """Save transforms of the origin object to the external file."""
        self.__saveLayer(index, self.traverser.saveTransforms)

    def savePurposes(self, index):
        """Save transforms of the origin object to the external file."""
        self.__saveLayer(index, self.traverser.savePurposes)

    def saveVariantsLayer(self, index):
        """Save variant layer of the origin object to the external file."""
        self.__saveLayer(index, self.traverser.saveVariantsLayer)

    def __saveLayer(self, index, func):
        if not index.isValid():
            return

        item = index.internalPointer()
        if item.getType() != TreeItem.TYPE_ORIGIN:
            return

        func(item.getName())

    def executeAction(self, action, index):
        """User pressed by one of the actions."""
        item = index.internalPointer()

        if action == TreeItem.ACTION_SURFACE:
            self.traverser.action(
                item.getDerivedOverride(
                    self.treeView().getLayer(), OVERRIDE_SHADER))
        elif action == TreeItem.ACTION_DISPLACEMENT:
            self.traverser.action(
                item.getDerivedOverride(
                    self.treeView().getLayer(), OVERRIDE_DISPLACEMENT))
        elif action == TreeItem.ACTION_ATTRIBUTE:
            self.traverser.action(
                item.getDerivedOverride(
                    self.treeView().getLayer(), OVERRIDE_ATTRIBUTE))
        elif action == TreeItem.ACTION_TRANSFORM:
            self.traverser.action(
                item.getDerivedOverride(
                    self.treeView().getLayer(), OVERRIDE_TRANSFORM))
        elif action == TreeItem.ACTION_FREEZETRANSFORMS:
            self.traverser.invertFrozen(item.getName())
        elif action == TreeItem.ACTION_ADDEXPRESSION:
            # The problem here is we shouldn't create an expression that already
            # exists. RND-474. So we need unique expressions that are generated
            # with pre-order iteration of the current tree.

            # Pre-order tree iteration
            past = []
            future = [""]

            while future:
                current = future.pop(0)

                # The expression.
                newExpression = "{obj}(/.*)?".format(obj=current)
                # Check if it exists.
                if not item.find(path=newExpression, skipAlembic=True):
                    break

                past.append(current)

                while past and not future:
                    current = past.pop(0)
                    # TODO: we querry the scene but we need to querry TreeItem,
                    # it will be much faster.
                    fromMaya = self.traverser.dir(item.getName(), current)

                    if not fromMaya:
                        continue

                    future += fromMaya

            myCount = item.childCount()
            self.beginInsertRows(
                index, myCount, myCount)
            expr = TreeItem(item, self.traverser, expression=newExpression)
            self.endInsertRows()
            self.traverser.setGroup(item.getName(), newExpression, "")
            self.traverser.setExpressionWeight(item.getName(),
                                               newExpression,
                                               myCount)
        elif action == TreeItem.ACTION_REFRESH:
            self.traverser.refresh(item.getName())
        elif action == TreeItem.ACTION_EXPAND:
            self.treeView().setExpanded(
                index, not self.treeView().isExpanded(index))

    def executeContextAction(self, action, index, point):
        """User pressed with right button by one of the actions."""
        self.treeView().showContextMenu(point, action)

    def groupExpressions(self, indices, groupIndex=None):
        # Get origin
        item = indices[0].internalPointer()
        originItem = item.getOriginItem()
        originIndex = self.indexFromItem(originItem)

        if groupIndex:
            group = groupIndex.internalPointer()
            groupName = group.getName()
        else:
            # Create group
            groupTemplate = "Group%02i"
            groupNumber = 0

            # Get the unique group name
            while True:
                groupNumber += 1
                groupName = groupTemplate % groupNumber
                isUnique = True

                for c in range(originItem.childCount()):
                    child = originItem.child(c)

                    if child.getType() != TreeItem.TYPE_EXPGROUP:
                        continue

                    if child.getName() == groupName:
                        isUnique = False
                        break

                if isUnique:
                    break
            # Get the location index for the newly created group
            grpIndex = originItem.groupIndexLocator(groupName)
            # Create node
            self.beginInsertRows(originIndex, grpIndex, grpIndex)
            group = TreeItem(
                originItem, self.traverser, expressionGroup=groupName, indexLocation=grpIndex)
            self.endInsertRows()

            groupIndex = self.indexFromItem(group)
            # Expand group
            self.treeView().setExpandedChildren(groupIndex, True)

        # Just to be sure that we have all the children obtained. If we don't do
        # it now, the group item will not have flag obtained and the group will
        # recreate oll the items it has when querying. We don't need duplicates.
        group.obtainChildren()

        # Get origin item and current render layer
        origin = group.getOriginItem().getName()
        layer = self.treeView().getLayer()

        # Get any overrides that the group item could have.
        # We want to apply those overrides to every sub-item in this group.
        groupOverrides = {}
        overrides = [OVERRIDE_ATTRIBUTE, OVERRIDE_DISPLACEMENT, OVERRIDE_SHADER]
        for overrideType in overrides:
            override = group.getOverride(layer, overrideType)
            if override:
                groupOverrides[overrideType] = override

        # Move the expressions
        for index in indices:
            item = index.internalPointer()
            indexRow = index.row()
            if index.parent().internalPointer().getType() == TreeItem.TYPE_ORIGIN:
                # Since we create a group item above the expressions in the tree,
                # the first expression after the groups, row n, becomes row n + 1.
                # To this point we haven't updated that index list so we need to increment
                # the current index row to point the correct item when endMoveRows() is invoked.
                indexRow += 1

            self.beginMoveRows(
                index.parent(),
                indexRow,
                indexRow,
                groupIndex,
                group.childCount())

            # Reparent
            # TODO: Don't use childItems directly
            item.parent().childItems.remove(item)
            item.setParent(group)

            # Re-apply overrides from the group to each item.
            for overrideType, override in groupOverrides.items():
                item.setOverride(override, layer, overrideType)

                self.traverser.setOverride(origin,
                                           layer,
                                           item.alembicObject or item.expression,
                                           override,
                                           overrideType)

            self.traverser.setGroup(
                originItem.getName(), item.getName(), groupName)

            self.endMoveRows()
        # Clear selection
        self.treeView().clearSelection()

    def unGroupExpressions(self, indices):
        # Get origin
        item = indices[0].internalPointer()
        originItem = item.getOriginItem()
        originItemName = originItem.getName()
        originIndex = self.indexFromItem(originItem)

        # Move the expressions
        for index in indices:
            item = index.internalPointer()
            self.beginMoveRows(
                index.parent(),
                index.row(),
                index.row(),
                originIndex,
                originItem.childCount())

            if item.parent().getName() == originItemName:
                # In case of ungrouping an expression that doesn't belong to a group.
                return

            # Reparent
            # TODO: Don't use childItems directly
            item.parent().childItems.remove(item)
            item.setParent(originItem)

            self.traverser.setGroup(
                originItem.getName(), item.getName(), "")

            self.endMoveRows()
        self.treeView().clearSelection()

    def reOrderExpression(self, source, row):
        """Handle reordering of Expressions """
        index = self.indexFromItem(source)
        originName = source.getOriginItem().getName()
        parentIndex = index.parent()
        parent = parentIndex.internalPointer()

        positionFrom = index.row()
        positionTo = row

        if positionFrom == positionTo or positionFrom == positionTo - 1:
            # The user drops the item to the position right before or right
            # after this item. Nothing to do.
            return

        self.beginMoveRows(parentIndex,
                           positionFrom,
                           positionFrom,
                           parentIndex,
                           positionTo)

        if positionFrom < positionTo:
            # We move the item to the buttom, it means we extract this item
            # from the list and the new position should be decreased.
            # List:
            # [a, b, c, d]
            # If we move a to the end of the list, we extract #1 first and the
            # list became like this:
            # [b, c, d]
            # Thus, we need to insert it after #3:
            # [b, c, d, a]
            # Qt detects it, that's why we do it after beginMoveRows
            positionTo -= 1

        # TODO: don't use variables of an object directly
        parent.childItems.insert(positionTo, parent.childItems.pop(positionFrom))

        counter = 0
        for i in parent.childItems:
            if i.getType() == TreeItem.TYPE_EXPRESSION:
                self.traverser.setExpressionWeight(originName, i.getName(), counter)
            counter += 1

        self.endMoveRows()

    def mimeData(self, indexes):
        """
        Return an object that contains serialized items of data corresponding
        to the list of indexes specified.
        """
        mimeData = QtCore.QMimeData()
        encodedData = QtCore.QByteArray()

        for index in indexes:
            if index.isValid() and index.column() == 0:
                item = index.internalPointer()
                encodedData = QtCore.QByteArray(item.getName())
                mimeData.setData(self.MIME_TYPE, encodedData)
                break

        return mimeData

    def mimeTypes(self):
        """Return the list of MIME types allowed for drop."""
        return [self.MIME_TYPE]

    def dropMimeData(self, data, action, row, column, parentIndex):
        """
        Handle the data supplied by a drag and drop operation that ended with
        the given action.
        """
        if action == QtCore.Qt.IgnoreAction:
            return True

        # Index/Item to drop at. We can't use row/column because it looks like
        # it's a random value. Qt passes each time different value and it's not
        # possible to know what the user droped at. Another options is getting
        # selection.
        # We can get the Index at the dropped position
        indices = [self.treeView().dropIndex]
        if not indices or len(indices) != 1:
            # We can't drop to several items.
            return False

        # Index/Item to drop at.
        index = indices[0]
        item = index.internalPointer()

        if data.hasFormat(TreeModel.MIME_TYPE):
            # It's expression item
            expression = data.data(self.MIME_TYPE).data()

            if not expression:
                return False

            # Trying to find it in the current origin
            originItem = item.getOriginItem()
            # Find the expression item
            expressionItem = originItem.find(path=expression, skipAlembic=True)

            if expressionItem:
                if expressionItem.parent() == item.parent():
                    # We only need to reorder in case of parents are equals.
                    # Expression and Item are in the same tree parent.
                    if item.getType() != TreeItem.TYPE_EXPGROUP:
                        self.reOrderExpression(expressionItem, row)
                        return True

                if expressionItem.parent().getType() == TreeItem.TYPE_EXPGROUP:
                    if expressionItem.parent() == item:
                        # Don't do anything if the expression is dropped to the
                        # group it already belongs to.
                        return False

                    if item.getType() == TreeItem.TYPE_EXPGROUP:
                        # When drop Item is a different group than the parent.
                        # Change Group of the expression
                        self.groupExpressions(
                            [self.indexFromItem(expressionItem)], index)
                        return True

                    if item.getType() == TreeItem.TYPE_ORIGIN:
                        # When expression dropped at Origin and belongs to a Group,
                        # Then ungroup.
                        self.unGroupExpressions([self.indexFromItem(expressionItem)])
                        return True

                    if item.parent().getType() == TreeItem.TYPE_ORIGIN:
                        # When Dropped on an expression outside of the group.
                        # Ungroup and reoder the list according to the drop index.
                        self.unGroupExpressions([self.indexFromItem(expressionItem)])
                        self.reOrderExpression(expressionItem, row)
                        return True

                if item.getType() == TreeItem.TYPE_EXPRESSION:
                    if item.parent().getType() == TreeItem.TYPE_EXPGROUP:
                        # We allow drop an expression directly inbetween grouped expressions.
                        # This will Group and reorder
                        self.groupExpressions(
                            [self.indexFromItem(expressionItem)], self.indexFromItem(item.parent()))
                        self.reOrderExpression(expressionItem, row)
                        return True

                if item.getType() == TreeItem.TYPE_EXPGROUP:
                    # When expressions are dropped on a Group.
                    # group
                    self.groupExpressions(
                        [self.indexFromItem(expressionItem)], index)
                    return True

            else:
                # If wrong Origin don't do anything
                # This will avoid creating a new expression in other walterStandin on Drag&Drop
                return False

            return True

        elif data.hasFormat('application/x-maya-data') and data.hasText():
            # Get item
            originItem = item.getOriginItem()

            if item.getType() == TreeItem.TYPE_EXPGROUP:
                itemsToSet = [item.child(i) for i in range(item.childCount())]
            else:
                itemsToSet = [item]

            originName = originItem.getName()
            layer = self.treeView().getLayer()

            # Dropped maya object. We need to create an override.
            mayaObject = data.text()
            overrides = self.traverser.produceOverrides(mayaObject)

            for override, overrideType in overrides:
                # Set tree view first
                self.setData(index, override, TreeModel.dataType(overrideType))

                # Set override for all the necessary objects
                for i in itemsToSet:
                    self.traverser.setOverride(
                        originName,
                        layer,
                        i.alembicObject or i.expression,
                        override,
                        overrideType)

            return True

        return False


class TreeViewDelegate(BaseDelegate):
    """Custom display delegate for the tree items."""

    EXPANDED_ARROW = (
        dpiScale(QtCore.QPointF(12.0, -2.5)),
        dpiScale(QtCore.QPointF(22.0, -2.5)),
        dpiScale(QtCore.QPointF(17.0, 2.5)))
    COLLAPSED_ARROW = (
        dpiScale(QtCore.QPointF(15.0, -5.0)),
        dpiScale(QtCore.QPointF(20.0, 0.0)),
        dpiScale(QtCore.QPointF(15.0, 5.0)))
    ARROW_COLOR = QtGui.QColor(189, 189, 189)

    def paint(self, painter, option, index):
        """Main entry point of drawing the cell."""
        if not index.isValid():
            return

        item = index.internalPointer()

        isVisible = False
        if item.itemType == TreeItem.TYPE_ALEMBIC:
            isVisible = pm.walterStandin(
                isVisible=(item.getOriginItem().getName(), item.alembicObject))
        else:
            isVisible = item.isVisible

        if isVisible:
            self.textColor = self.treeView().palette().text().color()
        else:
            self.textColor = item.COLOR_TEXT_HIDDEN

        super(TreeViewDelegate, self).paint(painter, option, index)

        rect = deepcopy(option.rect)
        self.drawArrowDragLock(painter, rect, index)

    def drawArrowDragLock(self, painter, rect, index):
        """Draw the expansion arrow on the nodes that want it."""
        painter.save()

        arrow = None
        if index.data(CHILD_COUNT):
            center = index.data(QtCore.Qt.SizeHintRole).height() / 2
            painter.translate(rect.left(), rect.top() + center)
            # Draw the arrow
            if self.treeView().isExpanded(index):
                arrow = self.EXPANDED_ARROW
            else:
                arrow = self.COLLAPSED_ARROW

            painter.setBrush(self.ARROW_COLOR)
            painter.setPen(QtCore.Qt.NoPen)
            painter.drawPolygon(arrow)

            cursorPosition = self.treeView().mapFromGlobal(QtGui.QCursor.pos())
            if rect.contains(cursorPosition):
                x = cursorPosition.x()
                arrowPoints = [p.x() for p in arrow]
                minX = min(arrowPoints) + rect.left() - EXPAND_SENSITIVITY
                maxX = max(arrowPoints) + rect.left() + EXPAND_SENSITIVITY
                if x >= minX and x <= maxX:
                    # Save the action to expand
                    self.lastHitAction = TreeItem.ACTION_EXPAND

        painter.restore()

    def updateEditorGeometry(self, editor, option, index):
        """Update the editor's rectangle for the item specified by index."""
        super(TreeViewDelegate, self).updateEditorGeometry(
            editor, option, index)

        if not index.isValid():
            return

        item = index.internalPointer()

        # We need to update the viewport each time the user types something in
        # the editor.
        editor.textChanged.connect(
            lambda t, o=item.getOriginObject():
            self.treeView().highlightViewport(o, t))


class TreeItem(BaseItem):
    """
    A Python object used to return row/column data, and keep note of it's
    parents and/or children. It's a tree data structure with parent and
    children.
    """

    # TODO: Resolve the circular references between objects to be able to delete
    # all the objects in the structure. The circular references between objects
    # may prevent the objects from deleting. It can only be remedied by
    # explicitly breaking the cycles.

    SHADERBALL_ICONS = {}

    # Colors
    COLOR_ORIGIN = QtGui.QColor(113, 142, 164)
    COLOR_ALEMBIC = QtGui.QColor(73, 109, 137)
    COLOR_HIDDEN_ALEMBIC = QtGui.QColor(73, 73, 73)
    COLOR_TEXT_HIDDEN = QtGui.QColor(135, 135, 135)
    COLOR_EXPRESSION = QtGui.QColor(18, 54, 82)
    COLOR_INVALID_EXPRESSION = QtGui.QColor(255, 54, 82)
    COLOR_EXPGROUP = QtGui.QColor(4, 32, 55)
    COLOR_UNDEFINED = QtGui.QColor(0, 0, 0)

    # Enum
    (ACTION_NONE,
     ACTION_TRANSFORM,
     ACTION_SURFACE,
     ACTION_DISPLACEMENT,
     ACTION_ATTRIBUTE,
     ACTION_FREEZETRANSFORMS,
     ACTION_ADDEXPRESSION,
     ACTION_REFRESH,
     ACTION_EXPAND) = range(9)

    # Type enum
    (TYPE_UNDEFINED,
     TYPE_ORIGIN,
     TYPE_ALEMBIC,
     TYPE_EXPRESSION,
     TYPE_EXPGROUP) = range(5)

    OVERRIDES_THAT_LOCK = [
        OVERRIDE_SHADER_ABC,
        OVERRIDE_DISPLACEMENT_ABC,
        OVERRIDE_ATTRIBUTE_ABC]

    def __init__(
            self,
            parentItem,
            traverser,
            originObject=None,
            alembicObject=None,
            expression=None,
            expressionGroup=None,
            indexLocation=None):

        self.ALEMBIC_ICON = QtGui.QPixmap(":/out_objectSet.png")
        self.GROUP_ICON = QtGui.QPixmap(":/out_transform.png")
        self.MESH_ICON = QtGui.QPixmap(":/out_mesh.png")
        self.EXPRESSION_ICON = QtGui.QPixmap(":/out_shaderList.png")
        self.EXPGROUP_ICON = QtGui.QPixmap(":/out_displayLayer.png")
        self.TRANSFORM_ICON = BaseItem.dpiScaledIcon(":/out_locator.png")
        self.SURFACE_DEFAULT_ICON = \
            BaseItem.coloredIcon(":/out_blinn.png", QtGui.QColor(240, 240, 240, 255))
        self.DISPLACEMENT_ICON = BaseItem.dpiScaledIcon(":/out_displacementShader.png")
        self.ATTRIBUTE_ICON = BaseItem.dpiScaledIcon(":/out_list.png")
        self.EDITABLE_ATTRIBUTE_ICON = \
            BaseItem.coloredIcon(":/out_list.png", QtGui.QColor(113, 142, 164, 255))
        self.ADD_ICON = BaseItem.dpiScaledIcon(":/nodeGrapherAddNodes.png")
        self.REFRESH_ICON = BaseItem.dpiScaledIcon(":/refreshGray.png")
        self.REMOVE_ICON = BaseItem.dpiScaledIcon(":/deleteActive.png")
        self.LOCK_ICON = BaseItem.dpiScaledIcon(":/Lock_ON.png")

        """Called after the instance has been created."""
        self.traverser = traverser
        self.childItems = []
        self.childrenObtained = False
        self.overrides = {o: {} for o in OVERRIDES}
        if expressionGroup:
            self.setParent(parentItem, indexLocation)
        else:
            self.setParent(parentItem)

        self.originObject = originObject
        self.alembicObject = alembicObject
        self.expression = expression
        self.expressionGroup = expressionGroup

        # RND-584: In the case an item represents
        # a regex, the expression might invalid.
        self.isValidExpression = True

        # Find out the type of this tree item
        if self.originObject:
            self.itemType = self.TYPE_ORIGIN
        elif self.alembicObject:
            self.itemType = self.TYPE_ALEMBIC
        elif self.expression:
            self.itemType = self.TYPE_EXPRESSION
        elif self.expressionGroup:
            self.itemType = self.TYPE_EXPGROUP
        else:
            self.itemType = self.TYPE_UNDEFINED

        self.surfaceIconColor = "default"

        self.isVisible = True
        self.isPseudoRoot = False
        if self.alembicObject:
            self.isPseudoRoot = pm.walterStandin(isPseudoRoot=(
                self.getOriginItem().getName(), self.alembicObject))

    def getName(self):
        """The label of the item."""
        if self.getType() == self.TYPE_ORIGIN:
            return self.originObject
        elif self.getType() == self.TYPE_ALEMBIC:
            if self.alembicObject == "/":
                # It's a root node
                return "Walter Root"

            return self.alembicObject.split('/')[-1]
        elif self.getType() == self.TYPE_EXPRESSION:
            return self.expression
        elif self.getType() == self.TYPE_EXPGROUP:
            return self.expressionGroup

        return "Unknown"

    def setName(self, name):
        """Set the name of the item. It can be an expression or a group."""
        if self.getType() == self.TYPE_ORIGIN:
            self.originObject = name
        if self.getType() == self.TYPE_EXPRESSION:
            self.expression = name
        elif self.getType() == self.TYPE_EXPGROUP:
            self.expressionGroup = name

    def getType(self):
        """Return the type of this tree item."""
        return self.itemType

    def getBackgroundColor(self):
        """
        The background color of current node. It can be different depending on
        the item type.
        """
        return QtGui.QColor(71, 71, 71)

    def getLabelColor(self):
        """
        Return the color of the  of the current item. It can be different
        depending on the item type.
        """
        if self.getType() == self.TYPE_ORIGIN:
            return self.COLOR_ORIGIN
        elif self.getType() == self.TYPE_ALEMBIC:

            isVisible = pm.walterStandin(isVisible=(
                self.getOriginItem().getName(), self.alembicObject))

            if self.isPseudoRoot or isVisible:
                return self.COLOR_ALEMBIC
            else:
                return self.COLOR_HIDDEN_ALEMBIC

        elif self.getType() == self.TYPE_EXPRESSION:

            if not self.isVisible:
                return self.COLOR_HIDDEN_ALEMBIC

            if self.isValidExpression:
                return self.COLOR_EXPRESSION
            else:
                return self.COLOR_INVALID_EXPRESSION

        elif self.getType() == self.TYPE_EXPGROUP:

            if not self.isVisible:
                return self.COLOR_HIDDEN_ALEMBIC
            return self.COLOR_EXPGROUP

        return self.COLOR_UNDEFINED

    def getHeight(self):
        """
        Return the height of the current item. It can be different depending on
        the item type.
        """
        if self.originObject:
            height = ITEM_HEIGHT * 4 / 3
        else:
            height = ITEM_HEIGHT

        return QtCore.QSize(250, height)

    def getIcon(self):
        """
        The icon of current node. It can be different depending on the item
        type.
        """
        if self.getType() == self.TYPE_ORIGIN:
            return self.ALEMBIC_ICON
        elif self.getType() == self.TYPE_ALEMBIC:
            if self.childItems:
                return self.GROUP_ICON

            # If it doesn't have children, it's a mesh
            return self.MESH_ICON
        elif self.getType() == self.TYPE_EXPRESSION:
            return self.EXPRESSION_ICON
        elif self.getType() == self.TYPE_EXPGROUP:
            return self.EXPGROUP_ICON

        return None

    def getOriginItem(self):
        """Recursively search the the origin parent TreeItem in parents."""
        if self.originObject:
            return self

        return self.parent().getOriginItem()

    def getWalterStandinItem(self):
        """Recursively search the the origin parent TreeItem in parents."""
        parent = self.parent()
        if not parent.parent():
            return self

        return parent.getWalterStandinItem()

    def getOriginObject(self):
        """Recursively search the the origin object name in parents."""
        return self.getOriginItem().originObject

    def getProperties(self):
        """Gets the properties of that item."""
        properties = []
        if self.getType() == self.TYPE_ALEMBIC:
            originObject = self.getOriginObject()
            properties = self.traverser.props(originObject, self.alembicObject)

        return properties

    def obtainChildren(self):
        """Load child items if they are not loaded yet."""
        if self.childrenObtained:
            return

        if self.getType() == self.TYPE_ORIGIN:
            TreeItem(self, self.traverser, alembicObject='/')
            children = self.traverser.expressionGroups(self.originObject)
            if children:
                for child in sorted(children):
                    TreeItem(self, self.traverser, expressionGroup=child)
        elif self.getType() == self.TYPE_ALEMBIC:
            originObject = self.getOriginObject()
            children = self.traverser.dir(originObject, self.alembicObject)
            if children:
                for child in children:
                    TreeItem(self, self.traverser, alembicObject=child)
        elif self.getType() == self.TYPE_EXPRESSION:
            # TODO: We are not decided yet what exactly should be inside
            # expression
            pass
        elif self.getType() == self.TYPE_EXPGROUP:
            originObject = self.getOriginObject()
            children = self.traverser.expressions(
                originObject, self.expressionGroup)
            if children:
                for child in children:
                    TreeItem(self, self.traverser, expression=child)
        else:
            # It's an invisible root node. We need to get all the alembic
            # objects and pre-load their children.
            nodes = self.traverser.origins()
            if nodes:
                for node in nodes:
                    item = TreeItem(self, self.traverser, originObject=node)
                    item.obtainChildren()

        self.childrenObtained = True

    def find(self, origin=None, path=None, skipAlembic=False):
        """Find an item with exact path in children."""
        self.obtainChildren()

        if origin:
            # We need to find the origin item first.
            # TODO: it only works if it's the root node.
            for c in self.childItems:
                if c.getOriginObject() == origin:
                    # Continue searching without origin
                    return c.find(path=path, skipAlembic=skipAlembic)

            # Couldn't find origin
            return

        # Searching the path in children
        for c in self.childItems:
            # Check if it's the object we are looking for.
            if path == c.alembicObject or path == c.expression:
                return c

            childType = c.getType()
            if childType == self.TYPE_EXPGROUP:
                # Looking in the expression of this groups
                found = c.find(path=path, skipAlembic=skipAlembic)

                if found:
                    return found

            elif not skipAlembic and childType == self.TYPE_ALEMBIC:
                # True if it's an alembic root item
                isAbcRoot = c.alembicObject == '/'

                # Add / to the end to be sure it's not a part of the object
                # name. Root is an exception because there is no object '//'.
                childrenPath = c.alembicObject + ('' if isAbcRoot else '/')

                if path.startswith(childrenPath):
                    found = c.find(path=path, skipAlembic=skipAlembic)

                    # We should continue if we was unable to find it in the abc
                    # root because it can be an expression item.
                    if not found and isAbcRoot:
                        continue

                    return found

    def hasOverride(self, overrideType):
        if self.getType() == self.TYPE_EXPGROUP:
            # It's a group. We should return an override only on the case all
            # the children overrides are the same. It will show the override
            # button in the UI.
            childCount = self.childCount()
            for i in range(childCount):
                if self.child(i).hasOverride(overrideType):
                    return True
        else:
            if self.overrides[overrideType]:
                return True

        return False

    def getOverride(self, layer, overrideType):
        """Return the override of the current item."""
        if self.getType() == self.TYPE_EXPGROUP:
            # It's a group. We should return an override only on the case all
            # the children overrides are the same. It will show the override
            # button in the UI.
            childCount = self.childCount()
            isSameOverride = True
            # If there are no children, it should return None, exactly what we
            # need.
            override = self.child(0)
            if override:
                override = override.getOverride(layer, overrideType)
            # Check the first override with all the overrides of the children.
            for i in range(1, childCount):
                if override != self.child(i).getOverride(layer, overrideType):
                    isSameOverride = False
                    break

            if isSameOverride:
                return override

        else:
            return self.overrides[overrideType].get(layer)

    def setOverride(self, override, layer, overrideType):
        """Save the override of the given layer."""
        if self.getType() == self.TYPE_EXPGROUP:
            for c in self.childItems:
                c.setOverride(override, layer, overrideType)
        else:
            self.overrides[overrideType][layer] = override

    def getDerivedOverride(self, layer, overrideType):
        """Return the inherited override."""
        currentOverride = self.getOverride(layer, overrideType)
        if currentOverride:
            return currentOverride

        parent = self.parent()
        if parent:
            return parent.getDerivedOverride(layer, overrideType)

    def getActions(self, layer):
        """
        The list of actions to draw on the right side of the item in the tree
        view. It should return the data in the following format:
        [(icon1, opacity1, action1), (icon2, opacity2, action2), ...]
        """
        actions = []
        if self.getType() == self.TYPE_ALEMBIC or \
                self.getType() == self.TYPE_EXPRESSION or \
                self.getType() == self.TYPE_EXPGROUP:
            for override in OVERRIDES:
                # Get opacity of the button
                if self.getOverride(layer, override):
                    opacity = 1.0
                elif self.getDerivedOverride(layer, override):
                    opacity = 0.25
                else:
                    opacity = None

                if opacity:
                    if override == OVERRIDE_SHADER:
                        action = self.ACTION_SURFACE
                        icon = self.getShaderBallIcon()
                    elif override == OVERRIDE_DISPLACEMENT:
                        action = self.ACTION_DISPLACEMENT
                        icon = self.DISPLACEMENT_ICON
                    elif override == OVERRIDE_ATTRIBUTE:
                        action = self.ACTION_ATTRIBUTE
                        icon = self.EDITABLE_ATTRIBUTE_ICON
                    elif override == OVERRIDE_TRANSFORM:
                        action = self.ACTION_TRANSFORM
                        icon = self.TRANSFORM_ICON
                    elif override == OVERRIDE_SHADER_ABC:
                        action = self.ACTION_SURFACE
                        icon = self.SURFACE_DEFAULT_ICON
                        opacity = opacity * 0.7
                    elif override == OVERRIDE_DISPLACEMENT_ABC:
                        action = self.ACTION_DISPLACEMENT
                        icon = self.DISPLACEMENT_ICON
                        opacity = opacity * 0.7
                    elif override == OVERRIDE_ATTRIBUTE_ABC:
                        action = self.ACTION_ATTRIBUTE
                        icon = self.ATTRIBUTE_ICON
                        opacity = opacity * 0.7

                    actions.append((icon, opacity, action))
                else:
                    actions.append((None, None, None))
        elif self.originObject:
            # TODO: Querying Maya each time we need to redraw can be slow. We
            # need to cache it somehow.
            freezeOpacity = \
                0.75 * self.traverser.getFrozen(self.originObject) + 0.25
            actions.append(
                (self.LOCK_ICON, freezeOpacity, self.ACTION_FREEZETRANSFORMS))
            actions.append((self.ADD_ICON, 1.0, self.ACTION_ADDEXPRESSION))
            actions.append((self.REFRESH_ICON, 1.0, self.ACTION_REFRESH))

        return actions

    def flags(self):
        """The item's flags."""
        # We support drops for every type of the tree item. If it's origin or
        # group, we can drop expression, If it's group/expression/object, we can
        # drop material/displacement/override.
        flags = QtCore.Qt.ItemIsDropEnabled

        if self.getType() == self.TYPE_EXPRESSION:
            flags |= QtCore.Qt.ItemIsDragEnabled

            # Don't allow to edit expressions from Alembic.
            locked = False
            for o in self.OVERRIDES_THAT_LOCK:
                if self.overrides[o]:
                    locked = True
                    break

            if not locked:
                flags |= QtCore.Qt.ItemIsEditable
        elif self.getType() == self.TYPE_EXPGROUP:
            flags |= QtCore.Qt.ItemIsEditable

        # We need to return int. Otherwise bitwise operations don't work.
        return flags

    def getIndent(self):
        """The text indent."""
        return dpiScale(40)

    def groupIndexLocator(self, group):
        """Get the position of where the group should be inserted on creation"""
        # It's necessary to find the proper position to insert the group.
        # Normally at this point we have something like this:
        # self
        #  + Walter Root
        #  | + object
        #  + Group01
        #  | + /.*metall.*
        #  + Group02
        #  + /.*
        #
        # We need to insert new group to the place before expressions and we
        # need to keep the alphabetical order of groups.
        position = 0
        for item in self.childItems:
            if item.getType() == self.TYPE_EXPRESSION:
                # We need to break because the groups should always be
                # created before expressions.
                break

            if item.getType() != self.TYPE_EXPGROUP:
                # With this block we are skipping Walter Root and objects.
                position += 1
                continue

            if group < item.getName():
                break
            position += 1
        return position

    def setSurfaceIconColor(self, value):
        """Set the rgb value to the shaderball on the shader."""
        override = self.getOverride('defaultRenderLayer', OVERRIDE_SHADER)
        if override:
            self.traverser.setPreviewColorAttrib(override, value)

    def getSurfaceIconColor(self):
        """Get the stored value of the shaderball."""
        override = self.getOverride('defaultRenderLayer', OVERRIDE_SHADER)
        if override:
            return self.traverser.getPreviewColorAttrib(override)

    def getShaderBallIcon(self):
        """Get the shaderball icon currently set on the item"""
        color = self.getSurfaceIconColor()
        if color is not None:
            icon = self.SHADERBALL_ICONS.get(color, None)
            if not icon:
                icon = BaseItem.coloredIcon(
                    ":/out_blinn.png",
                    QtGui.QColor(color[0], color[1], color[2], 255))
                self.SHADERBALL_ICONS[color] = icon
            return icon

        return self.SURFACE_DEFAULT_ICON
