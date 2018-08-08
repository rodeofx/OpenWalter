"""
Render script for Katana.

It can be passed to Katana like this:
katana --script=katana-test-render.py scene-file.katana

It takes the Render node in the scene and renders it to the file
scene-file.katana.exr. Unlike the katana batch render, it's not necessary to
specify the render node and the output file will be saved in the directory with
the scene and it will have the same name as the scene. Very useful for tests.

"""

from PyUtilModule import KatanaFile
import NodegraphAPI
import PyUtilModule.RenderManager
import logging
import os
import sys


log = logging.getLogger('TestSeriesBatch')


def messageHandler(sequenceID, message):
    message = message.replace('\r', '\n')
    for line in message.strip('\n').split('\n'):
        if line:
            log.info(line)


# The scene is the last argument of the command line
katanaScene = sys.argv[-1]
KatanaFile.Load(katanaScene)

# Get all the render nodes of the scene
renderNodes = NodegraphAPI.GetAllNodesByType("Render")
if len(renderNodes) != 1:
    print "The scene should contain one render node"
    exit(1)

# Render the first render node
renderNode = renderNodes[0]

luaScript = """
Interface.SetAttr(
    'renderSettings.outputs.primary.locationType', StringAttribute('file'))
Interface.SetAttr(
    'renderSettings.outputs.primary.locationSettings.renderLocation',
    StringAttribute('/tmp/{scene}.exr'))
""".format(scene=os.path.basename(katanaScene))

# Create a node that overrides file output
overrideNode = NodegraphAPI.CreateNode(
    type='OpScript', parent=NodegraphAPI.GetRootNode())
overrideNode.getParameter('CEL').setValue('( (/root) )', 0, final=False)
overrideNode.getParameter('script.lua').setValue(luaScript, 0, final=False)

# Connect it to the render node
port3D = renderNode.getInputPortByIndex(0).getConnectedPort(0)
overrideNode.getInputPortByIndex(0).connect(port3D)
overrideNode.getOutputPortByIndex(0).connect(renderNode.getInputPortByIndex(0))

# Start the render
settings = PyUtilModule.RenderManager.RenderingSettings()
settings.frame = 1
settings.asynch = False
settings.interactiveMode = False
settings.tilePrioritizer = PyUtilModule.RenderManager.TILE_PRIORITIZER_NONE
settings.asynchRenderMessageCB = messageHandler

PyUtilModule.RenderManager.StartRender(
    renderMethodName=PyUtilModule.RenderManager.RenderModes.DISK_RENDER,
    node=renderNode,
    settings=settings)
