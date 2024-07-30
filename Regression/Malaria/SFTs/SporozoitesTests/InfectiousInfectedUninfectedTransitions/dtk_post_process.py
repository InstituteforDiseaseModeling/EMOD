#!/usr/bin/python

import dtk_test.dtk_sft as sft
import pandas as pd
import json
import re
import csv
import matplotlib.pyplot as plt
import numpy as np
from collections import Counter

"""
Vectors, if they don't die, should become infected when they have an oocyst, then infectious when they get sporozoites,
but as those sporozoites die off - the vector could become un-infected (if it has no oocysts) or infected (if it has
oocysts) instead of infectious. 

For this test, I turned off vector mortality and I am looking at changing numbers in the ReportVectorStatsMalariaGenetics.csv

"""

config_name = "Config_Name"


def load_emod_parameters(config_filename="config.json", debug=False):
    """
    Reads the config filename and takes out relevant parameters.
    Args:
        config_filename: config filename
        debug: when true, the parameters get written out as a json.

    Returns:

    """

    """reads config file and populates params_obj
    :param config_filename: name of config file (config.json)
    :param debug: write out parsed data on true
    :returns param_obj:     dictionary with KEY_CONFIG_NAME, etc., keys (e.g.)
    """
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {config_name: cdj[config_name]}

    if debug:
        with open("DEBUG_emod_params.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj


def create_report_file(param_obj, report_name, vectorstats_filename, output_folder, debug=False):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Test name: {str(param_obj[config_name])} \n")
        vectorstats_df = pd.read_csv(output_folder+ "\\" + vectorstats_filename)
        num_vectors = 200
        # verifying no vectors die
        if not (vectorstats_df['VectorPopulation'] == num_vectors).all():
            outfile.write("We seem to have a variable vector population, please turn off vector mortality.\n")
        else:
            vector_went_from_infectious_to_infected = False
            vector_went_from_infectious_to_clean = False
            infected = "STATE_INFECTED"
            infectious = "STATE_INFECTIOUS"
            clean = "STATE_ADULT"
            infected_previous = vectorstats_df[infected].iloc[0]
            infectious_previous = vectorstats_df[infectious].iloc[0]
            clean_previous = vectorstats_df[clean].iloc[0]
            for time in range(1, len(vectorstats_df['Time'])):
                infected_now = vectorstats_df[infected].iloc[time]
                infectious_now = vectorstats_df[infectious].iloc[time]
                clean_now = vectorstats_df[clean].iloc[time]
                # looking for vectors that move from infectious to infected at any time
                infected_difference = infected_previous - infected_now
                infectious_difference = infectious_previous - infectious_now
                clean_difference = clean_previous - clean_now
                if infectious_difference < -clean_difference:
                    vector_went_from_infectious_to_infected =True
                    outfile.write(f"GOOD: Found vectors moving from infectious to infected on day {time}.\n")

                if infectious_difference == -clean_difference and infected_difference == 0 and infectious_difference != 0:
                    vector_went_from_infectious_to_clean = True
                    outfile.write(f"GOOD: Found vectors moving from infectious to adult on day {time}.\n")

                #updating for next timestep
                infectious_previous = infectious_now
                infected_previous = infected_now
                clean_previous = clean_now

            if not vector_went_from_infectious_to_clean or not vector_went_from_infectious_to_infected:
                success = False

        outfile.write(sft.format_success_msg(success))


def application(output_folder="output",
                config_filename="config.json",
                vectorstats_filename="ReportVectorStatsMalariaGenetics.csv",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("vectorstats_filename: " + vectorstats_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename)
    create_report_file(param_obj, report_name, vectorstats_filename, output_folder, debug)


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
