#!/usr/bin/python

import pandas as pd
import os
import json
import matplotlib.pyplot as plt
import numpy as np
import math
import seaborn as sns
from scipy import stats
"""
SUMMARY: Plot theoretical and observed relationship duration distributions
INPUT: 
   1. pfa_simple.json
   2. output/RelationshipEnd.csv
OUTPUT: figs/RelationshipDuration_python.png
NOTE: This script is using libraries "matplotlib" and "seaborn" to generate the graph.  
"""


class DemoPfaKeys():
    defaults = "Defaults"
    society = "Society"
    transitory = "TRANSITORY"
    informal = "INFORMAL"
    marital = "MARITAL"
    relationship_parameters = "Relationship_Parameters"
    heterogeneity = "Duration_Weibull_Heterogeneity"
    scale = "Duration_Weibull_Scale"


class ReportHeader():
    rel_type = "Rel_type"
    scheduled_duration_year = "Scheduled_duration_year"
    rel_actual_end_time = "Rel_actual_end_time"
    rel_start_time = "Rel_start_time"
    rel_scheduled_end_time = "Rel_scheduled_end_time"
    type = "Type"


relationship_table = {0: DemoPfaKeys.transitory, 1: DemoPfaKeys.informal, 2: DemoPfaKeys.marital}


def parse_relationship_end_report(report_path="output", report_filename="RelationshipEnd.csv"):
    """
    parse relationship report and store the content as a data frame
    :param report_path:
    :param report_filename:
    :return: report_df
    """
    report_file = os.path.join(report_path, report_filename)
    report_df = pd.read_csv(report_file)
    report_df[ReportHeader.rel_type] = report_df.filter(like=ReportHeader.rel_type)
    DAYS_PER_YEAR = 365
    report_df[ReportHeader.scheduled_duration_year] = (report_df[ReportHeader.rel_scheduled_end_time] -
                                                 report_df[ReportHeader.rel_start_time]) / DAYS_PER_YEAR

    return report_df


def parse_demo_file(demo_file_name="../../../../STI/SFTs/Inputs/pfa_simple.json"):
    """
    Parse demo file and get Duration_Weibull_Heterogeneity and Duration_Weibull_Scale for transitory, informal and
    marital relationships.
    :param demo_file_name:
    :return: demo_object
    """
    with open(demo_file_name, 'r') as demo_file:
        society = json.load(demo_file)[DemoPfaKeys.defaults][DemoPfaKeys.society]
        transitory = {DemoPfaKeys.heterogeneity:
                          society[DemoPfaKeys.transitory][DemoPfaKeys.relationship_parameters][DemoPfaKeys.heterogeneity],
                      DemoPfaKeys.scale:
                          society[DemoPfaKeys.transitory][DemoPfaKeys.relationship_parameters][DemoPfaKeys.scale]}
        informal = {DemoPfaKeys.heterogeneity:
                        society[DemoPfaKeys.informal][DemoPfaKeys.relationship_parameters][DemoPfaKeys.heterogeneity],
                    DemoPfaKeys.scale:
                        society[DemoPfaKeys.informal][DemoPfaKeys.relationship_parameters][DemoPfaKeys.scale]}
        marital = {DemoPfaKeys.heterogeneity:
                       society[DemoPfaKeys.marital][DemoPfaKeys.relationship_parameters][DemoPfaKeys.heterogeneity],
                   DemoPfaKeys.scale:
                       society[DemoPfaKeys.marital][DemoPfaKeys.relationship_parameters][DemoPfaKeys.scale]}

        demo_object = {DemoPfaKeys.transitory: transitory,
                       DemoPfaKeys.informal: informal,
                       DemoPfaKeys.marital: marital}
    return demo_object


def plot_relationship_duration(end_df, demo_object, file_dir="figs", fig_name="RelationshipDuration_python_sns.png"):
        if not os.path.isdir(file_dir):
            os.mkdir(file_dir)

        # plot pdf for rel_duration and theoretical pdf for weibull distribution.
        fig = plt.figure()
        ax = fig.add_axes([0.12, 0.12, 0.6, 0.76])

        # end_df[ReportHeader.rel_type] = end_df[ReportHeader.rel_type].astype('int64')

        color_table = {0: 'red', 1: 'green', 2: '#1E90FF'}
        for rel_key in relationship_table.keys():
            df = end_df[end_df[ReportHeader.rel_type] == rel_key]
            color = color_table[rel_key]
            num_bins = 100
            x = np.linspace(math.floor(df[ReportHeader.scheduled_duration_year].min()),
                            math.ceil(df[ReportHeader.scheduled_duration_year].max()), num_bins)

            # plot pdf for rel_duration in years from RelationshipEnd.csv
            #df[ReportHeader.scheduled_duration_year].plot.area(color=color)
            #df[ReportHeader.scheduled_duration_year].plot.density(ind=x, color=color)
            sns.kdeplot(df[ReportHeader.scheduled_duration_year], color=color, shade=True)

            # plot theoretical pdf
            ax.plot(x, stats.weibull_min.pdf(x,
                                             c=1/demo_object[relationship_table[rel_key]][DemoPfaKeys.heterogeneity],
                                             loc=0,
                                             scale=demo_object[relationship_table[rel_key]][DemoPfaKeys.scale]),
                    color=color, linestyle='dashed')
        ax.set_xlim(left=-1, right=40.5)
        ax.set_ylim(bottom=-0.01, top=0.8)
        ax.set_xlabel("Durations(years)")
        ax.set_ylabel("Density")
        ax.set_title(f"Relationship Duration by Type")
        ax.legend(["Transitory", "Transitory\n(theoretical)",
                   "Informal", "Informal\n(theoretical)",
                   "Marital", "Marital\n(theoretical)"], loc='center left', bbox_to_anchor=(1, 0.5))
        plt.grid(color='gray', linestyle='-', linewidth=0.1)
        # ax.set_facecolor("lightgray")
        plt.savefig(os.path.join(file_dir, fig_name))
        plt.show()


def application(output_folder="output", report_filename="RelationshipEnd.csv",
                pfa_filename="../../../../STI/SFTs/Inputs/pfa_simple.json", fig_dir="figs",
                fig_name="RelationshipDuration_python_sns.png"):
    end_df = parse_relationship_end_report(report_path=output_folder, report_filename=report_filename)
    demo_obj = parse_demo_file(demo_file_name=pfa_filename)
    plot_relationship_duration(end_df, demo_obj, file_dir=fig_dir, fig_name=fig_name)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-r', '--report', default="RelationshipEnd.csv", help="Relationship report to load "
                                                                              "(RelationshipEnd.csv)")
    parser.add_argument('-p', '--pfa_filename', default="../../../../STI/SFTs/Inputs/pfa_simple.json",
                        help="pfa json file to load (../../../../STI/SFTs/Inputs/pfa_simple.json)")
    parser.add_argument('-d', '--fig_dir', default="figs", help="directory to save the plot(figs)")
    parser.add_argument('-n', '--fig_name', default="RelationshipDuration_python_sns.png",
                        help="filename for plot(RelationshipDuration_python_sns.png)")

    args = parser.parse_args()

    application(output_folder=args.output, report_filename=args.report,
                pfa_filename=args.pfa_filename, fig_dir=args.fig_dir,
                fig_name=args.fig_name)

