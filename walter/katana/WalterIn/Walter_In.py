# Copyright 2017 Rodeo FX. All rights reserved.

from Nodes3DAPI.Manifest import AssetAPI
from Nodes3DAPI.Manifest import FnAttribute, FnGeolibServices
from Nodes3DAPI.Manifest import NodegraphAPI
from Nodes3DAPI.TimingUtils import GetModifiedFrameTime
from Nodes3DAPI.TimingUtils import GetTimingParameterHints
from Nodes3DAPI.TimingUtils import GetTimingParameterXML
import Nodes3DAPI
import logging

log = logging.getLogger('Nodes3DAPI.Walter_In')


class Walter_In(Nodes3DAPI.Node3D):

    def __init__(self):
        try:
            Nodes3DAPI.Node3D.__init__(self)
            self.addOutputPort('out')
            self.getParameters().parseXML(Parameter_XML)
        except Exception as exception:
            log.exception(
                'Error initializing %s node: %s' % (
                    self.__class__.__name__, str(exception)))

    def addParameterHints(self, attrName, inputDict):
        inputDict.update(ExtraHints.get(attrName, {}))
        inputDict.update(GetTimingParameterHints(attrName))

    def _getOpChain(self, interface):
        interface.setMinRequiredInputs(0)
        graphState = interface.getGraphState()
        frameTime = graphState.getTime()
        modifiedFrameTime = GetModifiedFrameTime(self, frameTime)
        locationName = self.getParameter('name').getValue(frameTime)
        abcAsset = self.getParameter('abcAsset').getValue(frameTime)
        abcAsset = AssetAPI.ResolvePath(abcAsset, int(modifiedFrameTime))
        addForceExpand = \
            int(self.getParameter('addForceExpand').getValue(frameTime))
        addBounds = str(self.getParameter('addBounds').getValue(frameTime))
        fps = float(self.getParameter('fps').getValue(frameTime))
        useOnlyShutter = \
            self.getParameter('advanced.useOnlyShutterOpenCloseTimes')
        variantsListAttrs = []
        variantsList = self.getParameter('advanced.variantsList')
        for variant in variantsList.getChildren():
            variantsListAttrs.append(variant.getValue(frameTime))
        useOnlyShutterOpenCloseTimes = int(useOnlyShutter.getValue(frameTime))
        addDefaults = self.getParameter('advanced.addDefaultAttrs')
        addDefaultAttrs = str(addDefaults.getValue(frameTime))
        forceUSD = self.getParameter('advanced.forceUSD')
        forceUSDInt = int(forceUSD.getValue(frameTime))
        cookInstances = self.getParameter('advanced.cookInstances')
        cookInstancesInt = int(cookInstances.getValue(frameTime))
        argsGb = FnAttribute.GroupBuilder()
        graphStateBuilt = graphState.edit().setTime(modifiedFrameTime).build()
        argsGb.set('system', graphStateBuilt.getOpSystemArgs())
        argsGb.set('fileName', abcAsset)
        argsGb.set('addForceExpand', addForceExpand)
        argsGb.set('variantsList', FnAttribute.StringAttribute(
            variantsListAttrs))
        argsGb.set('fps', FnAttribute.DoubleAttribute(fps))
        argsGb.set('addBounds', addBounds)
        argsGb.set('useOnlyShutterOpenCloseTimes',
                   useOnlyShutterOpenCloseTimes)
        argsGb.set('addDefaultAttrs', addDefaultAttrs)
        argsGb.set('forceUSD', forceUSDInt)
        argsGb.set('cookInstances', cookInstancesInt)
        sscb = FnGeolibServices.OpArgsBuilders.StaticSceneCreate()
        sscb.addSubOpAtLocation(locationName, 'WalterIn', argsGb.build())

        lightListVal = self.getParameter('addToLightList').getValue(frameTime)
        camListVal = self.getParameter('addToCameraList').getValue(frameTime)

        if lightListVal or camListVal:
            groupBuilder = FnAttribute.GroupBuilder().set('fileName', abcAsset)
            groupBuilder.set('scenegraphPath', locationName)

            groupBuilder.set(
                'setLightList', FnAttribute.IntAttribute(int(lightListVal)))

            groupBuilder.set(
                'setCameraList', FnAttribute.IntAttribute(int(camListVal)))

            sscb.addSubOpAtLocation(
                '/root/world',
                'WalterInAddToLightAndCameraLists',
                groupBuilder.build())
        interface.appendOp('StaticSceneCreate', sscb.build())


ExtraHints = {
    'Walter_In.name': {'widget': 'newScenegraphLocation'},
    'Walter_In.abcAsset': {
        'widget': 'walterLayers',
        'assetTypeTags': 'geometry|alembic',
        'fileTypes': 'usd|usda|usdb|usdc|abc|ass',
        'help': 'Specify the Alembic (.abc) or USD (.usd, .usda, .usdc) files.',
        'context': AssetAPI.kAssetContextAlembic},
    'Walter_In.addForceExpand': {'widget': 'boolean'},
    'Walter_In.addBounds': {
        'widget': 'popup',
        'options': [
            'none',
            'root',
            'children',
            'both'],
        'help': """
            Specify where the overall bounds should be placed:
            The root location; it's direct children; both or none of these.

            Adding the bounds to the root has the disadvantage of producing the
            wrong bounds when the same root location is used by several
            Walter_IN nodes (the last one being loaded will overwrite the
            bounds added by the other ones). Adding the bounds to the direct
            children of the root location has the disadvantage of repeating the
            same overall bounds on each child, which means that these bounds can
            be potentially bigger than the real bounds of that child.
        """},
    'Walter_In.addToLightList': {
        'widget': 'boolean',
        'help': """
            For layers expected to contain ligths, this enables a
            light-weight traversal of the composed layers at <i>/root/world</i> so that
            the light paths may be included in <b>lightList</b>. It's
            enable by default as we assume that if there are lights in a layer,
            it is on purpose.
        """},
    'Walter_In.addToCameraList': {
        'widget': 'boolean',
        'help': """
            For layers expected to contain cameras, this enables a
            light-weight traversal of the composed layers at <i>/root/world</i> so that
            the camera paths may be included in <b>globals.cameraList</b>. It's
            disabled by default as it does work at a shallower point in the
            scene independent of any downstream actions. In typical cases, the
            initial traversal will be trivial and is then cached. Even so, it's
            good practice to enable this only when necessary.
        """},
    'Walter_In.advanced.useOnlyShutterOpenCloseTimes': {
        'widget': 'boolean',
        'help': """
            If set, forces the Alembic cache to only use the time samples
            corresponding to shutter open and close times when 'maxTimeSamples'
            is set to 2 in a RenderSettings node.
        """},
    'Walter_In.advanced.addDefaultAttrs': {
        'widget': 'popup',
        'options': [
            'none',
            'Arnold'],
        'help': """
            Specify if it's necessary to add a default set of attributes.
        """},
    'Walter_In.advanced.forceUSD': {
        'widget': 'boolean',
        'help': """
            Force USD engine. When enabled it uses USD to load Alembics.
        """},
    # Because of some bugs with the USD Arnold Procedural, instances are
    # always expanded as Katana instances (cookInstances=True).
    # cf. RND-661
    'Walter_In.advanced.cookInstances': {
        'widget': 'boolean',
        'readOnly': True,
        'help': """
            Warning: This option is Read-Only and forced to be True because
            of the USD Arnold Procedural that is not yet able to handle
            properly instances.

            Whether you keep renderer procedurals (False) or "expand"
            instances to be actual Katana instances (True, default).
            This parameter has an impact only if USD engine is used.
        """},
    'Walter_In.advanced.variantsList': {
        'widget': 'walterVariantsToolbar',
        'help': """
            Edit the variants.
         """}}

Parameter_XML = """
    <group_parameter>
        <string_parameter name='name' value='/root/world/geo/asset'/>
        <string_parameter name='abcAsset' value=''/>
        <number_parameter name='addForceExpand' value='1'/>
        <string_parameter name='addBounds' value='root'/>
        <number_parameter name='fps' value='24'/>

        <number_parameter name='addToLightList' value='1'/>
        <number_parameter name='addToCameraList' value='0'/>

        %s

        <group_parameter name='advanced'>
            <number_parameter name='useOnlyShutterOpenCloseTimes' value='0'/>
            <string_parameter name='addDefaultAttrs' value='Arnold'/>
            <number_parameter name='forceUSD' value='0'/>
            <number_parameter name='cookInstances' value='1'/>
            <stringarray_parameter name='variantsList'/>
        </group_parameter>

    </group_parameter>
    """ % (GetTimingParameterXML(),)

NodegraphAPI.RegisterPythonNodeType('Walter_In', Walter_In)
NodegraphAPI.AddNodeFlavor('Walter_In', '3d')
NodegraphAPI.AddNodeFlavor('Walter_In', 'procedural')
