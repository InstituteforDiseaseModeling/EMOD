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


color_by_group = ['r', 'b']

plot_channel_names = [ 'Latent TB Population', 'Active TB Population', 'Num ProviderOrdersTBTest', 'Num TBTestDefault', 'Num TBTestNegative', 'Num TBTestPositive' ]

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
            x_range = range( 0, data['Header']['Timesteps'] * data['Header']['Simulation_Timestep'], data['Header']['Simulation_Timestep'])
            channel_data = data['Channels'][channel_name]['Data']
            if channel_name.startswith( 'Num' ):
                data_to_plot = reduce(cumulative, channel_data, [])
                subplot.plot( x_range[:-1], data_to_plot[:-1], color_by_group[ trace_idx ], label=labels[ trace_idx ] )
            else:
                subplot.plot( x_range[:-1], channel_data[:-1], color_by_group[ trace_idx ], label=labels[ trace_idx ] )

        #subplot.tick_params( axis='both', which='major', labelsize=8 )
        first_entry = file_data[0]
        data = first_entry[1]
        plt.xticks( range( 0, data['Header']['Timesteps'] * data['Header']['Simulation_Timestep'] + 1, 200 ) , fontsize = 8)
        plt.yticks(fontsize = 8)
        plt.ylim(0, 10200)
        if len(filenames) > 1:
            plt.ylim(0, 20200)
        
        plt.title( channel_name.replace('Num ', 'Cumulative '), fontsize=8 )
        #subplot.legend( loc = 7 )
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
