"""A set of useful utils."""

from .Qt import __binding__ as qtBinding

try:
    import maya.cmds as cmds
except:
    cmds = None

_DPI_SCALE = \
    1.0 \
    if not hasattr(cmds, "mayaDpiSetting") else \
    cmds.mayaDpiSetting(query=True, realScaleValue=True)


def dpiScale(value):
    """Scale the value according to the current DPI of the current monitor."""
    return _DPI_SCALE * value


def toPyObject(obj):
    """Call QVariant.toPyObject on the case we are in PyQt."""
    # QAbstractItemModel.data() returns QVariant in PyQt and a python type in
    # PySide. The result of this function should be the same for all the Qts.
    if obj and qtBinding == "PyQt4":
        return obj.toPyObject()

    return obj
