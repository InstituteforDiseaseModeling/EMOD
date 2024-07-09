#!/usr/bin/python

import dtk_test.dtk_sft as dtk_sft
import json
import pandas as pd
"""
The test:
Release microsporidia+ vectors, kill them all after they lay eggs, make sure the new vectors are all microsprodia+
Eggs become adult vectors on Day 15, which means that if duration_to_disease =  {"Times": [0,30], "Values": [1,0]}, 
where values are the probability of a vector getting infected when biting an infectious person - that probability 
should be about 50% on the first feed, if all of human population have malaria. 

Sweep: as we vary Times's end parameter, we should see changes in the proportion

Important settings:
                "Microsporidia":[
                {
                    "Female_To_Egg_Transmission_Probability": 1,
                    "Duration_To_Disease_Acquisition_Modification": {
                        "Times": [    0,30],
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
        dur_to_dis = cdj["Vector_Species_Params"][0]["Microsporidia"][0]["Duration_To_Disease_Acquisition_Modification"]
        time_to_disease = dur_to_dis["Times"][1]
        check_time = 26
        microsporidia_duration = check_time - 4  # eggs laid on day 3, but are at 0 for that day, so -4
        outfile.write("Config_name = {}\n".format(config_name))

        def calculate_infection_probability(end_time: int = 30, first_feed: int = microsporidia_duration):
            # assumes Times are [0, end_time] and Values [1, 0]
            if end_time <= first_feed:
                return 0
            else:
                return 1 - (first_feed / end_time)
        vector_stats_df = pd.read_csv("output//ReportVectorStats.csv")
        infection_adult = int(vector_stats_df.loc[(vector_stats_df['Time'] == check_time), "STATE_INFECTED"].item())
        total_adult = int(vector_stats_df.loc[(vector_stats_df['Time'] == check_time), "IndoorBitesCount"].item())
        infection_probability = calculate_infection_probability(time_to_disease, microsporidia_duration)
        success = dtk_sft.test_binomial_95ci(infection_adult, total_adult,
                                             infection_probability, outfile,
                                             "female vectors should've been infected with malaria")
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
