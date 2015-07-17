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


color_by_group = ['r', 'b', 'g', 'y', 'm']

plot_channel_names = [ 'Active TB Population', 'Active TB Population MDR', 'Active TB Population Acquired MDR' ]

def plot_file_data(filenames, labels, save, title):

    def cumulative(aggregate, value):
        aggregate.append(aggregate[-1] + value if len(aggregate) else value)
        return aggregate

    file_data = []

    for filename in filenames:
        with open( filename, 'r' ) as handle:
            file_data.append( (filename, json.load( handle )) )

    pp = PdfPages( 'figures.pdf' ) if save else None

    plt.figure()
    fig = gcf()
    #fig.suptitle( title )

    for plot_idx, channel_name in enumerate(plot_channel_names):
        num_rows = 2
        num_cols = 3

        subplot = plt.subplot( num_rows, num_cols, plot_idx + 1 )

        for trace_idx, entry in enumerate(file_data):
            filename, data = entry
            channel_data = data['Channels'][channel_name]['Data']
            x_range = range( 0, data['Header']['Timesteps'] * data['Header']['Simulation_Timestep'], data['Header']['Simulation_Timestep'])

            if not 'Acquired' in channel_name:
                subplot.plot( x_range[:-1], channel_data[:-1], color_by_group[ trace_idx ], label=labels[ trace_idx ] )
                plt.ylim(0, 10200)
            else:
                numerator = data['Channels']['Active TB Population MDR']['Data']
                fraction = map(lambda x,y: (x / y) if y else 0, channel_data, numerator)
                subplot.plot( x_range[:-1], fraction[:-1], color_by_group[ trace_idx ], label='Fraction MDR from Acquisition' )
                plt.ylim(0, 1.02)

        #subplot.tick_params( axis='both', which='major', labelsize=8 )
        first_entry = file_data[0]
        data = first_entry[1]
        plt.xticks( range( 0, data['Header']['Timesteps'] * data['Header']['Simulation_Timestep'] + 1, 50 ) , fontsize = 8)
        plt.yticks(fontsize = 8)
        plt.title( channel_name.replace('Num ', 'Cumulative '), fontsize=8 )
        #subplot.legend( loc = 'best' )
        plt.xlabel( 'Simulation Time, days', fontsize = 8 )
        plt.ylabel( 'Number of People', fontsize = 8 )

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
