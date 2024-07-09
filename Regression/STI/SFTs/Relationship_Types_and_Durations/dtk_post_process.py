#!/usr/bin/python

if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_STI_Support as sti_support
import json
from scipy import stats
import matplotlib.pyplot as plt
from scipy.stats import gaussian_kde
import numpy as np
import math

"""
This test is developed from STI scenario: 01_STI_Network\D_Relationship_Types_and_Durations.

STI model supports four different types of heterosexual relationships: transitory, informal, marital, and commercial.  
Each relationship duration is governed by a Weibull distribution, which determines the duration that is assigned to a 
relationship upon formation. The duration time is then used to calculate the scheduled end time of the relationship.

In this test, we load data from output files: RelationshipStart.csv and RelationshipEnd.csv and perform the 
following tests:

1. In RelationshipEnd.csv, the actual relationship end time should match the schedule relationship end time.
2. RelationshipStart.csv should match RelationshipEnd.csv.
3. Durations for each relationship type should pass the kstest for expected Weibull distribution. Parameters for Weibull
   distribution are loaded from demographics overlay file for each type.

Output: scientific_feature_report.txt
        INFORMAL_duration.png
        INFORMAL_pdf.png
        MARITAL_duration.png
        MARITAL_pdf.png
        TRANSITORY_duration.png
        TRANSITORY_pdf.png
        unexpected_rel_IDs_in_end_report.csv(if test failed in step 2)
        
"""


def parse_demo_file(demo_file_name="../Inputs/pfa_simple.json"):
    """
        Parse demo file and get Duration_Weibull_Heterogeneity and Duration_Weibull_Scale for transitory, informal and
        marital relationships.

    Args:
        demo_file_name:

    Returns: demo_object

    """
    with open(demo_file_name, 'r') as demo_file:
        society = json.load(demo_file)[sti_support.DemoPfaKeys.defaults][sti_support.DemoPfaKeys.society]
        transitory = {sti_support.DemoPfaKeys.heterogeneity:
                          society[sti_support.DemoPfaKeys.transitory]
                          [sti_support.DemoPfaKeys.relationship_parameters]
                          [sti_support.DemoPfaKeys.heterogeneity],
                      sti_support.DemoPfaKeys.scale:
                          society[sti_support.DemoPfaKeys.transitory]
                          [sti_support.DemoPfaKeys.relationship_parameters]
                          [sti_support.DemoPfaKeys.scale]}
        informal = {sti_support.DemoPfaKeys.heterogeneity:
                        society[sti_support.DemoPfaKeys.informal]
                        [sti_support.DemoPfaKeys.relationship_parameters]
                        [sti_support.DemoPfaKeys.heterogeneity],
                    sti_support.DemoPfaKeys.scale:
                        society[sti_support.DemoPfaKeys.informal]
                        [sti_support.DemoPfaKeys.relationship_parameters]
                        [sti_support.DemoPfaKeys.scale]}
        marital = {sti_support.DemoPfaKeys.heterogeneity:
                       society[sti_support.DemoPfaKeys.marital]
                       [sti_support.DemoPfaKeys.relationship_parameters]
                       [sti_support.DemoPfaKeys.heterogeneity],
                   sti_support.DemoPfaKeys.scale:
                       society[sti_support.DemoPfaKeys.marital]
                       [sti_support.DemoPfaKeys.relationship_parameters]
                       [sti_support.DemoPfaKeys.scale]}

        demo_object = {sti_support.DemoPfaKeys.transitory: transitory,
                       sti_support.DemoPfaKeys.informal: informal,
                       sti_support.DemoPfaKeys.marital: marital}
    return demo_object


def create_report_file(end_df, start_df, demo_object, end_report_name, start_report_name, config_filename,
                       output_report_name, debug):
    succeed = True
    with open(output_report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")
        output_report_file.write(f"Checking if actual relationship durations and scheduled relationship durations are "
                                 f"matched in {end_report_name}.\n")
        for row in end_df.itertuples(index=False):
            actual_duration = row.Actual_duration
            scheduled_duration = row.Scheduled_duration
            # all relationships are end due to breakup since there are no vital dynamic and migration in this test.
            # ToDo: is 1 the simulation timestep or update period?
            if not (actual_duration >= math.ceil(scheduled_duration) and actual_duration - scheduled_duration <= 1):
                succeed = False
                output_report_file.write(f"\t BAD: the actual relationship duration for Rel_ID = "
                                         f"{row.Rel_ID} is incorrect. The schedule relationship "
                                         f"duration is {scheduled_duration} and actual relationship duration is "
                                         f"{actual_duration}.\n")
        if succeed:
            output_report_file.write("\t GOOD: actual and scheduled durations are matched.\n")
        else:
            output_report_file.write("\t BAD: actual and scheduled durations aren't matched.\n")

        output_report_file.write(f"Checking if {start_report_name} and {end_report_name} are matched.\n")
        durations = {sti_support.DemoPfaKeys.transitory: [],
                     sti_support.DemoPfaKeys.informal: [],
                     sti_support.DemoPfaKeys.marital: []}
        result = True
        simulation_duration = dtk_sft.get_simulation_duration(config_filename)

        for row in start_df.itertuples(index=False):
            rel_end = row.Rel_scheduled_end_time
            rel_type = row.Rel_type
            # RelationshipEnd.csv contains only relationships which has end time before the simulation end time.
            if rel_end < simulation_duration - 1:
                rel_start = row.Rel_start_time
                rel_id = row.Rel_ID
                a_id = row.A_ID  # male id
                b_id = row.B_ID  # female id

                end_df_row = end_df[end_df[sti_support.ReportHeader.rel_ID] == rel_id]
                if end_df_row.empty:  # RelationshipEnd.csv doesn't have this relationship.
                    succeed = result = False
                    output_report_file.write(f"\tBAD: {sti_support.ReportHeader.rel_ID} = {rel_id} is not found in "
                                             f"{end_report_name}, while it's in {start_report_name}.\n")
                else:
                    # comment out this line to use filter at the end instead of drop row at every time step for better performance.
                    # end_df.drop(end_df[end_df[ReportHeader.rel_ID] == rel_id].index, inplace=True)

                    # check relationship start time
                    rel_start_2 = end_df_row[sti_support.ReportHeader.rel_start_time].iloc[0]
                    if rel_start != rel_start_2:
                        succeed = result = False
                        output_report_file.write(f"\t BAD: the {sti_support.ReportHeader.rel_start_time} for "
                                                 f"{sti_support.ReportHeader.rel_ID} = {rel_id} "
                                                 f"is {rel_start} in {start_report_name} while it's {rel_start_2} in "
                                                 f"{end_report_name}.\n")

                    # check relationship end time
                    rel_end_2 = end_df_row[sti_support.ReportHeader.rel_scheduled_end_time].iloc[0]
                    if rel_end != rel_end_2:
                        succeed = result = False
                        output_report_file.write(f"\t BAD: the {sti_support.ReportHeader.rel_scheduled_end_time} "
                                                 f"for {sti_support.ReportHeader.rel_ID} = {rel_id} is {rel_end} "
                                                 f"in {start_report_name} "
                                                 f"while it's {rel_end_2} in {end_report_name}.\n")

                    # check male id
                    a_id_2 = end_df_row[sti_support.ReportHeader.male_ID].iloc[0]
                    if a_id != a_id_2:
                        succeed = result = False
                        output_report_file.write(f"\t BAD: the {sti_support.ReportHeader.a_ID} for "
                                                 f"{sti_support.ReportHeader.rel_ID} = {rel_id} "
                                                 f"is {a_id} in {start_report_name} while it's {a_id_2} in "
                                                 f"{end_report_name}.\n")

                    # check female id
                    b_id_2 = end_df_row[sti_support.ReportHeader.female_ID].iloc[0]
                    if b_id != b_id_2:
                        succeed = result = False
                        output_report_file.write(f"\t BAD: the {sti_support.ReportHeader.b_ID} for "
                                                 f"{sti_support.ReportHeader.rel_ID} = {rel_id} "
                                                 f"is {b_id} in {start_report_name} while it's {b_id_2} in "
                                                 f"{end_report_name}.\n")

                    # check relationship type
                    rel_type_2 = end_df_row[sti_support.ReportHeader.rel_type].iloc[0]
                    if rel_type != rel_type_2:
                        succeed = result = False
                        output_report_file.write(f"\t BAD: the {sti_support.ReportHeader.rel_type} for "
                                                 f"{sti_support.ReportHeader.rel_ID} = {rel_id} "
                                                 f"is {rel_type} in {start_report_name} while it's {rel_type_2} in "
                                                 f"{end_report_name}.\n")

            # store duration(year) for all relationships in RelationshipStart.csv including those have end time
            # longer than simulation duration.
            relationship = sti_support.relationship_table[rel_type]
            scheduled_duration_year = row.Scheduled_duration_year
            durations[relationship].append(scheduled_duration_year)

        # collect all rel_ids from RelationshipStart.csv for those have end time before simulation ends.
        expected_rel_IDs = start_df[start_df[sti_support.ReportHeader.rel_scheduled_end_time] <
                                    simulation_duration - 1][sti_support.ReportHeader.rel_ID]
        # remove these rel_ids in RelationshipEnd.csv and store the remaining rel_ids if any in a new data frame.
        # there should not be any remaining rel_ids so the new df should be empty
        unexpected_rel_IDs_in_end_df = end_df[~end_df[sti_support.ReportHeader.rel_ID].isin(expected_rel_IDs)]

        # check if the new df is empty
        if not unexpected_rel_IDs_in_end_df.empty:
            succeed = result = False
            unexpected_rel_IDs_csv = "unexpected_rel_IDs_in_end_report.csv"
            output_report_file.write(f"\t BAD: Some Relationships in {end_report_name} are not found in "
                                     f"{start_report_name}. Please see {unexpected_rel_IDs_csv} for a list of these "
                                     f"relationships.\n")
            with open(unexpected_rel_IDs_csv, 'w') as end_csv_file:
                unexpected_rel_IDs_in_end_df.to_csv(end_csv_file)

        if result:
            output_report_file.write(f"\t GOOD: {end_report_name} and {start_report_name} are matched.\n")
        else:
            output_report_file.write(f"\t BAD: {end_report_name} and {start_report_name} aren't matched.\n")

        output_report_file.write("Checking if relationship durations are from expected Weibull distributions.\n")

        result = True
        for rel_type, duration in durations.items():
            output_report_file.write(f"\t Running kstest for Weibull distribution for rel_type = {rel_type}.\n")
            scale = demo_object[rel_type][sti_support.DemoPfaKeys.scale]
            heterogeneity = demo_object[rel_type][sti_support.DemoPfaKeys.heterogeneity]
            if debug:
                output_report_file.write(f"\t scale = {scale}, "
                                         f"heterogeneity = {heterogeneity}, "
                                         f"shape = {1/heterogeneity}.\n")
            ks_result = dtk_sft.test_weibull(duration,
                                             scale,
                                             1/heterogeneity,
                                             output_report_file)
            if not ks_result:
                succeed = result =False
                output_report_file.write(f"\t BAD: ks test for {rel_type} failed.\n")
            else:
                output_report_file.write(f"\t GOOD: ks test for {rel_type} passes.\n")
            dist_weibull_scipy = stats.weibull_min.rvs(c=1/heterogeneity,
                                                       loc=0,
                                                       scale=scale,
                                                       size=len(duration))
            # plot rel_duration and scipy weibull distribution for each relationship type.
            dtk_sft.plot_data(duration, dist_weibull_scipy, label1=f"{rel_type} duration", label2="weibull distribution",
                              title=f"Relationship Duration_{rel_type}\n"
                                    f"{sti_support.DemoPfaKeys.heterogeneity} = "
                                    f"{round(heterogeneity, 2)}, "
                                    f"{sti_support.DemoPfaKeys.scale} = {round(scale, 2)}",
                              xlabel="data points", ylabel="duration(years)",
                              category=f"{rel_type}_duration", sort=True, overlap=True)

            # plot pdf for rel_duration, scipy weibull distribution and theoretical pdf for weibull distribution.
            fig = plt.figure()
            density = gaussian_kde(dist_weibull_scipy)  # Representation of a kernel-density estimate using Gaussian kernels
                                                        # work best for unimodal distribution
            num_bins = 100
            x_scipy = np.linspace(math.floor(min(dist_weibull_scipy)), math.ceil(max(dist_weibull_scipy)), num_bins)
            x = np.linspace(math.floor(min(duration)), math.ceil(max(duration)), num_bins)

            # plot pdf for rel_duration in years from RelationshipStart.csv
            start_df[start_df[sti_support.ReportHeader.rel_type] == sti_support.inv_relationship_table[rel_type]] \
            [sti_support.ReportHeader.scheduled_duration_year].plot.density(ind=x, color="red")

            # plot pdf for scipy data
            plt.plot(x_scipy, density(x_scipy), color="blue")

            # plot theoretical pdf
            plt.plot(x, stats.weibull_min.pdf(x,
                                              c=1/heterogeneity,
                                              loc=0,
                                              scale=scale),
                     color="green", linestyle='dashed')
            plt.xlabel("Durations(years)")
            plt.title(f"{rel_type} durations pdf")
            plt.legend([f"{rel_type} durations", "scipy weibull distribution", "weibull pdf(theoretical)"], loc=0)
            plt.savefig(f'{rel_type}_pdf.png')
            if dtk_sft.check_for_plotting():
                plt.show()
            plt.close(fig)

        if result:
            output_report_file.write(f"\t GOOD: relationship durations are from expected Weibull distributions.\n")
        else:
            output_report_file.write(f"\t BAD: relationship durations aren't from expected Weibull distributions.\n")

        output_report_file.write(dtk_sft.format_success_msg(succeed))

        return succeed


def application(output_folder="output", stdout_filename="test.txt",
                end_report_name="RelationshipEnd.csv",
                start_report_name="RelationshipStart.csv",
                config_filename="config.json", pfa_filename="pfa_simple.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("end_report_name: " + end_report_name + "\n")
        print("start_report_name: " + start_report_name + "\n")
        print("config_filename: " + config_filename + "\n")
        print("pfa_filename: " + pfa_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done(stdout_filename)

    end_df = sti_support.parse_relationship_end_report(report_path=output_folder, report_filename=end_report_name)
    start_df = sti_support.parse_relationship_start_report(report_path=output_folder, report_filename=start_report_name)
    demo_obj = parse_demo_file(demo_file_name=pfa_filename)
    create_report_file(end_df, start_df, demo_obj, end_report_name, start_report_name, config_filename, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-e', '--end_report', default="RelationshipEnd.csv",
                        help="Relationship end report to parse (RelationshipEnd.csv)")
    parser.add_argument('-S', '--start_report', default="RelationshipStart.csv",
                        help="Relationship start report to parse (RelationshipStart.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-p', '--pfa', default="pfa_simple.json",
                        help="PFA file name to load (pfa_simple.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                end_report_name=args.end_report,
                start_report_name=args.start_report,
                config_filename=args.config, pfa_filename=args.pfa,
                report_name=args.reportname, debug=args.debug)

