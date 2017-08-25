#!/usr/bin/python

import argparse
import csv
import json
import matplotlib.pyplot as plt
import pylab
import sys

plt.rcParams['legend.loc'] = 'best'

class PlottingError(Exception):
    def __init__(self, message):
        Exception.__init__(self, message)


def processChannels(channelSpec):
    expansion = { 'S': 'Susceptible Population', 'E': 'Exposed Population', 'I': 'Infectious Population', 'R': 'Recovered Population', 'W': 'Waning Population' }
    expansion['F'] = 'Latent Fast TB Prevalence'
    expansion['L'] = 'Latent Slow TB Prevalence'
    expansion['A'] = 'Active Presymptomatic Prevalence'
    expansion['P'] = 'Active Sx Smear pos Prevalence'
    expansion['N'] = 'Active Sx Smear neg Prevalence'
    expansion['X'] = 'Active Sx Extrapulm Prevalence'
    #[ ]'Active MDR TB Prevalence'
    #[X]'Active Presymptomatic Prevalence'
    #[ ]'Active Smear neg Prevalence'
    #[X]'Active Sx Extrapulm Prevalence'
    #[X]'Active Sx Smear neg Prevalence'
    #[X]'Active Sx Smear pos Prevalence'
    #[ ]'Active Symptomatic Prevalence'
    #[ ]'Active TB Prevalence'
    #[ ]'Fraction Smear pos that is MDR'
    #[ ]'Fraction Sx MDR that is evolved'
    #[ ]'Fraction Sx that is MDR'
    #[ ]'Infectiousness Fast Progressors'
    #[X]'Latent Fast TB Prevalence'
    #[X]'Latent TB Prevalence'
    #[ ]'MDR TB Prevalence'
    channels = []
    for entry in channelSpec:
        upper = entry.upper()
        try:
            channels.append(expansion[upper])
        except:
            print "Unknown channel '%s'" % (entry)
    
    if len(channels) == 0:
        raise PlottingError("No channels found to plot.")
    
    return channels


def loadJsonFromFile(filename):
    data = {}
    try:
        handle = open(filename)
        text = handle.read()
        handle.close()
    except (Exception) as e:
        raise PlottingError("Error opening/reading file - '%s'" % (e))
    try:
        data = json.loads(text)
    except (Exception) as e:
        raise PlottingError("Error '%s' parsing text from '%s'" % (e, filename))

    return data

def start_plotting(title):
    figure = plt.figure(1, figsize=(11.0, 8.5))  # in inches
    axes = figure.add_subplot(111)
    if title:
        axes.set_title(title)
    return figure, axes
    

def finish_plotting(save):
    plt.ylim(plt.ylim()[0]-0.005, plt.ylim()[1]+0.005)
    plt.legend()
#    plt.legend(loc='upper right')
    if save:
        filename = save + '.png'
        print "Saving chart to '%s'" % (filename)
        try:
            pylab.savefig( filename, bbox_inches='tight', orientation='landscape', dpi=128 )
        except (Exception) as e:
            print "Error '%s' saving chart to '%s'" % (e, filename)
    plt.show()

    pass


def write_csv_file(filename, csv_data):
    with open(filename + '.csv', 'wb') as handle:
        csv_writer = csv.writer(handle, dialect=csv.excel)
        for channel in csv_data:
            row = [channel]
            row.extend(csv_data[channel])
            csv_writer.writerow(row)

    pass

def main(filename, title, channels, save, xl):
    json_data = loadJsonFromFile(filename)
    (figure, axes) = start_plotting(title)

    csv_data = {}

    try:
        for channel in channels:
            print 'Plotting %s...' % (channel)
            if not channel == 'Latent Slow TB Prevalence':
                values = json_data['Channels'][channel]['Data']
            else:
                latent = json_data['Channels']['Latent TB Prevalence']['Data']
                fast = json_data['Channels']['Latent Fast TB Prevalence']['Data']
                values = [(L-F) for L,F in zip(latent, fast)]
            csv_data[channel] = values
            axes.plot(values, label=channel)
    except (KeyError) as e:
        raise PlottingError("Didn't find entry %s of ['Channels']['%s']['Data'] in '%s'" % (e, channel, filename))

    finish_plotting(save)

    if xl:
        write_csv_file(filename, csv_data)

    pass


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('filename', help='channel data filename [InsertChart.json]', nargs='?', default='InsetChart.json')
    parser.add_argument('-c', '--channels', help='channel[s] to display [SEIRWFLAPNX]', nargs='?', default='SEIRW', type=str)
    parser.add_argument('-t', '--title', help='Chart Title', type=str)
    parser.add_argument('-s', '--save', help='save to filename', type=str)
    parser.add_argument('-x', '--xl', default=False, action='store_true', help='save selected channels to comma separated value (csv) file')
    args = parser.parse_args()
    
    try:
        channels = processChannels(args.channels)
        
        print "Plotting '%s'" % (args.filename)
        print "Displaying channels '%s'" %(args.channels)
        print "Chart Title = '%s'" % (args.title)
        print "Save filename = '%s'" % (args.save)

        main(args.filename, args.title, channels, args.save, args.xl)
    except (PlottingError) as pe:
        print pe
    except (Exception) as e:
        print "Caught exception '%s' during execution." % e
