#!/usr/bin/python

import dtk_test.dtk_sft as dtk_sft
import json
import pandas as pd
"""
The test:
Start with 0 vectors. Release microsporidia+ males and females. 
Set "Female_To_Egg_Transmission_Probability" to various values and make sure approximately that 
percentage of eggs has microsporidia

Sweep: Sweep the "Female_To_Egg_Transmission_Probability" parameter.
"""

female_to_egg = "Female_To_Egg_Transmission_Probability"
CONFIG_NAME = "Config_Name"


def create_report_file(config_filename, report_name, debug=False):
    with open(report_name, "w") as outfile:
        with open(config_filename) as infile:
            cdj = json.load(infile)["parameters"]
        female_to_egg_prob = cdj["Vector_Species_Params"][0]["Microsporidia"][0][female_to_egg]
        config_name = cdj[CONFIG_NAME]
        outfile.write("Config_name = {}\n".format(config_name))
        vector_stats_df = pd.read_csv("output//ReportVectorStats.csv")
        microsporidia_eggs = 0
        not_microsporidia_eggs = 0
        for time in (range(6, 18, 2)):
            microsporidia_eggs += int(vector_stats_df.loc[(vector_stats_df['Time'] == time), "HasMicrosporidia-STATE_EGG"].item())
            not_microsporidia_eggs += int(vector_stats_df.loc[(vector_stats_df['Time'] == time), "NoMicrosporidia-STATE_EGG"].item())

        total_eggs = microsporidia_eggs + not_microsporidia_eggs
        success = dtk_sft.test_binomial_95ci(microsporidia_eggs, total_eggs,
                                            female_to_egg_prob, outfile,
                                            "eggs should've been laid with microsporidia")
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
