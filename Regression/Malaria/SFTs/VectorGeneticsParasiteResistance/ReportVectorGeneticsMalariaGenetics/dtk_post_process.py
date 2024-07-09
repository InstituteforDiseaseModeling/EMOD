#!/usr/bin/python

import dtk_test.dtk_sft as sft
import pandas as pd
from math import isclose
import json

"""
This test verifies that the accumulation for data in ReportVectorGeneticsMalariaGenetics report. Specifically, 
NumVectorsWithSporozoites_** and AvgNumSporozoitesPerVector columns broken down by the alleles of the vectors. 
We accumulate data from the logging values and compare to what's in the report. 
"""

config_name = "Config_Name"
released_genomes = ["X-b:X-b",
                    "X-a:X-b",
                    "X-b:X-a",
                    "X-a:X-a"]


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
    param_obj = {config_name: cdj[config_name]}

    if debug:
        with open("DEBUG_emod_params.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj


def parse_output_file(output_file="test.txt", debug=False):
    """

    Args:
        output_file:
        debug:

    Returns:

    """

    num_sporozoites_per_vector = {"X-b:X-b": [],
                                  "X-a:X-b": [],
                                  "X-b:X-a": [],
                                  "X-a:X-a": []}
    average_sprozoites_per_vector_per_genome_per_day = {"X-b:X-b": {},
                                                        "X-a:X-b": {},
                                                        "X-b:X-a": {},
                                                        "X-a:X-a": {}}
    sporozoites_day_dict = {"X-b:X-b": {"AA": {}, "AG": {}, "other": {}},
                            "X-a:X-b": {"AA": {}, "AG": {}, "other": {}},
                            "X-b:X-a": {"AA": {}, "AG": {}, "other": {}},
                            "X-a:X-a": {"AA": {}, "AG": {}, "other": {}}}
    time = 0
    with open(output_file) as logfile:
        for line in logfile:
            if "has sporozoites = " in line:
                # line:  Vector 3172 with X-a:X-b has sporozoites = 10012 GG has 2000, 10013 AA has 262,
                # 10013 GG has 248, 10013 AG has 237, 10013 GA has 253
                words = line.split("=")
                # doing it this was because linus and windows have different beginning of line
                vector_genome = words[0].split()[-3]
                if vector_genome not in released_genomes:
                    raise ValueError(
                        f"Unexpected genome - '{vector_genome}' detected in the {output_file} log lines.\n")
                sporozoites = words[1].split(",")
                # AA and AG, GA and GG are "other"
                total_sporozoites_in_vector = 0
                aa_present = False
                ag_present = False
                other_present = False
                for sporozoite in sporozoites:
                    data = sporozoite.split()
                    sp_barcode = data[1]
                    sp_number = int(data[3])
                    total_sporozoites_in_vector += sp_number
                    if sp_barcode == "AA":
                        aa_present = True
                    elif sp_barcode == "AG":
                        ag_present = True
                    else:
                        other_present = True

                num_sporozoites_per_vector[vector_genome].append(total_sporozoites_in_vector)
                if aa_present:
                    sporozoites_day_dict[vector_genome]["AA"][time] += 1
                if ag_present:
                    sporozoites_day_dict[vector_genome]["AG"][time] += 1
                if other_present:
                    sporozoites_day_dict[vector_genome]["other"][time] += 1

            elif "Update(): Time" in line:
                # record all the things
                for vector_genome in released_genomes:
                    this_genome = num_sporozoites_per_vector[vector_genome]
                    if len(this_genome) == 0:
                        average_sprozoites_per_vector_per_genome_per_day[vector_genome][time] = 0
                    else:
                        average_sprozoites_per_vector_per_genome_per_day[vector_genome][time] = sum(
                            this_genome) / len(this_genome)
                num_sporozoites_per_vector = {"X-b:X-b": [],
                                              "X-a:X-b": [],
                                              "X-b:X-a": [],
                                              "X-a:X-a": []}
                # update the time set next time to 0
                time = int(float(sft.get_val("Time: ", line)))
                for genome in released_genomes:
                    sporozoites_day_dict[genome]["AA"][time] = 0
                    sporozoites_day_dict[genome]["AG"][time] = 0
                    sporozoites_day_dict[genome]["other"][time] = 0

    if debug:
        with open("sporozoites_day_dict.txt", "w") as help_file:
            help_file.write(f"{sporozoites_day_dict}")
        with open("average_sprozoites_per_vector_per_genome_per_day.txt", "w") as help_file:
            help_file.write(f"{average_sprozoites_per_vector_per_genome_per_day}")

    return sporozoites_day_dict, average_sprozoites_per_vector_per_genome_per_day


def create_report_file(param_obj, report_name, output_report_file, sporozoites_day_dict,
                       average_sprozoites_per_vector_per_genome_per_day, debug=False):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Test name: {str(param_obj[config_name])} \n")
        report_df = pd.read_csv(output_report_file)
        if not len(report_df):
            success = False
            outfile.write(sft.sft_no_test_data)
            outfile.write(f"No data in {report_df}\n")
        # AvgNumSporozoitesPerVector
        report_df['index'] = report_df.reset_index().index
        for row_number in range(len(report_df["index"])):
            row = report_df.loc[report_df['index'] == row_number]
            report_time = int(row["Time"].iloc[0])
            report_genome = str(row["Genome"].iloc[0])
            report_NumVectorsWithSporozoites_AA = int(row["NumVectorsWithSporozoites_AA"].iloc[0])
            report_NumVectorsWithSporozoites_AG = int(row["NumVectorsWithSporozoites_AG"].iloc[0])
            report_NumVectorsWithSporozoites_Other = int(row["NumVectorsWithSporozoites_Other"].iloc[0])
            report_AvgNumSporozoitesPerVector = float(row["AvgNumSporozoitesPerVector"].iloc[0])
            from_logging_AA = sporozoites_day_dict[report_genome]["AA"][report_time]
            from_logging_AG = sporozoites_day_dict[report_genome]["AG"][report_time]
            from_logging_Other = sporozoites_day_dict[report_genome]["other"][report_time]
            from_logging_average = round(average_sprozoites_per_vector_per_genome_per_day[report_genome][report_time], 2)
            # check average:
            tolerance = 0.11  # because rounding differences between windows and linux
            if not isclose(from_logging_average, report_AvgNumSporozoitesPerVector, abs_tol=0.11):
                success = False
                outfile.write(f"BAD: on Time {report_time} for Genome {report_genome} the AvgNumSporozoitesPerVector was "
                              f"{report_AvgNumSporozoitesPerVector} but we expected {from_logging_average}.\n")
            elif debug:
                outfile.write(f"GOOD: on Time {report_time} for Genome {report_genome} the AvgNumSporozoitesPerVector was "
                              f"{report_AvgNumSporozoitesPerVector} and we expected {from_logging_average}.\n")
            if from_logging_AA != report_NumVectorsWithSporozoites_AA:
                success = False
                outfile.write(f"BAD: on Time {report_time} for Genome {report_genome} the NumVectorsWithSporozoites_AA was "
                              f"{report_NumVectorsWithSporozoites_AA} but we expected {from_logging_AA}.\n")
            elif debug:
                outfile.write(f"GOOD: on Time {report_time} for Genome {report_genome} the NumVectorsWithSporozoites_AA was "
                              f"{report_NumVectorsWithSporozoites_AA} and we expected {from_logging_AA}.\n")
            if from_logging_AG != report_NumVectorsWithSporozoites_AG:
                success = False
                outfile.write(f"BAD: on Time {report_time} for Genome {report_genome} the NumVectorsWithSporozoites_AG was "
                              f"{report_NumVectorsWithSporozoites_AG} but we expected {from_logging_AG}.\n")
            elif debug:
                outfile.write(f"GOOD: on Time {report_time} for Genome {report_genome} the NumVectorsWithSporozoites_AG was "
                              f"{report_NumVectorsWithSporozoites_AG} and we expected {from_logging_AG}.\n")
            if from_logging_Other != report_NumVectorsWithSporozoites_Other:
                success = False
                outfile.write(f"BAD: on Time {report_time} for Genome {report_genome} the NumVectorsWithSporozoites_Other was "
                              f"{report_NumVectorsWithSporozoites_Other} but we expected {from_logging_Other}.\n")
            elif debug:
                outfile.write(f"GOOD: on Time {report_time} for Genome {report_genome} the NumVectorsWithSporozoites_Other was "
                              f"{report_NumVectorsWithSporozoites_Other} and we expected {from_logging_Other}.\n")

        outfile.write(sft.format_success_msg(success))


def application(output_folder="output",
                config_filename="config.json",
                stdout="test.txt",
                report_name=sft.sft_output_filename,
                output_report_file="ReportVectorGeneticsMalariaGenetics_SillySkeeter_Female_GENOME.csv",
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("stdout_filename: " + stdout + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename)
    sporozoites_day_dict, average_sprozoites_per_vector_per_genome_per_day = parse_output_file(stdout, debug)
    create_report_file(param_obj, report_name, output_folder + "/" + output_report_file, sporozoites_day_dict,
                       average_sprozoites_per_vector_per_genome_per_day, debug)


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
