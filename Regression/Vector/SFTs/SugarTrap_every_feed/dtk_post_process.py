#!/usr/bin/python

import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import pandas as pd

import os

"""
In this test, we are verifying that all the vectors, male and female are reduced correctly by the SugarTrap
male vectors sugar feed every day and female vectors sugar feed according to what's on their Vector_Sugar_Feeding_Frequency
This is a "Vector_Sugar_Feeding_Frequency": "VECTOR_SUGAR_FEEDING_EVERY_FEED" test.
So we expect the male vectors to be reduced three times while female vectors only once, 
as we are running the intervention for three days and female vectors bite every 3 days.
With  "Initial_Effect": 0.35, there should be 0.65 of total female population left, while only 0.275 male 
vector population left

"""


def create_biting_report_file(param_obj,report_name, output_folder, report_vector_stats, debug=False):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Test name: {str(param_obj[veds.ConfigKeys.CONFIG_NAME])} \n"
                      f"Run number: {str(param_obj[veds.ConfigKeys.RUN_NUMBER])} \n")
        rsv_path = os.path.join(output_folder, report_vector_stats)
        vectors_df = pd.read_csv(rsv_path)
        sexes = ["STATE_ADULT", "STATE_MALE"]
        allowable_error = 0.01
        # make sure that step 1 has 0.3 vectors of step 0
        for sex in sexes:
            expected_proportion = 0.65 if sex == "STATE_ADULT" else 0.275
            orig = vectors_df.iloc[0][sex]
            after_intervention = vectors_df.iloc[4][sex]
            if abs(after_intervention/orig - expected_proportion) > allowable_error:
                success = False
                outfile.write(f"BAD: We were expecting {expected_proportion} of {sex} vectors to still be alive, "
                              f"but {after_intervention/orig} are.\n")
            else:
                outfile.write(f"GOOD: We're seeing about {expected_proportion} of {sex} vectors left over after"
                              f" the intervention.\n")

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
