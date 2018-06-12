import argparse
import glob
import math
import os
import re
import sys


class ArgumentError(Exception):
    pass


def GlobFiles(fileSpecs):
    '''Expand (potentially) wildcarded file specification into filelist'''
    fileList = []
    currentDirectory = os.getcwd()
    for root, dirs, files in os.walk(currentDirectory):
        for spec in fileSpecs:
            for file in glob.fnmatch.filter(files, spec):
                fileList.append(os.path.join(root, file))

    return fileList


def ReadTextFile(filename):
    with open(filename, 'r') as handle:
        text = handle.read()
    
    return text


def ReadNLines(filename, n):
    ''' Read first n lines of a text file '''
    with open(filename, 'r') as handle:
        fileText = ''
        for index in range(n):
            fileText += handle.readline()

    return fileText


def ProcessFiles(fileList, templateText):

    troubleFiles = []
    skipPaths = 'cajun|dependencies|jsonspirit|lz4|rapidjson|snappy|unittest'
    skipFiles = set(['stdafx.h', 'targetver.h', 'random.h', 'dllmain.cpp', 'stdafx.cpp', 'pdistmex.cpp', 'tarball.h', 'tarball.cpp'])
    lines = len(templateText.split('\n'))

    for filename in fileList:
        directory, name = os.path.split(filename.lower())
        if not re.search(skipPaths, directory) and not name in skipFiles:
            fileText = ReadNLines(filename, lines)
            if fileText.find(templateText) < 0:
                troubleFiles.append(filename)
        else:
            print('Skipping: {0} (found in skip path/files list).'.format(filename), file=sys.stderr)

    return troubleFiles


def ReportTroubleFiles(troubleFiles):
    fileCount = len(troubleFiles)
    if fileCount > 0:
        print('The following {0} files have problems (missing or not matching the given text):'.format(fileCount))
        index = 1
        countWidth = int(math.ceil(math.log10(fileCount)))
        formatString = '{{0:{0}}} {{1}}'.format(countWidth)
        for filename in troubleFiles:
            print(formatString.format(index, filename))
            index += 1
    else:
        print('No problem files found.')


def main(templateFile, testPatterns):
    
    fileList = GlobFiles(testPatterns)
    templateText = ReadTextFile(templateFile)
    troubleFiles = ProcessFiles(fileList, templateText)
    ReportTroubleFiles(troubleFiles)

    
def ValidateArguments(args):
    argc = len(args)
    if argc < 3:
        raise ArgumentError('Usage: ' + args[0] + ' templateFilename file(s)')
    
    templateFile = args[1]
    testPatterns = []
    for index in range(2, argc):
        testPatterns.append(args[index])
    
    return templateFile, testPatterns


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('template')
    parser.add_argument('patterns', nargs='+')
    args = parser.parse_args()
    try:
        # templateFile, testPatterns = ValidateArguments(sys.argv)
        # main(templateFile, testPatterns)
        main(args.template, args.patterns)
    except ArgumentError as e:
        print(e)
