#!/usr/bin/python

import json
import dtk_test.dtk_sft as dtk_sft
import math
import numpy as np
import pandas as pd

"""
Support library for the vector life expectancy tests
"""

KEY_CONFIG_NAME = "Config_Name"
VECTOR_SPECIES_PARAMS = "Vector_Species_Params"
ADULT_LIFE_EXPECTANCY = "Adult_Life_Expectancy"
MALE_LIFE_EXPECTANCY = "Male_Life_Expectancy"
AIR_TEMPERATURE = "Base_Air_Temperature"
SIM_DURATION = "Simulation_Duration"
ANTHROPOPHILY = "Anthropophily"
INFECTIOUS_MORTALITY = "Infectious_Human_Feed_Mortality_Factor"
HUMAN_FEED_MORTALITY = "Human_Feeding_Mortality"
DAY_BETWEEN_FEEDS = "Days_Between_Feeds"
MICROSPORIDIA = "Microsporidia"
STRAIN_NAME = "Strain_Name"
STRAIN_A = "Strain_A"
MICROSPORIDIA_MALE_MORTALITY_MODIFIER = "Male_Mortality_Modifier"
MICROSPORIDIA_FEMALE_MORTALITY_MODIFIER = "Female_Mortality_Modifier"
MALES = "males"
FEMALES = "females"


def mortality_from_age(age):
    # this is taken directly from Eradication.exe code
    # VectorPopulation.cpp function: inline static float mortalityFromAge
    e = np.exp(0.2 * age)
    return 0.006 * e / (1.0 + (0.0449999981 * (e - 1)))


def dry_heat_mortality(temperature):
    # this is taken directly from Eradication.exe code
    # VectorPopulation.cpp function:  inline static float dryHeatMortality
    return .001 * np.exp(temperature - 32.0)


def local_mortality(adult_life_expectancy, age, temperature, dt=1, wolb_free=1, microsporidia_modifier=1):
    # this formula is taken from the Eradication.exe code
    # VectorPopulation.cpp function: void VectorPopulation::UpdateLocalMatureMortalityProbabilityTable
    if age == 0:
        from_age = 0
    else:
        from_age = mortality_from_age(age)
    local_adult_mortality = 1 / (adult_life_expectancy + dry_heat_mortality(temperature))
    return 1 - (float(np.exp(-dt * (local_adult_mortality + from_age) * wolb_free) * microsporidia_modifier))


def check_all_vectors_died(dataframe, sex, outfile, debug=False):
    success = True
    initial_population = dataframe["population_before_deaths"].tolist()[0]
    total_deaths = dataframe["dead_without_feed"].sum()
    if total_deaths != initial_population:
        success = False
        if total_deaths > initial_population:
            if debug:
                outfile.write(f"BAD: There are more deaths than there was initial population. This means this cohort was "
                              f"merged with another cohort that turned 60.\n")
        elif debug:
            outfile.write(f"BAD: Not all {sex} vectors tracked died, total initial population before deaths = "
                          f"{initial_population}, total deaths = {total_deaths}, therefore the data "
                          f"cannot be used to calculate average age of death.\n")
    else:
        if debug:
            outfile.write(f"GOOD: All {sex} vectors tracked died, total initial population before deaths = "
                          f"{initial_population}, total deaths = {total_deaths}.\n")
    return success


def daily_mortality_check_average_lifespan_calculation(dataframe, life_expectancy,
                                                       temperature, outfile,
                                                       aging=True,
                                                       mortality_modifier=1):
    """
        Checks that daily mortality is within 95% confidence using the binomial tests, then calculates average lifepan
        for the observed deaths.
    Args:
        dataframe: dataframe of vector deaths cleaned for only fully died out cohorts
        life_expectancy: Life_Expectancy setting from the config.json
        temperature: temperature for dry heat deaths calculation
        outfile: log file for test results
        aging: Enable_Vector_Aging setting in the config
        mortality_modifier: mortality modifier due to microsporidia

    Returns: list containing, whether the test passed (True/False) and Average lifespan of vectors

    """
    rows = dataframe.shape[0]
    mortality_test = []
    total_life = 0
    for row in range(rows):
        mortality_test.append(local_test_binomial_95ci(dataframe["dead_without_feed"].iloc[row],
                                                       dataframe["population_before_deaths"].iloc[row],
                                                       local_mortality(life_expectancy,
                                                                       dataframe["age"].iloc[row] if aging else 0,
                                                                       temperature, mortality_modifier), outfile, "vector deaths"))
        total_life += dataframe["dead_without_feed"].iloc[row] * dataframe["age"].iloc[row]

    successes = mortality_test.count(True)
    failures = mortality_test.count(False)
    total_deaths = dataframe["dead_without_feed"].sum()
    return successes, failures,  total_life / total_deaths


def brute_force_theoretical_lifespan(initial_population, life_expectancy, temperature,
                                     aging=True, mortality_modifier=1):
    """
        This function calculated theoretical life span of of a vector by going step by step and
    calculating theoretical deaths due to life_expetancy, age, and dry heat for every step, summarizing them
    and finding the average
    Args:
        initial_population: population of a cohort before any deaths occur
        life_expectancy: Life_Expectancy setting from the config.json
        temperature: temperature for dry heat deaths calculation
        aging: Enable_Vector_Aging setting in the config
        mortality_modifier: mortality modifier due to microsporidia

    Returns: Calculated theoretical lifespan based on factors given

    """
    theoretical_total_deaths = 0
    theoretical_total_life = 0
    theoretical_population_before_death = initial_population
    for age in range(1, 2000):
        mortality = local_mortality(life_expectancy, age if aging else 0, temperature, mortality_modifier)
        theoretical_deaths = theoretical_population_before_death * mortality
        theoretical_total_deaths += theoretical_deaths
        theoretical_population_before_death = theoretical_population_before_death - theoretical_deaths
        theoretical_total_life += theoretical_deaths * age
    return theoretical_total_life / theoretical_total_deaths


def local_test_binomial_95ci(num_success, num_trials, prob, report_file, category):
    """
    This function test if a binomial distribution falls within the 95% confidence interval
        returns "not_enough_data" if the numbers are too small
    Args:
        num_success: the number of successes
        num_trials: the number of trial
        prob: the probability of success for one trial
        report_file: for error reporting
        category:

    Returns:
    True, False, "not_enough_data"
    """

    # calculate the mean and  standard deviation for binomial distribution
    mean = num_trials * prob
    standard_deviation = math.sqrt(prob * (1 - prob) * num_trials)
    # 95% confidence interval
    lower_bound = mean - 2 * standard_deviation
    upper_bound = mean + 2 * standard_deviation
    success = True
    if mean < 5 or num_trials * (1 - prob) < 5:
        # The general rule of thumb for normal approximation method is that
        # the sample size n is "sufficiently large" if np >= 5 and n(1-p) >= 5
        # for cases that binomial confidence interval will not work
        success = "not_enough_data"
        # report_file.write("There is not enough sample size in group {0}: mean = {1}, sample size - mean = {2}.
        # \n".format(category, mean,num_trials * (1 - prob)))
    elif num_success < lower_bound or num_success > upper_bound:
        success = False
        # report_file.write("BAD: For category {0}, the success cases is {1}, expected 95% confidence interval ( {2},"
        #                   " {3}).\n".format(category, num_success,lower_bound, upper_bound))
    return success


def load_emod_parameters(config_filename="config.json", debug=False):
    """
        Function loads relevant parameters from the config file and returns them as a dictionary
    Args:
        config_filename:  config file name (usually config.json)
        debug:  Flag, if true - prints out the dictionary object being returned

    Returns:
        Dictionary with parameters of interest
    """

    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {}
    param_obj[ADULT_LIFE_EXPECTANCY] = cdj[VECTOR_SPECIES_PARAMS][0][ADULT_LIFE_EXPECTANCY]
    param_obj[MALE_LIFE_EXPECTANCY] = cdj[VECTOR_SPECIES_PARAMS][0][MALE_LIFE_EXPECTANCY]
    param_obj[ANTHROPOPHILY] = cdj[VECTOR_SPECIES_PARAMS][0][ANTHROPOPHILY]
    param_obj[DAY_BETWEEN_FEEDS] = cdj[VECTOR_SPECIES_PARAMS][0][DAY_BETWEEN_FEEDS]
    param_obj[KEY_CONFIG_NAME] = cdj[KEY_CONFIG_NAME]
    param_obj[AIR_TEMPERATURE] = cdj[AIR_TEMPERATURE]
    param_obj[SIM_DURATION] = cdj[SIM_DURATION]
    param_obj[HUMAN_FEED_MORTALITY] = cdj[HUMAN_FEED_MORTALITY]
    param_obj[INFECTIOUS_MORTALITY] = cdj[VECTOR_SPECIES_PARAMS][0][INFECTIOUS_MORTALITY]

    param_obj[MICROSPORIDIA_MALE_MORTALITY_MODIFIER] = 1
    param_obj[MICROSPORIDIA_FEMALE_MORTALITY_MODIFIER] = 1
    if len(cdj[VECTOR_SPECIES_PARAMS][0][MICROSPORIDIA]) == 1:
        param_obj[MICROSPORIDIA_MALE_MORTALITY_MODIFIER] = cdj[VECTOR_SPECIES_PARAMS][0][MICROSPORIDIA][0][MICROSPORIDIA_MALE_MORTALITY_MODIFIER]
        param_obj[MICROSPORIDIA_FEMALE_MORTALITY_MODIFIER] = cdj[VECTOR_SPECIES_PARAMS][0][MICROSPORIDIA][0][MICROSPORIDIA_FEMALE_MORTALITY_MODIFIER]

    if debug:
        with open("DEBUG_param_object.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj


def parse_output_file(output_filename="test.txt", debug=False):
    """
        Creates a dataframe to store filtered information for each time step
    Args:
        output_filename: file to parse (test.txt)
        debug: write out parsed data on true

    Returns:
         Dataframe with the relevant data
    """

    time = "Time: "
    timestep = 0
    female_cohort = "Update Female Deaths"
    male_cohort = "Update Male Deaths"
    key_age = "age="
    key_died_before_feeding = "died_before_feeding="
    key_population_before_deaths = "population_before_deaths="
    key_cohort_id = "cohort_id="
    key_male_deaths = "died="
    key_feed_death = "died_feed_related="
    key_microsporidia = "microsporidia="
    vector_deaths_lists = []
    with open(output_filename) as logfile:
        with open("debug_filtered_lines.txt", "w") as outfile:
            for line in logfile:
                if time in line:
                    if debug:
                        outfile.write(line)
                    timestep = float(dtk_sft.get_val(time, line))
                if female_cohort in line:
                    if debug:
                        outfile.write(line)
                    microsporidia = int(dtk_sft.get_val(key_microsporidia, line))
                    age = float(dtk_sft.get_val(key_age, line))
                    dead_before_feeding = int(dtk_sft.get_val(key_died_before_feeding, line))
                    cohort_id = int(dtk_sft.get_val(key_cohort_id, line))
                    population_before_deaths = int(dtk_sft.get_val(key_population_before_deaths, line))
                    dead_from_feed = int(dtk_sft.get_val(key_feed_death, line))
                    vector_deaths_lists.append([timestep, "female", cohort_id, microsporidia, age,
                                                population_before_deaths,
                                                dead_before_feeding, dead_from_feed])

                elif male_cohort in line:
                    if debug:
                        outfile.write(line)
                    microsporidia = int(dtk_sft.get_val(key_microsporidia, line))
                    age = float(dtk_sft.get_val(key_age, line))
                    dead = int(dtk_sft.get_val(key_male_deaths, line))
                    population_before_deaths = int(dtk_sft.get_val(key_population_before_deaths, line))
                    cohort_id = int(dtk_sft.get_val(key_cohort_id, line))
                    vector_deaths_lists.append([timestep, "male", cohort_id, microsporidia,
                                                age, population_before_deaths, dead, 0])

    vector_deaths_df = pd.DataFrame(vector_deaths_lists, columns=["timestep", "sex", "cohort_id", "microsporidia",
                                                                  "age",
                                                                  "population_before_deaths", "dead_without_feed",
                                                                  "dead_from_feed"])
    if debug:
        vector_deaths_df.to_csv("vector_deaths_df.csv")

    return vector_deaths_df


def check_results(mortality_test_results, sex, success_rate, outfile):
    total_successes = 0
    total_failures = 0
    success = True
    for result in mortality_test_results:
        total_successes += result[0]
        total_failures += result[1]

    total_valid_tries = total_failures + total_successes
    if not total_valid_tries:
        success = False
        outfile.write(f"BAD: For {sex} cohorts, there are no valid data to check data.\n")
    elif total_successes / total_valid_tries < success_rate:
        success = False
        outfile.write(f"BAD: For all {sex} cohorts out of {total_valid_tries} valid tests, "
                      f"only {total_successes}"
                      f" passed. That's {total_successes / total_valid_tries} "
                      f"success rate, "
                      f"which is lower than our desired success rate of {success_rate}.\n")
    else:
        outfile.write(f"GOOD: For all {sex} out of {total_valid_tries} valid tests, "
                      f"{total_successes}"
                      f" passed. That's {total_successes / total_valid_tries} "
                      f"success rate, "
                      f"which is higher than our desired success rate of {success_rate}.\n")

    return success
