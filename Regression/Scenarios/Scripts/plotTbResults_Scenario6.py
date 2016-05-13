from __future__ import division #allows you to do non-integer (ie floating point) division
from __future__ import print_function
import argparse
import json
import matplotlib.pyplot as plt
import numpy
import os
import subprocess
import sys
from matplotlib.backends.backend_pdf import PdfPages
from pylab import *


color_by_group = [ 'k', 'm', 'r']

_plots = [
    { 'title': 'Population', 'channels': [ { 'label': 'Public', 'data': lambda cd: cd['Statistical Population Public']['Data'] }, { 'label': 'Private', 'data': lambda cd: cd['Statistical Population Private']['Data'] } ] },
    { 'title': 'Active TB Prevalence', 'channels': [ { 'data': lambda cd: map(lambda x,y: (x / y) if y else 0, cd['Active TB Population']['Data'], cd['Statistical Population']['Data']) } ] }
]


def label_for_trace(channel_label, trace_name):
    trace_label = None

    if channel_label:
        trace_label = channel_label
        if trace_name:
            trace_label += ' ' + trace_name
        #else: # don't append empty trace_name
    else:
        if trace_name:
            trace_label = trace_name
        #else: # both are empty, trace_label is None

    return trace_label


def plot_file_data(filenames, labels, save, title):

    file_data = []

    for filename in filenames:
        with open( filename, 'r' ) as handle:
            file_data.append( (filename, json.load( handle )) )

    pp = PdfPages( 'figures.pdf' ) if save else None

    plt.figure()
    fig = gcf()
    #fig.suptitle( title )

    for plot_idx, plot_info in enumerate(_plots):
        num_rows = 1
        num_cols = 2

        subplot = plt.subplot( num_rows, num_cols, plot_idx + 1 )
        label_present = False

        trace_index = 0
        for channel_info in plot_info['channels']:
            for entry in file_data:
                file_name, channel_data = entry
                x_range = range( 0, 36505, 5)
                trace_data = channel_info['data'](channel_data['Channels'])
                trace_color = color_by_group[ trace_index ]
                if plot_idx == 1:
                    trace_color = color_by_group[ trace_index+2]
                trace_label = label_for_trace(channel_info['label'] if ('label' in channel_info) else None, file_name if (len(file_data) > 1) else None)
                label_present = True if trace_label else label_present
                subplot.plot(  x_range[:-1], trace_data[:-1], trace_color, label=trace_label )
                trace_index += 1

        #subplot.tick_params( axis='both', which='major', labelsize=8 )
        first_entry = file_data[0]
        data = first_entry[1]
        plt.xticks( range( 0, 36505, 7300), [0, 20, 40, 60, 80, 100] )
        plt.yticks(fontsize = 8)
        plt.title( plot_info['title'], fontsize=8 )
        if label_present:
            subplot.legend( loc = 'best', fontsize = 8)
        plt.xlabel( 'Simulation Time, years', fontsize = 8)
        if plot_idx == 0:
            plt.ylabel( 'Number of People', fontsize = 8 )
            plt.ylim(-1000, 110000)
        else:
            plt.ylabel( 'Prevalence', fontsize = 8 )

    plt.tight_layout( rect=[ 0,0,1,0.95 ] )

    if pp:
        pp.savefig(fig)
        pp.close()

    pass


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('-f', '--file', default=[], action='append', help='Source data filename(s) [Report_Scenarios.json]')
    parser.add_argument('-l', '--label', default=[], action='append', help='Source data label')
    parser.add_argument('-s', '--save', default=False, action='store_true')
    parser.add_argument('-t', '--title', default='No Title')
    args = parser.parse_args()

    plot_file_data(args.file, args.label, args.save, args.title)

    plt.show()

    pass
