#!/usr/bin/python

import dtk_test.dtk_sft as dtk_sft
import json
import pandas as pd
import numpy as np

"""
This test checks cohort(compartmentalized) vectors and should be in sync with the invididuals/track_all vectors test
The test: We release two strains - males with Strain_A and females with Strain_B, we know the expected
proportion of eggs infected with each strain from the formulas below. We use the ReportMicrosporidia.csv
to look at those numbers and compare with expected. 

Sweep: 
Sweep the "Male_To_Egg_Transmission_Probability" for Strain_A parameter.
Sweep the "Female_To_Egg_Transmission_Probability" for Strain_B parameter.

prob_infected = 1 - (1 - female_to_egg_tran_prob) * (1 - male_to_egg-tran_prob)
prob_infected_by_female = prob_infected * female_to_egg_tran_prob / (female_to_egg_tran_prob + male_to_egg)
num_infected_by_male = num_infected - num_infected_by_female
..but also num_infected_by_male = prob_infected * male_to_egg_tran_prob / (female_to_egg_tran_prob + male_to_egg)
"""

MALE_TO_EGG = "Male_To_Egg_Transmission_Probability"
FEMALE_TO_EGG = "Female_To_Egg_Transmission_Probability"
KEY_CONFIG_NAME = "Config_Name"


def create_report_file(config_filename, report_name, debug=False):
    success = True
    with open(report_name, "w") as outfile:
        with open(config_filename) as infile:
            cdj = json.load(infile)["parameters"]
        if cdj["Vector_Species_Params"][0]["Microsporidia"][0]["Strain_Name"] == "Strain_A":
            strain_a_male_to_egg_prob = cdj["Vector_Species_Params"][0]["Microsporidia"][0][MALE_TO_EGG]
            strain_b_female_to_egg_prob = cdj["Vector_Species_Params"][0]["Microsporidia"][1][FEMALE_TO_EGG]
        else:
            strain_a_male_to_egg_prob = cdj["Vector_Species_Params"][0]["Microsporidia"][1][MALE_TO_EGG]
            strain_b_female_to_egg_prob = cdj["Vector_Species_Params"][0]["Microsporidia"][0][FEMALE_TO_EGG]
        prob_infected = 1 - (1 - strain_b_female_to_egg_prob) * (1 - strain_a_male_to_egg_prob)
        prob_infected_by_female_strain_b = prob_infected * strain_b_female_to_egg_prob / (
                strain_b_female_to_egg_prob + strain_a_male_to_egg_prob)
        prob_infected_by_male_strain_a = prob_infected * strain_a_male_to_egg_prob / (
                strain_b_female_to_egg_prob + strain_a_male_to_egg_prob)
        config_name = cdj[KEY_CONFIG_NAME]
        simulation_duration = cdj["Simulation_Duration"]
        outfile.write("Config_name = {}\n".format(config_name))
        vector_stats_df = pd.read_csv("output//ReportMicrosporidia.csv")
        infected_successes = []
        proportion_successes = []
        for time in range(simulation_duration):
            print(
                f"{vector_stats_df.loc[vector_stats_df['Time'] == time][vector_stats_df['MicrosporidiaStrain'] == 'NoMicrosporidia']['STATE_EGG'].item()}")
            no_microsporidia_eggs_today = int(vector_stats_df.loc[vector_stats_df['Time'] == time][
                                                  vector_stats_df['MicrosporidiaStrain'] == 'NoMicrosporidia'][
                                                  'STATE_EGG'].item())
            strain_a_eggs_today = int(vector_stats_df.loc[vector_stats_df['Time'] == time][
                                          vector_stats_df['MicrosporidiaStrain'] == 'Strain_A']['STATE_EGG'].item())
            strain_b_eggs_today = int(vector_stats_df.loc[vector_stats_df['Time'] == time][
                                          vector_stats_df['MicrosporidiaStrain'] == 'Strain_B']['STATE_EGG'].item())
            total_eggs = no_microsporidia_eggs_today + strain_a_eggs_today + strain_b_eggs_today
            infected_total_eggs = strain_b_eggs_today + strain_a_eggs_today
            if total_eggs == 0:
                outfile.write(f"Day {time}: No eggs found. It's ok. Moving on.\n")
            else:
                outfile.write(f"Day: {time} ")
                if prob_infected == 1:
                    if infected_total_eggs == total_eggs:
                        portion_infected_check = True
                    else:
                        portion_infected_check = False
                else:
                    portion_infected_check = dtk_sft.test_binomial_99ci(infected_total_eggs, total_eggs,
                                                           prob_infected, outfile,
                                                           f"There should be {prob_infected} portion "
                                                           f"or {int(prob_infected * total_eggs)} "
                                                           f"of microsporidia eggs ")
                infected_successes.append(portion_infected_check)
                if prob_infected_by_female_strain_b == 0:
                    if strain_b_eggs_today == 0:
                        strain_b_check = True
                    else:
                        strain_b_check = False
                else:
                    strain_b_check = dtk_sft.test_binomial_99ci(strain_b_eggs_today, total_eggs,
                                                                prob_infected_by_female_strain_b, outfile,
                                                                f"There should be {prob_infected_by_female_strain_b} portion "
                                                                f"or {int(prob_infected_by_female_strain_b * total_eggs)} "
                                                                f"of Strain_B eggs ")
                if prob_infected_by_male_strain_a == 0:
                    if strain_a_eggs_today == 0:
                        strain_a_check = True
                    else:
                        strain_a_check = False
                else:
                    strain_a_check = dtk_sft.test_binomial_99ci(strain_a_eggs_today, total_eggs,
                                                                prob_infected_by_male_strain_a, outfile,
                                                                f"There should be {prob_infected_by_male_strain_a} portion "
                                                                f"or {int(prob_infected_by_male_strain_a * total_eggs)} "
                                                                f"of Strain_A eggs ")
                if (strain_b_check and strain_a_check) or portion_infected_check:
                    little_success = True
                else:
                    little_success = False
                proportion_successes.append(little_success)
        if all(proportion_successes):
            outfile.write(
                f"GOOD: for all days, the proportions of Strain_B  and Strain_A eggs to all eggs was "
                f"as expected.\n")
        else:
            outfile.write(
                f"BAD: Not for all days, the proportions of Strain_B  and Strain_A eggs to all eggs was "
                f"as expected. We had {len(proportion_successes) - np.count_nonzero(proportion_successes)} "
                f"failures.\n")
            success = False

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
