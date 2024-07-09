#!/usr/bin/python

from scipy.stats import linregress
import numpy as np
import pandas as pd
import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import dtk_test.dtk_sft_svetlana as svet

"""
Spec:
    Duration. The duration of larval development shall be a function of air temperature on the first day of its
     larval stage. The function shall behave according to the following functional form: the Arrhenius equation, 
     a1 * exp( -a2 / T ), with T in degrees Kelvin. The 1/the rate calculated using this function signifies the 
     average of an exponentially distributed duration of the larval development.
    Relevant parameters:
    
    Aquatic_Arrhenius_1: 9752291.727
    Aquatic_Arrhenius_2: 5525.492

This test tests the Arrhenius rate of larval development duration.
"""

def local_larval_growth_mod_calc(previous_larva_count, capacity):
    if previous_larva_count > capacity:
        return capacity / previous_larva_count
    else:
        return 1


def parse_output_file(stdout, debug=False):
    """
    Parses following lines, example:

    GetLarvalDevelopmentProgress returning 0.055726 based on temperature 38.327450, and growth modifier 0.289180.
    GetLocalLarvalGrowthModifier returning 0.289180 based on current larval count 3050, capacity 882.000000.
    Hatching 3606 female eggs and pushing into larval queues (bits=0), id = 512.
    Immature adults emerging from larva queue: population = 1, genome bits = 0, id = 512, age = 0.

    Args:
        stdout: log file in which to look for the lines
        debug: flag, if True, a file with all the lines of interest is written out

    Returns: two data frames with parsed data

    """
    data = {}
    larval_dev_data = []
    larva_dev = []
    sft.wait_for_done()
    id_duration = {}
    filtered_lines = []
    time = 0.0
    identities = []
    durations = []
    populations = []
    temperatures = []
    with open(stdout) as logfile:
        for line in logfile:
            if "GetLarvalDevelopmentProgress" in line:
                temperature_k = float(sft.get_val("based on temperature ", line)) + 273.15
                progress = float(sft.get_val("GetLarvalDevelopmentProgress returning ", line))
                local_larval_growth_mod = float(sft.get_val("and growth modifier ", line))
                temperatures.append(temperature_k)
                larva_dev.append(progress / local_larval_growth_mod)
                filtered_lines.append(line)
            if "GetLocalLarvalGrowthModifier" in line:
                capacity = float(sft.get_val("capacity ", line))
                previous_larva_count = float(sft.get_val("based on current larval count ", line))
                local_larval_growth_modifier = float(sft.get_val("GetLocalLarvalGrowthModifier returning ", line))
                larval_dev_datum = (previous_larva_count, capacity, local_larval_growth_modifier)
                larval_dev_data.append(larval_dev_datum)
                filtered_lines.append(line)
            if ("larval queues" or "Update(): Time" or "GetLarvalDevelopmentProgress" or "Immature adults") in line:
                filtered_lines.append(line)
            if "Time:" in line:
                time = float(sft.get_val("Time: ", line))
            if "larval queues" in line:
                identity = float(sft.get_val("id = ", line))
                id_duration[str(identity)] = time
            if "Immature adults" in line:
                identity = float(sft.get_val("id = ", line))
                id_duration[str(identity)] = time - id_duration[str(identity)]
                identities.append(identity)
                durations.append(id_duration[str(identity)])
                population = float(sft.get_val("population = ", line))
                populations.append(population)
    data["temperature"] = temperatures
    data["larva_dev"] = larva_dev
    arrhen_df = pd.DataFrame(data)
    arrhen_df = arrhen_df.sort_values(by='temperature')
    growth_mod_df = pd.DataFrame(larval_dev_data)
    growth_mod_df.columns = ['prev_larva_count', 'capacity', 'growth_mod']
    if debug:
        with open("DEBUG_filtered_lines.txt") as debug_file:
            debug_file.writelines(filtered_lines)
    return arrhen_df, growth_mod_df


def create_report_file(param_obj, arrhen_df, growth_mod_df, report_name, debug=False):

    success = True
    with open(report_name, "w") as report_file:
        if len(arrhen_df) == 0 or len(growth_mod_df) == 0:
            report_file.write(sft.sft_no_test_data)
            raise ValueError("No test data found!")
        report_file.write("# Test name: " + param_obj[veds.ConfigKeys.CONFIG_NAME] + ", Run number: " +
                          str(param_obj[veds.ConfigKeys.RUN_NUMBER]) + "\n# Test"
                          " compares between the actual and expected larval growth mods to verify "
                          "larvae develop at Arrhenius rate as a function of temperature.\n")
        # Testing the Arrhenius function
        arrhen_df["theoretical_dev"] = None
        for index in arrhen_df.index:
            temperature_K = float(arrhen_df.at[index, "temperature"])
            theoretical_larva_dev = veds.larval_dur_calc(temperature_K, param_obj)
            arrhen_df.at[index, "theoretical_dev"] = theoretical_larva_dev
            actual_larva_dev = float(arrhen_df.at[index, "larva_dev"])
            tolerance = 5e-2 * theoretical_larva_dev
            if abs(theoretical_larva_dev - actual_larva_dev) > tolerance:
                success = False
                low_acceptance_bound = theoretical_larva_dev - tolerance
                high_acceptance_bound = theoretical_larva_dev + tolerance
                report_file.write("BAD: At temperature {0}, larva_dev={1}, which is not "
                                  "within acceptance "
                                  "range of ({2}, {3}).  Expected {4}.\n"
                                  .format(temperature_K, actual_larva_dev, low_acceptance_bound,
                                          high_acceptance_bound,
                                          theoretical_larva_dev))
        if success:
            report_file.write("GOOD: For all temperatures, larva_dev was within 5% of "
                              "theoretical.\n")
        svet.plot_scatter_dist_w_avg(arrhen_df['larva_dev'], xaxis=arrhen_df['temperature'],
                                     theory=arrhen_df['theoretical_dev'],
                                     title='Arrhenius Larva Dev vs. Temperature',
                                     xlabel='temperature Kelvin', ylabel='theoretical larva dev rate',
                                     category='development_rate')
        arrhen_df["ln_df"] = np.log(arrhen_df["larva_dev"].astype('float64'))
        arrhen_df["theoretical_ln_df"] = np.log(arrhen_df["theoretical_dev"].astype('float64'))
        arrhen_df["1 / T"] = 1 / arrhen_df["temperature"].astype('float64')
        svet.plot_scatter_dist_w_avg(arrhen_df['ln_df'], xaxis=arrhen_df['1 / T'],
                                     theory=arrhen_df['theoretical_ln_df'],
                                     title='ln(Arrhenius Larva Dev) vs. 1/Temperature',
                                     xlabel='1/ temperature Kelvin', ylabel='ln(theoretical larva dev rate)',
                                     category='ln_development_rate')

        actual_slope, actual_intercept, actual_r, actual_p, actual_std_err = \
            linregress(arrhen_df["1 / T"], arrhen_df["ln_df"])
        theoretical_slope, theoretical_intercept, theoretical_r, theoretical_p, theoretical_std_err =\
            linregress(arrhen_df["1 / T"], arrhen_df["theoretical_ln_df"])
        if abs(actual_slope - theoretical_slope) > 5e-2 * abs(theoretical_slope):
            success = False
            report_file.write("BAD: Slope of logarithmic Arrhenius ({}) not within 5% of the theoretical ()"
                              ".\n".format(actual_slope, theoretical_slope))
        else:
            report_file.write("GOOD: Logarithmic Arrhenius function slope ({}) was within 5% of theoretical slope ({})"
                              ".\n".format(actual_slope, theoretical_slope))

        # Testing the larva growth modifier
        theoretical_growth_mod = []
        for index in growth_mod_df.index:
            prev_larva_count = growth_mod_df.at[index, 'prev_larva_count']
            capacity = growth_mod_df.at[index, 'capacity']
            theory_growth_mod = local_larval_growth_mod_calc(prev_larva_count, capacity)
            theoretical_growth_mod.append(theory_growth_mod)
            actual_growth_mod = growth_mod_df.at[index, 'growth_mod']
            if abs(theory_growth_mod-actual_growth_mod) > 5e-2 * theory_growth_mod:
                success = False
                report_file.write(
                    f"BAD: Local larval growth modifier {actual_growth_mod} not within 5% of "
                    f"theoretical, {theory_growth_mod}."
                    f" Previous_larva_count={prev_larva_count}.\n")
        svet.plot_scatter_dist_w_avg(growth_mod_df['growth_mod'], theory=theoretical_growth_mod,
                                     label1="actual_growth_mod", label2="theory_growth_mod",
                                     title="Theoretical vs. actual growth mod", category="growth_modifier_data")
        if success:
            report_file.write("GOOD: Local larval growth mod was within 5% of the theoretical growth mod.\n")
        report_file.write(sft.format_success_msg(success))


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
    arrhen_df, growth_mod_df = parse_output_file(stdout, debug)
    create_report_file(param_obj, arrhen_df, growth_mod_df, report_name, debug)


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
