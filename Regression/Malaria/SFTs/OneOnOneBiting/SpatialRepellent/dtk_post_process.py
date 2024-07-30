#!/usr/bin/python

import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import pandas as pd
import numpy as np
import json

"""
We're distributing SpatialRepellent to one node at 0.5 Initial Effect, this means that on the first day of spatial
repellent one node will have twice as many bites as the other, and on the second day, because half the vectors 
attempting to feed on day 1 didn't and will try again. There will be (total vectors)/(feeding frequency) * 0.5 biting
 the first day and the other half biting the second day. 
 Which means we expect there to be ((total vectors)/(feeding frequency) * 1.5) * 0.5 biting the second day
 which means while one node on the second day will be getting (total vectors)/(feeding frequency) total bites
 the node with the spatial repellent will be getting (total vectors)/(feeding frequency) * 0.75 total bites the second 
 day. 
 So I am expecting a 1:0.75 proportion of bites or 57% of bites in one node and 43% of bites on the other one. 
Making each node have one person to make things easier.

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
    humans, human_bites, bites_by_day = veds.track_bites(stdout, start_time=2, end_time=2, debug=True)
    veds.create_biting_report_file(param_obj=param_obj, humans=humans, bites_by_day=bites_by_day,
                                   report_name=report_name, human_bites=human_bites,
                                   with_intervention_proportion=0.43,
                                   without_intervention_proportion=0.57,
                                   expected_people_with_intervention=1,
                                   expected_people_without_intervention=1,
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
