"""API to control Walter."""

# Copyright 2017 Rodeo FX. All rights reserved.

OVERRIDE_TRANSFORM = 'transform'
OVERRIDE_SHADER = 'shader'
OVERRIDE_DISPLACEMENT = 'displacement'
OVERRIDE_ATTRIBUTE = 'attribute'


class Walter(object):
    """
    This object represents the Walter API. The Walter Panel interacts with Maya
    (in future it will interact with other packages) using this object.
    """
    # Keeps track of the single instance of the class
    __instance = None

    def __new__(cls, *args, **kwargs):
        """
        Create the instance if it hasn't been created already.

        :returns: The new object instance.
        """
        if not cls.__instance:
            # Remember the instance so that no more are created.
            instance = super(Walter, cls).__new__(cls, *args, **kwargs)

            # Implementation details. We can switch them depending on
            # the environment.
            import walterPanel.walterMayaTraverser
            instance.details = \
                walterPanel.walterMayaTraverser.WalterMayaImplementation()

            cls.__instance = instance

        return cls.__instance

    def origins(self):
        """
        Return the list of all the origin items to display in the tree.

        :returns: The list of all the origin items in the scene.
        :rtype: list
        """
        return self.details.origins()

    def dir(self, origin, path):
        """
        Return the list of child objects of the given path.

        :Example:

        .. code:: python

           from pprint import pprint
           from walter import Walter

           objects = Walter().dir(
               "walterStandinShape1", "/ships_greyjoy_shd_hi/mesh")
           pprint(objects)

        :Output:

        .. code:: python

           [u'/ships_greyjoy_shd_hi/mesh/M_defaultShip_GRP',
            u'/ships_greyjoy_shd_hi/mesh/M_segmentsBreakA_GRP',
            u'/ships_greyjoy_shd_hi/mesh/M_segmentsBreakB_GRP']

        :param str origin: The stand-in object that contains the tree.
        :param str path: The path to the sub-object.
        :returns: The list of all the origin items in the scene.
        :rtype: list
        """
        return self.details.dir(origin, path)

    def props(self, origin, path):
        """
        Return the list of properties of the given path.

        :Example:

        .. code:: python

           from pprint import pprint
           from walter import Walter

           properties = Walter().props(
              "walterStandinShape1", "/ships_greyjoy_shd_hi/mesh")
           pprint(properties)

        :Output:

        .. code:: python

           [u'/ships_greyjoy_shd_hi/mesh/M_defaultShip_GRP',
            u'/ships_greyjoy_shd_hi/mesh/M_segmentsBreakA_GRP',
            u'/ships_greyjoy_shd_hi/mesh/M_segmentsBreakB_GRP']

        :param str origin: The stand-in object that contains the tree.
        :param str path: The path to the sub-object.
        :returns: The list of all the properties of the object at the given path.
        :rtype: list
        """
        return self.details.props(origin, path)

    def setOverride(self, origin, layer, path, material, overrideType):
        """
        Save the material/displacement from the Maya nodes. It's looking for the
        connections of the Walter Stand-in object and creates/replaces the
        connection to the material.

        :Example:

        .. code:: python

           from walter import Walter
           from walter import OVERRIDE_SHADER

           # Assign lambert1 to the root of walterStandinShape1
           Walter().setOverride(
               "walterStandinShape1",
               "defaultRenderLayer",
               "/",
               "lambert1",
               OVERRIDE_SHADER)

        :param str origin: The stand-in object.
        :param str layer: Render layer to save the material.
        :param str path: The full Alembic path of the sub-object.
        :param str material: The material/displacement/walterOverride object.
        :param str overrideType:
            OVERRIDE_ATTRIBUTE or OVERRIDE_DISPLACEMENT or OVERRIDE_SHADER
        """
        return self.details.setOverride(
            origin, layer, path, material, overrideType)

    def detachOverride(self, origin, layer, path, overrideType, overrideName):
        """
        Detach override from the path. It clears the connections of the given
        Alembic path.

        :Example:

        .. code:: python

           from walter import Walter
           from walter import OVERRIDE_SHADER

           # Detach shader from the root of walterStandinShape1
           Walter().detachOverride(
               "walterStandinShape1",
               "defaultRenderLayer",
               "/",
               OVERRIDE_SHADER)

        :param str origin: The stand-in object.
        :param str layer: Render layer to save the material.
        :param str path: The full Alembic path of the sub-object.
        :param str overrideType:
            OVERRIDE_ATTRIBUTE or OVERRIDE_DISPLACEMENT or OVERRIDE_SHADER
        """
        return self.details.detachOverride(origin, layer, path,
                                           overrideType, overrideName)

    def overrides(self, origin, layer, overrideType):
        """
        Return override assignment structure for given origin.

        :param str origin: The stand-in object.
        :param str layer: Render layer to save the material.
        :param str overrideType:
            OVERRIDE_ATTRIBUTE or OVERRIDE_DISPLACEMENT or OVERRIDE_SHADER
        :returns:
            The assignment structure. Looks like this:
            {'aiStandard1': ['/pSphere1/pTorus1'], "aiStandard2": ['/', '/box']}
        :rtype: dict
        """
        return self.details.overrides(origin, layer, overrideType)

    def action(self, obj):
        """
        Perform some action with the given object. It happens when the user
        clicks the button on the Alembic object of the tree view. Usually, it's
        select.

        :param str obj: The object to perform the action.
        """
        return self.details.action(obj)

    def layers(self, origin):
        """
        Return all the layers of the origin item.

        :param str origin: The stand-in object.
        :returns: All the render layers of the stand-in object.
        :rtype: list
        """
        return self.details.layers(origin)

    def currentLayer(self):
        """
        Return the currently editable layer.

        :returns: The current render layer.
        :rtype: str
        """
        return self.details.currentLayer()

    def produceOverrides(self, object):
        """
        Check what overrides it's possible to get from this object.

        :Example:

        .. code:: python

           print Walter().produceOverrides("lambert1")
           print Walter().produceOverrides("walterOverride1")
           print Walter().produceOverrides("initialShadingGroup")

        :Output:

        .. code:: python

           [('lambert1', 'shader')]
           [('walterOverride1', 'attribute')]
           [(u'lambert1', 'shader'), (u'displacementShader2', 'displacement')]

        :returns: A list with override objects and types.
        :rtype: list
        """
        return self.details.produceOverrides(object)

    def overrideType(self, object):
        """
        Deprecated. Use produceOverrides instead.

        Check the type of the object and return which override we can use. For a
        shader, it should return OVERRIDE_SHADER, for a displacement it should
        return OVERRIDE_DISPLACEMENT.

        :Example:

        .. code:: python

           print Walter().overrideType("lambert1")
           print Walter().overrideType("walterOverride1")

        :Output:

        .. code:: python

           shader
           attribute

        :returns: The override type.
        :rtype: str
        """
        return self.details.overrideType(object)

    def rename(self, origin, oldName, newName):
        """
        Rename expression in all the render layers.

        :Example:

        .. code:: python

           from walter import Walter
           from walter import OVERRIDE_SHADER

           # Create an expresion and assign lambert1 to it
           Walter().setOverride(
               "walterStandinShape1",
               "defaultRenderLayer",
               "/*",
               "lambert1",
               OVERRIDE_SHADER)

           # Rename the expression
           Walter().rename("walterStandinShape1", "/*", "/*/*")

        :param str origin: The stand-in object.
        :param str oldName: The given expression.
        :param str newName: The new expression.
        """
        return self.details.rename(origin, oldName, newName)

    def remove(self, origin, path):
        """
        Remove the expression in all the render layers.

        :Example:

        .. code:: python

           from walter import Walter
           # Remove the expression
           Walter().remove("walterStandinShape1", "/*/*")

        :param str origin: The stand-in object.
        :param str path: The given expression.
        """
        return self.details.remove(origin, path)

    def create(self, overrideType):
        """
        Create an override object and return its name. For now, it works with
        attributes only. It should create a walterOverride object.

        :Example:

        .. code:: python

           from walter import Walter
           from walter import OVERRIDE_ATTRIBUTE

           # Create walterOverride
           override = Walter().create(OVERRIDE_ATTRIBUTE)

           # Create an expression and assign the override on it
           Walter().setOverride(
               "walterStandinShape1",
               "defaultRenderLayer",
               "/pShpere*",
               override,
               OVERRIDE_ATTRIBUTE)

        :param str overrideType:
            OVERRIDE_ATTRIBUTE or OVERRIDE_DISPLACEMENT or OVERRIDE_SHADER
        :returns: The name of the new object.
        :rtype: str
        """
        return self.details.create(overrideType)

    def saveAssignment(self, origin, fileName=None):
        """
        Save the shader assignment to the external file. If the file is not
        specified, show the dialog.

        :param str origin: The stand-in object.
        :param str fileName: The target Alembic file.
        """
        return self.details.saveAssignment(origin, fileName)

    def saveAttributes(self, origin, fileName=None):
        """
        Save the walterOverride nodes to the external file. If the file is not
        specified, show the dialog.

        :param str origin: The stand-in object.
        :param str fileName: The target Alembic file.
        """
        return self.details.saveAttributes(origin, fileName)

    def saveMaterials(self, origin, fileName=None):
        """
        Save the materials to the external file. If the file is not specified,
        show the dialog.

        :param str origin: The stand-in object.
        :param str fileName: The target Alembic file.
        """
        return self.details.saveMaterials(origin, fileName)

    def saveTransforms(self, origin, fileName=None):
        """
        Save the transformation nodes to the external file. If the file is not
        specified, show the dialog.

        :param str origin: The stand-in object.
        :param str fileName: The target file.
        """
        return self.details.saveTransforms(origin, fileName)

    def savePurposes(self, origin, fileName=None):
        """
        Save the usd purposes to the external file. If the file is not
        specified, show the dialog.

        :param str origin: The stand-in object.
        :param str fileName: The target file.
        """
        return self.details.savePurposes(origin, fileName)

    def saveVariantsLayer(self, origin, fileName=None):
        """
        Save the usd variants layer to the external file. If the file is not
        specified, show the dialog.

        :param str origin: The stand-in object.
        :param str fileName: The target file.
        """
        return self.details.saveVariantsLayer(origin, fileName)

    def setGroup(self, origin, path, group):
        """
        Set the group of the given expression.

        :param str origin: The stand-in object.
        :param str path: The given expression.
        :param str group: The given group.
        """
        return self.details.setGroup(origin, path, group)

    def expressionGroups(self, origin):
        """
        Get all available expression groups.

        :param str origin: The stand-in object.
        """
        return self.details.expressionGroups(origin)

    def expressions(self, origin, group=None):
        """
        Get all the expressions of the specified group.

        :param str origin: The stand-in object.
        :param str group: The given group.
        """
        return self.details.expressions(origin, group)

    def exposeTransforms(self, origin, alembicObject):
        """
        Create the locations objects for the object and its parents in the maya
        scene and connect them to the walter standin so the transforms of
        created locations will be passed to the USD session layer.

        :Example:

        .. code:: python

           from walter import Walter
           from walter import OVERRIDE_SHADER

           # Create three locators to control the transforms of following sub
           # objects:
           # /challenger
           # /challenger/body
           # /challenger/body/left_wheel
           Walter().exposeTransforms(
               "walterStandinShape1",
               "/challenger/body/left_wheel")

        :param str origin: The stand-in object.
        :param str alembicObject: Sub-object to connect the created locator.
        :returns: The dictionary with created objects.
        :rtype: dict
        """
        return self.details.exposeTransforms(origin, alembicObject)

    def expressionWeight(self, origin, expression):
        """
        Get the weight of a givin expression.

        The weights are used to reorder expressions within the tree. Weights
        are indexes and can come from two sources, the DCC and from file.
        They are used to sort and preserve the ordering of expressions in
        a walter standin.

        :param str origin: The stand-in object.
        :param str expression: item to query it's weight
        :returns: weight
        :rtype: int
        """
        return self.details.expressionWeight(origin, expression)

    def setExpressionWeight(self, origin, expression, weight):
        """
        Sets the weight of a givin expression.

        The weights are used to reorder expressions within the tree. Weights
        are indexes and can come from two sources, the DCC and from file.
        They are used to sort and preserve the ordering of expressions in
        a walter standin.

        :param str origin: The stand-in object.
        :param str expression: item to query it's weight
        :param int weight: value given to the expression order
        """
        return self.details.setExpressionWeight(origin, expression, weight)

    def invertFrozen(self, origin):
        """
        Inverse the freeze flag of the stand-in object. This flag cleans up the
        sub-nodes and merges them together depending on the connected
        transformations. Frozen sub-objects are much faster if they contain lots
        of children.

        :param str origin: The stand-in object.
        """
        return self.details.invertFrozen(origin)

    def getFrozen(self, origin):
        """
        Get the freeze flag of the origin node.

        :param str origin: The stand-in object.
        """
        return self.details.getFrozen(origin)

    def refresh(self, origin):
        """
        Clear the USD playback cache.

        :param str origin: The stand-in object.
        """
        return self.details.refresh(origin)

    def setPreviewColorAttrib(self, shader, color):
        """Add the shaderball color attibute on the shader and sets it's value.

        It adds a float3 attribute called rdo_walter_previewColor on a given shader.

        :param str shader: name of the shader to store the attribute.
        :param tuple color: RGB values of range 0-255
        """
        return self.details.setPreviewColorAttrib(shader, color)

    def getPreviewColorAttrib(self, shader):
        """Get the RGB value of the shaderball

        :param str shader: name of the shader to store the attribute.
        :returns: rgb values
        :rtype: tuple
        """
        return self.details.getPreviewColorAttrib(shader)
