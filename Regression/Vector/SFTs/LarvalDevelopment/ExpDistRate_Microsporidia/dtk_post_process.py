#!/usr/bin/python

import pandas as pd
import numpy as np
import math
import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import dtk_test.dtk_sft_svetlana as svet

"""
Spec:
    The duration of larval development shall be a function of air temperature on the first day of
    its larval stage. The function shall behave according to the following functional form: the Arrhenius equation, 
    a1 * exp( -a2 / T ), with T in degrees Kelvin. The 1/the rate calculated using this function signifies the average 
    of an exponentially distributed duration of the larval development.
    Relevant Parameters:
    Aquatic_Arrhenius_1: 9752291.727
    Aquatic_Arrhenius_2: 5525.492 

    Testing the larval development progress - the duration is based on the cohort IDs.  The cohort IDs allow the 
    hatching of eggs into larvae to be tracked and later, the development into immature vectors.

"""


def create_report_file(param_obj, report_name, temperature_k, output_df, debug=False):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write("# Test name: " + param_obj[veds.ConfigKeys.CONFIG_NAME] + ", Run number: " +
                      str(param_obj[veds.ConfigKeys.RUN_NUMBER]) +
                      "\n"
                      "# Test compares between the actual and expected larval development durations \n"
                      "# to verify larvae develop with an exponential distribution, and the mean \n"
                      "# of the development distribution is equal to 1/the Arrhenius rate at that temperature.\n")
        if output_df is None:
            outfile.write(sft.sft_no_test_data)
            raise ValueError("No data found in the stdout file!")
        outfile.write(str(output_df))
        outfile.write("\n")
        microsporidia_larval_growth_modifier = param_obj["Larval_Growth_Modifier"]
        # rounding up the number of days, because 6.5 days means larva will be fully developed on day 7
        theoretical_larva_dev_dur = sft.round_up(1 / veds.larval_dur_calc(temperature_k, param_obj), 0)
        distribution = []
        for index, row in output_df.iterrows():
            distribution += [int(output_df.at[index, 'durations'])] * int(output_df.at[index, 'populations'])
        actual_larva_dev_dur = np.mean(distribution)
        outfile.write("Theoretical larval dev for temperature {}: {}, actual = {}"
                      "\n".format(temperature_k, theoretical_larva_dev_dur, actual_larva_dev_dur))

        # now checking the microsporidia numbers
        distribution = []
        # dividing by the growth modifier, because looking at duration
        theoretical_larva_dev_dur_m = sft.round_up(theoretical_larva_dev_dur / microsporidia_larval_growth_modifier, 0)
        for index, row in output_df.iterrows():
            distribution += [int(output_df.at[index, 'durations_m'])] * int(output_df.at[index, 'populations_m'])
        actual_larva_dev_dur_m = np.mean(distribution)
        outfile.write("With Microsporidia, Theoretical larval dev for temperature {}: {}, actual = {}"
                      "\n".format(temperature_k, theoretical_larva_dev_dur_m, actual_larva_dev_dur_m))


        # Now testing to see whether or not an exponential distribution is followed
        a2 = param_obj[veds.ConfigKeys.VectorSpeciesParams.AQUATIC_ARRHENIUS_2]
        lower_a2 = a2 / 2
        higher_a2 = a2 * 2
        a2_vector = np.linspace(lower_a2, higher_a2, 100)
        rate_list = []
        for a2 in a2_vector:
            rate = param_obj[veds.ConfigKeys.VectorSpeciesParams. AQUATIC_ARRHENIUS_1] * math.exp(-(a2) / (temperature_k))
            rate_list.append(rate)
        if not svet.is_exponential_behavior(rate_list, outfile, "larval development"):
            success = False
        svet.plot_scatter_dist_w_avg(output_df['populations'],
                                     xaxis=output_df['durations'], title='Larval Development Duration'
                                     ' vs. Population', xlabel='Duration (d)', ylabel='Population',
                                     actual_avg=actual_larva_dev_dur, theory_avg=theoretical_larva_dev_dur,
                                     category="larval_development_duration")
        svet.plot_scatter_dist_w_avg(output_df['populations_m'],
                                     xaxis=output_df['durations_m'], title='Larval Development Duration Microsporidia'
                                     ' vs. Population', xlabel='Duration (d)', ylabel='Population',
                                     actual_avg=actual_larva_dev_dur_m, theory_avg=theoretical_larva_dev_dur_m,
                                     category="larval_development_duration_microsporidia")
        svet.plot_scatter_dist_w_avg(rate_list, title='Exponential Distribution modifying Arrhenius param',
                                     xaxis=a2_vector, xlabel="A2 param", category='a2_plot')
        tolerance = 5e-2 * theoretical_larva_dev_dur
        if abs(theoretical_larva_dev_dur - actual_larva_dev_dur) > tolerance:
            success = False
            outfile.write("BAD: The 1/the rate and the average of the exp dist larval "
                          "development duration were not within 5%.\n")
        else:
            outfile.write("GOOD: The 1/the rate and the average of the exp dist larval "
                          "development duration were within 5%.\n")
        tolerance = 5e-2 * theoretical_larva_dev_dur_m
        if abs(theoretical_larva_dev_dur_m - actual_larva_dev_dur_m) > tolerance:
            success = False
            outfile.write(f"BAD: With Microsporidia, The 1/the rate and the average {actual_larva_dev_dur_m}  "
                          f"of the exp dist larval "
                          f"development duration {theoretical_larva_dev_dur_m} were not within 5%.\n")
        else:
            outfile.write("GOOD: With Microsporidia, The 1/the rate and the average of the exp dist larval "
                          "development duration were within 5%.\n")
        outfile.write(sft.format_success_msg(success))
        if debug:
            print(param_obj)


def parse_output_file(stdout, debug=False):
    """
    Parses following lines, example:

    GetLarvalDevelopmentProgress returning 0.055726 based on temperature 38.327450, and growth modifier 0.289180.
    Hatching 3606 female eggs and pushing into larval queues (bits=0), id = 512.
    Immature adults emerging from larva queue: population = 1, genome bits = 0, id = 512, age = 0.

    Args:
        stdout: log file in which to look for the lines
        debug: flag, if True, a file with all the lines of interest is written out

    Returns: temperature of the simulation and a data frame of data
    """
    id_duration = {}
    filtered_lines = []
    time = 0.0
    sft.wait_for_done()
    durations = []
    durations_m = []
    populations = []
    populations_m = []
    population_duration = {}
    population_duration_m = {}
    temperatures = []
    temperature_k = 0
    with open(stdout) as logfile:
        for line in logfile:
            if "GetLarvalDevelopmentProgress" in line:
                temperature_k = float(sft.get_val("based on temperature ", line)) + 273.15
                temperatures.append(temperature_k)
                filtered_lines.append(line)
            # if ("larval queues" or "Update(): Time" or "GetLarvalDevelopmentProgress" or "Immature adults") in line:
            #     filtered_lines.append(line)
            if "Time:" in line:
                filtered_lines.append(line)
                time = float(sft.get_val("Time: ", line))
            if "larval queues" in line:
                filtered_lines.append(line)
                cohort_id = int(sft.get_val("id = ", line))
                id_duration[cohort_id] = time
                has_microsporidia = int(sft.get_val("has microsporidia ", line))
                if has_microsporidia:
                    population_duration_m[id_duration[cohort_id]] = 0
                else:
                    population_duration[id_duration[cohort_id]] = 0
            if "Immature adults" in line:
                filtered_lines.append(line)
                has_microsporidia = int(sft.get_val("has microsporidia ", line))
                cohort_id = int(sft.get_val("id = ", line))
                id_duration[cohort_id] = time - id_duration[cohort_id]
                population = int(sft.get_val("population = ", line))
                if has_microsporidia:
                    durations_m.append(id_duration[cohort_id])
                    populations_m.append(population)
                    population_duration_m[id_duration[cohort_id]] += population
                else:
                    durations.append(id_duration[cohort_id])
                    populations.append(population)
                    population_duration[id_duration[cohort_id]] += population
    if len(set(temperatures)) > 1:
        raise ValueError("There is more than one temperature!  This test is supposed to be at constant temperature.")
    d = {"populations": [value for value in population_duration.values() if value != 0],
         "durations": [float(key) for key in population_duration.keys() if population_duration[key] != 0],
         "populations_m": [value for value in population_duration_m.values() if value != 0],
         "durations_m": [float(key) for key in population_duration_m.keys() if population_duration_m[key] != 0]
         }
    output_df = pd.DataFrame(d)
    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as debug_file:
            debug_file.writelines(filtered_lines)
    return temperature_k, output_df


def application(output_folder="output",
                config_filename="config.json",
                report_name=sft.sft_output_filename,
                stdout="test.txt",
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    config_obj = veds.load_emod_parameters(config_filename)
    temperature_k, output_df = parse_output_file(stdout, debug)
    create_report_file(config_obj, report_name, temperature_k, output_df, debug)


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
