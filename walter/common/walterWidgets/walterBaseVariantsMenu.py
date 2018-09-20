"""
Base classes for USD variants. It's written on Python to be
able to use complex UI and PySide.
"""

# Copyright 2017 Rodeo FX. All rights reserved.
import json
from .Qt import QtWidgets
from walterComplexMenu import ComplexMenu


class VariantSet():
    """Per variantset variants list."""

    def __init__(self, name):
        self.name = name
        self.variants = []
        self.selectedVariant = ''

    def setVariants(self, variants, selection):
        self.variants = variants
        self.selectedVariant = selection


class BaseVariantAction(QtWidgets.QAction):
    def __init__(self, primPath, index, variantName,
                 variantSetName, isSelected, menu):
        super(BaseVariantAction, self).__init__(variantName, menu)
        self.menu = menu
        self.index = index
        self.primPath = primPath
        self.variantName = variantName
        self.variantSetName = variantSetName
        self.setCheckable(isSelected)
        self.setChecked(isSelected)
        self.triggered.connect(self.__trigger)

    def _setVariantValue(self):
        pass

    def __trigger(self):
        self.menu.uncheckedActions()
        self._setVariantValue()
        self.setCheckable(True)
        self.setChecked(True)


class BaseVariantSetMenu(ComplexMenu):
    def __init__(self, primPath, index, variantSet, parent):
        super(BaseVariantSetMenu, self).__init__(parent)
        self.parent = parent
        self.setTitle(variantSet.name)
        self.actionList = []

        for variant in variantSet.variants:
            isSelected = variant == variantSet.selectedVariant
            action = self._constructVariantAction(
                primPath, index, variant,
                variantSet.name, isSelected)

            self.addAction(action)
            self.actionList.append(action)

    def _constructVariantAction(self, primPath, index, variant,
                                variantSet, isSelected):
        pass

    def uncheckedActions(self):
        for action in self.actionList:
            action.setCheckable(False)
            action.setChecked(False)

    def mouseReleaseEvent(self, event):
        """Implementation of ComplexMenu."""

        action = self.activeAction()
        ctrlKey = self.ctrlKeyEvent()
        if action and action.isEnabled() and ctrlKey:
            # QMenu will not disapear if the user clicked by disabled action.
            action.setEnabled(False)
            action.trigger()
            super(ComplexMenu, self).mouseReleaseEvent(event)
            action.setEnabled(True)
        else:
            super(ComplexMenu, self).mouseReleaseEvent(event)


class BaseVariantsMenu(QtWidgets.QMenu):
    """Menu for editing walter variants."""

    class ObjectVariantSet():
        """Per object variantsets list."""

        def __init__(self, name):
            self.name = name
            self.variantSets = []

        def addVariantSet(self, variantSet):
            self.variantSets.append(variantSet)

    def __init__(self, parent=None):
        super(BaseVariantsMenu, self).__init__(parent)
        self.parent = parent
        self.nodePath = ''
        self.primPath = ''

    def reset(self, nodePath, primPath='', title=None,
              addSeparators=True, tearOff=True, recursively=True):
        self.clear()
        self.nodePath = nodePath
        self.primPath = primPath

        title_ = title
        if not title_:
            title_ = nodePath + '-' + primPath
        self.setTitle(title_)
        self.setWindowTitle(title_)
        self.setTearOffEnabled(tearOff)

        self.setStyleSheet("menu-scrollable: 1;")
        if addSeparators:
            self.setSeparatorsCollapsible(False)

        self.__constructMenu(
            self._getVariantList(recursively), addSeparators)

        return not self.isEmpty()

    def _getVariantList(self, recursively):
        pass

    def _createMenu(self, primPath, index, variantSet):
        pass

    def __constructMenu(self, variantSetsStr, addSeparators):
        objectVariantSets = []

        for variantStr in variantSetsStr:
            js = json.loads(variantStr)

            objectVariantSet = BaseVariantsMenu.ObjectVariantSet(
                js['prim'])

            for jsVariant in js['variants']:
                variantSet = VariantSet(jsVariant['set'])
                variantSet.setVariants(jsVariant['names'], jsVariant['selection'])
                objectVariantSet.addVariantSet(variantSet)

            objectVariantSets.append(objectVariantSet)

        # Creates a variantset menu per object
        for idx, objectVariantSet in enumerate(objectVariantSets):

            if addSeparators:
                action = self.addAction(objectVariantSet.name)
                # Cause a diplay bug. The titles are cropped
                # action.setSeparator(True)
                action.setEnabled(False)

            for variantSet in objectVariantSet.variantSets:
                self.addMenu(self._createMenu(
                    objectVariantSet.name,
                    idx, variantSet))
