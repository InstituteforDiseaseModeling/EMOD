#!/usr/bin/python

import dtk_test.dtk_sft as sft
import pandas as pd
from math import exp
import json

"""
This test verifies that, within a vector (possibly within the same vector), 
with SPOROZOITE_MORTALITY trait being activated or not, sprorozoite mortality
is applied correctly. 

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
        dictionary with parameters of interest
    """

    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {config_name: cdj[config_name],
                 sporozoite_life_expectancy: cdj[parasite_genetics][sporozoite_life_expectancy],
                 # looking up the mortality modifier (there's only one)
                 "mortality_modifier": cdj["Vector_Species_Params"][0]["Gene_To_Trait_Modifiers"][0]["Trait_Modifiers"][0]["Modifier"]}
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
    sporozoites_dict = {'initial': [], 'remaining': [], 'barcode': [], 'cohort': []}
    sporozoite_cohort_vector_genome = {}
    released_genomes = ["X-b:X-b",
                        "X-a:X-b",
                        "X-b:X-a",
                        "X-a:X-a"]
    with open(stdout) as logfile:
        for line in logfile:
            if "Time: " in line:
                time += 1
            elif "Sporozoites cohort id" in line:
                # line: Sporozoites cohort id 11254 initial 7938 remaining 7655 genome GG
                initial = int(sft.get_val("initial ", line))
                remaining = int(sft.get_val("remaining ", line))
                barcode = line.split()[-1]
                cohort_id = int(sft.get_val("cohort id ", line))
                sporozoites_dict["initial"].append(initial)
                sporozoites_dict["remaining"].append(remaining)
                sporozoites_dict["barcode"].append(barcode)
                sporozoites_dict["cohort"].append(cohort_id)
            elif "has sporozoites = " in line:
                # line:  Vector 3172 with X-a:X-b has sporozoites = 10012 GG has 2000, 10013 AA has 262,
                # 10013 GG has 248, 10013 AG has 237, 10013 GA has 253
                words = line.split("=")
                # doing it this was because linus and windows have different beginning of line
                vector_genome = words[0].split()[-3]
                if vector_genome not in released_genomes:
                    raise ValueError(
                        f"Unexpected genome - '{vector_genome}' detected in the {stdout} log lines.\n")
                sporozoites = words[1].split(",")
                for sporozoite in sporozoites:
                    data = sporozoite.split()
                    cohort_id = int(data[0])
                    if cohort_id not in sporozoite_cohort_vector_genome:
                        sporozoite_cohort_vector_genome[cohort_id] = vector_genome
        sporozoites_dict["genome"] = []
        for row in range(len(sporozoites_dict["initial"])):
            spor_cohort = int(sporozoites_dict["cohort"][row])
            genome = sporozoite_cohort_vector_genome[spor_cohort]
            sporozoites_dict["genome"].append(genome)

    if True:
        pd.DataFrame(sporozoites_dict).to_csv("sporozoites_dict_DEBUG.csv")
        with open("vector_genome_sporozoite_cohort_id.json", "w") as log:
            log.write(f"{sporozoite_cohort_vector_genome}")

    return sporozoites_dict


def create_report_file(param_obj, report_name, sporozoites_dict, debug=False):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Test name: {str(param_obj[config_name])} \n")
        mortality_modifier_AA = param_obj["mortality_modifier"]
        if not len(sporozoites_dict):
            success = False
            outfile.write(sft.sft_no_test_data)
            outfile.write("No data in sporozoites_dict\n")
        else:
            yes = 0
            no = 0
            calc_mort = (1.0 - exp(- 1.0 / param_obj[sporozoite_life_expectancy]))
            calc_mort_AA = calc_mort * mortality_modifier_AA
            for log_line in range(0, len(sporozoites_dict["initial"])):
                expected_mortality_prob = calc_mort
                num_initial = sporozoites_dict["initial"][log_line]
                num_remaining = sporozoites_dict["remaining"][log_line]
                barcode = sporozoites_dict["barcode"][log_line]
                genome = sporozoites_dict["genome"][log_line]
                if barcode == "AA" and genome == 'X-b:X-b':
                    # if AA then the mortality probability is different, assigning a different one
                    expected_mortality_prob = calc_mort_AA
                if num_initial > 100:  # ignoring smaller batches
                    dead = num_initial - num_remaining
                    if sft.test_binomial_95ci(dead, num_initial,
                                              expected_mortality_prob, outfile,
                                              f"Sprorozoite death rate for genome {barcode}"):
                        yes += 1
                    else:
                        no += 1

            if yes / (yes + no) < 0.95:
                outfile.write(f"BAD: Too many instances of sporozoite death being not within 95% confidence interval for"
                              f" binomial draw, only {yes} out of {yes+no} within confidence interval. \n")
                success = False
            else:
                outfile.write("GOOD: More than 95% of sporozoite death was within 95% confidence interval "
                              "for binomial draw.")

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
