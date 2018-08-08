
def registerWalterAssign():
    """
    Registers a new WalterAssign node type.
    """

    from Katana import Nodes3DAPI
    from Katana import FnAttribute

    def buildWalterAssignOpChain(node, interface):
        """
        Defines the callback function used to define the Ops chain for the
        node type being registered.

        @type node: C{Nodes3DAPI.NodeTypeBuilder.WalterAssign}
        @type interface: C{Nodes3DAPI.NodeTypeBuilder.BuildChainInterface}
        @param node: The node for which to define the Ops chain
        @param interface: The interface providing the functions needed to set
            up the Ops chain for the given node.
        """
        # Get the current frame time
        frameTime = interface.getGraphState().getTime()

        # Set the minimum number of input ports
        interface.setMinRequiredInputs(1)

        argsGb = FnAttribute.GroupBuilder()

        # Parse the CEL parameter
        parm = node.getParameter('CEL')
        if parm:
            argsGb.set('CEL', parm.getValue(frameTime))

        # Parse the root parameter
        parm = node.getParameter('root')
        if parm:
            argsGb.set('root', parm.getValue(frameTime))

        # Parse the abcAsset parameter
        parm = node.getParameter('abcAsset')
        if parm:
            argsGb.set('fileName', parm.getValue(frameTime))

        # Add the WalterAssign Op to the Ops chain
        interface.appendOp('WalterAssign', argsGb.build())

    # Create a NodeTypeBuilder to register the new type
    nodeTypeBuilder = Nodes3DAPI.NodeTypeBuilder('Walter_Assign')

    # Add an input port
    nodeTypeBuilder.setInputPortNames(('in',))

    # Build the node's parameters
    gb = FnAttribute.GroupBuilder()
    gb.set('CEL', FnAttribute.StringAttribute('/root/world/geo/asset//*'))
    gb.set('root', FnAttribute.StringAttribute('/root/world/geo/asset'))
    gb.set('abcAsset', FnAttribute.StringAttribute(''))

    # Set the parameters template
    nodeTypeBuilder.setParametersTemplateAttr(gb.build())

    # Set parameter hints
    nodeTypeBuilder.setHintsForParameter('CEL', {'widget': 'cel'})
    nodeTypeBuilder.setHintsForParameter(
        'abcAsset', {'widget': 'walterLayers'})
    # Set the callback responsible to build the Ops chain
    nodeTypeBuilder.setBuildOpChainFnc(buildWalterAssignOpChain)

    # Build the new node type
    nodeTypeBuilder.build()


# Register the node
registerWalterAssign()
