#!/usr/bin/python

import json
import os.path as path
import dtk_test.dtk_sft as sft
import numpy as np
import math
import random

KEY_CONFIG_NAME = "Config_Name"
KEY_RUN_NUMBER = "Run_Number"
KEY_CAMPAIGN_DURATIONS = "Durations"
KEY_CAMPAIGN_DIP = "Daily_Import_Pressures"
KEY_CAMPAIGN_NAME = "Campaign_Filename"
KEY_DEMOGRAPHICS_NAME = "Demographics_Filenames"

KEY_TOTAL_TIMESTEPS = "Simulation_Duration"
KEY_START_TIME = "Start_Time"
KEY_INITIAL_POPULATION = "InitialPopulation"
KEY_NEW_INFECTIONS = "New Infections"
KEY_STATISTICAL_POPULATION = "Statistical Population"

"""
The ImportPressure intervention extends the ImportCases outbreak event; rather 
than importing a deterministic number of cases on a scheduled day, ImportPressure 
applies a set of per-day rates of importation of infected individuals, over a 
corresponding set of durations. For example, an ImportPressure intervention set 
with Durations = [10, 10] and Daily_Import_Pressures = [0.1, 5.0] will import 
infected individuals at a Poisson rate of 0.1 per day for 10 days after distribution, 
and will then raise the importation rate to 5.0 per day for the next 10 days, and 
then it will expire and be disposed of. Durations and Daily_Import_Pressures must 
be vectors of equal length. ImportPressure inherits from Outbreak; the Antigen and 
Genome parameters are defined as they are for all Outbreak events.

A note on behavior in single-node vs multi-node simulations -- Daily_Import_Pressures
vector defines a rate of per-day importation for each node that the intervention is 
distributed to. If the example intervention described above, with 
Daily_Import_Pressures = [0.1, 5.0], is distributed to 10 nodes, then the 
total importation rate summed over all nodes will be 1/day and 50/day during the 
two time periods. The user should be aware of this behavior, and divide the vector 
or per-day importation rates by the number of nodes if appropriate.
"""


# region pre_process support
def generate_durations(length, max_duration):
    """
    generate a random list of durations(integer)
    :param length: length of the list
    :param max_duration: total duration of this list
    :return: list of durations
    """
    durations = []
    total_duration = 0
    for i in range(length):
        random_duration = random.random() + 1  # at least 1 day
        total_duration += random_duration
        durations.append(random_duration)
    ratio = max_duration / float(total_duration)
    durations = [int(x * ratio) for x in durations]
    durations[-1] = max_duration - sum(durations) + durations[-1]
    return durations


def generate_rates(length, max_rate):
    """
    generate a random list of rates(float)
    :param length: length of the list
    :param max_rate: maximum rate in thie list
    :return: list of rates
    """
    rates = []
    for i in range(length):
        random_max = random.random() * max_rate  # more randomness
        random_rate = random.random() * random_max
        rates.append(random_rate)
    return rates


def set_random_campaign_file(durations, rates, campaign_filename="campaign.json", campaign_template_filename=
                             "campaign_template.json", debug=False):
    """
    set Durations and Daily_Import_Pressures in campaign file
    :param campaign_filename: name of campaign file(campaign.json)
    :param campaign_template_filename: name of campaign template file (campaign_template.json)
    :param debug:
    :return: None
    """
    with open(campaign_template_filename, "r") as infile:
        campaign_json = json.load(infile)
    campaign_json["Events"][0]["Event_Coordinator_Config"]["Intervention_Config"][KEY_CAMPAIGN_DURATIONS] = durations
    campaign_json["Events"][0]["Event_Coordinator_Config"]["Intervention_Config"][KEY_CAMPAIGN_DIP] = rates
    with open(campaign_filename, "w") as outfile:
        json.dump(campaign_json, outfile, indent=4, sort_keys=True)

    if debug:
        print("durations are : {}.\n".format(durations))
        print("daily import pressures are : {}.\n".format(rates))


# endregion

# region post_process support
def load_emod_parameters(config_filename="config.json", debug=False):
    """reads config file and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_START_TIMESTEP, etc., keys (e.g.)
    """
    cdj = None
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {}
    param_obj[KEY_TOTAL_TIMESTEPS] = cdj[KEY_TOTAL_TIMESTEPS]
    param_obj[KEY_START_TIME] = cdj[KEY_START_TIME]
    param_obj[KEY_CONFIG_NAME] = cdj[KEY_CONFIG_NAME]
    param_obj[KEY_RUN_NUMBER] = cdj[KEY_RUN_NUMBER]
    param_obj[KEY_CAMPAIGN_NAME] = cdj[KEY_CAMPAIGN_NAME]
    param_obj[KEY_DEMOGRAPHICS_NAME] = cdj[KEY_DEMOGRAPHICS_NAME]

    if debug:
        print(param_obj)
    return param_obj


def load_campaign_file(campaign_filename="campaign.json", debug=False):
    """reads campaign file and populates campaign_obj

    :param campaign_filename: campaign.json file
    :returns: campaign_obj structure, dictionary with KEY_CAMPAIGN_DURATIONS, etc., keys (e.g.)
    """
    with open(campaign_filename) as infile:
        cf = json.load(infile)
    campaign_obj = {}
    durations = cf["Events"][0]["Event_Coordinator_Config"]["Intervention_Config"][KEY_CAMPAIGN_DURATIONS]
    campaign_obj[KEY_CAMPAIGN_DURATIONS] = durations
    daily_import_pressures = cf["Events"][0]["Event_Coordinator_Config"]["Intervention_Config"][KEY_CAMPAIGN_DIP]
    campaign_obj[KEY_CAMPAIGN_DIP] = daily_import_pressures

    if debug:
        print("Durations are: {0}.\n Daily_Import_Pressures are: {1}.".format(durations, daily_import_pressures))
        with open("DEBUG_campaign_object.json", "w") as campaign_obj_file:
            json.dump(campaign_obj, campaign_obj_file, indent=4)

    return campaign_obj


def load_demographics_file(demographics_filename="demographics.json", debug=False):
    """reads demographics file and populates demographics_obj

    :param demographics_filename: demographics.json file
    :returns: demographics_obj structure, dictionary with KEY_INITIAL_POPULATION etc., keys (e.g.)
    """
    with open(demographics_filename) as infile:
        df = json.load(infile)
    demographics_obj = {}
    initial_population = df["Nodes"][0]["NodeAttributes"][KEY_INITIAL_POPULATION]
    demographics_obj[KEY_INITIAL_POPULATION] = initial_population

    if debug:
        with open("DEBUG_demographics_object.json", "w") as demographics_obj_file:
            json.dump(demographics_obj, demographics_obj_file, indent=4)

    return demographics_obj


def parse_json_report(output_folder="output", insetchart_name="InsetChart.json", debug=False):
    """creates report_data_obj structure with keys

    :param insetchart_name: InsetChart.json file with location (output/InsetChart.json)
    :returns: report_data_obj structure, dictionary with KEY_NEW_INFECTIONS etc., keys (e.g.)
    """
    insetchart_path = path.join(output_folder, insetchart_name)
    with open(insetchart_path) as infile:
        icj = json.load(infile)["Channels"]

    report_data_obj = {}

    new_infections = icj[KEY_NEW_INFECTIONS]["Data"]
    report_data_obj[KEY_NEW_INFECTIONS] = new_infections
    statistical_population = icj[KEY_STATISTICAL_POPULATION]["Data"]
    report_data_obj[KEY_STATISTICAL_POPULATION] = statistical_population

    if debug:
        with open("DEBUG_data_InsetChart.json", "w") as outfile:
            json.dump(report_data_obj, outfile, indent=4)

    return report_data_obj


def create_report_file(param_obj, campaign_obj, demographics_obj, report_data_obj, report_name, debug=False,):
    """creates report file from data_obj structures
    :returns: scientific feature report with details of the tests
    """
    with open(report_name, "w") as outfile:
        total_timesteps = param_obj[KEY_TOTAL_TIMESTEPS]
        start_timestep = param_obj[KEY_START_TIME]
        initial_population = demographics_obj[KEY_INITIAL_POPULATION]
        rates = campaign_obj[KEY_CAMPAIGN_DIP]
        durations = campaign_obj[KEY_CAMPAIGN_DURATIONS]
        outfile.write(
            "# Test name: " + str(param_obj["Config_Name"]) + ", Run number: " + str(param_obj["Run_Number"]) +
            "\n# Test compares the statistical population with the"
            " calculated population and tests the Poisson distribution for new infections.\n")
        try:
            new_infections = report_data_obj[KEY_NEW_INFECTIONS]
            statistical_population = report_data_obj[KEY_STATISTICAL_POPULATION]
            length = len(rates)
            start_duration = start_timestep
            new_infections_dict = {}
            calculate_new_population = initial_population
            for i in range(length):
                rate = rates[i]
                duration = durations[i]
                calculate_new_population = rate * duration + calculate_new_population
                end_duration = duration + start_duration
                if rate not in new_infections_dict:
                    new_infections_dict[rate] = []
                for j in range(start_duration + 1, end_duration + 1):
                    if j < total_timesteps + start_timestep:
                        new_infections_dict[rate].append(new_infections[j])
                        j += 1
                    else:
                        break
                if end_duration > total_timesteps + start_timestep:
                    calculate_new_population -= rate * (end_duration - total_timesteps - start_timestep)
                    break
                start_duration = end_duration
            if end_duration < total_timesteps + start_timestep:
                rate = 0.0
                if rate not in new_infections_dict:
                    new_infections_dict[rate] = []
                for j in range(end_duration + 1, len(new_infections)):
                    new_infections_dict[rate].append(new_infections[j])
            if debug:
                with open("DEBUG_new_infections_parsed.json", "w") as new_infections_file:
                    json.dump(new_infections_dict, new_infections_file, indent=4)

            # test statistical population channel
            diff_population = math.fabs(calculate_new_population - statistical_population[-1])
            if debug:
                print("calculated population is {0}, statistical population "
                      "from InsetChart is {1}.".format(calculate_new_population,
                                                       statistical_population[-1]))
            error_tolerance = math.fabs(calculate_new_population - initial_population) * 0.1
            low_acceptance_bound = round(calculate_new_population - error_tolerance)
            high_acceptance_bound = round(calculate_new_population + error_tolerance)
            if debug:
                print("diff_population is {0}, error_tolerance is {1}".format(diff_population, error_tolerance))
            success = diff_population < error_tolerance
            result_string = "GOOD" if success else "BAD"
            within_acceptance_bounds = " " if success else " not "
            outfile.write("{0}: statistical population is {1}, which is{2}within range ({3}, {4})"
                          ".  Expected about {5}.\n".format(result_string, statistical_population[-1],
                                                            within_acceptance_bounds, low_acceptance_bound,
                                                            high_acceptance_bound, calculate_new_population))
            # test poisson distribution for new infections
            for rate in new_infections_dict:
                dist = new_infections_dict[rate]
                title = "rate = " + str(rate)
                result = sft.test_poisson(dist, rate, route=title, report_file=outfile, normal_approximation=False)
                if not result:
                    success = False
                numpy_distro = np.random.poisson(rate, len(dist))
                sft.plot_data(dist, numpy_distro,
                              title="new infections for {}".format(title),
                              label1="new infection from model, {}".format(title),
                              label2="Poisson distro from numpy",
                              xlabel="data points", ylabel="new infection",
                              category="plot_data_{0}".format(title), show=True)
                sft.plot_probability(dist, numpy_distro,
                                     title="probability mass function for {}".format(title),
                                     label1="new infection probability from model",
                                     label2="new infection probability from numpy distro",
                                     category="plot_probability_{0}".format(title), show=True)
        except IndexError:
            success = False
            outfile.write("BAD: There is no data in the InsetChart report")
            print("IndexError: There is no data in the InsetChart report")
        outfile.write(sft.format_success_msg(success))
        if debug:
            print("SUMMARY: Success={0}\n".format(success))
        return success
# endregion
