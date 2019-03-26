#!/usr/bin/python

import json
import dtk_test.dtk_sft as sft
import numpy as np
import math
import dtk_test.dtk_ImportPressure_Support as ips

KEY_TOTAL_TIMESTEPS = "Simulation_Duration"
KEY_START_TIME = "Start_Time"
KEY_CAMPAIGN_DURATIONS = "Durations"
KEY_CAMPAIGN_DIP = "Daily_Import_Pressures"
KEY_INITIAL_POPULATION = "InitialPopulation"
KEY_NEW_INFECTIONS = "New Infections"
KEY_STATISTICAL_POPULATION = "Statistical Population"
KEY_NODE_COUNT = "NodeCount"
KEY_NODE_LIST_COUNT = "Node_List_Count"
KEY_CONFIG_NAME = "Config_Name"
KEY_CAMPAIGN_NAME = "Campaign_Filename"
KEY_DEMOGRAPHICS_NAME = "Demographics_Filenames"
KEY_RUN_NUMBER = "Run_Number"

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
    node_list = cf["Events"][0]["Nodeset_Config"]["Node_List"]
    campaign_obj[KEY_NODE_LIST_COUNT] = len(node_list)

    if debug:
        with open("DEBUG_campaign_object.json", "w") as campaign_obj_file:
            json.dump(campaign_obj, campaign_obj_file, indent=4)

    return campaign_obj


def load_demographics_file(demographics_filename="demographics_multiplenodes.json", debug=False):
    """reads demographics file and populates demographics_obj

    :param demographics_filename: demographics.json file
    :returns: demographics_obj structure, dictionary with KEY_INITIAL_POPULATION etc., keys (e.g.)
    """
    with open(demographics_filename) as demographics_infile:
        df = json.load(demographics_infile)
    demographics_obj = {}
    initial_population = 0
    for node in df["Nodes"]:
        initial_population += node["NodeAttributes"][KEY_INITIAL_POPULATION]
    demographics_obj[KEY_INITIAL_POPULATION] = initial_population
    node_count = df["Metadata"]["NodeCount"]
    demographics_obj[KEY_NODE_COUNT] = node_count

    if debug:
        with open("DEBUG_demographics_object.json", "w") as demographics_obj_file:
            json.dump(demographics_obj, demographics_obj_file, indent=4)

    return demographics_obj


def create_report_file(param_obj, campaign_obj, demographics_obj, report_data_obj, report_name, debug=False):
    with open(report_name, "w") as outfile:
        total_timesteps = param_obj[KEY_TOTAL_TIMESTEPS]
        start_timestep = param_obj[KEY_START_TIME]
        initial_population = demographics_obj[KEY_INITIAL_POPULATION]
        rates = [x * campaign_obj[KEY_NODE_LIST_COUNT] for x in campaign_obj[KEY_CAMPAIGN_DIP]]
        durations = campaign_obj[KEY_CAMPAIGN_DURATIONS]
        new_infections = report_data_obj[KEY_NEW_INFECTIONS]
        statistical_population = report_data_obj[KEY_STATISTICAL_POPULATION]
        length = len(rates)
        start_duration = start_timestep
        new_infections_dict = {}
        calculate_new_population = initial_population
        outfile.write("# Test name: " + str(param_obj[KEY_CONFIG_NAME]) + ", Run number: " +
                      str(param_obj[KEY_RUN_NUMBER]) +
                      "\n# Test compares the statistical population with the"
                      " calculated population and tests the Poisson distribution for new infections.\n")
        for i in range(length):
            rate = rates[i]
            duration = durations[i]
            calculate_new_population = rate * duration + calculate_new_population
            end_duration = duration + start_duration
            if rate not in new_infections_dict:
                new_infections_dict[rate] = []
            for j in range(start_duration + 1, end_duration + 1):
                new_infections_dict[rate].append(new_infections[j])
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
        outfile.write("{0}: statistical population is {1}, which is{2}within range of ({3}, {4}).  "
                      "Expected about {5}.\n".format(result_string, statistical_population[-1],
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
                          category="plot_data_{0}".format(title), show=True,
                          sort=True)
            sft.plot_probability(dist, numpy_distro,
                                 title="probability mass function for {}".format(title),
                                 label1="new infection probability from model",
                                 label2="new infection probability from numpy distro",
                                 category="plot_probability_{0}".format(title), show=True)

        outfile.write(sft.format_success_msg(success))
        if debug:
            print("SUMMARY: Success={0}\n".format(success))
        return success


def application(output_folder="output",
                config_filename="config.json", campaign_filename="campaign.json",
                demographics_filename="demographics_multiplenodes.json",
                insetchart_name="InsetChart.json",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("campaign_filename: " + campaign_filename + "\n")
        print("demographics_filename: " + demographics_filename + "\n")
        print("insetchart_name: " + insetchart_name + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()

    param_obj = ips.load_emod_parameters(config_filename, debug)
    campaign_obj = load_campaign_file(param_obj[KEY_CAMPAIGN_NAME], debug)
    demographics_obj = load_demographics_file(param_obj[KEY_DEMOGRAPHICS_NAME][-1], debug)
    report_data_obj = ips.parse_json_report(output_folder, insetchart_name, debug)

    sft.plot_data(report_data_obj[KEY_NEW_INFECTIONS],
                  title="new infections",
                  label1="New Infections",
                  label2="NA",
                  xlabel="time steps", ylabel="new infection",
                  category='New_infections',
                  show=True)
    sft.plot_data(report_data_obj[KEY_STATISTICAL_POPULATION],
                  title="Statistical Population",
                  label1="Statistical Population",
                  label2="NA",
                  xlabel="time steps", ylabel="Statistical Population",
                  category='Statistical_population',
                  show=True, line=True)
    create_report_file(param_obj, campaign_obj, demographics_obj, report_data_obj, report_name,
                        debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-i', '--insetchartname', default="InsetChart.json", help="insetchart to test(InsetChart.json)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()
    application(output_folder=args.output,
                insetchart_name=args.insetchartname,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)
