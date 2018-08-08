#!/usr/bin/env python
# Copyright 2017 Rodeo FX. All rights reserved.

""" Script to merge all the plugInfo json files from USD
into one single json file. """

import argparse
import glob
import json
import os
import sys


def _findPlugInfo(rootDir):
    """ Find every pluginInfo.json files below the root directory.
    :param str rootDir: the search start from here
    :return: a list of files path
    :rtype: [str]
    """
    files = []
    for root, dirnames, filenames in os.walk(rootDir):
        files.extend(glob.glob(root + '/plugInfo.json'))

    return files


def _mergePlugInfos(pluginInfoPaths, output):
    """ Merge all plugInfo.json files into one.
    :param [str] pluginInfoPaths: list of path to plugInfo.json
    :param str output: the output path.
    """
    allPluginInfo = []

    for file in pluginInfoPaths:
        strBuffer = ''
        with open(file, 'rb') as f:
            for line in f:
                if not line.startswith('#'):
                    strBuffer += line
        if strBuffer:
            plugInfo = json.loads(strBuffer)
            plugins = plugInfo.get('Plugins')
            if plugins:
                for plugin in plugins:
                    libraryPath = plugin.get('LibraryPath')
                    if libraryPath:
                        baseName = os.path.basename(libraryPath)
                        name, _ = os.path.splitext(baseName)
                        plugin['LibraryPath'] = name

                        root = plugin.get('Root')
                        if root:
                            if name.startswith('lib'):
                                name = name[3:]
                            plugin['Root'] = './' + name

                allPluginInfo += plugins

    result = {"Plugins": allPluginInfo}
    with open(output, 'w') as outfile:
        json.dump(result, outfile, sort_keys=True, indent=4)


def _getArgs():
    parser = argparse.ArgumentParser(
        prog=sys.argv[0], description='Merge every Usd plugInfo.json into one')
    parser.add_argument('usdRootDir', action='store',
                        type=str,
                        help='The USD install path')

    parser.add_argument('walterPlugInfo', action='store',
                        type=str, nargs='*',
                        help='The pluginInfo.json(s) from Walter')

    parser.add_argument('output', action='store',
                        type=str,
                        help='The output path')

    return parser.parse_args()


if __name__ == '__main__':

    args = _getArgs()
    pluginInfoPaths = _findPlugInfo(args.usdRootDir)
    pluginInfoPaths += args.walterPlugInfo
    _mergePlugInfos(pluginInfoPaths, args.output)
