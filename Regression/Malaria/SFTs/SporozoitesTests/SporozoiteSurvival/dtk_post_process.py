#!/usr/bin/python

import dtk_test.dtk_sft as sft
import pandas as pd
from math import exp
import json

"""
Spec: 
We may want to use a threshold in the future to account for the non-linear relationship between sporozoite 
load and infection probability as in (Aleshnick, 2020), the practical implication being that a small subset 
of highly infected mosquitoes may contribute disproportionately to malaria transmission in the field. This is 
related to the action employed by the current parameter Base_Sporozoite_Survival_Fraction, without the ability 
to do thresholding as suggested by literature. 

"""

config_name = "Config_Name"
sporozoite_life_expectancy = "Sporozoite_Life_Expectancy"
parasite_genetics = "Parasite_Genetics"


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
    param_obj = {config_name: cdj[config_name],
                 sporozoite_life_expectancy: cdj[parasite_genetics][sporozoite_life_expectancy]}

    if debug:
        with open("DEBUG_emod_params.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj


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
    sporozoites_dict = {'initial': [], 'remaining': []}
    with open(stdout) as logfile:
        for line in logfile:
            if "Time: " in line:
                time += 1
            if "Sporozoites cohort id" in line:
                # line: Sporozoites cohort id 11254 initial 7938 remaining 7655 genome GG
                initial = int(sft.get_val("initial ", line))
                remaining = int(sft.get_val("remaining ", line))
                sporozoites_dict["initial"].append(initial)
                sporozoites_dict["remaining"].append(remaining)

    if debug:
        pd.DataFrame(sporozoites_dict).to_csv("sporozoites_dict_DEBUG.csv")

    return sporozoites_dict


def create_report_file(param_obj, report_name, sporozoites_dict, debug=False):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Test name: {str(param_obj[config_name])} \n")
        if not len(sporozoites_dict):
            success = False
            outfile.write(sft.sft_no_test_data)
            outfile.write("No data in progress_df or !\n")
        else:
            small_ones = {'initial': 0, 'remaining': 0}
            expected_mortality_rate = (1.0 - exp(- 1.0/param_obj[sporozoite_life_expectancy]))
            yes = 0
            no = 0
            for log_line in range(0, len(sporozoites_dict)-1):
                num_initial = sporozoites_dict["initial"][log_line]
                num_remaining = sporozoites_dict["remaining"][log_line]
                if num_initial > 100:
                    dead = num_initial - num_remaining
                    if sft.test_binomial_95ci(dead, num_initial,
                                              expected_mortality_rate, outfile, "Sprorozoite death rate"):
                        yes += 1
                    else:
                        no += 1
                else:
                    small_ones["initial"] = small_ones["initial"] + num_initial
                    small_ones["remaining"] = small_ones["remaining"] + num_remaining
            if small_ones["initial"] > 100:
                dead = small_ones["initial"] - small_ones["remaining"]
                if sft.test_binomial_95ci(dead, small_ones["initial"],
                                          expected_mortality_rate, outfile, "Sprorozoite death rate"):
                    yes += 1
                else:
                    no += 1

            if yes / (yes + no) < 0.95:
                outfile.write("BAD: Too many instances of sporozoite death being not within 95% confidence interval for"
                              " binomial draw.\n")
                success = False
            else:
                outfile.write("GOOD: More than 95% of sporozoite death was within 95% confidence interval "
                              "for binomial draw")

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
    param_obj = load_emod_parameters(config_filename)
    sporozoites_dict = parse_output_file(stdout, debug)
    create_report_file(param_obj, report_name, sporozoites_dict, debug)


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
