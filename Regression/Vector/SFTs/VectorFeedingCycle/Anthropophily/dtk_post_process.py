#!/usr/bin/python

import pandas as pd
from scipy.stats import linregress
import numpy as np
import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds

"""
Spec: 
    Anthropophily is the proportion of vector bites that are directed towards humans (outdoor/indoor biting) and towards
    something else (animal feed, artificial diet). 

This test verifies that the proportion of human directed bites is (looking_to_feed)*(Anthropophily)
"""


def create_report_file(param_obj, report_name, totals, debug=False):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f'Running {param_obj["Config_Name"]} test. \n')
        if totals["looking_to_feed"] == 0:
            outfile.write(sft.sft_no_test_data)
            raise ValueError("There are no vectors looking to feed, please check the DEBUG_df.csv")
        anthropophily = param_obj["Anthropophily"]
        attempt_human_feed = totals["attempt_feed_indoor"] + totals["attempt_feed_outdoor"]
        if sft.test_binomial_99ci(attempt_human_feed, totals["looking_to_feed"], anthropophily, outfile,
                                  "anthropophily"):
            outfile.write(f"GOOD: There were {attempt_human_feed} attempted human feeds out of total of "
                          f"{totals['looking_to_feed']} attempted feeds. That's within 99.75% confidence "
                          f"interval for Anthropophily of {anthropophily}.\n")
        else:
            success = False
            outfile.write(f"BAD: There were {attempt_human_feed} attempted human feeds out of total of "
                          f"{totals['looking_to_feed']} attempted feeds. That's NOT within 99.75% confidence "
                          f"interval for Anthropophily of {anthropophily}.\n")
        outfile.write(sft.format_success_msg(success))


def application(output_folder="output",
                config_filename="config.json",
                stdout="test.txt",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("stdout_filename: " + stdout + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    param_obj = veds.load_emod_parameters(config_filename)
    totals = veds.parse_for_feeding_choice(stdout, debug)
    create_report_file(param_obj, report_name, totals, debug)


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
