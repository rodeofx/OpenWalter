"""
Attribute Editor tamplate for walterStandin node. It's written on Python to be
able to use complex UI and PySide.
"""

# Copyright 2017 Rodeo FX. All rights reserved.

import os
import pymel.core as pm
from walterWidgets import ComplexMenu
from walterWidgets.Qt import QtWidgets
from walterWidgets.Qt import QtGui
from walterWidgets.Qt import QtCore


class StagePurposeMenu(ComplexMenu):
    def __init__(self, indices, parent):
        super(StagePurposeMenu, self).__init__(parent)

        self.setTitle('Purposes')

        self.indices = []
        for index in indices:
            item = index.internalPointer()
            if item.expressionGroup:
                for i in range(item.childCount()):
                    childIndex = index.child(i, 0)
                    self.indices.append(childIndex)
            else:
                self.indices.append(index)

        purpose = pm.walterStandin(getPurpose=(
            item.getOriginItem().getName(),
            item.alembicObject))

        self.defaultAction = self.addAction('Default')
        self.defaultAction.setCheckable(True)
        self.defaultAction.triggered.connect(self.__onToggleDefaultPurpose)

        self.proxyAction = self.addAction('Proxy')
        self.proxyAction.setCheckable(True)
        self.proxyAction.triggered.connect(self.__onToggleProxyPurpose)

        self.renderAction = self.addAction('Render')
        self.renderAction.setCheckable(True)
        self.renderAction.triggered.connect(self.__onToggleRenderPurpose)

        self.guideAction = self.addAction('Guide')
        self.guideAction.setCheckable(True)
        self.guideAction.triggered.connect(self.__onToggleGuidePurpose)

        self.__setActionCheckState(purpose)

    def __onTogglePurpose(self, purpose):
        item = self.indices[0].internalPointer()
        pm.walterStandin(setPurpose=(
            item.getOriginItem().getName(),
            item.alembicObject, purpose))

    def __setActionCheckState(self, purpose):
        self.defaultAction.setChecked(False)
        self.proxyAction.setChecked(False)
        self.renderAction.setChecked(False)
        self.guideAction.setChecked(False)

        purpose = purpose.lower()
        if purpose == 'default':
            self.defaultAction.setChecked(True)

        if purpose == 'proxy':
            self.proxyAction.setChecked(True)

        if purpose == 'render':
            self.renderAction.setChecked(True)

        if purpose == 'guide':
            self.guideAction.setChecked(True)

    def __onToggleDefaultPurpose(self):
        self.__onTogglePurpose('default')

    def __onToggleProxyPurpose(self):
        self.__onTogglePurpose('proxy')

    def __onToggleRenderPurpose(self):
        self.__onTogglePurpose('render')

    def __onToggleGuidePurpose(self):
        self.__onTogglePurpose('guide')

    def keyReleaseEvent(self, event):
        if self.isVisible():
            self.parentWidget().close()

    def mouseReleaseEvent(self, event):
        """Implementation of ComplexMenu."""
        action = self.activeAction()
        ctrlKey = self.ctrlKeyEvent()
        if action and action.isEnabled() and ctrlKey:
            action.setEnabled(False)
            action.trigger()

            super(ComplexMenu, self).mouseReleaseEvent(event)
            self.__setActionCheckState(action.text())
            action.setEnabled(True)
        else:
            super(ComplexMenu, self).mouseReleaseEvent(event)


class RenderPurposeMenu(ComplexMenu):
    def __init__(self, parent):
        super(RenderPurposeMenu, self).__init__(parent)

        self.nodePath = None
        self.setTitle('Purposes')

    def reset(self, nodePath):
        self.clear()
        self.nodePath = nodePath

        purposeArray = pm.walterStandin(
            getRenderPurpose=(self.nodePath))

        self.proxyAction = QtWidgets.QAction('Proxy', self)
        self.addAction(self.proxyAction)
        self.proxyAction.setCheckable(True)
        self.proxyAction.setChecked('proxy' in purposeArray)
        self.proxyAction.triggered.connect(self.__onSetProxyPurpose)

        self.guideAction = QtWidgets.QAction('Guide', self)
        self.addAction(self.guideAction)
        self.guideAction.setCheckable(True)
        self.guideAction.setChecked('guide' in purposeArray)
        self.guideAction.triggered.connect(self.__onSetGuidePurpose)

    def __onSetPurpose(self, purpose):
        purpose = None
        if self.proxyAction.isChecked():
            if not purpose:
                purpose = 'proxy'
            else:
                purpose = purpose + ':proxy'

        if self.guideAction.isChecked():
            if not purpose:
                purpose = 'guide'
            else:
                purpose = purpose + ':guide'

        pm.walterStandin(setRenderPurpose=(
            self.nodePath, purpose))

    def __onSetProxyPurpose(self):
        self.__onSetPurpose('proxy')

    def __onSetGuidePurpose(self):
        self.__onSetPurpose('guide')

    def keyReleaseEvent(self, event):
        if self.isVisible():
            self.parentWidget().close()

    def mouseReleaseEvent(self, event):
        """Implementation of ComplexMenu."""
        action = self.activeAction()
        ctrlKey = self.ctrlKeyEvent()
        if action and action.isEnabled() and ctrlKey:
            action.setEnabled(False)
            action.trigger()

            super(ComplexMenu, self).mouseReleaseEvent(event)
            action.setEnabled(True)
        else:
            super(ComplexMenu, self).mouseReleaseEvent(event)


class RenderPurposeButton(QtWidgets.QToolButton):

    def __init__(self, toolbar):
        super(RenderPurposeButton, self).__init__(toolbar)
        self.purposeMenu = RenderPurposeMenu(self)
        self.setMenu(self.purposeMenu)

        self.setText('Purposes')

        ICONS_PATH = os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            "walterWidgets/icons")

        self.setIcon(QtGui.QIcon(os.path.join(ICONS_PATH, "show.png")))
        self.setPopupMode(QtWidgets.QToolButton.InstantPopup)
        self.setToolButtonStyle(QtCore.Qt.ToolButtonTextBesideIcon)
        toolbar.addWidget(self)

    def resetMenu(self, nodePath):
        self.purposeMenu.reset(nodePath)
