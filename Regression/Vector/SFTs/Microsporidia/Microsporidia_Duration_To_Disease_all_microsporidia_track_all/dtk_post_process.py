#!/usr/bin/python

import dtk_test.dtk_sft as dtk_sft
import json
import pandas as pd
"""
The test:
This test verifies that 1. Released Microsprodia+ female vectors cannot get infected. 2. That microsporidia+ eggs
are fully immune and the new adult microsporidia+ vectors that come from eggs are fully immune by the time they 
feed if the duration_to_disease is shorter than egg-to-adult process. 

Release microsporidia+ female vectors into an infected human population. Have them lay eggs. Make sure all vectors
have microsporidia and no one gets infected. 

Important settings:
                "Microsporidia":[
                {
                    "Female_To_Egg_Transmission_Probability": 1,
                    "Male_To_Egg_Transmission_Probability": 0,
                    "Duration_To_Disease_Acquisition_Modification": {
                        "Times": [    0,9],
                        "Values": [ 1,0 ]
                    }
                }

"""

KEY_CONFIG_NAME = "Config_Name"


def create_report_file(config_filename, report_name, debug=False):
    with open(report_name, "w") as outfile:
        with open(config_filename) as infile:
            cdj = json.load(infile)["parameters"]
        config_name = cdj[KEY_CONFIG_NAME]
        success = True
        outfile.write("Config_name = {}\n".format(config_name))
        vector_stats_df = pd.read_csv("output//ReportVectorStats.csv")
        columns_to_check = ["NoMicrosporidia-STATE_INFECTIOUS", "NoMicrosporidia-STATE_INFECTED",
                            "NoMicrosporidia-STATE_ADULT", "NoMicrosporidia-STATE_MALE", "NoMicrosporidia-STATE_EGG",
                            "NoMicrosporidia-STATE_LARVA", "NoMicrosporidia-STATE_IMMATURE",
                            "HasMicrosporidia-STATE_INFECTED", "HasMicrosporidia-STATE_INFECTIOUS"]
        for check_column in columns_to_check:
            if not (vector_stats_df[check_column] == 0).all():
                success = False
                outfile.write(f"BAD: column {check_column} has non-zero values where only zeroes "
                              f"are expected.\n")
        outfile.write(dtk_sft.format_success_msg(success))


def application(output_folder="output", stdout_filename="test.txt", config_filename="config.json",
                report_name=dtk_sft.sft_output_filename, debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done()
    create_report_file(config_filename, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout, config_filename=args.config,
                report_name=args.reportname, debug=args.debug)
