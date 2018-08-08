"""
Attribute Editor tamplate for walterStandin node. It's written on Python to be
able to use complex UI and PySide.
"""

# Copyright 2017 Rodeo FX. All rights reserved.

import pymel.core as pm
import maya.cmds as cmds
from walterWidgets.Qt import QtCore
from walterWidgets import ComplexMenu


class VisibilityMenu(ComplexMenu):
    def __init__(self, indices, parent):
        super(VisibilityMenu, self).__init__(parent)

        self.setTitle('Visibility')

        self.indices = []
        for index in indices:
            item = index.internalPointer()
            if item.expressionGroup:
                for i in range(item.childCount()):
                    childIndex = index.child(i, 0)
                    self.indices.append(childIndex)
            else:
                self.indices.append(index)

        if len(self.indices) == 1:
            item = self.indices[0].internalPointer()
            if not item.expression:
                hideAllExceptedThisAction = self.addAction(
                    'Hide all excepted this')
                hideAllExceptedThisAction.triggered.connect(
                    self.__onHideAllExceptedThisAction)

        visAction = self.addAction('Set Visible')
        visAction.triggered.connect(self.__onSetVisible)
        hideAction = self.addAction('Set Hidden')
        hideAction.triggered.connect(self.__onHide)

    def __refeshModel(self):
        model = self.indices[0].model()
        model.dataChanged.emit(QtCore.QModelIndex(),
                               QtCore.QModelIndex())

        pm.refresh()
        cmds.refresh()

    def __onHideAllExceptedThisAction(self):

        item = self.indices[0].internalPointer()
        pm.walterStandin(hideAllExceptedThis=(
            item.getOriginItem().getName(),
            item.alembicObject))
        self.__refeshModel()

    def __onSetVisible(self):
        for index in self.indices:
            item = index.internalPointer()
            origin = item.getOriginItem().getName()

            if item.expression:
                primPaths = pm.walterStandin(primPathsMatchingExpression=(
                    origin, item.expression))

                for primPath in primPaths:
                    pm.walterStandin(setVisibility=(
                        origin,
                        primPath,
                        True))

            else:
                pm.walterStandin(setVisibility=(
                    origin,
                    item.alembicObject,
                    True))

            item.isVisible = True

        self.__refeshModel()

    def __onHide(self):
        for index in self.indices:
            item = index.internalPointer()
            origin = item.getOriginItem().getName()

            if item.expression:
                primPaths = pm.walterStandin(primPathsMatchingExpression=(
                    origin, item.expression))
                for primPath in primPaths:
                    pm.walterStandin(setVisibility=(
                        origin,
                        primPath,
                        False))

            else:
                pm.walterStandin(setVisibility=(
                    origin,
                    item.alembicObject,
                    False))

            item.isVisible = False
        self.__refeshModel()

    def keyReleaseEvent(self, event):
        if self.isVisible():
            self.parentWidget().close()

    def mouseReleaseEvent(self, event):
        """Implementation of ComplexMenu."""
        action = self.activeAction()
        ctrlKey = self.ctrlKeyEvent()
        if action and action.isEnabled() and ctrlKey:
            action.setEnabled(False)

            if action.isCheckable():
                action.toggle()
            else:
                action.trigger()

            super(ComplexMenu, self).mouseReleaseEvent(event)
            action.setEnabled(True)
        else:
            super(ComplexMenu, self).mouseReleaseEvent(event)
