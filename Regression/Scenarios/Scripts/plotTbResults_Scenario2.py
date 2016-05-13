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


# module level variables
main_dir = None
dll_age_bins = ['0-4', '5-9', '10-14', '15-19', '20-24', '25-29', '30-34', '35-39', '40-44', '45-49', '50-54', '55-59', '60-64', '65-69', '70-74', '75-79', '80-84', '85-89', '90-94', '95+' ]
#sorry don't know how to get this to output from the dll
color_by_group = ['r', 'b', 'g', 'y', 'm']
sim_tstep = 5


def plotChannelAtTime(channel_name, output_data, time_list, subplot):

    channel_age_bin = [0] * len(dll_age_bins)
    bar_width = 1/len(time_list) - 0.1

    for time_index, time_to_plot in enumerate(time_list):

        for age_bin_index, age_bin_name in enumerate(dll_age_bins):
            channel_age_bin[age_bin_index] = output_data["Channels"][channel_name + ' '+ age_bin_name]["Data"][time_to_plot]

        ticks = [x+(time_index) * bar_width for x in range(1, len(dll_age_bins)+1, 1)]
        subplot.bar(ticks, channel_age_bin, bar_width, color= color_by_group[time_index + 2], edgecolor = 'none', label = ' year '+ str(int(time_to_plot * sim_tstep / 365)), align='center')
        plt.xlim(-0.5, 19)

        subplot.tick_params(axis = 'both', which= 'major', labelsize=7)

    locs, labels = plt.xticks(ticks, dll_age_bins)
    plt.setp(labels, rotation=90)
    subplot.legend(fontsize = 'xx-small', loc = 1)
    plt.xlabel("Age group", fontsize = 8)
    plt.ylabel("Number of people", fontsize = 8)
    plt.title( 'Population Distribution', fontsize = 8 )

    pass


def plotChannelOverTime(channel_name, channel_name_normalization, output_data, groups, subplot):

    for group_index, group in enumerate(groups):

        channel_group = output_data["Channels"][channel_name + ' '+ group]["Data"]

        if channel_name_normalization != '':
            channel_group = [x/y if y else 0 for x, y in zip(channel_group, output_data["Channels"][channel_name_normalization + ' '+ group]["Data"])]

        xvalues = [x/(365/sim_tstep) for x in range(1, len(channel_group)+1, 1)]
        plt.xlabel("Simulation Year", fontsize = 8)

        subplot.plot(xvalues, channel_group, color = color_by_group[group_index], label = str(group))
        subplot.tick_params(axis = 'both', which= 'major', labelsize=7)

    subplot.legend(fontsize = 'x-small', loc = 1)
    plt.ylim(-0.02, 0.55)
    plt.title( channel_name, fontsize = 8 )

    pass


def compareBaselineToNewDemographics(compare_path, save):

    with open( os.path.join(compare_path, 'Report_Scenarios.json'), 'r' ) as handle:
        output_data = json.load( handle )

    num_chans = output_data["Header"]["Channels"]
    pp = PdfPages('figures2.pdf') if save else None
    groups = ['Adult', 'Child']

    plt.figure()
    fig = gcf()
    #fig.suptitle(compare_path)

    time_list = [int(i) for i in [0, 1825/sim_tstep, 3650/sim_tstep]]

    #bar graph of the population
    subplot = plt.subplot( 2,3, 1)
    plotChannelAtTime('Statistical Population', output_data, time_list, subplot)
    #disease state of adults and children over time
    subplot = plt.subplot( 2,3, 2)
    plotChannelOverTime('Latent TB Population', 'Statistical Population', output_data, groups, subplot)
    plt.title( 'Latent TB Prevalence', fontsize = 8 )
    plt.ylabel("Proportion", fontsize = 8)
    subplot = plt.subplot( 2,3, 3)
    plotChannelOverTime('Active TB Population', 'Statistical Population', output_data, groups, subplot)
    plt.title( 'Active TB Prevalence', fontsize = 8 )
    plt.ylabel("Proportion", fontsize = 8)
    subplot = plt.subplot( 2,3, 4)
    plotChannelOverTime('Immune Population', 'Statistical Population', output_data, groups, subplot)
    plt.title( 'Immune Prevalence', fontsize = 8 )
    plt.ylabel("Proportion", fontsize = 8)
    plt.ylim(0,1.1)
    subplot.legend(fontsize = 'x-small', loc = 3)

    subplot = plt.subplot( 2,3, 5)
    plotChannelOverTime('Latent TB Fast Population', 'Statistical Population', output_data, groups, subplot)
    plt.title( 'Latent Fast TB Prevalence', fontsize = 8 )
    plt.ylabel("Proportion", fontsize = 8)
    subplot = plt.subplot( 2,3, 6)
    plotChannelOverTime('Latent TB Slow Population', 'Statistical Population', output_data, groups, subplot)
    plt.title( 'Latent Slow TB Prevalence', fontsize = 8 )
    plt.ylabel("Proportion", fontsize = 8)
    #save the last figure
    plt.tight_layout( rect = [0, 0, 1, 0.95])

    if pp:
        pp.savefig(fig)
        pp.close()

    pass



if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('compare', help='Comparison inset chart filename')
    parser.add_argument('-s', '--save', default=False, action='store_true')
    parser.add_argument('-t', '--timestep', default=5, type=int)
    args = parser.parse_args()

    sim_tstep = args.timestep

    compareBaselineToNewDemographics(args.compare, args.save)

    plt.show()

    pass
