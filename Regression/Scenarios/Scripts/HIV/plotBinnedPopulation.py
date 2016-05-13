#!/usr/bin/python

# SUMMARY: Plot the population by age group
# INPUT ARGUMENTS: (1) Path to BinnedReport.json
# OUTPUT: figure drawn on screen


from __future__ import print_function
import argparse
import json
import matplotlib.pyplot as pyplot


def main(filename, channel):

    with open(filename, 'rb') as handle:
        report = json.load(handle)

    labels = report['Header']['Subchannel_Metadata']['MeaningPerAxis'][0][0]
    data = report['Channels'][channel]['Data']

    figure = pyplot.figure('{0} by Age'.format(channel))
    pyplot.axes([0.1,0.1,0.71,0.8])
    for (trace, label) in zip(data, labels):
        pyplot.plot(trace, label=label)

    pyplot.title('{0} by Age'.format(channel))
    pyplot.xlabel('Simulation Day')
    pyplot.ylabel('Population')
    pyplot.legend(loc=(1.03, 0.135), prop={"size":10})

    pyplot.show()

    pass


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('filename', default='BinnedReport.json')
    parser.add_argument('-c', '--channel', default='Population')
    args = parser.parse_args()
    main(args.filename, args.channel)

    pass
