from __future__ import division #allows you to do non-integer (ie floating point) division
import argparse
import json
import matplotlib.pyplot as plt
import numpy
import os
import subprocess
from matplotlib.backends.backend_pdf import PdfPages
from pylab import *


def plot_SEIR(filename, figurename):

    with open(filename, 'r') as handle:
        baseline_output_data = json.load( handle )

    channel_data = baseline_output_data['Channels']

    pp = PdfPages(figurename) if figurename else None

    for plot_idx, chan_title in enumerate(sorted(baseline_output_data["Channels"])):
        num_rows = 4
        num_cols = 4
        num_graphs = num_rows*num_cols

        if plot_idx % num_graphs ==0:
            if plot_idx != 0:
                plt.tight_layout( rect = [0, 0, 1, 0.95])
                if pp:
                    pp.savefig(fig)

            figure_num = plot_idx/num_graphs
            plt.figure()
            fig = gcf()
            fig.suptitle("Red = {0}".format( filename ))

        subplot = plt.subplot( num_rows,num_cols, plot_idx +1 - figure_num*num_graphs)
        subplot.plot( baseline_output_data["Channels"][chan_title]["Data"], 'r')
        subplot.tick_params(axis = 'both', which= 'major', labelsize=8)
        plt.xticks(range(0, baseline_output_data["Header"]["Timesteps"] + 1, 100))
        plt.title( chan_title, fontsize = 8 )

    plt.figure()
    fig = gcf()
    fig.suptitle(filename)
    subplot = plt.subplot( 1,1,1 )
    susceptible = [x -x*(y+z+a) for x, y, z, a in zip(channel_data["Statistical Population"]["Data"],
                                                      channel_data["TB Immune Fraction"]["Data"],
                                                      channel_data["Latent TB Prevalence"]["Data"],
                                                      channel_data["Active TB Prevalence"]["Data"])]
    latent_pop = [x*y for x, y in zip(channel_data["Statistical Population"]["Data"],
                                      channel_data["Latent TB Prevalence"]["Data"])]
    active_pop = [x*y for x, y in zip(channel_data["Statistical Population"]["Data"],
                                      channel_data["Active TB Prevalence"]["Data"])]
    recovered_pop = [x*y for x, y in zip(channel_data["Statistical Population"]["Data"],
                                         channel_data["TB Immune Fraction"]["Data"])]
    subplot.plot( susceptible, 'b', label='Susceptible')
    subplot.plot( latent_pop, 'g', label='Latent')
    subplot.plot( active_pop, 'r', label='Active')
    subplot.plot( recovered_pop, 'c', label='Recovered')
    subplot.legend(loc = 7)
    plt.xlabel("Simulation Time")
    plt.ylabel("Number of people")

    plt.tight_layout( rect = [0, 0, 1, 0.95])

    #save the last figure
    if pp:
        pp.savefig(fig)
        pp.close()

    plt.show()

    pass


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('filename', help='Inset chart filename')
    parser.add_argument('-s', '--save', help='Filename to save chart')
    args = parser.parse_args()

    plot_SEIR(args.filename, args.save)

    pass
