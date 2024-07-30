#!/usr/bin/python

import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import pandas as pd
import os

"""
In this test, we are verifying that all the vectors, male and female are reduced correctly by the SugarTrap
male vectors sugar feed every day and female vectors sugar feed according to what's on their Vector_Sugar_Feeding_Frequency
This is a "Vector_Sugar_Feeding_Frequency": "VECTOR_SUGAR_FEEDING_ON_EMERGENCY_ONLY" test.
For this test, we let the vectors lay eggs for one day, then kill them off and watch the effect on 
female and male vectors on emergence and following days. We verify the number of expected vectors by 
halfing the number of immature vectors before the emergence (it's rough, but it's good enough).
With the sugar trap being 0.25 effective, we expect there to be 0.75 male and female vectors left. 
Two days after the emergence, we expect the female vectors numbers to remain the same (no more new vectors 
emerging), but male vectors sugar feed every day, so their population will be 0.75*0.75 =0.5625 ~ 0.563
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
            immature_loc = vectors_df["STATE_IMMATURE"].ne(0).idxmax()
            expected_proportion = 0.75
            # this number counts both male and female vectors, but they are about 50/50
            orig = vectors_df.iloc[immature_loc]["STATE_IMMATURE"] / 2
            after_intervention = vectors_df.iloc[immature_loc+1][sex]
            if abs(after_intervention/orig - expected_proportion) > allowable_error:
                success = False
                outfile.write(f"BAD: We were expecting {expected_proportion} of {sex} vectors to still be alive, "
                              f"but {after_intervention/orig} are.\n")
            else:
                outfile.write(f"GOOD: We're seeing about {expected_proportion} of {sex} vectors left over after"
                              f" the intervention.\n")
            # verifying that males continue to get decremented, but females don't
            # Two days after the emergence, we expect the female vectors numbers to remain the same
            # (no more new vectors emerging), but male vectors sugar feed every day, so their population will be
            # 0.75*0.75 =0.5625 ~ 0.563
            expected_proportion = 1 if sex == "STATE_ADULT" else 0.563
            orig = vectors_df.iloc[immature_loc+1][sex]
            after_intervention = vectors_df.iloc[immature_loc+3][sex]
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
                report_vector_stats = "ReportVectorStats.csv",
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
