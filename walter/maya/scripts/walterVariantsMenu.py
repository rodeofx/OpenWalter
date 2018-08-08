"""
Variant menu for walterStandin node. It's written on Python to be
able to use complex UI and PySide.
"""

# Copyright 2017 Rodeo FX. All rights reserved.

import pymel.core as pm
from walterWidgets.Qt import QtCore
from walterWidgets.Qt import QtGui
from walterWidgets.Qt import QtWidgets
import walterWidgets


class VariantAction(walterWidgets.BaseVariantAction):
    def __init__(self, primPath, variantName,
                 variantSetName, isSelected, menu):
        super(VariantAction, self).__init__(
            primPath, 0, variantName,
            variantSetName, isSelected, menu)

    def _setVariantValue(self):
        """Implementation of BaseVariantAction."""
        pm.walterStandin(
            setVariant=(self.menu.parent.nodePath, self.primPath,
                        self.variantName, self.variantSetName))


class VariantSetMenu(walterWidgets.BaseVariantSetMenu):
    def __init__(self, primPath, variantSet, parent):
        super(VariantSetMenu, self).__init__(
            primPath, 0, variantSet, parent)

    def _constructVariantAction(self, primPath, index, variant,
                                variantSetName, isSelected):
        """Implementation of BaseVariantSetMenu."""
        return VariantAction(primPath, variant, variantSetName,
                             isSelected, self)


class VariantsMenu(walterWidgets.BaseVariantsMenu):
    """Menu for editing walter variants."""

    def __init__(self, parent):
        super(VariantsMenu, self).__init__(parent)

    def _getVariantList(self):
        """Implementation of BaseVariantsMenu."""
        return pm.walterStandin(getVariants=(self.nodePath))

    def _createMenu(self, primPath, index, variantSet):
        """Implementation of BaseVariantsMenu."""
        return VariantSetMenu(primPath, variantSet, self)


class VariantButton(QtWidgets.QToolButton):

    def __init__(self, toolbar):
        super(VariantButton, self).__init__(toolbar)
        self.variantsMenu = VariantsMenu(self)
        self.setMenu(self.variantsMenu)

        self.setText('Variants')
        self.setIcon(QtGui.QIcon(":/syncOn.png"))
        self.setPopupMode(QtWidgets.QToolButton.InstantPopup)
        self.setToolButtonStyle(QtCore.Qt.ToolButtonTextBesideIcon)
        toolbar.addWidget(self)

    def resetMenu(self, nodePath):
        isEnable = self.variantsMenu.reset(nodePath)
        self.setEnabled(isEnable)
