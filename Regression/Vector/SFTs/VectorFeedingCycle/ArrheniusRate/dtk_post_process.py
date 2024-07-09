#!/usr/bin/python

import pandas as pd
from scipy.stats import linregress
import numpy as np
import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import dtk_test.dtk_sft_svetlana as svet

"""
Spec: 
    The duration of Aedes feeding cycle shall be a function of air temperature on the day of previous feed. 
    The function shall behave according to the following functional form: the Arrhenius equation, a1 * exp( -a2 / T ), 
    where T is temperature in degrees Kelvin. The 1/the rate calculated using this function signifies the average of 
    an exponentially distributed duration of the feeding cycle.
    Relevant parameters:
    Temperature_Dependent_Feeding_Cycle: "ARRHENIUS_DEPENDENCE"
    Cycle_Arrhenius_1	4.09E+10 (a1)
    Cycle_Arrhenius_2	7740     (a2)

This test tests the Arrhenius rate of the vector feeding cycle. 
"""


def parse_output_file(stdout, debug=False):
    """
    Parses following lines, example:

    Mean gonotrophic cycle duration = 10.54870 days at 15.77 degrees C.

    Args:
        stdout: the logging file to parse
        debug: flag, if true writes out the lines that were used to collect the data

    Returns: data frame

    """
    d = {}
    temperatures = []
    durations = []
    with open(stdout) as logfile:
        for line in logfile:
            if "Mean gonotrophic " in line:
                temperature_k = float(sft.get_val("days at ", line)) + 273.15
                duration_in_days = float(sft.get_val("gonotrophic cycle duration = ", line))
                if duration_in_days not in durations and temperature_k not in temperatures:
                    durations.append(1 / duration_in_days)
                    temperatures.append(temperature_k)
    d["temperature"] = temperatures
    d["rate"] = durations
    output_df = pd.DataFrame(d)
    output_df = output_df.sort_values(by='temperature')
    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as debug_file:
            debug_file.write(str(output_df))
    return output_df


def create_report_file(param_obj, report_name, output_df, debug=False):
    success = True
    with open(report_name, "w") as outfile:
        if len(output_df) == 0:
            outfile.write(sft.sft_no_test_data)
            raise ValueError("No data in DataFrame!")
        outfile.write("# Test name: " + param_obj[veds.ConfigKeys.CONFIG_NAME] + ", Run number: " +
                          str(param_obj[veds.ConfigKeys.RUN_NUMBER]) + "\n# Test"
                          " compares between the actual_feeding cycle and expected feeding cycle durations to verify "
                          "mosquitoes feed at Arrhenius rate as a function of temperature.\n")
        output_df["theoretical rate"] = None
        a2 = param_obj[veds.ConfigKeys.VectorSpeciesParams.CYCLE_ARRHENIUS_2]
        for index in output_df.index:
            temperature_K = float(output_df.at[index, "temperature"])
            theoretical_rate = veds.vector_dur_calc(temperature_K, param_obj)
            output_df.at[index, "theoretical rate"] = theoretical_rate
            actual_delay_factor = float(output_df.at[index, "rate"])
            if abs(theoretical_rate-actual_delay_factor) > 5e-2 * theoretical_rate:
                success = False
                low_acceptance_bound = theoretical_rate - 5e-2 * theoretical_rate
                high_acceptance_bound = theoretical_rate + 5e-2 * theoretical_rate
                outfile.write(f"BAD: At temperature {temperature_K}, delay factor={actual_delay_factor}, which is "
                              f"not within acceptance range of ({low_acceptance_bound}, {high_acceptance_bound}). "
                              f"Expected {theoretical_rate}.\n")
        if success:
            outfile.write("GOOD: For the temperature, feeding cycle - {} was within 5% of "
                          "theoretical - {}.\n".format(actual_delay_factor, theoretical_rate))
        svet.plot_scatter_dist_w_avg(output_df['rate'], xaxis=output_df['temperature'],
                                     theory=output_df['theoretical rate'],
                                     title='Arrhenius Feeding Cycle vs. Temperature',
                                     xlabel='temperature Kelvin', ylabel='feeding cycle rate',
                                     category='feeding_rate')
        output_df["ln_df"] = np.log(output_df["rate"].astype('float64'))
        output_df["theoretical_ln_df"] = np.log(output_df["theoretical rate"].astype('float64'))
        output_df["1 / T"] = 1 / output_df["temperature"].astype('float64')

        svet.plot_scatter_dist_w_avg(output_df['ln_df'], xaxis=output_df['1 / T'],
                                     theory=output_df['theoretical_ln_df'],
                                     title='ln(Arrhenius Feeding Cycle) vs. 1/Temperature',
                                     xlabel='1 / temperature Kelvin', ylabel='ln(feeding cycle rate)',
                                     category='ln_feeding_rate')
        actual_slope, actual_intercept, actual_r, actual_p, actual_std_err = \
            linregress(output_df["1 / T"], output_df["ln_df"])
        theoretical_slope, theoretical_intercept, theoretical_r, theoretical_p, theoretical_std_err = \
            linregress(output_df["1 / T"], output_df["theoretical_ln_df"])
        if abs(actual_slope - (-a2) - (abs(theoretical_slope - (-a2)))) > abs(theoretical_slope * 5e-2):
            success = False
            outfile.write("BAD: Slope of logarithmic Arrhenius = {} not within 5% of the theoretical - {}."
                          "\n".format(abs(actual_slope - (-a2)), abs(abs(theoretical_slope - (-a2)))))
        else:
            outfile.write("GOOD: Logarithmic Arrhenius function slope - {} was within 5% of theoretical slope - {}."
                          "\n".format(abs(actual_slope - (-a2)), abs(abs(theoretical_slope - (-a2)))))
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
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)
