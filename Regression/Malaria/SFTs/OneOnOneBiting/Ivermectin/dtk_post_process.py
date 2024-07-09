#!/usr/bin/python

import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import pandas as pd
import os

"""
The Ivermectin intervention is an individual intervention, which kills
Initial_Effect of vectors that bite a person (indoor and outdoor), 
so with Initial_Effect of 0.17 on all three people
we expect 0.17 of vectors that bite that day to die during feeding. . 
This does not affect males as they
do not bite people. 

"""


def create_biting_report_file(param_obj,report_name, output_folder, report_vector_stats, debug=False):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Test name: {str(param_obj[veds.ConfigKeys.CONFIG_NAME])} \n"
                      f"Run number: {str(param_obj[veds.ConfigKeys.RUN_NUMBER])} \n")
        rsv_path = os.path.join(output_folder, report_vector_stats)
        vectors_df = pd.read_csv(rsv_path)
        allowable_error = 0.01
        initial_effect = 0.17
        all_proportions = []
        location = ["Indoor", "Outdoor"]
        for time in range(2, 10):
            today = vectors_df[vectors_df["Time"] == time]
            for where in location:
                bites = today.iloc[0][f"{where}BitesCount"]
                feeding_deaths = today.iloc[0][f"DiedDuringFeeding{where}"]
                death_proportion = feeding_deaths/bites
                all_proportions.append(death_proportion)
                if abs(death_proportion - initial_effect) > allowable_error:
                    success = False
                    outfile.write(f"BAD: We're expecting {initial_effect} of {where} feeding vectors to die, but "
                                  f"{death_proportion} of {where} feeding vectors died,\n that's not within "
                                  f"our allowable error of {allowable_error}. Please check.\n")
                else:
                    outfile.write(f"GOOD: We're expecting {initial_effect} of {where} feeding vectors to die, and"
                                  f"{death_proportion} of {where} feeding vectors died,\n that's within "
                                  f"our allowable error of {allowable_error}.\n")

        average_actual_proportions = sum(all_proportions)/len(all_proportions)
        if abs(average_actual_proportions - initial_effect) > 0.001:
            success = False
            outfile.write(f"BAD: We expect average proportions to be very close to expected {initial_effect}, but "
                          f"we got {average_actual_proportions}. Please check.\n")
        else:
            outfile.write(f"GOOD: We expect average proportions to be very close to expected {initial_effect}, and "
                          f"we got {average_actual_proportions}. Good.\n")
        outfile.write(sft.format_success_msg(success))


def application(output_folder="output",
                config_filename="config.json",
                report_vector_stats="ReportVectorStats.csv",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    param_obj = veds.load_emod_parameters(config_filename)
    create_biting_report_file(param_obj=param_obj, report_name=report_name, output_folder=output_folder,
                              report_vector_stats=report_vector_stats, debug=debug)

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
                report_name=args.reportname, debug=args.debug)
