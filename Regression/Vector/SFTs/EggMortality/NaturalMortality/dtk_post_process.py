#!/usr/bin/python
from __future__ import division
import matplotlib.pyplot as plt
import numpy as np
import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds

"""
Spec:
    Natural mortality. Eggs experience daily egg mortality, which is independent of climatic factors
    Relevant parameters:
    Egg_Survival_Rate: 0.99
    
Test: We kill all the mosquitoes (so no new eggs are laid) and create artificial drought with campaign.json, 
and with "Drought_Egg_Hatch_Delay":0, make sure that no eggs hatch and eggs are only removed through natural mortality
so we have more data.  

This test verifies that the average egg survival rate is about 0.99, converted to probability.
    
"""


def create_report_file(param_obj, report_name, filtered_lines, debug=False):
    actual_survival_prob = []
    new_population = []
    # converting survival rate to probability of surviving rate
    theoretical_prob = param_obj[veds.ConfigKeys.VectorSpeciesParams.EGG_SURVIVAL_RATE]
    tolerance = 5e-2 * theoretical_prob
    success = True
    with open(report_name, 'w') as outfile:
        outfile.write(f"# Test name: {param_obj[veds.ConfigKeys.CONFIG_NAME]}, run number "
                      f"{param_obj[veds.ConfigKeys.RUN_NUMBER]}\n# Tests if the egg survival rate in the simulation "
                      f"matches the specified survival rate.\n")
        for line in filtered_lines:
            old_pop, new_pop = parse_line(line)
            new_population.append(new_pop)
            actual_prob = new_pop / old_pop
            actual_survival_prob.append(actual_prob)
            if old_pop > 50 and abs(theoretical_prob - actual_prob) > tolerance:
                success = False
                lower_bound = theoretical_prob + tolerance
                upper_bound = theoretical_prob - tolerance
                outfile.write(f"BAD: Theoretical rate, {theoretical_prob}, did not match actual rate, "
                              f"{actual_prob}  Should be within 5% boundaries: ({lower_bound}, {upper_bound}).\n")

        if success:
            outfile.write(
                        f"GOOD: Actual rates were within 5% of theoretical rate. \n")
        outfile.write(sft.format_success_msg(success))
    theoretical_prob_dist = [theoretical_prob] * len(actual_survival_prob)
    sft.plot_data(theoretical_prob_dist, actual_survival_prob, label1="theoretical survival probability",
                      label2='actual survival probability', title='Egg Survival Probability',
                      ylabel='Survival Probability', category="egg_mortality_data")


def parse_output_file(stdout, debug=False):
    filtered_lines = []
    with open(stdout) as logfile:
        for line in logfile:
            if "egg mortality" in line:
                filtered_lines.append(line)
        if True:
            with open("DEBUG_filtered_lines.txt", "w") as outfile:
                outfile.writelines(filtered_lines)
    return filtered_lines


def parse_line(line):
    """
    parse comma separated name value pair and return relevant variables
    :param line: line of text to parse
    e.g. line = 00:00:00 [0] [V] [VectorPopulation] Updating egg population due to egg mortality: old_pop = 1,
     new_pop = 0.
    """
    equals = '='
    scrunched_text = line.replace(equals, 'is')
    scrunched_text.replace('.', "")
    scrunched_text = scrunched_text.replace('is', ',')
    scrunched_text = scrunched_text[36::]
    text_list = scrunched_text.split(",")
    text_list = [text.strip() for text in text_list]
    values = [float(value) for value in text_list if '.' in value or value.isnumeric()]
    old_pop = values[0]
    new_pop = values[1]
    return old_pop, new_pop


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
    filtered_lines = parse_output_file(stdout, debug)
    create_report_file(param_obj, report_name, filtered_lines, debug)


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
