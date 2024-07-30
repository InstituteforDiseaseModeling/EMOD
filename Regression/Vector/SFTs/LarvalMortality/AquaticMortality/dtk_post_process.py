#!/usr/bin/python

import pandas as pd
import scipy.stats as stat
import numpy as np
import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import dtk_test.dtk_sft_svetlana as svet

"""
Spec:
    Natural mortality. Larvae shall experience a daily mortality rate.
    Relevant parameters:
    Aquatic_Mortality_Rate 0.12
    
    The rate is converted to probability of larva dying. This test looks at actual numbers of larva dying and compares
    to the probability.
"""


def calc_local_larval_mortality(aquatic_mortality, previous_larva_count, larval_cap):
    if previous_larva_count > larval_cap:
        aquatic_mortality = aquatic_mortality * (previous_larva_count / larval_cap)
    return aquatic_mortality


def parse_output_file(stdout, debug=False):
    """
    Parses following lines, example:

    GetLarvalMortalityProbability returning 0.221199 based on local larval mortality (0.250000),
    rainfall mortality (0.000000), and artifical mortality (0.000000).

    Adjusting larval population from 1094 to 850 based on overcrowding considerations, id = 509

    Args:
        stdout: log file in which to look for the lines
        debug: flag, if True, a file with all the lines of interest is written out

    Returns: data frame

    """
    d = {}
    larva_death_lines = []
    larva_death_prob = []
    time = 0
    times = []
    with open(stdout) as logfile:
        for line in logfile:
            if "based on local larval mortality" in line:
                larva_death_lines.append(line)  # collecting for debug
            elif "Adjusting larval population" in line:  # Here, we are ignoring cohorts because rate should be same
                larva_death_lines.append(line)
                old_pop = float(sft.get_val("from ", line))
                new_pop = float(sft.get_val("to ", line))
                larva_death_prob.append(1 - new_pop/old_pop)
                times.append(time)
            elif "Update(): Time" in line:
                time = float(sft.get_val("Time: ", line))
    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as debug_file:
            debug_file.writelines(larva_death_lines)
    d["time"] = times
    d["prob"] = larva_death_prob
    output_df = pd.DataFrame(d)
    return output_df


def create_report_file(param_obj, output_df, report_name, debug=False):
    aquatic_mortality = param_obj[veds.ConfigKeys.VectorSpeciesParams.AQUATIC_MORTALITY_RATE]
    aquatic_mortality_probability = float(1 - np.exp(-aquatic_mortality))
    success = True
    with open(report_name, "w") as outfile:
        if len(output_df) == 0:
            outfile.write(sft.sft_no_test_data)
            raise ValueError("No data!")
        outfile.write("# Test name: " + param_obj[veds.ConfigKeys.CONFIG_NAME] + ", Run number: " +
                      str(param_obj[veds.ConfigKeys.RUN_NUMBER]) + "\n# Test"
                      " compares between the actual and expected larval mortality rates (converted to probabilities).\n")
        outfile.write(f"\nTheoretical mortality probability: {aquatic_mortality_probability} from rate: "
                      f"{aquatic_mortality}\n")
        outfile.write(f"Actual mortality probability: {np.mean(output_df['prob'])}\n")
        statistic, pvalue = stat.ttest_1samp(output_df['prob'], aquatic_mortality_probability)
        if pvalue < 5e-2:
            success = False
            outfile.write("BAD: The means of the theoretical and actual mortality are significantly different.\n")
        else:
            outfile.write("GOOD: The means of the theoretical and actual mortality are not significantly different.\n")
        outfile.write(f'T-test results: statistic = {statistic}, p-value = {pvalue}\n')
        outfile.write(sft.format_success_msg(success))
    svet.plot_scatter_dist_w_avg(actual=output_df['prob'], theory=[aquatic_mortality_probability] * len(output_df['prob']),
                                 label2="theoretical_mortality", label1="actual_mortality",
                                 title="Theoretical vs. actual mortality", ylabel="mortality",
                                 category="larval_mortality_data")


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
    output_df = parse_output_file(stdout, debug)
    create_report_file(param_obj, output_df, report_name, debug)


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
