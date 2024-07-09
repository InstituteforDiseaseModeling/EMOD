#!/usr/bin/python

import numpy as np
import pandas as pd
import math
import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import dtk_test.dtk_sft_svetlana as svet

"""
Spec:
    The duration of egg development shall be a function of air temperature on the first day of its egg stage. 
    The function shall behave according to the following functional form: the Arrhenius equation, a1 * exp( -a2 / T ), 
    with T in degrees Kelvin. The 1/the rate calculated using this function signifies the average of an exponentially 
    distributed duration of the egg development, if Egg_Hatch_Delay_Distribution is set to EXPONENTIAL_DURATION(default).
    Relevant Parameters:
    Enable_Temperature_Dependent_Egg_Hatching: 1
    Egg_Hatch_Delay_Distribution: EXPONENTIAL_DURATION
    Egg_Arrhenius1 61599956.864
    Egg_Arrhenius2 5754.033

    Upon drought, i.e. when CC reduces to zero, eggs hatch at a reduced rate.
    Enable_Drought_Egg_Hatch_Delay: 1
    Drought_Egg_Hatch_Delay: 0.22

    Other relevant settings: 
    Enable_Egg_Mortality: 0 - egg mortality is disabled so the only decrease in egg population comes from hatching
    Egg_Hatch_Density_Dependence: "NO_DENSITY_DEPENDENCE" - only decrease in egg population comes from hatching
    Egg_Saturation_At_Oviposition: "NO_SATURATION" - only decrease in egg population comes from hatching

    Starting the hatching duration at day 1 because the distribution doesn't have the right average if the eggs with
    no duration are included, and eggs shouldn't have no duration anyway even though some might hatch right away.
    If they hatch on day 1, they took 1 day to hatch, etc. is how this is being interpreted.
    
    This test checks:
    1) The the egg hatch delay is calculated correctly based on temperature and the drought_egg_hatch_delay parameter
    2) the binomial 95% confidence test for each of the eggs hatch events passes 95% of the time, cohorts combined
        since the draws are independent and more data is better. 
    For each cohort: 
        1) ks test to verify the egg hatching time is a geometric distribution
        2) average duration of the hatching of the egg calculated by adding up all the hatch times 
            and dividing by number of eggs hatched is close to 1/(egg hatch delay) 
        3) all eggs hatch and therefore all eggs are used in the above calculations 
        4) all eggs hatch within a "reasonable" time, calculated at an ideal exponential decay having between 
            5 and 0.05 eggs left


    Failing one of these tests is still a successful pass. 
    
    Sweep Suggestions: Run_Number, Temperature (keep it constant), Drought_Egg_Hatch_Delay
    Please make sure the Simulation_Duration is long enough for all the eggs to hatch. They will hatch slower at 
    lower temperatures and smaller Drought_Egg_Hatch_Delay.
    
"""


def create_report_file(param_obj, report_name, drought_hatching_df, debug=False):
    with open(report_name, "w") as outfile:
        with open("failure_details.txt", "w") as test_details:
            success_list = []
            success = True
            binomial_test = []  # accumulating all the binomial tests results since they should be from the
            # same probability
            simulation_duration = param_obj[veds.ConfigKeys.SIMULATION_DURATION]
            outfile.write("Test name: " + param_obj[veds.ConfigKeys.CONFIG_NAME] + ", Run number: " +
                          str(param_obj[veds.ConfigKeys.RUN_NUMBER]) + "\n" +
                          "    This test checks: \n" +
                          "1) The the egg hatch delay is calculated correctly based on temperature and the "
                          "drought_egg_hatch_delay parameter\n"
                          "2) the binomial 95% confidence test for each of the eggs hatch events passes 95% of the "
                          "time, cohorts combined\n"
                          "    since the draws are independent and more data is better. \n"
                          "For each cohort:\n"
                          "  1) ks test to verify the egg hatching time is a geometric distribution\n"
                          "  2) average duration of the hatching of the egg calculated by adding up all the hatch"
                          "times\n"
                          "        and dividing by number of eggs hatched is close to 1/(egg hatch delay)\n"
                          "  3) all eggs hatch and therefore all eggs are used in the above calculations\n"
                          "  4) all eggs hatch within a \"reasonable\" time, calculated at an ideal exponential "
                          "decay having between\n"
                          "       5 and 0.05 eggs left.\n")
            if not len(drought_hatching_df["temp_k"].tolist()):
                success = False
                outfile.write(sft.sft_no_test_data)
            elif len(set(drought_hatching_df["temp_k"].tolist())) > 1:
                success = False
                outfile.write("BAD: There are several temperatures recorded, there should only be one constant"
                              " temperature. Please check log files. Temperatures present: \n")
                for temp in set(drought_hatching_df["temp_k"].tolist()):
                    outfile.write("{}\n".format(temp))
            elif len(set(drought_hatching_df["delay"].tolist())) > 1:
                success = False
                outfile.write("BAD: There is more than one egg hatch delay factor found. With constant temperature and"
                              " drought, there should only be one. Please check log files.\nDelays present:\n")
                for delay in set(drought_hatching_df["delay"].tolist()):
                    outfile.write("{}\n".format(delay))
            else:
                kelvin_temperature = float(drought_hatching_df.loc[1]["temp_k"])
                theoretical_delay_factor = veds.egg_dur_calc(kelvin_temperature, param_obj)
                actual_delay_factor = float(drought_hatching_df.loc[1]["delay"])
                five_percent_error = 0.05 * theoretical_delay_factor
                low_acceptance_bound = theoretical_delay_factor - five_percent_error
                high_acceptance_bound = theoretical_delay_factor + five_percent_error
                if abs(theoretical_delay_factor - actual_delay_factor) > five_percent_error:
                    success_list.append(False)
                    outfile.write(
                        "BAD: At temperature {}, actual_delay_factor={}, which is not within acceptance range "
                        "of ({}, {}).  Expected {}."
                        "\n".format(kelvin_temperature, actual_delay_factor, low_acceptance_bound,
                                    high_acceptance_bound, theoretical_delay_factor))
                else:
                    success_list.append(True)
                    outfile.write("GOOD: At temperature {}, actual_delay_factor={}, was within acceptance range "
                                  "of ({}, {}).  Expected {}."
                                  "\n".format(kelvin_temperature, actual_delay_factor, low_acceptance_bound,
                                              high_acceptance_bound, theoretical_delay_factor))
                cohort_ids = set(drought_hatching_df["cohort_id"].tolist())
                for cohort_id in cohort_ids:
                    cohort_df = drought_hatching_df[drought_hatching_df["cohort_id"] == cohort_id]
                    hatched_list = np.array(cohort_df["hatched"])
                    egg_pop_list = np.array(cohort_df["egg_pop"])
                    # verifying that the eggs hatch in a "reasonable time"
                    actual_time_for_eggs_to_hatch = len(egg_pop_list)
                    if egg_pop_list[actual_time_for_eggs_to_hatch - 1] == hatched_list[actual_time_for_eggs_to_hatch - 1]:
                        # verifying that the last entry in the df is the last egg hatching, which means
                        # they all hatch before the end of the sim
                        success_list.append(True)
                        outfile.write("GOOD: Last entree for hatching and egg population for cohort {} is the last "
                                      "egg hatching at time {}.\n".format(cohort_id, len(egg_pop_list)))

                        eggs_left_for_shortest = 5
                        eggs_left_for_longest = 0.05
                        initial_eggs = egg_pop_list[0]
                        theo_shortest_time = math.floor(veds.when_x_eggs_left(eggs_left_for_shortest, initial_eggs,
                                                                              theoretical_delay_factor))
                        theo_longest_time = math.ceil(veds.when_x_eggs_left(eggs_left_for_longest, initial_eggs,
                                                                            theoretical_delay_factor))
                        if theo_longest_time >= actual_time_for_eggs_to_hatch >= theo_shortest_time:
                            success_list.append(True)
                            outfile.write("GOOD: Eggs finished hatching between {} and {}, by {}."
                                          "\n".format(theo_shortest_time, theo_longest_time,
                                                      actual_time_for_eggs_to_hatch))
                        else:
                            success_list.append(False)
                            outfile.write("BAD: Eggs finished hatching not between {} and {}, by {}."
                                          "\n".format(theo_shortest_time,
                                                      theo_longest_time, actual_time_for_eggs_to_hatch))
                        distribution_under_test = svet.convert_binomial_chain_to_geometric(
                            [int(x) for x in cohort_df["hatched"].tolist()])
                        success_list.append(
                            svet.test_geometric_binomial(distribution_under_test, theoretical_delay_factor,
                                                         report_file=test_details, category="egg hatching"))
                        duration_list = hatched_list * np.array(range(1, len(hatched_list) + 1))
                        actual_average = float(sum(duration_list)) / sum(hatched_list)
                        theoretical_average = 1 / theoretical_delay_factor

                        dist_geo = sft.create_geometric_dis(theoretical_delay_factor, hatched_list[0],
                                                            len(hatched_list))
                        svet.plot_scatter_dist_w_avg(hatched_list, xaxis=range(len(hatched_list)), theory=dist_geo,
                                                     actual_avg=actual_average,
                                                     theory_avg=theoretical_average,
                                                     title="Eggs Left to Hatch Daily",
                                                     xlabel="Time (days)", ylabel="Number of Eggs Hatched",
                                                     category="plot_geo_{}.png".format(cohort_id))
                        outfile.write("The actual 1/the rate that the average of the exp dist should equal for cohort"
                                      " {}: {}\n".format(cohort_id, theoretical_average))
                        tolerance = 5e-2 * theoretical_average
                        if abs(theoretical_average - actual_average) > tolerance:
                            success_list.append(False)
                            outfile.write("BAD: The 1/the rate and the average of the exp dist egg "
                                          "hatch duration were not within 5% for cohort {}. "
                                          "Expected: {}, actual: {}.\n".format(cohort_id, theoretical_average,
                                                                               actual_average))
                        else:
                            success_list.append(True)
                            outfile.write("GOOD: The 1/the rate and the average of the exp dist egg "
                                          "hatch duration were within 5% for cohort {}. "
                                          "Expected: {}, actual: {}.\n".format(cohort_id, theoretical_average,
                                                                               actual_average))
                        for index, row in cohort_df.iterrows():
                            result, valid = svet.test_binomial_95ci(num_success=row["hatched"],
                                                                    num_trials=row["egg_pop"],
                                                                    prob=theoretical_delay_factor,
                                                                    report_file=test_details,
                                                                    category="is number of hatched eggs "
                                                                             "reasonable (cohort {})"
                                                                    .format(cohort_id))
                            if valid:
                                binomial_test.append(result)
                    else:
                        outfile.write(sft.format_success_msg(False))
                        outfile.write("BAD: For cohort {}, not all eggs have hatched, last entree has {} eggs left."
                                      "\n".format(cohort_id, egg_pop_list[len(egg_pop_list) - 1]))
                        outfile.write("Please re-run the simulation again with enough timesteps for eggs to hatch.\n")
                        return

                if sum(binomial_test) >= math.floor(0.95 * len(binomial_test)):
                    success_list.append(True)
                    outfile.write(
                        "GOOD: {} of {} binomial tests checking hatching binomial draw per time step passed. "
                        "Expected at least {} to pass.\n".format(sum(binomial_test), len(binomial_test),
                                                                 math.floor(0.95 * len(binomial_test))))
                else:
                    success_list.append(False)
                    outfile.write(
                        "BAD: Only, {} of {} binomial tests checking hatching binomial draw per time step passed. "
                        "We were expecting at least {} to pass. Please check errors in the logs."
                        "\n".format(sum(binomial_test), len(binomial_test),
                                    math.floor(0.95 * len(binomial_test))))
                successes = sum(success_list)
                checks = len(success_list)
                if successes == checks:
                    success = True
                    outfile.write("GOOD: All {} checks pass.\n".format(checks))
                elif successes == checks - 1 or successes == checks - 2:
                    success = True
                    outfile.write("OK: {} of {} checks pass.\n".format(successes, checks))
                elif successes < checks - 1:
                    success = False
                    outfile.write(
                        "BAD: Only {} of {} checks pass. Please check the failures.\n".format(successes, checks))

        outfile.write(sft.format_success_msg(success))

    if debug:
        print(param_obj)
        print(sft.format_success_msg(success))


def parse_output_file(stdout, debug=False):
    """
    parse comma separated name value pair and return relevant variables
    :param line: line of text to parse
    e.g. line = 00:00:00 [0] [V] [VectorPopulation] temperature = 27.362499, local density dependence modifier is
    1.000000, egg hatch delay factor is 0.297831, current population is 4632, hatched is 1379.
    :returns: temperature in Kelvin and egg hatch delay factor
    """
    filtered_lines = []
    temperature_k = []
    egg_hatch_delay = []
    egg_population = []
    eggs_hatched = []
    hatching_proportions = []
    egg_cohort_id = []
    with open(stdout) as logfile:
        for line in logfile:
            if "Time: " in line:
                filtered_lines.append(line)
                time = int(float(sft.get_val("Time: ", line)))
            elif "temperature =" in line:
                filtered_lines.append(line)
                kelvin_temperature = float(sft.get_val("temperature = ", line)) + 273.15
                egg_hatch_delay_factor = float(sft.get_val("egg hatch delay factor is ", line))
                hatched = float(sft.get_val("hatched is ", line))
                population = float(sft.get_val("current population is ", line))
                cohort_id = float(sft.get_val("id = ", line))
                hatched_proportion = float(hatched / population)
                temperature_k.append(kelvin_temperature)
                egg_hatch_delay.append(egg_hatch_delay_factor)
                egg_population.append(population)
                eggs_hatched.append(hatched)
                hatching_proportions.append(hatched_proportion)
                egg_cohort_id.append(cohort_id)
    drought_hatching_df = pd.DataFrame.from_dict({"temp_k": temperature_k, "cohort_id": egg_cohort_id,
                                                  "delay": egg_hatch_delay, "hatched": eggs_hatched,
                                                  "egg_pop": egg_population, "proportion": hatching_proportions})
    if debug:
        with open("DEBUG_filtered_lines.txt", 'w') as debug_file:
            debug_file.writelines(filtered_lines)
        drought_hatching_df.to_csv("DEBUG_drought_hatching.csv")

    return drought_hatching_df


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
    drought_hatching_df = parse_output_file(stdout, debug)
    create_report_file(config_obj, report_name, drought_hatching_df, debug)


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
