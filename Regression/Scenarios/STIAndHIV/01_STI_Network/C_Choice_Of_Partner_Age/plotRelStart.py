#!/usr/bin/python

import pandas as pd
import os
import json
from scipy import stats
from plotnine import *
"""
SUMMARY: Plot age gaps of partnerships formed for each relationship type
 INPUT: output/RelationshipStart.csv
 OUTPUT:
   1. figs/RelationshipsFormed_Transitory_Python.png
   2. figs/RelationshipsFormed_Informal_Python.png
   3. figs/RelationshipsFormed_Marital_Python.png
   ** Note that figures are only created when at least one relationship of the corresponding type is observed

"""


class DemoPfaKeys():
    defaults = "Defaults"
    society = "Society"
    transitory = "TRANSITORY"
    informal = "INFORMAL"
    marital = "MARITAL"
    pair_formation_parameters = "Pair_Formation_Parameters"
    joint_probabilities = "Joint_Probabilities"


class ReportHeader():
    rel_type = "Rel_type"
    a_age = "A_age"
    a_agebin = "A_agebin"
    b_age = "B_age"
    b_agebin = "B_agebin"
    percent = "Percent"
    counts = "counts"


relationship_table = {0: DemoPfaKeys.transitory, 1: DemoPfaKeys.informal, 2: DemoPfaKeys.marital}


def parse_relationship_start_report(report_path="output", report_filename="RelationshipStart.csv"):
    """
    parse relationship report and store the content as a data frame
    :param report_path:
    :param report_filename:
    :return: report_df
    """
    report_file = os.path.join(report_path, report_filename)
    report_df = pd.read_csv(report_file)
    report_df[ReportHeader.rel_type] = report_df.filter(like=ReportHeader.rel_type)
    report_df[ReportHeader.a_agebin] = report_df[ReportHeader.a_age].apply(lambda x: "<40" if x < 40 else "40+")
    report_df[ReportHeader.b_agebin] = report_df[ReportHeader.b_age].apply(lambda x: "<40" if x < 40 else "40+")

    return report_df


def plot_relationship_duration(start_df, file_dir="figs"):
        if not os.path.isdir(file_dir):
            os.mkdir(file_dir)

        for rel_type, rel_name in relationship_table.items():
            if start_df[start_df[ReportHeader.rel_type] == rel_type].empty:
                continue
            df_c = start_df[start_df[ReportHeader.rel_type] == rel_type].\
                groupby([ReportHeader.a_agebin, ReportHeader.b_agebin]).\
                size().reset_index(name=ReportHeader.counts)
            df_c[ReportHeader.percent] = df_c[ReportHeader.counts] / df_c[ReportHeader.counts].sum()

            fig_name = f"RelationshipsFormed_{rel_name}_Python.png"
            g = (ggplot() +
                 geom_point(data=start_df,mapping=aes(x=ReportHeader.a_age, y=ReportHeader.b_age), color="orange", size=0.5) +
                 coord_fixed() +
                 annotate("text", x=25, y=25,
                          label=str(int(round(100 * df_c[(df_c[ReportHeader.a_agebin] == "<40") &
                                                         (df_c[ReportHeader.b_agebin] == "<40")]
                          [ReportHeader.percent].iloc[0]))) + '%', color="black", size=25) +
                 annotate("text", x=50, y=25,
                          label=str(int(round(100 * df_c[(df_c[ReportHeader.a_agebin] == "40+") &
                                                         (df_c[ReportHeader.b_agebin] == "<40")]
                          [ReportHeader.percent].iloc[0]))) + '%', color="black", size=25) +
                 annotate("text", x=50, y=50,
                          label=str(int(round(100 * df_c[(df_c[ReportHeader.a_agebin] == "40+") &
                                                         (df_c[ReportHeader.b_agebin] == "40+")]
                          [ReportHeader.percent].iloc[0]))) + '%', color="black", size=25) +
                 xlab("Male Age") +
                 ylab("Female Age") +
                 ggtitle(rel_name))

            g.save(os.path.join(file_dir, fig_name))


def application(output_folder="output", report_filename="RelationshipStart.csv", fig_dir="figs"):
    start_df = parse_relationship_start_report(report_path=output_folder, report_filename=report_filename)
    plot_relationship_duration(start_df, file_dir=fig_dir)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-r', '--report', default="RelationshipStart.csv",
                        help="Relationship report to load (RelationshipStart.csv)")
    parser.add_argument('-d', '--fig_dir', default="figs", help="directory to save the plot(figs)")

    args = parser.parse_args()

    application(output_folder=args.output, report_filename=args.report, fig_dir=args.fig_dir)

