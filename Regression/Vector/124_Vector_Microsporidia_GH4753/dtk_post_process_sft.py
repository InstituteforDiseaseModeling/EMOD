#!/usr/bin/python

import dtk_test.dtk_sft as dtk_sft
import json
import pandas as pd

"""
The test:
Release male vectors and female vectors one day later so the female vectors mate and get microsporidia on the same day 
as they are leased. Then, 14 days later, infect everyone in population. If microspridia was not protecting the vectors, 
then on their first feed - they would all get malaria based on our settings below. However, we expect a reduced 
amount of vectors to get infected - only about 25% (15 days is 75% into the infection, so it microsporidia protects 75%)
Vectors feed on odd days, so Day 15 is the day we are gathering our information on the feed malaria exposure.



Sweep: as we vary Times's end parameter, we should see changes in the proportion

Important settings:
                "Days_Between_Feeds": 15
                "Acquire_Modifier": 1
                "Microsporidia":[
                {
                    "Male_To_Female_Transmission_Probability": 1,
                    "Duration_To_Disease_Acquisition_Modification": {
                        "Times": [0, 20],
                        "Values": [1, 0]
                    }
                }

"""

KEY_CONFIG_NAME = "Config_Name"


def create_report_file(config_filename, report_name, debug=False):
    with open(report_name, "w") as outfile:
        with open(config_filename) as config:
            cdj = json.load(config)["parameters"]
        config_name = cdj[KEY_CONFIG_NAME]
        dur_to_dis = cdj["Vector_Species_Params"][0]["Microsporidia"][0]["Duration_To_Disease_Acquisition_Modification"]
        time_to_disease = dur_to_dis["Times"][1]
        time_from_disease = 15  # fem. vectors feed and get infected on day 1, feed again on day 16 (after outbreak)
        outfile.write("Config_name = {}\n".format(config_name))

        def calculate_infection_probability(end_time: int = 30, first_feed: int = time_from_disease):
            # assumes Times are [0, end_time] and Values [1, 0]
            if end_time <= first_feed:
                return 0
            else:
                return 1 - (first_feed / end_time)

        vector_stats_df = pd.read_csv("output//ReportVectorStats.csv")
        no_infection_adult = int(vector_stats_df.loc[(vector_stats_df['Time'] == 16), "STATE_ADULT"].item())
        infection_adult = int(vector_stats_df.loc[(vector_stats_df['Time'] == 16), "STATE_INFECTED"].item())
        total_adult = no_infection_adult + infection_adult
        infection_probability = calculate_infection_probability(time_to_disease, time_from_disease)
        print(total_adult)
        print(infection_adult)
        print(no_infection_adult)
        print(infection_probability)
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
