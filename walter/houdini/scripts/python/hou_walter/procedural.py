import hou
import htoa
from htoa.properties import addArnoldProperties

HTOA_VERSION = htoa.__version__


def addProperties(node):
    """Create the HDA parameters and nodes (if not already done)

    :hou.HDADefinition node: the node calling onCreated.
    """

    _addProperties(node)


def _addProperties(node):
    """ Create all required parameters.

    :hou.HDADefinition node: the node receiving the new parameters.
    """

    parmTemplateGroup = node.parmTemplateGroup()

    if parmTemplateGroup.findFolder('Walter'):
        return

    addArnoldProperties(node, force=True)

    # To Get the lattest parmTemplateGroup with the Arnold group.
    parmTemplateGroup = node.parmTemplateGroup()

    parmName = 'ar_filename'
    dsoParm = hou.StringParmTemplate(parmName,
                                     'Procedural DSO',
                                     1,
                                     default_value=('walterProcedural',),
                                     is_hidden=True)

    ar_procedural_type = hou.StringParmTemplate(
        'ar_procedural_type',
        'Procedural Type',
        1,
        default_value=('walter',),
        is_hidden=True)

    ar_translate_as = hou.StringParmTemplate(
        'ar_translate_as',
        'Translate As',
        1,
        default_value=('arnold_procedural',),
        is_hidden=True)

    ar_bounding_box_padding = hou.FloatParmTemplate(
        'ar_bounding_box_padding',
        'Bounds Padding', 1, is_hidden=True)

    ar_data = hou.StringParmTemplate(
        'ar_data', 'Data String', 1, is_hidden=True)

    user_parms_separator = hou.SeparatorParmTemplate('user_parms_separator')

    parmName = 'ar_filePaths'
    filePaths = hou.StringParmTemplate(
        parmName, 'Layers', 1, default_value=('',), is_hidden=True)

    display_mode = hou.MenuParmTemplate(
        'display_mode', 'Display Mode', ('0', '1'),
        menu_labels=('None', 'Hydra'), default_value=0)

    layerPath = hou.StringParmTemplate(
        'layerPath#', 'Path', 1, default_value=('',),
        string_type=hou.stringParmType.FileReference,
        file_type=hou.fileType.Geometry,
        tags={'filechooser_pattern': '*.abc *.usd *.usda',
              'script_callback' : "kwargs['node'].hdaModule().update(kwargs['node'])",
              'script_callback_language': 'python'})

    parmName = 'ar_layers'
    layers = hou.FolderParmTemplate(
        parmName, 'Layers', parm_templates=([layerPath]),
        folder_type=hou.folderType.MultiparmBlock,
        tags={'script_callback': "kwargs['node'].hdaModule().update(kwargs['node'])",
              'script_callback_language': 'python'})

    parmName = 'ar_objectPath'
    objectPath = hou.StringParmTemplate(
        parmName, "Prim Path", 1, default_value=('/',),
        join_with_next=True)

    pick_path = hou.ButtonParmTemplate(
        "pick_path", '', is_label_hidden=True,
        tags={'button_icon': 'BUTTONS_tree',
              'script_callback':
                    "kwargs['node'].node('display/walter').parm('pickObjectPath').pressButton()",
              'script_callback_language': 'python'})

    parmName = 'ar_sessionLayer'
    sessionLayer = hou.StringParmTemplate(
        parmName, "Session Layer", 1, default_value=('',), is_hidden=True)

    parmName = 'ar_mayaStateLayer'
    mayaStateLayer = hou.StringParmTemplate(
        parmName, 'Maya State Layer', 1, default_value=('',), is_hidden=True)

    parms = [dsoParm,
             ar_procedural_type,
             ar_translate_as,
             ar_bounding_box_padding,
             ar_data,
             user_parms_separator,
             filePaths,
             display_mode,
             layers,
             objectPath,
             pick_path,
             sessionLayer,
             mayaStateLayer]

    parms = [p for p in parms if p is not None]

    walterFolder = hou.FolderParmTemplate(
        'walter', 'Walter', parm_templates=(parms),
        folder_type=hou.folderType.Tabs, is_hidden=False,
        ends_tab_group=False, tags={})

    xfo_idx = parmTemplateGroup.findIndicesForFolder('Transform')
    parmTemplateGroup.insertBefore(xfo_idx, walterFolder)

    node.setParmTemplateGroup(parmTemplateGroup)


def setDisplayNode(node):
    walterNode = node.node('display/walter')
    expression = 'chs("../../ar_objectPath")'
    parm = walterNode.parm('primPath')
    parm.setExpression(expression)


def setFilePaths(node):
    """Join layer paths into one string seperated by ':'
    The resulting string is the one use by walterProcedural
    """

    filePathsParm = node.parm('ar_filePaths')

    parmName = 'ar_layers'
    layers = [p.unexpandedString() for p in node.parm(parmName).multiParmInstances()]
    filePathsParm.set(':'.join(layers))

    walterNode = node.node('display/walter')

    layersParm = walterNode.parm('layers')
    for i in range(len(layersParm.multiParmInstances())):
        layersParm.removeMultiParmInstance(0)

    for path in layers:
        layersParm.insertMultiParmInstance(0)

    i = 0
    for p in layersParm.multiParmInstances():
        p.set(layers[i])
        i = i + 1
