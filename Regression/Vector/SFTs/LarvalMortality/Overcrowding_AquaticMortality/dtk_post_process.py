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
    Aquatic_Mortality_Rate: 0.12
    Larval_Density_Dependence: "UNIFORM_WHEN_OVERPOPULATION"

This test tests overcrowding and aquatic mortality as both are used in the the local_larval_mortality number.
If either is wrong, the test will fail. Aquatic Mortality is taken from the config file.
"""


def calc_local_larval_mortality(aquatic_mortality, previous_larva_count, larval_cap):
    if previous_larva_count > larval_cap:
        aquatic_mortality = aquatic_mortality * (previous_larva_count / larval_cap)
    return aquatic_mortality


def parse_output_file(stdout, debug=False):
    """
    Parses following lines, example:

    GetLocalLarvalMortality returning 0.909896 based on previous larval count 75237, current larval count 2563,
    and capacity 9922.499023.

    Args:
        stdout: log file in which to look for the lines
        debug: flag, if True, a file with all the lines of interest is written out

    Returns: data frame
    """
    d = {}
    data = []
    larva_count_list = []
    larva_cap_list = []
    larva_mortality_list = []
    filtered_lines = []
    with open(stdout) as logfile:
        for line in logfile:
            if "GetLocalLarvalMortality" in line:
                previous_larva_count = float(sft.get_val("previous larval count ", line))
                larval_cap = float(sft.get_val("and capacity ", line))
                local_larval_mortality = float(sft.get_val("returning ", line))
                data.append((previous_larva_count, larval_cap, local_larval_mortality))
                larva_cap_list.append(larval_cap)
                larva_count_list.append(previous_larva_count)
                larva_mortality_list.append(local_larval_mortality)
                filtered_lines.append(line)
    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as debug_file:
            debug_file.writelines(filtered_lines)
    d["larva_count"] = larva_count_list
    d["larva_cap"] = larva_cap_list
    d["larva_mortality"] = larva_mortality_list
    output_df = pd.DataFrame(d)
    return output_df


def create_report_file(param_obj, output_df, report_name, debug=False):
    aquatic_mortality = param_obj[veds.ConfigKeys.VectorSpeciesParams.AQUATIC_MORTALITY_RATE]
    success = True
    output_df['theoretical_mortality'] = None
    with open(report_name, "w") as outfile:
        if len(output_df) == 0:
            outfile.write(sft.sft_no_test_data)
            raise ValueError("No data!")
        outfile.write("# Test name: " + param_obj[veds.ConfigKeys.CONFIG_NAME] + ", Run number: " +
                      str(param_obj[veds.ConfigKeys.RUN_NUMBER]) + "\n# Test"
                      " compares between the actual and expected egg larval mortality rates to verify "
                      "that larva die at the correct rate in overcrowding conditions.\n")
        for index in output_df.index:
            theory = calc_local_larval_mortality(aquatic_mortality, output_df.at[index, 'larva_count'],
                                                 output_df.at[index, 'larva_cap'])
            output_df.at[index, 'theoretical_mortality'] = theory
            actual = output_df.at[index, 'larva_mortality']
            if abs(theory-actual) > 1e-3:
                success = False
                outfile.write(f"BAD: Theoretical morality, {theory}, was over 0.001 off from actual"
                              f" mortality, {actual}.\n")
        if success:
            outfile.write("GOOD: Theoretical mortality and actual mortality were always within 0.001.\n")
        statistic, pvalue = stat.ttest_1samp(output_df['larva_mortality'], np.mean(output_df['theoretical_mortality']))
        if pvalue < 5e-2:
            success = False
            outfile.write("BAD: The means of the theoretical and actual mortality are significantly different.\n")
        else:
            outfile.write("GOOD: The means of the theoretical and actual mortality are not significantly different.\n")
        outfile.write(f'T-test results: statistic = {statistic}, p-value = {pvalue}\n')
        outfile.write(sft.format_success_msg(success))
    svet.plot_scatter_dist_w_avg(output_df['larva_mortality'], theory=output_df['theoretical_mortality'],
                                 label2="theoretical_mortality", label1="actual_mortality",
                                 title="Theoretical vs. actual mortality", ylabel="mortality",
                                 category="overcrowding_and_aquatic_mortality")


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
