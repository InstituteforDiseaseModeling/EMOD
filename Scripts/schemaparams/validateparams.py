import sys
import json
import re

class ArgumentError(Exception):
    pass

class JsonLoadError(Exception):
    pass

class ProcessingError(Exception):
    pass

def TryLoadJsonSchema(schemaFile):
    handle = open(schemaFile)
    jsonData = json.load(handle)
    handle.close()

    return jsonData

def KeysOf(dictionary):
    keys = set()
    keys.update(dictionary.keys())
    for value in dictionary.values():
        if isinstance(value, dict):
            keys.update(KeysOf(value))

    return keys

def ExtractParamsFromSchema(schemaFile):
    try:
        jsonData = TryLoadJsonSchema(schemaFile)
    except UnicodeDecodeError as e:
        raise JsonLoadError(e)
    
    params = set()
    
    for section in jsonData['config'].keys():
        sectionInfo = jsonData['config'][section]
        params.update(KeysOf(sectionInfo))
        
    interventionEvents = jsonData['interventions']['Events'][0]
    params.update(KeysOf(interventionEvents))
   
    return params

def TryExtractParamsFromGrep(grepFile):
    params = set()
    
    handle = open(grepFile)
    grepLines = handle.readlines()
    for line in grepLines:
        noComments = re.sub('(//.*)|(/\*.*\*/)', '', line, re.S)
        trimmedLine = noComments.strip()
        match = re.search('initConfig(?:TypeMap)?\s*\([^"]*"([^"]+)"', trimmedLine)
        if match:
            params.add(match.group(1))
        
    handle.close()
    
    return params

def ExtractParamsFromGrep(grepFile):
    try:
        params = TryExtractParamsFromGrep(grepFile)
    except ArithmeticError as e:
        raise ProcessingError(e)
    
    return params

def TryProcessingSchemaAndParams(schemaFile, grepFile):
    schemaParams = ExtractParamsFromSchema(schemaFile)
    greppedParams = ExtractParamsFromGrep(grepFile)
    diff = greppedParams - schemaParams
    
    handle = open('schemaKeys.txt', 'w+')
    schemaList = list(schemaParams)
    schemaList.sort()
    for param in schemaList:
        print >> handle, param
    handle.close()
    
    handle = open('codeParams.txt', 'w+')
    codeParams = list(greppedParams)
    codeParams.sort()
    for param in codeParams:
        print >> handle, param
    handle.close()
    
    return diff

def ProcessIgnoreFile(ignoreFile):
    ignoreList = []
    
    if (len(ignoreFile) > 0):
        handle = open(ignoreFile)
        lines = handle.readlines()
        for line in lines:
            noComments = re.sub('(//.*)|(/\*.*\*/)', '', line, re.S)
            trimmedLine = noComments.strip()
            if (len(trimmedLine) > 0):
                ignoreList.append(trimmedLine)
        
        handle.close()
    
    return ignoreList

def main(schemaFile, grepFile, ignoreFile):
    # print 'main(', schemaFile, ',', grepFile, ')'
    try:
        params = TryProcessingSchemaAndParams(schemaFile, grepFile)
    except JsonLoadError as e:
        raise ProcessingError(e)
    
    ignoreList = ProcessIgnoreFile(ignoreFile)
    
    paramList = list(params)
    paramList.sort()
    for param in paramList:
        if (not param in ignoreList):
            print param

def ValidateCommandLine(argv):
    argc = len(argv)
    if argc < 3:
        raise ArgumentError('Usage: ' + argv[0] + ' schemaFile grepFile [ignoreFile]')
    schemaFilename = argv[1]
    grepFilename   = argv[2]
    if (len(argv) > 3):
        ignoreFilename = argv[3]
    else:
        ignoreFilename = ''
    
    return schemaFilename, grepFilename, ignoreFilename

if __name__ == '__main__':
    try:
        schemaFile, grepFile, ignoreFile = ValidateCommandLine(sys.argv)
        main(schemaFile, grepFile, ignoreFile)
    except ArgumentError as e:
        print e
    except ProcessingError as e:
        print e