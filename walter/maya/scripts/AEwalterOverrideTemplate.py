"""
Attribute Editor tamplate for walterOverride node. It's written on Python to be
able to use complex UI and PySide.
"""

# Copyright 2017 Rodeo FX. All rights reserved.

from walterWidgets.Qt import QtGui
from walterWidgets.Qt import QtWidgets

from walterWidgets.Qt import shiboken
from pymel.core.uitypes import AETemplate
from walterWidgets import dpiScale
from walterWidgets import ComplexMenu
import maya.OpenMayaUI as OpenMayaUI
import maya.cmds as cmds

try:
    # Look for arnold, if not found load Arnold 5 by default
    from arnold import ai_version
    ARNOLD_VERSION = ai_version.AiGetVersion()[0]
except ImportError:
    ARNOLD_VERSION = '5'

# List of all the attributes.
ATTRIBUTES = [
    {'name': 'walterVisibility',
     'label': 'Visibility',
     'group': 'Visibility',
     'default': True},
    {'name': 'walterCastsShadows',
     'label': 'Casts Shadows',
     'group': 'Visibility',
     'default': True},
    {'name': 'walterPrimaryVisibility',
     'label': 'Primary Visibility',
     'group': 'Visibility',
     'default': True},
    {'name': 'walterVisibleInDiffuseReflection',
     'label': 'Diffuse Reflection',
     'group': 'Visibility',
     'default': True,
     'ai_version': "5"},
    {'name': 'walterVisibleInReflections',
     'label': 'Visible in Reflections',
     'group': 'Visibility',
     'default': True,
     'ai_version': "4"},
    {'name': 'walterVisibleInRefractions',
     'label': 'Visible in Refractions',
     'group': 'Visibility',
     'default': True,
     'ai_version': "4"},
    {'name': 'walterVisibleInSpecularReflection',
     'label': 'Specular Reflection',
     'group': 'Visibility',
     'default': True,
     'ai_version': "5"},
    {'name': 'walterVisibleInDiffuse',
     'label': 'Visible in Diffuse',
     'group': 'Visibility',
     'default': True,
     'ai_version': "4"},
    {'name': 'walterVisibleInDiffuseTransmission',
     'label': 'Diffuse Transmission',
     'group': 'Visibility',
     'default': True,
     'ai_version': "5"},
    {'name': 'walterVisibleInGlossy',
     'label': 'Visible in Glossy',
     'group': 'Visibility',
     'default': True,
     'ai_version': "4"},
    {'name': 'walterVisibleInSpecularTransmission',
     'label': 'Specular Transmission',
     'group': 'Visibility',
     'default': True,
     'ai_version': "5"},
    {'name': 'walterVisibleInVolume',
     'label': 'Volume',
     'group': 'Visibility',
     'default': True,
     'ai_version': "5"},

    {'name': 'walterUsePrimaryShader',
     'label': 'Material Source',
     'group': 'Material',
     'default': [
         'Use material from Walter Panel',
         'Use material from Hypershade']},

    {'name': 'walterSubdivType',
     'label': 'Subdivision Type',
     'group': 'Subdivision',
     'type': 'string',
     'default': ['none', 'catclark', 'linear']},
    {'name': 'walterSubdivIterations',
     'label': 'Subdivision Iterations',
     'group': 'Subdivision',
     'default': 1},
    {'name': 'walterSubdivAdaptiveMetric',
     'label': 'Adaptive Metric',
     'group': 'Subdivision',
     'type': 'string',
     'default': ['auto', 'edge_length', 'flatness']},
    {'name': 'walterSubdivAdaptiveError',
     'label': 'Adaptive Error',
     'group': 'Subdivision',
     'default': 0.0},
    {'name': 'walterSubdivAdaptiveSpace',
     'label': 'Adaptive Space',
     'group': 'Subdivision',
     'type': 'string',
     'default': ['raster', 'object']},
    {'name': 'walterSubdivUvSmoothing',
     'label': 'Uv Smoothing',
     'group': 'Subdivision',
     'type': 'string',
     'default': ['pin_corners', 'pin_borders', 'linear', 'smooth']},
    {'name': 'walterSubdivSmoothDerivs',
     'label': 'Smooth Derivs',
     'group': 'Subdivision',
     'default': False},

    {'name': 'walterDispHeight',
     'label': 'Displacement Height',
     'group': 'Displacement',
     'default': 1.0},
    {'name': 'walterDispPadding',
     'label': 'Displacement Padding',
     'group': 'Displacement',
     'default': 0.0},
    {'name': 'walterDispZeroValue',
     'label': 'Displacement Zero Value',
     'group': 'Displacement',
     'default': 0.0},
    {'name': 'walterDispAutobump',
     'label': 'Displacement Autobump',
     'group': 'Displacement',
     'default': True},

    {'name': 'walterSssSetname',
     'label': 'SSS Set',
     'group': 'SSS',
     'default': ''},

    {'name': 'walterSelfShadows',
     'label': 'Self Shadows',
     'default': True},
    {'name': 'walterOpaque',
     'label': 'Opaque',
     'default': True},
    {'name': 'walterMatte',
     'label': 'Matte',
     'default': False}]
BUTTON_POSITION = 380
BUTTON_SIZE = 18
PREFIX = 'walter'
PREFIX_SIZE = len(PREFIX)
PLUS_BUTTON_NAME = "walterAddAttributeButton"
CURRENT_VERSION = 1


def walterOverrideInitialize(nodeName):
    if not cmds.objExists(nodeName):
        # This happens if someone created end deleted walterOverride in the
        # scripts
        return

    # All the attributes of the current node.
    listAttr = cmds.listAttr(nodeName)
    customAttrs = [a for a in listAttr if a[:PREFIX_SIZE] == PREFIX]

    # Check if we have a version attribute. We didn't create it for nodes of
    # version 0.
    if 'version' not in listAttr:
        # We need the version attribute to track versions.
        cmds.addAttr(
            nodeName,
            longName='version',
            attributeType='long',
            defaultValue=CURRENT_VERSION)

        if not customAttrs:
            # It means we just created it. We already specified the version and
            # we have nothing to convert
            return
        else:
            # Version has never been specified. It's a very old node.
            version = 0
    else:
        version = cmds.getAttr(nodeName + '.version')

    if version < 1:
        # Update this node to the version 1. The only difference in one single
        # attribute walterSubdivType.
        if 'walterSubdivType' in customAttrs:
            # Remove enum walterSubdivType and create a string one
            attrName = 'walterSubdivType'
            attr = next(a for a in ATTRIBUTES if a['name'] == attrName)
            plugName = "{node}.{attr}".format(node=nodeName, attr=attrName)

            subdivType = cmds.getAttr(plugName) or 0
            cmds.deleteAttr(nodeName, attribute=attrName)
            cmds.addAttr(
                        nodeName,
                        longName=attrName,
                        dataType="string")
            # Set the new value
            cmds.setAttr(
                plugName,
                attr['default'][subdivType],
                type='string')

    if version < CURRENT_VERSION:
        cmds.setAttr(nodeName + '.version', CURRENT_VERSION)


def filterAttributesByVersion(attributes):
    """helper to filter attibute list accordind to arnold version.

    This function will check the arnold version loaded and filter ATTRIBUTES' list
    according to their compatibility.

    :param attributes: series of attibutes dictionaries.
    :type attributes: list
    :return: filtered attributes
    :rtype: list
    """
    attribs = []
    for item in attributes:
        if "ai_version" not in item or ARNOLD_VERSION in item.get("ai_version"):
            attribs.append(item)

    return attribs


class AEwalterOverrideTemplate(AETemplate):
    """Attribute Editor template using python."""

    def __init__(self, nodeName):
        """Generate the layout."""
        super(AEwalterOverrideTemplate, self).__init__(nodeName)

        # Don't do anything if it's not the attribute editor.
        if int(cmds.about(v=True)) <= 2016 and \
           not cmds.setParent(query=True).startswith('Maya'):
            return

        if int(cmds.about(v=True)) >= 2018 and \
           not cmds.setParent(query=True).startswith('AttributeEditor'):
            return

        self.beginScrollLayout()

        # The main layout.
        self.beginLayout("Walter", collapse=False)

        group = None

        # Create all the attributes
        for attr in ATTRIBUTES:
            currentGroup = attr.get('group')

            # Greate a group
            if currentGroup != group:
                if group:
                    self.endLayout()
                if currentGroup:
                    self.beginLayout(currentGroup, collapse=False)
                group = currentGroup

            self.callCustom(
                self.attributeBuild,
                self.attributeReplace,
                attr['name'])

        if group:
            self.endLayout()

        # Create a context menu. We use "message" attribute to have access to
        # the AE, but we don't create an actual control.
        # It's here to be called after all the UI items are created because we
        # we need everything built when we hide and show the layouts.
        self.callCustom(self.build, self.replace, "message")

        self.endLayout()

        # Ability to add custom layouts from external plugins.
        cmds.callbacks(
            nodeName,
            executeCallbacks=True,
            hook="AETemplateCustomContent")

        # Put all the rest to the separate layout.
        self.addExtraControls()

        self.endScrollLayout()

    def setNodeName(self, nodeName):
        """Set the name of the current node."""
        self._nodeName = nodeName

    def addAttr(self, attrName=None, attrType=None):
        """Called when the user selected an attribute in the context menu."""
        if attrName:
            attr = next(a for a in ATTRIBUTES if a['name'] == attrName)
            default = attr['default']
            if isinstance(default, bool):
                cmds.addAttr(
                    self.nodeName,
                    longName=attrName,
                    attributeType="bool",
                    defaultValue=default)
            elif isinstance(default, int):
                cmds.addAttr(
                    self.nodeName,
                    longName=attrName,
                    attributeType="long",
                    defaultValue=default)
            elif isinstance(default, float):
                cmds.addAttr(
                    self.nodeName,
                    longName=attrName,
                    attributeType="double",
                    defaultValue=default)
            elif isinstance(default, str):
                cmds.addAttr(
                    self.nodeName,
                    longName=attrName,
                    dataType="string")
            elif isinstance(default, list):
                if attr.get('type') == 'string':
                    cmds.addAttr(
                        self.nodeName,
                        longName=attrName,
                        dataType="string")
                    # Set default
                    cmds.setAttr(
                        "%s.%s" % (self.nodeName, attrName),
                        default[0],
                        type='string')
                else:
                    cmds.addAttr(
                        self.nodeName,
                        longName=attrName,
                        attributeType="enum",
                        enumName=":".join(default))

        if attrType:
            existedAttrs = cmds.listAttr(self.nodeName)

            i = 1
            while True:
                attrName = PREFIX + "_custom%i" % i
                attrNameR = '%sR' % attrName
                attrNameG = '%sG' % attrName
                attrNameB = '%sB' % attrName
                if attrName not in existedAttrs and \
                        attrNameR not in existedAttrs and \
                        attrNameG not in existedAttrs and \
                        attrNameB not in existedAttrs:
                    break
                i += 1
            if attrType == 'color':
                cmds.addAttr(
                    self.nodeName,
                    longName=attrName,
                    usedAsColor=True,
                    attributeType='float3')
                cmds.addAttr(
                    self.nodeName,
                    longName=attrNameR,
                    attributeType='float',
                    parent=attrName)
                cmds.addAttr(
                    self.nodeName,
                    longName=attrNameG,
                    attributeType='float',
                    parent=attrName)
                cmds.addAttr(
                    self.nodeName,
                    longName=attrNameB,
                    attributeType='float',
                    parent=attrName)
            elif attrType == 'string':
                cmds.addAttr(self.nodeName, longName=attrName, dt=attrType)
            else:
                cmds.addAttr(self.nodeName, longName=attrName, at=attrType)

    def createContextMenu(self, *args):
        """Create context menu."""
        existedAttrs = cmds.listAttr(self.nodeName)

        menu = ComplexMenu()

        group = None
        for attr in filterAttributesByVersion(ATTRIBUTES):
            name = attr['name']
            if name in existedAttrs:
                continue

            currentGroup = attr.get('group')

            # Add a separator if necessary.
            # TODO: in Qt5 it's possible to add a separator with a text
            if currentGroup != group:
                if not menu.isEmpty():
                    menu.addSeparator()
                group = currentGroup

            action = menu.addAction(attr['label'])
            action.triggered.connect(lambda a=name: self.addAttr(attrName=a))

        if not menu.isEmpty():
            menu.addSeparator()

        action = menu.addAction('Custom Int')
        action.triggered.connect(lambda: self.addAttr(attrType='long'))
        action = menu.addAction('Custom Float')
        action.triggered.connect(lambda: self.addAttr(attrType='double'))
        action = menu.addAction('Custom Color')
        action.triggered.connect(lambda: self.addAttr(attrType='color'))
        action = menu.addAction('Custom String')
        action.triggered.connect(lambda: self.addAttr(attrType='string'))

        # popup() is asynchronous, exec_() is synchronous.
        menu.exec_(QtGui.QCursor.pos())

    def build(self, plug):
        """Called to create some UI objects when a new node type is edited."""
        # Get the current widget Maya name.
        currentWidgetName = cmds.setParent(query=True)
        # Convert it to C++ ptr.
        ptr = OpenMayaUI.MQtUtil.findLayout(currentWidgetName)
        # Convert it to PySide object.
        currentWidget = shiboken.wrapInstance(long(ptr), QtWidgets.QWidget)
        # Get the Layout.
        currentLayout = currentWidget.parent().parent().parent()

        # Trying to get the plus button. If pymel created this object twice, we
        # already have this button.
        button = currentLayout.findChild(QtWidgets.QPushButton, PLUS_BUTTON_NAME)
        if button:
            button.clicked.disconnect()
        else:
            # Create a button.
            button = QtWidgets.QPushButton(currentLayout)
            button.setObjectName(PLUS_BUTTON_NAME)
            # TODO: Set dynamic size. For now it's OK to have this because AE
            # size is fixed.
            button.setGeometry(
                dpiScale(BUTTON_POSITION + 5),
                2,
                dpiScale(BUTTON_SIZE),
                dpiScale(BUTTON_SIZE))
            button.setText("+")
            button.setStyleSheet("background-color: #333333;")

        button.clicked.connect(self.createContextMenu)
        button.show()

        # Create empty layout. We will put all the custom attributes there.
        cmds.columnLayout("walterCustomLayout")
        cmds.setParent("..")

        self.replace(plug)

    def replace(self, plug):
        """
        Called if an attribute editor already exists and another node of the
        same type is now to be edited.
        """
        self.setNodeName(plug.split('.')[0])

        # All the attributes of the current node.
        listAttr = cmds.listAttr(self.nodeName)

        # Get the current widget Maya name.
        currentWidgetName = cmds.setParent(query=True)
        # Convert it to C++ ptr.
        ptr = OpenMayaUI.MQtUtil.findLayout(currentWidgetName)
        # Convert it to PySide object.
        currentWidget = shiboken.wrapInstance(long(ptr), QtWidgets.QWidget)
        # Get the Layout.
        currentLayout = currentWidget.parent().parent().parent()
        # Get a button
        button = currentLayout.findChild(QtWidgets.QPushButton, PLUS_BUTTON_NAME)
        # Reconnect the button. We need it to be sure the button is connected
        # with proper object. Sometimes pymel creates several AETemplate
        # objects, in this way we need to be sure that the button is connected
        # with proper object.
        button.clicked.disconnect()
        button.clicked.connect(self.createContextMenu)

        # The "Walter" layout that contains everything.
        mainLayout = cmds.layout(currentWidgetName, query=True, parent=True)
        # Iterate all the children of the layout.
        for layout in cmds.layout(mainLayout, query=True, childArray=True):
            fullName = '|'.join([mainLayout, layout])

            # Check if current item is a frameLayout
            if not cmds.frameLayout(fullName, query=True, exists=True):
                continue

            group = cmds.frameLayout(fullName, query=True, label=True)

            # Check if we need to display this group
            visible = False

            # TODO: It's not a perfect way to iterate the attributes this way.
            # But we have a few of them, so for now it's OK.
            for attr in ATTRIBUTES:
                if attr.get('group') == group and attr['name'] in listAttr:
                    visible = True
                    continue

            # Hide/show
            cmds.frameLayout(fullName, edit=True, visible=visible)

        # Delete all old UIs of custom attributes
        customPath = cmds.layout(
            "walterCustomLayout", query=True, fullPathName=True)
        childLayouts = cmds.layout(
            "walterCustomLayout", query=True, childArray=True)
        for layout in childLayouts or []:
            fullName = '|'.join([customPath, layout])
            cmds.deleteUI(fullName)

        # Switch to the layout with custom attributes
        cmds.setParent(customPath)

        # List of predefined attributes
        known = [attr['name'] for attr in ATTRIBUTES]
        # Find attributes that will be exported but they wasn't defined.
        customAttrs = [
            a for a in listAttr if a[:PREFIX_SIZE] == PREFIX and a not in known]
        for a in customAttrs:
            currentPlug = '.'.join([self.nodeName, a])

            if cmds.addAttr(currentPlug, query=True, parent=True) != a:
                # We are here because the current attribute is a child of some
                # another attribute. We need to skip it.
                continue

            cmds.rowLayout(
                numberOfColumns=4,
                columnWidth4=(
                    BUTTON_POSITION * 6 / 14,
                    BUTTON_POSITION * 2 / 14,
                    BUTTON_POSITION * 6 / 14 - BUTTON_SIZE,
                    BUTTON_SIZE))

            nameLayout = a + "NameLayout"
            attrLayout = a + "Layout"
            buttonLayout = a + "Button"

            # Text field with the attribute name.
            text = a[PREFIX_SIZE:]
            if text[0] == '_':
                text = text[1:]

            cmds.textField(
                nameLayout,
                text=text,
                changeCommand=
                    lambda
                        text,
                        n=self.nodeName,
                        a=a,
                        nl=nameLayout,
                        al=attrLayout,
                        bl=buttonLayout:
                    self.renameAttr(n, a, PREFIX + '_' + text, nl, al, bl))

            # The group that repesents an attribute.
            control = self.getControllerGroup(currentPlug, custom=True)
            control(attrLayout, attribute=currentPlug, label="")
            # Remove the text label from the group. We already have a text field
            children = cmds.layout(attrLayout, query=True, childArray=True)
            path = cmds.layout(attrLayout, query=True, fullPathName=True)
            if children and len(children) > 1:
                # Remove the first. It's a text label.
                cmds.deleteUI(children.pop(0))
                # Hide all except the second item. We can't remove them because
                # Maya use them when it changes the current attribute of the
                # group.
                children.pop(0)
                for item in children:
                    fullItem = '|'.join([path, item])

                    if cmds.layout(fullItem, query=True, exists=True):
                        cmds.layout(fullItem, edit=True, visible=False)

                    if cmds.control(fullItem, query=True, exists=True):
                        cmds.control(fullItem, edit=True, visible=False)

            # Empty space for filling
            cmds.rowLayout(numberOfColumns=1)
            cmds.setParent('..')

            # The button to remove the attribute.
            cmds.button(
                buttonLayout,
                label="-",
                width=BUTTON_SIZE,
                height=BUTTON_SIZE,
                command=lambda c, a=a: self.deleteAttr(a))

            cmds.setParent("..")

        # Return the parent.
        cmds.setParent(currentWidgetName)

    def renameAttr(self, node, old, new, nameLayout, attrLayout, buttonLayout):
        """
        Rename the given attribute to the name given in the "new" argument.
        Update the UI layout.
        """
        # Rename Maya attribute
        plug = '.'.join([node, old])
        cmds.renameAttr(plug, new)

        # Update the layout
        cmds.textField(
            nameLayout,
            edit=True,
            changeCommand=
                lambda
                    text,
                    n=node,
                    a=new,
                    nl=nameLayout,
                    al=attrLayout,
                    bl=buttonLayout:
                self.renameAttr(n, a, PREFIX + '_' + text, nl, al, bl))

        # Update the button.
        cmds.button(
            buttonLayout,
            edit=True,
            command=lambda c, a=new: self.deleteAttr(a))

        # Update the field.
        plug = '.'.join([node, new])
        control = self.getControllerGroup(plug)
        control(attrLayout, edit=True, attribute=plug)

    def deleteAttr(self, attrName):
        """Delete a dynamic attribute from the node."""
        cmds.deleteAttr(self.nodeName, attribute=attrName)

    def getControllerGroup(self, plug, custom=False):
        """Return a function that creates a controller depending of the type."""
        attrType = cmds.getAttr(plug, type=True)
        if attrType == 'enum':
            return cmds.attrEnumOptionMenuGrp
        elif attrType == 'double':
            return cmds.attrFieldSliderGrp
        elif attrType == 'float3':
            return cmds.attrColorSliderGrp
        elif attrType == 'string' and not custom:
            return self.attrTextFieldGrp
        return cmds.attrControlGrp

    def attributeBuild(self, plug):
        """Called to create the UI objects when a new node type is edited."""
        node, attrName = plug.split('.')
        # Check if attribute exists.
        exists = attrName in cmds.listAttr(node)

        if not exists:
            # Hide the layout. It's not created yet, but Maya created a parent
            # and its height is about 1 pixel. We don't need an extra space.
            self.setVisible(False)
        else:
            group = attrName + "Grp"
            button = attrName + "Btn"
            layout = attrName + "Layout"

            # Create the layout
            cmds.rowLayout(
                layout,
                columnWidth2=(BUTTON_POSITION - 5, BUTTON_SIZE),
                numberOfColumns=2)

            # Form the nice name.
            attr = next(a for a in ATTRIBUTES if a['name'] == attrName)
            label = attr['label']

            control = self.getControllerGroup(plug)
            control(group, attribute=plug, label=label)

            cmds.button(
                button,
                label="-",
                width=BUTTON_SIZE,
                height=BUTTON_SIZE)

            cmds.setParent("..")

            self.attributeReplace(plug)

    def attributeReplace(self, plug):
        """Called to create the UI objects when a new node type is edited."""
        node, attrName = plug.split('.')
        group = attrName + "Grp"
        button = attrName + "Btn"
        layout = attrName + "Layout"

        # Check if the layout exists.
        controlExists = cmds.control(layout, exists=True)

        # If it's not exist, try to create it. attributeBuild() will start this
        # method in the case of success.
        if not controlExists:
            self.attributeBuild(plug)
            return

        # Check if attribute exists.
        exists = attrName in cmds.listAttr(node)
        # Hide/show the layout
        self.setVisible(exists)

        # Edit the UI items
        if exists:
            control = self.getControllerGroup(plug)
            control(group, edit=True, attribute=plug)

            cmds.button(
                button,
                edit=True,
                command=lambda c, a=attrName: self.deleteAttr(a))

    def setVisible(self, visible):
        """Hide/show current layout."""
        layout = cmds.setParent(query=True)
        ptr = OpenMayaUI.MQtUtil.findLayout(layout)
        widget = shiboken.wrapInstance(long(ptr), QtWidgets.QWidget)
        widget.setVisible(visible)

    def attrOptionMenuGrpChanged(self, plug, value):
        """Callback called by attrTextFieldGrp when the user changes it."""
        cmds.setAttr(plug, value, type='string')

    def attrTextFieldGrp(self, group, **kwargs):
        """
        Create a pre-packaged collection of label text, and text field or option
        menu (if the default is a list) connected to a string attribute.
        Clicking on the option menu will change the attribute.
        """
        plug = kwargs['attribute']
        node, attrName = plug.split('.')
        attr = next(a for a in ATTRIBUTES if a['name'] == attrName)
        default = attr['default']
        current = cmds.getAttr(plug) or ""

        # Form the keyword arguments
        optionKwArgs = {}

        optionKwArgs['changeCommand'] = \
            lambda v, p=plug: self.attrOptionMenuGrpChanged(p, v)

        edit = kwargs.get('edit')
        if edit:
            optionKwArgs['edit'] = True

        label = kwargs.get('label')
        if label:
            optionKwArgs['label'] = label

        if isinstance(default, list):
            # It's an option
            select = 1

            cmds.optionMenuGrp(group, **optionKwArgs)

            for i, item in enumerate(default):
                if not edit:
                    # Add the items to the option menu.
                    cmds.menuItem(label=item)
                # Get the id of the current item.
                if item == current:
                    select = i + 1

            # Change the current item.
            cmds.optionMenuGrp(
                group,
                edit=True,
                select=select)
        else:
            # It's an text field
            cmds.textFieldGrp(group, **optionKwArgs)
            cmds.textFieldGrp(group, edit=True, text=current)
