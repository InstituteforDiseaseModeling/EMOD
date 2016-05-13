#!/usr/bin/python

from __future__ import print_function

import argparse
import json
import os
import sys

def loadJson(filename):
    file = open(filename)
    try:
        jsonData = json.load(file)
    except Exception as e:
        print('Error loading JSON file %s: %s' % (filename, e), file=sys.stderr)
        jsonData = {}
    file.close()

    return jsonData


def writeJson(json_data, filename):
    file = open(filename)
    json.dump(json_data, filename, indent=4, separators=(',', ': '), sort_keys=True)
    file.close()

def main(configFilename, baseConfigFilename, overwrite):
    print('Creating minimal config from %s and base config %s.' % (configFilename, baseConfigFilename), file=sys.stderr)
    configJson = loadJson(configFilename)['parameters']
    baseJson   = loadJson(baseConfigFilename)['parameters']
    parameters = {}
    for parameter in configJson.keys():
        if (not parameter in baseJson) or (baseJson[parameter] != configJson[parameter]):
            parameters[parameter] = configJson[parameter]
    minimalJson = {'Default_Config_Path': baseConfigFilename, 'parameters':parameters}
    if overwrite:
        writeJson(minimalJson, configFilename)
    else:
        minimalText = json.dumps(minimalJson, indent=4, separators=(',', ': '), sort_keys=True)
        print(minimalText)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('config', type=str)
    parser.add_argument('base', type=str)
    parser.add_argument('-o', '--overwrite', action='store_true', default=False)
    args = parser.parse_args()
    main(args.config, args.base, args.overwrite)
