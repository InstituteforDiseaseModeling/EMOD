#!/usr/bin/python

from __future__ import division
import pandas as pd
import numpy as np
from scipy.stats import linregress
import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import dtk_test.dtk_sft_svetlana as svet

"""
Tests that the eggs hatch at an Arrhenius rate.

"""


def create_report_file(param_obj, report_name, output_df, debug=False):
    success = True
    with open(report_name, "w") as report_file:
        report_file.write("# Test name: " + param_obj[veds.ConfigKeys.CONFIG_NAME] + ", Run number: " +
                          str(param_obj[veds.ConfigKeys.RUN_NUMBER]) + "\n# Test"
                          " compares between the actual_delay_factor and expected egg hatch delay factors to verify "
                          "eggs hatch at Arrhenius rate as a function of temperature.\n")
        if len(output_df) == 0:
            report_file.write(sft.sft_no_test_data)
            raise ValueError("No data found in DataFrame!")
        else:
            output_df["theoretical delay factor"] = None
            for index in output_df.index:
                temperature_K = float(output_df.at[index, "temperature Kelvin"])
                theoretical_delay_factor = 1/veds.egg_dur_calc(temperature_K, param_obj)
                output_df.at[index, "theoretical delay factor"] = theoretical_delay_factor
                actual_delay_factor = float(output_df.at[index, "egg hatch delay factor"])
                tolerance = 5e-2 * theoretical_delay_factor
                if abs(theoretical_delay_factor-actual_delay_factor) > tolerance:
                    success = False
                    low_acceptance_bound = theoretical_delay_factor - tolerance
                    high_acceptance_bound = theoretical_delay_factor + tolerance
                    report_file.write("BAD: At temperature {0}, actual delay factor={1}, which is not "
                                      "within acceptance "
                                      "range of ({2}, {3}).  Expected {4}.\n"
                                      .format(temperature_K, actual_delay_factor, low_acceptance_bound,
                                              high_acceptance_bound,
                                              theoretical_delay_factor))
            if success:
                report_file.write("GOOD: For all temperatures, observed egg hatch delay factor - {} was within 5% of "
                                  "theoretical - {}.\n".format(actual_delay_factor, theoretical_delay_factor))
            svet.plot_scatter_dist_w_avg(output_df['egg hatch delay factor'], xaxis=output_df['temperature Kelvin'],
                                         theory=output_df['theoretical delay factor'],
                                         title='Arrhenius Egg Development Duration vs. Temperature',
                                         xlabel='temperature Kelvin', ylabel='egg dev duration',
                                         category='egg_development_duration')
            output_df["ln_df"] = np.log(output_df["egg hatch delay factor"].astype('float64'))
            output_df["theoretical_ln_df"] = np.log(output_df["theoretical delay factor"].astype('float64'))
            output_df["1 / T"] = 1 / output_df["temperature Kelvin"].astype('float64')

            svet.plot_scatter_dist_w_avg(output_df['ln_df'], xaxis=output_df['1 / T'],
                                         theory=output_df['theoretical_ln_df'],
                                         title='ln(Arrhenius Egg Development Duration) vs. 1/Temperature',
                                         xlabel='1 / temperature Kelvin', ylabel='ln(egg dev duration)',
                                         category='ln_egg_development_duration')
            actual_slope, actual_intercept, actual_r, actual_p, actual_std_err = linregress(output_df["1 / T"], output_df["ln_df"])
            theoretical_slope, theoretical_intercept, theoretical_r, theoretical_p, theoretical_std_err = \
                linregress(output_df["1 / T"], output_df["theoretical_ln_df"])
            if abs(actual_slope - theoretical_slope) > 5e-2 * abs(theoretical_slope):
                success = False
                report_file.write("BAD: Slope of logarithmic Arrhenius - {} not within 5% of the theoretical -"
                                  " {}.\n".format(actual_slope, theoretical_slope))
            else:
                report_file.write("GOOD: Logarithmic Arrhenius function slope  - {} was within 5% of theoretical slope."
                                  " - {} \n".format(actual_slope, theoretical_slope))
        report_file.write(sft.format_success_msg(success))
    if debug:
        print(sft.format_success_msg(success))


def parse_output_file(stdout, debug=False):
    """
    Parses following lines, example:
    [VectorPopulation] temperature = 22.957451, local density dependence modifier is 1.000000, egg hatch delay factor
    is 0.224008, current population is 1, hatched is 1, id = 518.

    Args:
        stdout: the logging file to parse
        debug: flag, if true writes out the lines that were used to collect the data

    Returns: data frame

    """
    filtered_lines = []
    temperature_k_list = []
    egg_hatch_delay_list = []
    with open(stdout) as logfile:
        for line in logfile:
            if "temperature =" in line:
                filtered_lines.append(line)
                kelvin_temperature = float(sft.get_val("temperature = ", line)) + 273.15
                egg_hatch_delay_factor = float(sft.get_val("egg hatch delay factor is ", line))
                temperature_k_list.append(kelvin_temperature)
                egg_hatch_delay_list.append(1/egg_hatch_delay_factor)
        if debug:
            with open("DEBUG_filtered_lines.txt", "w") as outfile:
                outfile.writelines(filtered_lines)
    d = {"temperature Kelvin": temperature_k_list, "egg hatch delay factor": egg_hatch_delay_list}
    output_df = pd.DataFrame.from_dict(d)
    return output_df


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
    create_report_file(param_obj, report_name, output_df, debug)


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
                stdout=args.stdout,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)
