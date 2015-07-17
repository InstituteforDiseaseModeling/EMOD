import os
import json
import matplotlib.pyplot as plt
import numpy as np
import sys
import argparse

def openFile(filename):
    try:
        reportFile = open(filename)
    except Exception, e:
        print "Couldn't open '%s' for reading." % (filename)
        raise e
    return reportFile

def readFile(fileHandle):
    try:
        jsonText = fileHandle.read()
    except Exception, e:
        print "Couldn't read JSON data from file."
        raise e
    return jsonText

def parseJson(jsonText):
    try:
        jsonData = json.loads(jsonText)
    except Exception, e:
        print "Exception parsing JSON."
        raise e
    return jsonData

def getChannels(jsonData):
    try:
        channelData = jsonData['Channels']
    except Exception, e:
        print "Didn't find 'Channels' in JSON data."
        raise e
    return channelData

def getTraceName(channelTitle, keyValuePairs, grouping):
    # trace name will have channel title and any property:value pairs
    # which aren't being grouped
    traceName = channelTitle + ':'
    for keyValuePair in keyValuePairs:
        (key, value) = keyValuePair.split(':')
        if key not in grouping:
            traceName = traceName + keyValuePair + ','
    traceName = traceName[:-1]  # remove the trailing ',' (or ':' if only channel title)
    return traceName

def indexFor(traceName, channels, traceKeys, normalize, overlay):

    # all pools of the same channel overlaid
    if overlay:
        index = 0
        for channel in channels:
            if channel in traceName:
                break
            index += 1
            
    # each trace separate
    else:
        index = traceKeys.index(traceName)

    # if we're normalizing, there's a normalized trace per regular trace
    if normalize:
        index *= 2

    # matplotlib is 1-based (like MATLAB)
    return index+1

def titleFor(traceName, channels, traceKeys, normalize, overlay):

    # use channel name
    if overlay:
        for channel in channels:
            if channel in traceName:
                title = channel
                break
    else:
        title = traceName

    return title

def accumulateTraceData(args, poolData, poolKeys):

    traceValues   = {}
    statpopValues = {}

    for channelName in args.channels:
        print "Processing channel '%s'" % (channelName)

        for key in poolKeys:
            if channelName in key:
                print "Found channel '%s' in pool data '%s'" % (channelName, key)

                (channelTitle, keyValuePairs) = key.split(':',1)
                keyValuePairs = keyValuePairs.split(',')
                traceName = getTraceName(channelTitle, keyValuePairs, args.grouping)
                traceData   = np.array(poolData[ key ][ 'Data' ], dtype='float')
                statpopData = np.array(poolData[ key.replace(channelTitle, 'Statistical Population') ][ 'Data' ], dtype='float')

                if traceName not in traceValues:
                    print "New trace: '%s'" % (traceName)
                    traceValues[traceName]   = traceData
                    statpopValues[traceName] = statpopData
                else:
                    print "Add to trace: '%s'" % (traceName)
                    traceValues[traceName]   += traceData
                    statpopValues[traceName] += statpopData

    return (traceValues, statpopValues)

def plotTraces(args, traceValues, statpopValues):

    if len(traceValues) == 0:
        print "Didn't find requested channel(s) in property report."
        return

    if not args.overlay:
        plotCount = len(traceValues)
    else:
        plotCount = len(args.channels)

    if args.normalize:
        plotCount *= 2

    plt.figure(args.filename, figsize=(20,11.25))
    traceKeys = sorted(traceValues.keys())

    # plotting here
    for traceName in traceKeys:
        plotIndex = indexFor(traceName, args.channels, traceKeys, args.normalize, args.overlay)
        plt.subplot(plotCount, 1, plotIndex)
        plt.plot(traceValues[traceName], label=traceName)
        if args.normalize:
            plt.subplot(plotCount, 1, plotIndex+1)
            plt.plot(1000*traceValues[traceName]/statpopValues[traceName], label=traceName)

    # make it pretty
    ax = plt.subplot(plotCount, 1, 1)
    for traceName in traceKeys:
        plotIndex = indexFor(traceName, args.channels, traceKeys, args.normalize, args.overlay)
        plotTitle = titleFor(traceName, args.channels, traceKeys, args.normalize, args.overlay)
        plt.subplot(plotCount, 1, plotIndex)
        plt.title(plotTitle)
        plt.legend()
        if args.normalize:
            plt.subplot(plotCount, 1, plotIndex+1)
            plt.title(plotTitle + ' normalized per 1000')
            plt.legend()

    if args.saveFigure:
        plt.savefig('propertyReport.png')

    plt.show()

def main(args):

    try:
        reportFile  = openFile(args.filename)
        jsonText    = readFile(reportFile)
        jsonData    = parseJson(jsonText)
        poolData    = getChannels(jsonData)
        poolKeys    = sorted(poolData.keys())
        print 'Channels:Pools-'
        print json.dumps(poolKeys, indent=4)

    except Exception, e:
        raise e

    (traceValues, statpopValues) = accumulateTraceData(args, poolData, poolKeys)
    plotTraces(args, traceValues, statpopValues)

def processCommandline():
    parser = argparse.ArgumentParser(description='Property Report Plotting')
    parser.add_argument('filename', nargs='?', default='PropertyReport.json', help='property report filename [PropertyReport.json]')
    parser.add_argument('-c', '--channel', action='append', help='channel(s) to display [Infected]', metavar='channelName', dest='channels')
    parser.add_argument('-g', '--group',   action='append', help='property or properties by which to group data', metavar='propertyName', dest='grouping')
    parser.add_argument('-n', '--normalize', help='plot channel(s) normalized by statistical population', action='store_true')
    parser.add_argument('-o', '--overlay', help='overlay pools of the same channel', action='store_true')
    parser.add_argument('-s', '--save', help='save figure to disk', action='store_true', dest='saveFigure')
    args = parser.parse_args()
    if not args.channels:
        args.channels = ['Infected']
    if not args.grouping:
        args.grouping = []
    print "Filename: '%s'" % (args.filename)
    print "Channel(s):", args.channels
    print "Group:     ", args.grouping
    print "Normalize: ", args.normalize
    print "Overlay:   ", args.overlay
    print "Save:      ", args.saveFigure
    return args

if __name__ == '__main__':
    args = processCommandline()
    main(args)