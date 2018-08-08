"""
Map all Qt bindings.
"""

# Copyright 2017 Rodeo FX. All rights reserved.

import importlib
import sys

PYSIDE2_SUB_MODULES = ["QtWidgets", "QtGui", "QtCore"]
PYSIDE_SUB_MODULES = ["QtGui", "QtGui", "QtCore"]
SUB_MODULES = ["QtWidgets", "QtGui", "QtCore"]

Qt = sys.modules[__name__]


def _setup(module, pymodules, modules, moduleName=None):
    """Install common submodules"""

    Qt.__binding__ = moduleName or module.__name__

    for index, name in enumerate(pymodules):
        if hasattr(module, name):
            submodule = getattr(module, name)
        else:
            try:
                submodule = importlib.import_module(
                    module.__name__ + "." + name)
            except ImportError:
                continue

        setattr(Qt, modules[index], submodule)


def _maya():
    """Initialise PySide"""
    import maya.cmds as cmds
    version = cmds.about(v=True)

    if int(version) < 2018:
        import PySide as module
        _setup(module, PYSIDE_SUB_MODULES, SUB_MODULES)

        import shiboken as shModule
        setattr(Qt, 'shiboken', shModule)

    elif int(version) == 2018:
        import PySide2 as module
        _setup(module, PYSIDE2_SUB_MODULES, SUB_MODULES)

        import shiboken2 as shModule
        setattr(Qt, 'shiboken', shModule)


def _katana():
    """Initialise PyQt4 of Katana"""
    import UI4.Manifest as module
    _setup(module, PYSIDE_SUB_MODULES, SUB_MODULES, moduleName="PyQt4")


def _none():
    """Qt is not found"""
    Qt.__binding__ = None


def _install():
    available = (_maya, _katana, _none)

    for initFn in available:
        try:
            initFn()
            break

        except ImportError:
            pass


_install()
