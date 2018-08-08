"""Dialog widget to display to properties from the Walter panel."""

# Copyright 2017 Rodeo FX. All rights reserved.

import json
from walterWidgets.Qt import QtCore
from walterWidgets.Qt import QtWidgets


class WalterPropertiesTable(QtWidgets.QTableWidget):
    """Table that diplays (not edit) properties/overrides."""

    def __init__(self, parent):
        super(WalterPropertiesTable, self).__init__(parent)
        self.setColumnCount(3)
        self.verticalHeader().hide()
        self.horizontalHeader().setStretchLastSection(True)

    def __setItemAtColRow(self, text, col, row):
        item = QtWidgets.QTableWidgetItem(text)
        item.setFlags(QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable)
        self.setItem(col, row, item)

    def reset(self):
        """Clears the table content and reset its headers."""
        self.clear()
        self.setRowCount(0)
        self.setHorizontalHeaderLabels(['name', 'type', 'value'])

    def setContent(self, js, rowIndex):

        self.insertRow(rowIndex)
        self.__setItemAtColRow(js['name'], rowIndex, 0)

        if js['value'] is not None:
            typeStr = js['type']
            if js['arraySize'] > -1:
                typeStr += ' [' + str(js['arraySize']) + ']'

            self.__setItemAtColRow(typeStr, rowIndex, 1)
            self.__setItemAtColRow(str(js['value']), rowIndex, 2)

        else:
            self.__setItemAtColRow('', rowIndex, 1)
            self.__setItemAtColRow('', rowIndex, 2)


class WalterTitledPropertiesTable(QtWidgets.QWidget):
    """Widget that associates a title to a table."""

    def __init__(self, title, parent):
        super(WalterTitledPropertiesTable, self).__init__(parent)

        self.table = WalterPropertiesTable(self)

        mainLayout = QtWidgets.QVBoxLayout()
        mainLayout.addWidget(QtWidgets.QLabel(title))
        mainLayout.addWidget(self.table)
        self.setLayout(mainLayout)

    def setVisibility(self):
        """Hides/shows the widgets depending on the table content."""
        if self.table.rowCount() == 0:
            self.hide()
            return False
        else:
            self.show()
            return True


class WalterPropertiesDialog(QtWidgets.QDialog):
    """Dialog that diplays (not edit) properties and overrrides.
        cf. walterUsdConversion.h/cpp."""

    def __init__(self, parent):
        super(WalterPropertiesDialog, self).__init__(parent)

        # Two tables, for the properties and the overrides
        self.titledTables = [
            WalterTitledPropertiesTable('Properties', self),
            WalterTitledPropertiesTable('Overrides', self)]

        self.currentPrim = None
        splitter = QtWidgets.QSplitter()
        splitter.setOrientation(QtCore.Qt.Vertical)
        splitter.addWidget(self.titledTables[0])
        splitter.addWidget(self.titledTables[1])
        mainLayout = QtWidgets.QVBoxLayout()
        mainLayout.addWidget(splitter)
        self.setLayout(mainLayout)

    def __setTableContent(self, js):
        """Sets the table content after having parsed the properties."""
        if 'properties' in js:
            table = self.titledTables[0].table
            for idx, property_ in enumerate(js['properties']):
                table.setContent(property_, idx)

        if 'overrides' in js:
            table = self.titledTables[1].table
            for idx, property_ in enumerate(js['overrides']):
                table.setContent(property_, idx)

    def __hide(self):
        if self.isVisible():
            self.hide()
        self.currentPrim = None

    def showAtPosition(self, pos, flatPropertiesList):
        """Show the properties/overrides dialog. The widget automatically hides
        it-self if the flatPropertiesList is nul or it's shown already."""

        js = json.loads(flatPropertiesList)
        # Check if the dialog can be displayed/needs to be hidden.
        if 'overrides' not in js and 'properties' not in js:
            self.__hide()
            return

        if js['prim'] == self.currentPrim:
            self.__hide()
            return

        # Display it (only if the object has properties).
        self.currentPrim = js['prim']
        self.setWindowTitle(self.currentPrim)

        for titledTable in self.titledTables:
            titledTable.table.reset()

        self.__setTableContent(js)

        showDialog = False
        for titledTable in self.titledTables:
            if titledTable.setVisibility():
                showDialog = True

        if showDialog and not self.isVisible():
            self.move(pos.x(), pos.y())
            self.show()

        elif not showDialog:
            self.__hide()
