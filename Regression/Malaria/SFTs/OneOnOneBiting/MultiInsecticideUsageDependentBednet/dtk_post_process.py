#!/usr/bin/python

import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import pandas as pd
import numpy as np
import json

"""
Verifying that when one person is partially blocked, the other 2 person2 gets bitten more proportionally.
Indoor_Biting: 0.7
Giving one person 100% Repelling Bednet
which means one person gets bitter 100% of the time still
and person with bednet gets bitten 30% of original
this means that 0.3/(2+0.3) = 0.13 of bites are for the person with bednet
and 1/1.3 =  0.435 of bites are for each of the persons without bednets

"""


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
    humans, human_bites, bites_by_day = veds.track_bites(stdout, start_time=2, debug=True)
    veds.create_biting_report_file(param_obj=param_obj, humans=humans, bites_by_day=bites_by_day,
                                   report_name=report_name, human_bites=human_bites,
                                   with_intervention_proportion=0.13,
                                   without_intervention_proportion=0.435,
                                   expected_people_with_intervention=1,
                                   expected_people_without_intervention=2,
                                   x_error=1,
                                   debug=debug)

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
