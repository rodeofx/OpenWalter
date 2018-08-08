"""
Base class Menu for selecting walter attribute override or USD variants.
It's written on Python to be able to use complex UI and PySide.
"""

# Copyright 2017 Rodeo FX. All rights reserved.

from .Qt import QtCore
from .Qt import QtWidgets


class ComplexMenu(QtWidgets.QMenu):

    def __init__(self, parent=None):
        """Generate the layout."""
        super(ComplexMenu, self).__init__(parent)

    def ctrlKeyEvent(self):
        """Basic method to find if `ctrl` key is pressed."""
        keyHandle = QtWidgets.QApplication.keyboardModifiers()
        if keyHandle == QtCore.Qt.ControlModifier:
            return keyHandle

    def keyReleaseEvent(self, event):
        """On release of the key - trigger actions and close."""
        self.triggerActions()
        self.close()

    def mouseReleaseEvent(self, event):
        """Method to handle on mouse release."""
        # QMenu doesn't allow to keep the menu open if the user clicked by it.
        # The following trick allows.
        action = self.activeAction()
        ctrlKey = self.ctrlKeyEvent()
        if action and action.isEnabled() and ctrlKey:
            # QMenu will not disapear if the user clicked by disabled action.
            action.setEnabled(False)
            super(ComplexMenu, self).mouseReleaseEvent(event)
            action.setEnabled(True)
            action.setCheckable(1 - action.isChecked())
            action.setChecked(1 - action.isChecked())
        else:
            super(ComplexMenu, self).mouseReleaseEvent(event)

    def triggerActions(self):
        """Method triggres a list of selected actions from the menu"""
        for actionItem in self.actions():
            if not actionItem.isSeparator() and actionItem.isChecked():
                actionItem.trigger()
