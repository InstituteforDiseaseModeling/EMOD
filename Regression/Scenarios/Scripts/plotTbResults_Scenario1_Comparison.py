from __future__ import division #allows you to do non-integer (ie floating point) division
from __future__ import print_function
import argparse
import json
import matplotlib.pyplot as plt
import numpy
import os
import subprocess
from matplotlib.backends.backend_pdf import PdfPages
from pylab import *


def compareBaselineToNew(comparison_filename, baseline_filename, channels, save_filename):

    with open(baseline_filename, 'r') as handle:
        baseline_output_data = json.load( handle )

    baseline_channels = baseline_output_data['Channels']

    with open(comparison_filename, 'r') as handle:
        new_output_data = json.load( handle )

    comparison_channels = new_output_data['Channels']

    if len(channels) == 0:
        channels = sorted(baseline_channels)

    pp = PdfPages(save_filename) if save_filename else None

    for plot_idx, chan_title in enumerate(channels):
        num_rows = 3
        num_cols = 3
        num_graphs = num_rows*num_cols

        if (plot_idx % num_graphs) == 0:
            if not plot_idx == 0:
                plt.tight_layout( rect = [0, 0, 1, 0.95])
                if pp:
                    pp.savefig(fig)

            figure_num = plot_idx / num_graphs
            plt.figure()
            fig = gcf()
            #fig.suptitle("Red = baseline, Blue = " + comparison_filename)

        if chan_title != 'Latent Slow TB Prevalence':
            baseline_data = baseline_channels[chan_title]["Data"]
            comparison_data = comparison_channels[chan_title]["Data"]
        else:
            baseline_data = numpy.array(baseline_channels['Latent TB Prevalence']['Data']) - numpy.array(baseline_channels['Latent Fast TB Prevalence']['Data'])
            comparison_data = numpy.array(comparison_channels['Latent TB Prevalence']['Data']) - numpy.array(comparison_channels['Latent Fast TB Prevalence']['Data'])

        subplot = plt.subplot( num_rows,num_cols, plot_idx +1 - figure_num*num_graphs)
        subplot.plot( baseline_data, 'r')
        if comparison_filename != baseline_filename:
            subplot.plot( comparison_data, 'b')
        subplot.tick_params(axis = 'both', which= 'major', labelsize=8)
        plt.xticks(range(0, baseline_output_data["Header"]["Timesteps"] + 1, 200))
        if chan_title == 'Active Sx Smear neg Prevalence' or 'Active Sx Extrapulm Prevalence':
            plt.ylim(0, 0.25)
        if 'Latent' in chan_title:
            plt.ylim(0,0.45)
        plt.title( chan_title, fontsize = 8 )
        plt.xlabel('Simulation Time, days', fontsize = 8)
        plt.ylabel('Prevalence', fontsize = 8)

    plt.tight_layout( rect = [0, 0, 1, 0.95])

    #save the last figure
    if pp:
        pp.savefig(fig)
        pp.close()

    plt.show()
    pass


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('baseline', help='Baseline inset chart filename')
    parser.add_argument('compare', help='Comparison inset chart filename')
    parser.add_argument('-c', '--channel', default=[], action='append', help='Channel(s) to display')
    parser.add_argument('-s', '--save', help='Filename for saving figure(s)')
    args = parser.parse_args()
    print("Comparing '{0}' against '{1}'".format(args.compare, args.baseline))
    print("Saving figures to '{0}'".format(args.save) if args.save else "Not saving figures to file.")
    print("Displaying channels {0}".format(args.channel) if (len(args.channel) > 0) else "Displaying default channels.")
    compareBaselineToNew(args.compare, args.baseline, args.channel, args.save)

    pass