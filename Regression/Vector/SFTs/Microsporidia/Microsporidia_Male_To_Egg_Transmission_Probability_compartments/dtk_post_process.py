import dtk_test.dtk_sft as dtk_sft
import json
import pandas as pd
import numpy as np

"""
This test checks cohort(compartmentalized) vectors and should be in sync with the invididuals/track_all vectors test
The test: Verifying that infected males are able to infect eggs of the females they mate with a
at the probability of Male_To_Egg_Transmission_Probability when the females are not infected.
The sweep: "Male_To_Egg_Transmission_Probability"
"""


MICROSPORIDIA_MALE_TO_EGG_TRANSMISSION_PROBABILITY = "Male_To_Egg_Transmission_Probability"
KEY_CONFIG_NAME = "Config_Name"


def create_report_file(config_filename, report_name, debug=False):
    success = True
    with open(report_name, "w") as outfile:
        with open(config_filename) as infile:
            cdj = json.load(infile)["parameters"]
        male_to_egg_prob = cdj["Vector_Species_Params"][0]["Microsporidia"][0][
            MICROSPORIDIA_MALE_TO_EGG_TRANSMISSION_PROBABILITY]
        config_name = cdj[KEY_CONFIG_NAME]
        simulation_duration = cdj["Simulation_Duration"]
        outfile.write("Config_name = {}\n".format(config_name))
        vector_stats_df = pd.read_csv("output//ReportMicrosporidia.csv")
        successes = []
        for time in range(simulation_duration):
            print(f"{vector_stats_df.loc[vector_stats_df['Time'] == time][vector_stats_df['MicrosporidiaStrain'] == 'NoMicrosporidia']['STATE_EGG'].item()}")
            no_microsporidia_eggs_today = int(vector_stats_df.loc[vector_stats_df['Time'] == time][vector_stats_df['MicrosporidiaStrain'] == 'NoMicrosporidia']['STATE_EGG'].item())
            microsporidia_eggs_today = int(vector_stats_df.loc[vector_stats_df['Time'] == time][vector_stats_df['MicrosporidiaStrain'] == 'Strain_A']['STATE_EGG'].item())
            total_eggs = no_microsporidia_eggs_today + microsporidia_eggs_today
            if total_eggs == 0:
                outfile.write(f"Day {time}: No eggs found. It's ok. Moving on.\n")
            else:
                little_success = dtk_sft.test_binomial_95ci(microsporidia_eggs_today, total_eggs,
                                                            male_to_egg_prob, outfile,
                                                            f"There should be {male_to_egg_prob} portion "
                                                            f"of microsporidia eggs ")
                successes.append(little_success)
        ok = 0.95  # sometimes we're just outside the 95% confidence interval and that's ok
        if np.count_nonzero(successes) > len(successes) * ok:
            outfile.write(
                f"GOOD: For enough days, the proportions of microsprodia eggs to all eggs was {male_to_egg_prob}.\n")
        else:
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