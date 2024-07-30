#!/usr/bin/python

import dtk_test.dtk_sft as sft
import pandas as pd
from math import exp, isclose
import json

"""
This test verifies that oocyst_progression is modified correctly. 
All the parasites have the same barcode, there's a constant temperature, but depending on vector genome, 
the oocyst development is different. We make sure the oocyst progression is as expected
using the ReportVectorGeneticsMalariaGenetics report's AvgOocystDurationDays column.

"""

config_name = "Config_Name"
avg_oocyst_duration = "AvgOocystDurationDays"


def load_emod_parameters(config_filename="config.json", debug=False):
    """
    Reads the config filename and takes out relevant parameters.
    Args:
        config_filename: config filename
        debug: when true, the parameters get written out as a json.

    Returns:

    """

    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {config_name: cdj[config_name],
                 "infected_arrhenius1": cdj["Vector_Species_Params"][0]["Infected_Arrhenius_1"],
                 "infected_arrhenius2": cdj["Vector_Species_Params"][0]["Infected_Arrhenius_2"],
                 "oocyst_progress_modifier": cdj["Vector_Species_Params"][0]["Gene_To_Trait_Modifiers"][0]["Trait_Modifiers"][0]["Modifier"],
                 # convert from Celsius to Kelvin
                 "temperature_k": cdj["Base_Air_Temperature"] + 273.15}

    if debug:
        with open("DEBUG_emod_params.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj


def create_report_file(param_obj, report_name, output_report, debug=False):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Test name: {str(param_obj[config_name])} \n")
        vector_info_df = pd.read_csv(f'output/{output_report}')
        if not len(vector_info_df):
            success = False
            outfile.write(sft.sft_no_test_data)
            outfile.write(f"No data in {output_report}\n")
        else:
            infected_arrhenius1 = param_obj["infected_arrhenius1"]
            infected_arrhenius2 = param_obj["infected_arrhenius2"]
            temperature_k = param_obj["temperature_k"]
            oocyst_progress_modifier = param_obj["oocyst_progress_modifier"]
            days_to_infectious = round(1/(infected_arrhenius1 * exp(- infected_arrhenius2 / temperature_k)))
            modified_days_to_infectious = round(days_to_infectious * 1/oocyst_progress_modifier)
            modified_df = vector_info_df.loc[vector_info_df['Genome'] == "X-b:X-b"]

            def no_0(number):
                if number != 0:
                    return True
                return False

            modified_oocyst_duration = filter(no_0, modified_df[avg_oocyst_duration].to_list())
            if all(isclose(i, modified_days_to_infectious, abs_tol=0.6) for i in modified_oocyst_duration):
                outfile.write(f"GOOD: All vectors with X-b:X-b genes had the correct {avg_oocyst_duration} "
                              f"of {modified_days_to_infectious}\n")
            else:
                success = False
                outfile.write(f"BAD: We expected {avg_oocyst_duration} to be {modified_days_to_infectious} "
                              f"and they are {modified_oocyst_duration} \n")
            normal_df = vector_info_df.loc[vector_info_df['Genome'] != "X-b:X-b"]
            normal_oocyst_duration = filter(no_0, normal_df[avg_oocyst_duration].to_list())
            if all(isclose(i, days_to_infectious, abs_tol=0.6) for i in normal_oocyst_duration):
                outfile.write(f"GOOD: All vectors with X-b:X-b genes had the correct {avg_oocyst_duration} "
                              f"of {days_to_infectious}\n")
            else:
                success = False
                outfile.write(f"BAD: We expected {avg_oocyst_duration} to be {days_to_infectious} "
                              f"and they are {normal_oocyst_duration} \n")

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
    stdout_filename = "ReportVectorGeneticsMalariaGenetics_SillySkeeter_Female_GENOME.csv"
    create_report_file(param_obj, report_name, stdout_filename, debug)


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
