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


color_by_group = ['r', 'b', 'g']

plot_channel_names = [ 'Statistical Population', 'Latent TB Population', 'Active TB Population', 'Latent PendingRelapse TB Population', 'Active TB Population Retx' ]
trace_property_names = [ 'Public', 'Private', 'None' ]

def plot_file_data(filename, save, title):

    with open( filename, 'r' ) as handle:
        file_data = json.load(handle)

    pp = PdfPages( 'figures.pdf' ) if save else None

    plt.figure()
    fig = gcf()
    #fig.suptitle( title )

    for plot_idx, channel_name in enumerate(plot_channel_names):
        num_rows = 2
        num_cols = 3

        subplot = plt.subplot( num_rows, num_cols, plot_idx + 1 )
        x_range = range( 0, file_data['Header']['Timesteps'] * file_data['Header']['Simulation_Timestep'], file_data['Header']['Simulation_Timestep'])

        for trace_idx, property in enumerate(trace_property_names):
            full_channel_name = channel_name + ' ' + property
            channel_data = file_data['Channels'][full_channel_name]['Data']
            subplot.plot( x_range[:-1], channel_data[:-1], color_by_group[ trace_idx ], label=property )

        #subplot.tick_params( axis='both', which='major', labelsize=8 )
        plt.xticks( range( 0, file_data['Header']['Timesteps'] * file_data['Header']['Simulation_Timestep'] + 1, 200 ), fontsize=8 )
        plt.yticks(fontsize = 8)
        plt.title( channel_name, fontsize=8 )
        if plot_idx == 0:
            subplot.legend( loc = 1,  fontsize='x-small')
        plt.xlabel( 'Simulation Time, days', fontsize = 8 )
        plt.ylabel( 'Number of People', fontsize = 8 )
        if channel_name == 'Statistical Population':
            plt.ylim(0, 10000)
        else:
            plt.ylim(0, 3500)

    plt.tight_layout( rect=[ 0,0,1,0.95 ] )

    if pp:
        pp.savefig(fig)
        pp.close()

    pass


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('filename', default='Report_Scenarios.json', help='Source data filename [Report_Scenarios.json]')
    parser.add_argument('-s', '--save', default=False, action='store_true')
    parser.add_argument('-t', '--title', default=None)
    args = parser.parse_args()
    if not args.title:
        args.title = args.filename

    plot_file_data(args.filename, args.save, args.title)

    plt.show()

    pass
