#!/usr/bin/python

import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import pandas as pd
import numpy as np
import json

"""
SURFACE_AREA_DEPENDENT
The biting risk rises linearly from 7% to 23% over the first two years of life and then rises with
a shallower linear slope to the adult value at age 20.
"""


def parse_output_file(stdout, debug=False):
    """
    Parses following lines, example:

    Mean gonotrophic cycle duration = 10.54870 days at 15.77 degrees C.

    Args:
        stdout: the logging file to parse
        debug: flag, if true writes out the lines that were used to collect the data

    Returns: data frame

    """
    time = 0
    humans = {}
    human_bites = {}
    with open(stdout) as logfile:
        for line in logfile:
            if "Time: " in line:
                time += 1
            if "Created human with id" in line:
                human_id = int(sft.get_val("id ", line))
                age = float(sft.get_val("age=", line))
                humans[human_id] = age
            if "bit person" in line:
                human_bitten = int(sft.get_val("person ", line))
                if human_bitten in human_bites:
                    human_bites[human_bitten] += 1
                else:
                    human_bites[human_bitten] = 1

    return humans, human_bites


def biting_percentage(age_in_days):
    """
        Returns percent (ex 24.389074) of the surface area using for biting probability based on age
    Args:
        age_in_days: age of human in days

    Returns:
        percentage of surface area this human has compared to adult human that has 100%
    """
    if age_in_days < 731:
        return 7 + 0.022 * age_in_days
    elif age_in_days < 7301:
        return 23 + 0.0117*(age_in_days - 730)
    else:
        return 100


def create_report_file(param_obj, report_name, humans, human_bites, debug=False):
    success = True

    with open(report_name, "w") as outfile:
        outfile.write(f"Test name: {str(param_obj[veds.ConfigKeys.CONFIG_NAME])} \n"
                      f"Run number: {str(param_obj[veds.ConfigKeys.RUN_NUMBER])} \n")
        if not len(human_bites):
            success = False
        percent_area = {}
        biting_area_as_part_of_whole = {}
        total_percent = 0
        for human_id in humans:
            biting_perc = biting_percentage(humans[human_id])
            percent_area[human_id] = biting_perc
            total_percent += biting_perc
        for human_id in percent_area:
            biting_area_as_part_of_whole[human_id] = percent_area[human_id]/total_percent

        percent_biting_as_part_of_whole = {}
        total = 0
        for human_id in human_bites:
            total += human_bites[human_id]
        for human_id in human_bites:
            percent_biting_as_part_of_whole[human_id] = human_bites[human_id]/total

        allowable_error = 0.005
        for human_id in biting_area_as_part_of_whole:
            if abs(biting_area_as_part_of_whole[human_id] - percent_biting_as_part_of_whole[human_id]) > allowable_error:
                success = False
                outfile.write(f"BAD: for human {human_id}, we expected {biting_area_as_part_of_whole[human_id]} of bites"
                              f"to be received by them, but they received {percent_biting_as_part_of_whole[human_id]} and"
                              f"that's outside our allowable error of {allowable_error}.\n ")

        if debug:
            all_data = {"biting_area_calc": percent_area,
                        "biting_area_as_part_of_whole": biting_area_as_part_of_whole,
                        "human_bites": human_bites,
                        "humans": humans,
                        "percent_biting_as_part_of_whole": percent_biting_as_part_of_whole}
            with open('biting_data.json', 'w') as fp:
                json.dump(all_data, fp)

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
    humans, human_bites = parse_output_file(stdout, debug)
    create_report_file(param_obj, report_name, humans, human_bites, debug)


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
