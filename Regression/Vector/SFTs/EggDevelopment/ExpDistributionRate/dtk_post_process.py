#!/usr/bin/python

import numpy as np
import math
import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import dtk_test.dtk_sft_svetlana as svet

"""
Spect:
     The duration of egg development shall be a function of air temperature on the first day of its egg stage. 
    The function shall behave according to the following functional form: the Arrhenius equation, a1 * exp( -a2 / T ), 
    with T in degrees Kelvin. The 1/the rate calculated using this function signifies the average of an exponentially 
    distributed duration of the egg development, if Egg_Hatch_Delay_Distribution is set to EXPONENTIAL_DURATION (default).
    Relevant Parameters:
    Enable_Temperature_Dependent_Egg_Hatching: 1
    Egg_Hatch_Delay_Distribution: EXPONENTIAL_DURATION
    Egg_Arrhenius1 61599956.864
    Egg_Arrhenius2 5754.033
    
    Tests if the duration of egg development follows an exponential distribution.
"""


def create_report_file(param_obj, report_name, kelvin_temperature, egg_hatch_delay_factor, result_dict, debug=False):
    with open(report_name, "w") as outfile:
        success_list = []
        outfile.write("# Test name: " + param_obj[veds.ConfigKeys.CONFIG_NAME] + ", Run number: " +
                      str(param_obj[veds.ConfigKeys.RUN_NUMBER]) +
                      "\n# Test"
                      " compares between the actual_delay_factor and expected egg hatch delay factors to verify "
                      "eggs hatch with an exponential (geometric) distribution,\n# and the mean "
                      "of that distribution is"
                      " equal to 1/the Arrhenius rate at that temperature.\n")
        if kelvin_temperature is None:
            outfile.write(sft.sft_no_test_data)
            raise ValueError("No data found!")
        theoretical_delay_factor = veds.egg_dur_calc(kelvin_temperature, param_obj)
        actual_delay_factor = egg_hatch_delay_factor
        if abs(theoretical_delay_factor-actual_delay_factor) > 1e-3:
            success_list.append(False)
            low_acceptance_bound = theoretical_delay_factor - 1e-3
            high_acceptance_bound = theoretical_delay_factor + 1e-3
            outfile.write(f"BAD: At temperature {kelvin_temperature}, actual_delay_factor={actual_delay_factor}, "
                          f" which is not within acceptance range of ({low_acceptance_bound}, "
                          f"{high_acceptance_bound}).  Expected {theoretical_delay_factor}.\n")
        else:
            success_list.append(True)
            outfile.write(f"GOOD: For temperature {kelvin_temperature}, egg hatch delay factor was within 0.001 of "
                          "theoretical.\n")
        for cohort_id in result_dict.keys():
            hatched_list = result_dict[cohort_id][0]
            ind_list = result_dict[cohort_id][1]
            duration_list = result_dict[cohort_id][2]
            x = np.array(ind_list)
            y = np.array(hatched_list)
            actual_mean_duration = np.mean(duration_list)
            outfile.write(f"The average of the exponential distribution of egg development duration for cohort"
                          f" {cohort_id}: "
                          f"{actual_mean_duration}\n")
            theoretical_duration = 1 / theoretical_delay_factor
            # Now testing to see whether or not an exponential distribution is followed
            a2 = param_obj[veds.ConfigKeys.EGG_ARRHENIUS2]
            lower_a2 = a2 / 2
            higher_a2 = a2 * 2
            a2_vector = np.linspace(lower_a2, higher_a2, 100)
            rate_list = []
            for a2 in a2_vector:
                rate = param_obj[veds.ConfigKeys.EGG_ARRHENIUS1] * math.exp(-(a2) / (kelvin_temperature))
                rate_list.append(rate)
            if not svet.is_exponential_behavior(rate_list, outfile, "egg development"):
                success_list.append(False)
            else:
                success_list.append(True)
            svet.plot_scatter_dist_w_avg(rate_list, title='Exponential Distribution modifying Arrhenius param',
                                         xaxis=a2_vector, xlabel="A2 param", category='a2_plot')
            tolerance = 5e-2 * theoretical_duration
            if abs(theoretical_duration - actual_mean_duration) > tolerance:
                success_list.append(False)
                outfile.write(f"BAD: The 1/the rate and the average of the exp dist egg "
                              f"hatch duration - {actual_mean_duration} were not within 5% of"
                              f" {theoretical_duration} for cohort {cohort_id}.\n")
            else:
                success_list.append(True)
                outfile.write(f"GOOD: The 1/the rate and the average of the exp dist egg "
                              f"hatch duration - {actual_mean_duration} were within 5% of "
                              f"{theoretical_duration} for cohort {cohort_id}.\n")

        successes = sum(success_list)
        checks = len(success_list)
        if successes == checks:
            success = True
            outfile.write("GOOD: All {} checks pass.\n".format(checks))
        elif successes == checks - 1:
            success = True
            outfile.write("OK: {} of {} checks pass. Still good.\n".format(successes, checks))
        else:
            success = False
            outfile.write("BAD: Only {} of {} passed. Please check above for details."
                          "\n".format(successes, checks))
        outfile.write(sft.format_success_msg(success))

    if debug:
        print(param_obj)
        print(sft.format_success_msg(success))


def parse_line(line):
    """
    parse comma separated name value pair and return relevant variables
    :param line: line of text to parse
    e.g. line = 00:00:00 [0] [V] [VectorPopulation] temperature = 27.362499, local density dependence modifier is
    1.000000, egg hatch delay factor is 0.297831, current population is 4632, hatched is 1379.
    :returns: temperature in Kelvin and egg hatch delay factor
    """
    kelvin_temperature = float(sft.get_val("temperature = ", line)) + 273.15
    egg_hatch_delay_factor = float(sft.get_val("egg hatch delay factor is ", line))
    hatched = float(sft.get_val("hatched is ", line))
    population = float(sft.get_val("current population is ", line))
    id = float(sft.get_val("id = ", line))
    return kelvin_temperature, egg_hatch_delay_factor, hatched, population, id


def parse_output_file(debug, stdout):
    filtered_lines = []
    with open(stdout) as logfile:
        for line in logfile:
            if "temperature =" in line:
                filtered_lines.append(line)
        if debug:
            with open("DEBUG_filtered_lines.txt", "w") as outfile:
                outfile.writelines(filtered_lines)
    id_dict = {}
    result_dict = {}
    egg_hatch_delay_factors = []
    # In the original branch where this was coded there are multiple egg cohorts so this test is going to keep track
    # of each cohort with a unique ID.  This might not end up being used in master, but should work for any number
    # of egg cohorts defined in the code.  Right now Dengue-Ongoing only has one egg cohort at a time so this test
    # has been revised to fit the VectorGenetics2 branch.

    # Starting the hatching duration at day 1 because the distribution doesn't have the right average if the eggs with
    # no duration are included, and eggs shouldn't have no duration anyway even though some might hatch right away.
    # If they hatch on day 1, they took 1 day to hatch, etc. is how this is being interpreted.

    # We want to filter by id first.  Then we want to go through everything.
    for line in range(0, len(filtered_lines)):
        kelvin_temperature, egg_hatch_delay_factor, hatched, population, id = parse_line(filtered_lines[line])
        if id not in id_dict.keys():
            id_dict[id] = []
        id_dict[id].append(filtered_lines[line])
    # Going through each cohort with unique ID.
    for identity in id_dict.keys():
        duration_list = []
        hatched_list = []
        ind_list = []
        kelvin_temperature = 0
        egg_hatch_delay_factor = 0
        for line_number in range(0, len(id_dict[identity])):
            kelvin_temperature, egg_hatch_delay_factor, hatched, population, id = parse_line(id_dict[identity][line_number])
            hatched_list.append(hatched)
            ind_list.append(line_number)
            duration_list += [line_number + 1] * int(hatched_list[line_number])  # correcting for eggs with no duration
            egg_hatch_delay_factors.append(egg_hatch_delay_factor)
        result_dict[identity] = (hatched_list, ind_list, duration_list)
    if len(set(egg_hatch_delay_factors)) is not 1:
        factors = ""
        for factor in set(egg_hatch_delay_factors):
            factors += str(factor) + " "
        raise ValueError("The egg hatch delay factor is not constant.  There should only be 1 "
                         "temperature and thus 1 egg hatch delay factor. Delay factors are:"
                         "\n {}".format(factors))

    try:
        return kelvin_temperature, egg_hatch_delay_factor, result_dict
    except NameError:
        raise ValueError("You have missing data in the stdout file!")


def application(output_folder="output",
                config_filename="config.json",
                stdout="test.txt",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("stdout: " + stdout + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    config_obj = veds.load_emod_parameters(config_filename)
    kelvin_temperature, egg_hatch_delay_factor, result_dict = parse_output_file(debug, stdout)
    create_report_file(config_obj, report_name, kelvin_temperature, egg_hatch_delay_factor, result_dict, debug)


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
                stdout=args.stdout,
                report_name=args.reportname, debug=args.debug)
