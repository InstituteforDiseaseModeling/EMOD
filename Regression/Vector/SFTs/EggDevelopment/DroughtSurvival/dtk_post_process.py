#!/usr/bin/python

import re
import json
import math
import pdb
import numpy as np
import pandas as pd
import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import dtk_test.dtk_sft_svetlana as svet

"""
Tests both the presence of eggs during drought and if they survive according to the correct survival rate, which should 
be independent of climatic factors.

In the VectorGenetics2 branch, there are multiple cohorts of eggs.  However, this test only looks at the survival
rate of the egg population, that is to say the new population over the old population.  Since cohorts will have the 
same survival rate, everything about this test should be approximately the same, provided that the cohorts are 
dying at approximately the same rate, which they should to pass this test.  Therefore, this test treats all cohorts
the same assuming that they all die at approximately the same rate, and the test passes as is with two cohorts.

A note that maybe this should be checked because the cohorts are male and female, though there is not logging for this
yet.
"""


def parse_output_file(stdout, debug=True):
    eggs_present_list = []
    drought_survival_list = []  # egg survival rate should be the same regardless of climatic factors
    new_pop_list = []
    eggs_present = False
    d = {}
    filtered_lines = []
    time = 0
    time_list = []
    total_small_new_pop = total_small_old_pop = 0
    with open(stdout) as logfile:
        for line in logfile:
            if "Time:" in line:
                time = float(sft.get_val("Time: ", line))
                filtered_lines.append(line)
            if "Updating egg population" in line:
                eggs_present = True
                if "mortality" in line:
                    filtered_lines.append(line)
                    new_pop = float(sft.get_val("new_pop = ", line))
                    old_pop = float(sft.get_val("old_pop = ", line))
                    # Looking at the default survival rate here.  Population suddenly drops to 0 at the end, so to avoid
                    # this we are looking at populations of at least 10.
                    if new_pop >= 100:
                        drought_survival_rate = new_pop / old_pop
                        drought_survival_list.append(drought_survival_rate)
                        new_pop_list.append(new_pop)
                    elif old_pop != 0:
                        total_small_new_pop += new_pop
                        total_small_old_pop += old_pop
            time_list.append(time)
            eggs_present_list.append(eggs_present)

        drought_survival_rate = 0
        if total_small_old_pop > 0:
            drought_survival_rate = total_small_new_pop / total_small_old_pop
            drought_survival_list.append(drought_survival_rate)
            new_pop_list.append(total_small_new_pop)
        filtered_lines.append("DROUGHT small new pop {}/ small old pop{} = survival rate {}"
                              "\n".format(total_small_new_pop, total_small_old_pop, drought_survival_rate))
    d["eggs_present"] = eggs_present_list
    d["time"] = time_list
    output_df = pd.DataFrame.from_dict(d)
    output_df["time"] = output_df["time"].astype(int)
    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)
        output_df.to_csv("DEBUG_Dataframe.csv")
    return output_df, drought_survival_list, new_pop_list


def create_report_file(param_obj, report_name, output_df, drought_survival_list, new_pop_list, debug=False):
    success = True
    svet.plot_scatter_dist_w_avg(output_df['eggs_present'], xaxis=output_df["time"],
                                 title="Time vs. Eggs Present", xlabel="Time",
                                 ylabel="Eggs Present (1=Present, 0=Not)",
                                 category='eggs_present.png', label1="Eggs present")
    with open(report_name, "w") as outfile:
        if debug:
            with pd.option_context('display.max_rows', 400, 'display.max_columns', 3):
                outfile.write(str(output_df))
        outfile.write("# Test name: {}, run number {}\n# Tests egg presence during drought, and "
                      "if the egg survival rate in the simulation matches the specified survival rate."
                      "\n".format(param_obj[veds.ConfigKeys.CONFIG_NAME], param_obj[veds.ConfigKeys.RUN_NUMBER] ))
        if len(output_df) == 0:
            outfile.write(sft.sft_no_test_data)
            raise ValueError("No data found in DataFrame!")

        for index in output_df.index:
            if 150 < output_df.at[index, "time"] and not output_df.at[index, "eggs_present"]:
                success = False
                outfile.write("BAD: There aren't any eggs after time step 150, the vectors should come back.\n")
        if success:
            outfile.write("GOOD: Egg presence is correct; eggs are present throughout the simulation.\n")
        theoretical_rate = param_obj[veds.ConfigKeys.VectorSpeciesParams.EGG_SURVIVAL_RATE]
        tolerance = 5e-2 * theoretical_rate
        lower_bound = theoretical_rate + tolerance
        upper_bound = theoretical_rate - tolerance
        for actual_rate in drought_survival_list:
            if abs(theoretical_rate - actual_rate) > tolerance:
                success = False
                outfile.write("BAD: Theoretical rate, {}, did not match actual rate, {} Should be within 5% boundaries:"
                              " ({}, {}).\n".format(theoretical_rate, actual_rate, lower_bound, upper_bound))
        if success:
            outfile.write("GOOD: Theoretical rates were all within 5% of actual rate.\n")
        theoretical_rate_dist = [theoretical_rate] * len(drought_survival_list)
        svet.plot_scatter_dist_w_avg(drought_survival_list, theory=theoretical_rate_dist,
                                     xaxis=sorted(new_pop_list, reverse=True),
                                     label1='actual survival rate', label2='theoretical survival rate',
                                     title='Egg Survival Rate', xlabel='New Population', ylabel='Survival Rate',
                                     category='survival_vs_population', reverse_x=True)
        outfile.write(sft.format_success_msg(success))


def application(output_folder="output",
                config_filename="config.json",
                stdout_filename="test.txt",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")
    sft.wait_for_done()
    param_obj = veds.load_emod_parameters(config_filename)
    output_df, drought_survival_list, new_pop_list = parse_output_file(stdout_filename, debug)
    create_report_file(param_obj, report_name, output_df, drought_survival_list, new_pop_list, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output,
                config_filename=args.config,
                stdout_filename=args.stdout,
                report_name=args.reportname, debug=args.debug)
