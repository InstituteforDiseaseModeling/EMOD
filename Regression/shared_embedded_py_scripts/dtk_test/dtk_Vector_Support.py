#!/usr/bin/python
from __future__ import division
import math
import json
import dtk_test.dtk_sft as sft
import pandas as pd
import copy
import time

vector_species_params = ["Anthropophily", "Egg_Survival_Rate", "Cycle_Arrhenius_1", "Cycle_Arrhenius_2",
                         "Aquatic_Mortality_Rate", "Aquatic_Arrhenius_1", "Aquatic_Arrhenius_2",
                         "Infected_Arrhenius_1", "Infected_Arrhenius_2" ]


class ConfigKeys:
    CONFIG_NAME = "Config_Name"
    RUN_NUMBER = "Run_Number"
    EGG_HATCH_DELAY_DISTRIBUTION = "Egg_Hatch_Delay_Distribution"
    EGG_ARRHENIUS1 = "Egg_Arrhenius1"
    EGG_ARRHENIUS2 = "Egg_Arrhenius2"
    LAND_TEMPERATURE = "Base_Land_Temperature"
    AIR_TEMPERATURE = "Base_Air_Temperature"
    DROUGHT_EGG_HATCH_DELAY = "Drought_Egg_Hatch_Delay"
    VECTOR_SPECIES_PARAMS = "Vector_Species_Params"
    SIMULATION_DURATION = "Simulation_Duration"

    class VectorSpeciesParams:
        ANTHROPOPHILY = "Anthropophily"
        EGG_SURVIVAL_RATE = "Egg_Survival_Rate"
        CYCLE_ARRHENIUS_1 = "Cycle_Arrhenius_1"
        CYCLE_ARRHENIUS_2 = "Cycle_Arrhenius_2"
        AQUATIC_MORTALITY_RATE = "Aquatic_Mortality_Rate"
        AQUATIC_ARRHENIUS_1 = "Aquatic_Arrhenius_1"
        AQUATIC_ARRHENIUS_2 = "Aquatic_Arrhenius_2"
        INFECTED_ARRHENIUS_1 = "Infected_Arrhenius_1"
        INFECTED_ARRHENIUS_2 = "Infected_Arrhenius_2"


class Constants:
    CONVERT_C_TO_K = 273.15


def when_x_eggs_left(eggs_left, initial_eggs, egg_hatch_delay):
    return math.log(eggs_left / initial_eggs) / math.log(1 - egg_hatch_delay)


def arrhenius_calc(temperature_k, arrhenius_1, arrhenius_2):
    return arrhenius_1 * math.exp(-arrhenius_2 / temperature_k)


def egg_dur_calc(temperature_k, param_obj):
    rate = arrhenius_calc(temperature_k, param_obj[ConfigKeys.EGG_ARRHENIUS1], param_obj[ConfigKeys.EGG_ARRHENIUS2])
    rate = rate * param_obj[ConfigKeys.DROUGHT_EGG_HATCH_DELAY]
    return rate


def vector_dur_calc(temperature_k, param_obj):
    return arrhenius_calc(temperature_k, param_obj[ConfigKeys.VectorSpeciesParams.CYCLE_ARRHENIUS_1],
                          param_obj[ConfigKeys.VectorSpeciesParams.CYCLE_ARRHENIUS_2])


def larval_dur_calc(temperature_k, param_obj):
    return arrhenius_calc(temperature_k, param_obj[ConfigKeys.VectorSpeciesParams.AQUATIC_ARRHENIUS_1],
                          param_obj[ConfigKeys.VectorSpeciesParams.AQUATIC_ARRHENIUS_2])


def infected_progress_calc(temperature_k, param_obj):
    return arrhenius_calc(temperature_k, param_obj[ConfigKeys.VectorSpeciesParams.INFECTED_ARRHENIUS_1],
                          param_obj[ConfigKeys.VectorSpeciesParams.INFECTED_ARRHENIUS_2])


def parse_for_feeding_choice(stdout, debug=False):
    """
    Parses following lines, example:

    looking_to_feed = 2969 , died_before_feeding=0, successful_animal_feed=146 , attempt_feed_indoor=0 , attempt_feed_outdoor=1434

    Args:
        stdout: the logging file to parse
        debug: flag, if true writes out the lines that were used to collect the data

    Returns: data frame

    """
    d = {}
    looking_to_feed = []
    died_before_feeding = []
    successful_animal_feed = []
    attempt_feed_indoor = []
    attempt_feed_outdoor = []
    times = []
    time = 0
    with open(stdout) as logfile:
        for line in logfile:
            if "Time: " in line:
                time = int(float(sft.get_val("Time: ", line)))
            elif "looking_to_feed" in line:
                looking_to_feed.append(int(sft.get_val("looking_to_feed=", line)))
                died_before_feeding.append(int(sft.get_val("died_before_feeding=", line)))
                successful_animal_feed.append(int(sft.get_val("successful_animal_feed=", line)))
                attempt_feed_indoor.append(int(sft.get_val("attempt_feed_indoor=", line)))
                attempt_feed_outdoor.append(int(sft.get_val("attempt_feed_outdoor=", line)))
                times.append(time)
    d["looking_to_feed"] = looking_to_feed
    d["died_before_feeding"] = died_before_feeding
    d["successful_animal_feed"] = successful_animal_feed
    d["attempt_feed_indoor"] = attempt_feed_indoor
    d["attempt_feed_outdoor"] = attempt_feed_outdoor
    d["time"] = times
    totals = {"looking_to_feed": sum(looking_to_feed),
              "died_before_feeding": sum(died_before_feeding),
              "successful_animal_feed": sum(successful_animal_feed),
              "attempt_feed_indoor": sum(attempt_feed_indoor),
              "attempt_feed_outdoor": sum(attempt_feed_outdoor)}

    if totals["looking_to_feed"] == 0 or debug:
        pd.DataFrame.from_dict(d).to_csv("DEBUG.csv")

    return totals


def load_emod_parameters(config_filename):
    param_obj = {}
    cdj = json.loads(open(config_filename).read())["parameters"]
    try:
        param_obj[ConfigKeys.EGG_ARRHENIUS1] = cdj[ConfigKeys.EGG_ARRHENIUS1]
        param_obj[ConfigKeys.EGG_ARRHENIUS2] = cdj[ConfigKeys.EGG_ARRHENIUS2]
    except:
        print("No Egg_Arrhenius parameters in this sim.\n")
    finally:
        pass
    param_obj[ConfigKeys.SIMULATION_DURATION] = cdj[ConfigKeys.SIMULATION_DURATION]
    param_obj[ConfigKeys.CONFIG_NAME] = cdj[ConfigKeys.CONFIG_NAME]
    param_obj[ConfigKeys.RUN_NUMBER] = cdj[ConfigKeys.RUN_NUMBER]
    param_obj[ConfigKeys.EGG_HATCH_DELAY_DISTRIBUTION] = cdj[ConfigKeys.EGG_HATCH_DELAY_DISTRIBUTION]
    try:
        param_obj[ConfigKeys.DROUGHT_EGG_HATCH_DELAY] = cdj[ConfigKeys.DROUGHT_EGG_HATCH_DELAY]
    except:
        print("KeyError: Drought_Egg_Hatch_Delay key not found.\n")
    finally:
        pass
    try:
        num_species = len(cdj[ConfigKeys.VECTOR_SPECIES_PARAMS])
    except KeyError:
        num_species = 0
        print("KeyError: Vector_Species_Params key not found.\n")
    finally:
        pass
    if num_species != 0:
        if num_species > 1:
            print("Multiple entries found in Vector_Species_Params, getting the first entree.\n")
        first_entree = cdj[ConfigKeys.VECTOR_SPECIES_PARAMS][0]

        for param in vector_species_params:
            if param in first_entree:
                param_obj[param] = first_entree[param]

        param_obj["Larval_Growth_Modifier"] = 1
        if len(first_entree["Microsporidia"]) == 1:
            param_obj["Larval_Growth_Modifier"] = first_entree["Microsporidia"][0]["Larval_Growth_Modifier"]
    else:
        print("No entries found in Vector_Species_Params.\n")

    return param_obj


def track_bites(stdout, start_time=None, end_time=None, by_day=False, debug=False):
    """
    Parses following lines, example:

    Created human id 4 with age=8000.000000
    Vector 76 bit person 2 of age

    Args:
        stdout: the logging file to parse
        debug: flag, if true writes out the lines that were used to collect the data

    Returns: two dictionaries, humans with their ids and ages, and accumulated bites by their ids.

    """
    time = 0
    humans = {}
    human_bites = {}
    bites_by_day = {}
    cumulative_human_bites = {}
    with open(stdout) as logfile:
        for line in logfile:
            if "Time: " in line:
                bites_by_day[time] = copy.deepcopy(human_bites)
                for human in human_bites:
                    # accumulate
                    if start_time and end_time:
                        if start_time <= time <= end_time:
                            if human in cumulative_human_bites:
                                cumulative_human_bites[human] += human_bites[human]
                            else:
                                cumulative_human_bites[human] = human_bites[human]
                    elif start_time:
                        if start_time <= time:
                            if human in cumulative_human_bites:
                                cumulative_human_bites[human] += human_bites[human]
                            else:
                                cumulative_human_bites[human] = human_bites[human]
                    elif end_time:
                        if end_time >= time:
                            if human in cumulative_human_bites:
                                cumulative_human_bites[human] += human_bites[human]
                            else:
                                cumulative_human_bites[human] = human_bites[human]
                    else:
                        if human in cumulative_human_bites:
                            cumulative_human_bites[human] += human_bites[human]
                        else:
                            cumulative_human_bites[human] = human_bites[human]
                    # reset
                    human_bites[human] = 0
                time = int(float(sft.get_val("Time: ", line)))
            if "Created human id" in line:
                human_id = int(sft.get_val("id ", line))
                age = float(sft.get_val("age=", line))
                humans[human_id] = age
            if "bit person" in line:
                human_bitten = int(sft.get_val("person ", line))
                if human_bitten in human_bites:
                    human_bites[human_bitten] += 1
                else:
                    human_bites[human_bitten] = 1

    if debug:
        all_data = {"daily_human_bites": bites_by_day,
                    "humans": humans,
                    "cumulative_human_bites": cumulative_human_bites}

        with open('biting_data_DEBUG.json', 'w') as fp:
            json.dump(all_data, fp)

    return humans, cumulative_human_bites, bites_by_day


def bite_proportions(humans, human_bites, bites_by_day, with_intervention_proportion=None,
                     without_intervention_proportion=None,
                     expected_people_with_intervention=None, expected_people_without_intervention=None,
                     report=None, x_error=1, debug=False):
    success = True
    error = 0.035 * x_error
    percent_biting_as_part_of_whole = {}
    total = 0
    with_intervention = 0
    without = 0
    for human_id in human_bites:
        total += human_bites[human_id]
    for human_id in human_bites:
        portion = human_bites[human_id] / total
        percent_biting_as_part_of_whole[human_id] = portion
        if abs(portion - with_intervention_proportion) < error:
            with_intervention += 1
        elif abs(portion - without_intervention_proportion) < error:
            without += 1
    #
    biting_data_filename = "biting_data.json"
    # expect to have 2 without net and one with net
    with open(report, "a") as outfile:
        if without != expected_people_without_intervention or with_intervention != expected_people_with_intervention:
            success = False
            outfile.write(
                f"BAD: We were expecting {expected_people_with_intervention} person(s) with intervention and "
                f"{expected_people_without_intervention} person(s) without with their proportions"
                f" being {with_intervention_proportion} and {without_intervention_proportion} respectively.\n"
                f"We got {with_intervention} and {without} respectively. Please check {biting_data_filename} .\n")
        else:
            outfile.write(f"GOOD: We're getting the expected biting proportions, for the {without + with_intervention} "
                          f"people being bitten.\n")

    if debug or not success:
        all_data = {"humans": humans,
                    "human_bites": human_bites,
                    "biting_percents": percent_biting_as_part_of_whole,
                    "daily_human_bites": bites_by_day}
        with open(biting_data_filename, 'w') as fp:
            json.dump(all_data, fp)

    return success


def create_biting_report_file(param_obj, report_name, humans, human_bites, bites_by_day,
                              with_intervention_proportion=None,
                              without_intervention_proportion=None,
                              expected_people_with_intervention=None,
                              expected_people_without_intervention=None, x_error=1, debug=False):
    success = True
    with open(report_name, "a") as outfile:
        outfile.write(f"Test name: {str(param_obj[ConfigKeys.CONFIG_NAME])} \n"
                      f"Run number: {str(param_obj[ConfigKeys.RUN_NUMBER])} \n")
        if not len(human_bites):
            outfile.write("BAD: no human bites data.\n")
            success = False

    if success:
        success = bite_proportions(humans=humans, human_bites=human_bites, bites_by_day=bites_by_day,
                                   with_intervention_proportion=with_intervention_proportion,
                                   without_intervention_proportion=without_intervention_proportion,
                                   expected_people_with_intervention=expected_people_with_intervention,
                                   expected_people_without_intervention=expected_people_without_intervention,
                                   report=report_name, x_error=x_error, debug=debug)

    with open(report_name, "a") as outfile:
        outfile.write(sft.format_success_msg(success))
