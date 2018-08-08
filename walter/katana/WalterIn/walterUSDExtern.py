# Copyright 2017 Rodeo FX. All rights reserved.

import os
from ctypes import *

# Opens the walter dynamic lib to access c++ functions from Python.
katanaResources = os.environ.get('KATANA_RESOURCES').split(os.pathsep)

WALTER_IN_SO = None
for katanaResource in katanaResources:
    walterInDllPath = os.path.join(katanaResource, 'Ops', 'walterIn.so')
    if os.path.exists(walterInDllPath):
        WALTER_IN_SO = cdll.LoadLibrary(walterInDllPath)
        break

if WALTER_IN_SO is None:
    raise OSError(
        2, 'walterExternWrapper', 'cannot find walterIn.so WALTER_IN_SO')


class WalterUSDExtern(object):
    """Python wrapper of the C++ walterExtern class.
       See walterExtern.h for a complete documentation."""

    def __init__(self):
        self.obj = WALTER_IN_SO.WalterUSDExtern_new()

    def setIdentifier(self, identifier):
        """Sets the USD identifier (url/filepath)."""
        WALTER_IN_SO.WalterUSDExtern_setIdentifier.restype = None
        WALTER_IN_SO.WalterUSDExtern_setIdentifier.argtypes = [
            c_void_p, c_char_p]
        WALTER_IN_SO.WalterUSDExtern_setIdentifier(self.obj, identifier)

    def extractVariantsUsd(self):
        """Extracts the variants contained in the USD scene."""
        WALTER_IN_SO.WalterUSDExtern_extractVariantsUsd.restype = None
        WALTER_IN_SO.WalterUSDExtern_extractVariantsUsd.argtypes = [c_void_p]
        WALTER_IN_SO.WalterUSDExtern_extractVariantsUsd(self.obj)

    def getVariantsUsd(self):
        """Gets the variants list (per primitive)."""
        # Tell python to interpret the returned pointer as a string.
        WALTER_IN_SO.WalterUSDExtern_getVariantsUsd.restype = c_char_p
        WALTER_IN_SO.WalterUSDExtern_getVariantsUsd.argtypes = [c_void_p]
        return WALTER_IN_SO.WalterUSDExtern_getVariantsUsd(self.obj)

    def setVariantUsd(self, primPath, variantSetName, variantName):
        """Sets the variants of the primitive."""
        # Tell python to interpret the returned pointer as a string.
        WALTER_IN_SO.WalterUSDExtern_setVariantUsd.restype = c_char_p
        WALTER_IN_SO.WalterUSDExtern_setVariantUsd.argtypes = [
            c_void_p, c_char_p, c_char_p, c_char_p]
        return WALTER_IN_SO.WalterUSDExtern_setVariantUsd(
            self.obj, primPath, variantSetName, variantName)

    def setVariantUsdFlat(self, variants):
        """Sets the variants of the primitive."""
        WALTER_IN_SO.WalterUSDExtern_setVariantUsdFlat.restype = None
        WALTER_IN_SO.WalterUSDExtern_setVariantUsdFlat.argtypes = [
            c_void_p, c_char_p]
        WALTER_IN_SO.WalterUSDExtern_setVariantUsdFlat(
            self.obj, variants)

    def getPrimsCount(self):
        WALTER_IN_SO.WalterUSDExtern_getPrimsCount.restype = c_int
        WALTER_IN_SO.WalterUSDExtern_getPrimsCount.argtypes = [c_void_p]
        """Gets the number of USD primitives."""
        return WALTER_IN_SO.WalterUSDExtern_getPrimsCount(self.obj)
