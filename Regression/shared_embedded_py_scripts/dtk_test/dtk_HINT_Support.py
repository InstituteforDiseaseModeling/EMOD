#!/usr/bin/python

import json
import dtk_test.dtk_sft as sft
import math
import pandas as pd
import os
from dtk_test.dtk_General_Support import ConfigKeys, CampaignKeys, DemographicsKeys, InsetKeys
from scipy import stats
import unittest

"""
Support library for HINT single route SFTs
"""

channels = [InsetKeys.ChannelsKeys.Infected,
            InsetKeys.ChannelsKeys.New_Infections,
            InsetKeys.ChannelsKeys.Statistical_Population]


config_keys = [ConfigKeys.Config_Name, ConfigKeys.Simulation_Timestep,
               ConfigKeys.Simulation_Duration, ConfigKeys.Base_Infectivity,
               ConfigKeys.Run_Number, ConfigKeys.Demographics_Filenames,
               ConfigKeys.Enable_Heterogeneous_Intranode_Transmission]


matches = ["Update(): Time: ",
           "total contagion = ",
           "group_id = "
           ]


class Stdout:
    prob = "prob="
    stat_pop = "StatPop: "
    infected = "Infected: "
    contagion = "Contagion"
    group_id = "group_id"


def load_config_parameters(config_filename="config.json", keys=[ConfigKeys.Config_Name], debug=False):
    """
    reads config file and populates params_obj
    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with Config_Name, etc., keys (e.g.)
    """
    with open(config_filename) as infile:
        cdj = json.load(infile)[ConfigKeys.Parameters]
    param_obj = {}

    for key in keys:
        param_obj[key] = cdj[key]

    if debug:
        with open("DEBUG_param_object.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj


def load_campaign_file(campaign_filename="campaign.json", debug=False):
    """
    reads campaign file
    :param campaign_filename: campaign.json file
    :returns: campaign_obj structure, dictionary with Start_Day, etc., keys (e.g.)
    """
    with open(campaign_filename) as infile:
        cf = json.load(infile)
    campaign_obj = {CampaignKeys.Start_Day: [], CampaignKeys.Demographic_Coverage: [],
                    CampaignKeys.Property_Restrictions: []}
    # note that campaign_obj["Property_Restrictions"] will be a list of list of string.
    events = cf[CampaignKeys.Events]
    for event in events:
        start_day = event[CampaignKeys.Start_Day]
        campaign_obj[CampaignKeys.Start_Day].append(int(start_day))

        coverage = event[CampaignKeys.Event_Coordinator_Config][CampaignKeys.Demographic_Coverage]
        campaign_obj[CampaignKeys.Demographic_Coverage].append(float(coverage))

        property_restrictions = event[CampaignKeys.Event_Coordinator_Config][
            CampaignKeys.Property_Restrictions]
        campaign_obj[CampaignKeys.Property_Restrictions].append(property_restrictions)

    if debug:
        with open("DEBUG_campaign_object.json", 'w') as outfile:
            json.dump(campaign_obj, outfile, indent=4)

    return campaign_obj


def load_demo_overlay_file(demo_filename="demographics_4groups_overlay.json", demo_path="", debug=False):
    """
    laod IndividualProperties from demo overlay file which has single route and multiple individual property
    :param demo_filename: demo overlay file name, like "demographics_4groups_overlay.json"
    :param debug:
    :return:individual_property_list
    """
    with open(os.path.join(demo_path, demo_filename)) as infile:
        dof = json.load(infile)
    individual_properties = dof[DemographicsKeys.Defaults][DemographicsKeys.IndividualProperties]
    individual_property_list = []
    for individual_property in individual_properties:
        property_name = individual_property[DemographicsKeys.PropertyKeys.Property]
        values = individual_property[DemographicsKeys.PropertyKeys.Values]
        initial_distribution = individual_property[DemographicsKeys.PropertyKeys.Initial_Distribution]
        transmission_matrix = individual_property[DemographicsKeys.PropertyKeys.TransmissionMatrix][
            DemographicsKeys.PropertyKeys.Matrix]
        route = individual_property[DemographicsKeys.PropertyKeys.TransmissionMatrix][
            DemographicsKeys.PropertyKeys.Route]

        individual_property_list.append({DemographicsKeys.PropertyKeys.Property: property_name,
                                         DemographicsKeys.PropertyKeys.Values: values,
                                         DemographicsKeys.PropertyKeys.Initial_Distribution: initial_distribution,
                                         DemographicsKeys.PropertyKeys.Matrix: transmission_matrix,
                                         DemographicsKeys.PropertyKeys.Route: route})
    if debug:
        with open("DEBUG_property_object.json", 'w') as outfile:
            json.dump(individual_property_list, outfile, indent=4)
    return individual_property_list


def load_demo_mr_overlay_file(demo_filename="demographics_mr_4groups_trans_bt_overlay.json", demo_path="", debug=False):
    """
    laod IndividualProperties from demo overlay file which has multiple routes and multiple individual property
    :param demo_filename: demo overlay file name, like "demographics_4groups_overlay.json"
    :param debug:
    :return:individual_property_list
    """
    with open(os.path.join(demo_path, demo_filename)) as infile:
        dof = json.load(infile)
    individual_properties = dof[DemographicsKeys.Defaults][DemographicsKeys.IndividualProperties]
    individual_property_list = []
    for individual_property in individual_properties:
        property_name = individual_property[DemographicsKeys.PropertyKeys.Property]
        values = individual_property[DemographicsKeys.PropertyKeys.Values]
        initial_distribution = individual_property[DemographicsKeys.PropertyKeys.Initial_Distribution]
        transmission_matrix = individual_property[DemographicsKeys.PropertyKeys.TransmissionMatrix]
        for route in transmission_matrix:
            transmission_matrix[route] = transmission_matrix[route][DemographicsKeys.PropertyKeys.Matrix]

        individual_property_list.append({DemographicsKeys.PropertyKeys.Property: property_name,
                                         DemographicsKeys.PropertyKeys.Values: values,
                                         DemographicsKeys.PropertyKeys.Initial_Distribution: initial_distribution,
                                         DemographicsKeys.PropertyKeys.TransmissionMatrix: transmission_matrix})

    if debug:
        with open("DEBUG_property_object.json", 'w') as outfile:
            json.dump(individual_property_list, outfile, indent=4)
    return individual_property_list


def parse_stdout_file(stdout_filename="StdOut.txt", simulation_timestep=1, debug=False):
    """
    creates a dictionary to store filtered information for each time step
    :param output_filename: file to parse (StdOut.txt)
    :return:                stdout_df
    """
    filtered_lines = []
    with open(stdout_filename) as logfile:
        for line in logfile:
            if sft.has_match(line,matches):
                filtered_lines.append(line)
    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    # initialize variables
    time_step = index = 0
    stdout_df = pd.DataFrame(columns=[ConfigKeys.Simulation_Timestep, Stdout.stat_pop, Stdout.infected,
                                      Stdout.group_id, Stdout.contagion, Stdout.prob])
    stdout_df.index.name = 'index'
    stat_pop = infected = contagion = prob = group_id = None
    group_contagion = {}
    for line in filtered_lines:
        if matches[0] in line:
            stat_pop = int(sft.get_val(Stdout.stat_pop, line))
            infected = int(sft.get_val(Stdout.infected, line))
            for group_id in sorted(group_contagion):
                stdout_df.loc[index] = [time_step, stat_pop, infected, group_id,
                                        group_contagion[group_id][0], group_contagion[group_id][1]]
                index += 1
            group_contagion = {}
            time_step += simulation_timestep

        elif len(group_contagion) < 2 and matches[1] in line:
            contagion = float(sft.get_val(matches[1], line))
            prob = float(sft.get_val(Stdout.prob, line))
            group_id = int(sft.get_val(matches[2], line))
            if group_id not in group_contagion:
                group_contagion[group_id] = [contagion, prob]
    if debug:
        res_path = r'./DEBUG_filtered_from_logging.csv'
        stdout_df.to_csv(res_path)
    return stdout_df


def parse_property_report_json(report_name="PropertyReport.json", output_folder="output", property_keys=[], debug=False):
    """
    creates property_df data frame
    :param report_name: file to parse (PropertyReport.json)
    :param output_folder:
    :return: property_df: dataframe structure
    """
    property_path = os.path.join(output_folder, report_name)
    with open(property_path) as infile:
        icj = json.load(infile)[InsetKeys.Channels]

    property_dict = {}
    for key in property_keys:
        property_dict[key] = icj[key][InsetKeys.Data]
    property_df = pd.DataFrame.from_dict(property_dict)

    if debug:
        property_df.to_csv("DEBUG_data_{}.csv".format(report_name.split('.')[0]))

    return property_df


def build_values_from_property_values(property_values_list):
    """
    recursive method to build the nested property name and value pairs from a list of property names and values
    :param property_values_list: list of list of property name and values: [
    ["QualityOfCare:Seed_1", "QualityOfCare:Susceptible_1"],
    ["Risk:High", "Risk:Low"]]
    :return: values_list: [
    "QualityOfCare:Seed_1,Risk:High",
    "QualityOfCare:Seed_1,Risk:Low",
    "QualityOfCare:Susceptible_1,Risk:High",
    "QualityOfCare:Susceptible_1,Risk:Low"]
    """
    values_list = []
    if len(property_values_list) == 0:
        return values_list
    elif len(property_values_list) == 1:
        return property_values_list[0]

    #for i in range(len(property_values_list)):
    property_values = property_values_list[0]
    for value in property_values:
        values_list += [value+","+ new_value for new_value in build_values_from_property_values(property_values_list[1:])]
    return values_list

def build_channel_string_for_property(property_list, channels, debug=False):
    """
    build the channel name string based on individual property and base channel names
    :param property_list: list of individual property dictionary
    :param channels: base channel names, like "Infected"
    :param debug:
    :return: channel_strings
    """
    property_values_list = []
    for property_obj in property_list:
        property_name = property_obj[DemographicsKeys.PropertyKeys.Property]
        values = property_obj[DemographicsKeys.PropertyKeys.Values]
        property_values = []
        for value in values:
            property_values.append("{0}:{1}".format(property_name, value))
        property_values_list.append(property_values)
    values_list = build_values_from_property_values(property_values_list)
    channel_strings = []
    for channel in channels:
        for value in values_list:
            channel_strings.append("{0}:{1}".format(channel, value))
    if debug:
        with open("DEBUG_property_channel_string.txt",'w') as outfile:
            outfile.write("\n".join(channel_strings))
    return channel_strings

def test_new_infections(expected_new_infection, actual_new_infection, calculated_prob, failed_count, route, t, group,
         outfile, susceptible_population, file):
    if expected_new_infection < 5 or susceptible_population * (1 - calculated_prob) < 5:
        binom_pmf = stats.binom.pmf(k=actual_new_infection, n=susceptible_population, p=calculated_prob)
        if binom_pmf < 1e-3:
            failed_count += 1
            outfile.write("WARNING: at timestep {0}, new infections for {1} group route {2} is "
                          "{3}, expected = {4}, calculated binomial pmf is {5}.\n"
                          "".format(t, group, route, actual_new_infection,
                                    expected_new_infection, binom_pmf))

    elif not sft.test_binomial_99ci(num_success=actual_new_infection, num_trials=susceptible_population,
                                        prob=calculated_prob, report_file=file,
                                        category="new infections for {0} at time {1}".format(group, t)):
        failed_count += 1
        # math.sqrt(prob * (1 - prob) * num_trials)
        standard_deviation = math.sqrt(
            calculated_prob * (1 - calculated_prob) * susceptible_population)
        # 99% confidence interval
        lower_bound = expected_new_infection - 3 * standard_deviation
        upper_bound = expected_new_infection + 3 * standard_deviation
        outfile.write("WARNING: at timestep {0}, new infections for {1} group route {2} is {3},"
                      " expected within 99% binomial interval ({4}, {5}) with mean = {6}\n"
                      "".format(t, group, route, actual_new_infection,
                                lower_bound, upper_bound, expected_new_infection))
    return failed_count


def compare_new_infections_channels(inset_channels, property_df, duration, inset_chart_obj, outfile, insetchart_name,
                                    property_report_name):
    success = True
    # add new columns in property_df which is the sum of the new infection by route columns
    for channel in inset_channels:
        column_to_add = [c for c in property_df.columns if channel in c]
        property_df[channel] = property_df[column_to_add].sum(axis=1)
    for t in range(duration):
        for channel in inset_channels:
            new_infection_insetchart = inset_chart_obj[channel][t]
            new_infection_property = property_df[channel].iloc[t]
            if new_infection_property != new_infection_insetchart:
                success = False
                outfile.write("    BAD: at time step {0}, the {1} from {2} is {3}, while {4} reports {5} "
                              "in total.\n".format(t, channel, insetchart_name, new_infection_insetchart,
                                                  property_report_name, new_infection_property))
    sft.plot_data(inset_chart_obj[InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT],
                      property_df[InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT].tolist(),
                      label1=insetchart_name,
                      label2=property_report_name,
                      title=InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT,
                      xlabel='day', ylabel='new_infections',
                      category=InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT,
                      line=False, alpha=0.5, overlap=True, sort=False)
    sft.plot_data(inset_chart_obj[InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT],
                      property_df[InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT].tolist(),
                      label1=insetchart_name,
                      label2=property_report_name,
                      title=InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT,
                      xlabel='day', ylabel='new_infections',
                      category=InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT,
                      line=False, alpha=0.5, overlap=True, sort=False)
    return success


def compare_contagion_channels(inset_channels, property_channels, property_df, duration, inset_chart_obj, outfile,
                               insetchart_name, property_report_name):
    success = True
    # add new columns in property_df which is the sum of the contagion by route columns
    for channel in property_channels:
        column_to_add = [c for c in property_df.columns if channel in c]
        property_df[channel] = property_df[column_to_add].sum(axis=1)
    for i in range(len(inset_channels)):
        inset_channel = inset_channels[i]
        property_channel = property_channels[i]
        for t in range(duration):
            contagion_insetchart = inset_chart_obj[inset_channel][t]
            contagion_property = property_df[property_channel].iloc[t]
            if math.fabs(contagion_property - contagion_insetchart) > 5e-2 * contagion_insetchart:
                success = False
                outfile.write("    BAD: at time step {0}, the {1} from {2} is {3}, while {4} reports {5} "
                              "in total.\n".format(t, inset_channel, insetchart_name, contagion_insetchart,
                                                  property_report_name, contagion_property))
        sft.plot_data(inset_chart_obj[inset_channel],
                          property_df[property_channel].tolist(),
                          label1=insetchart_name,
                          label2=property_report_name,
                          title=inset_channel,
                          xlabel='day', ylabel=property_channel,
                          category=inset_channel,
                          line=False, alpha=0.5, overlap=True, sort=False)

    return success


class RunTest(unittest.TestCase):
    def test_build_values_from_property_values_1(self):
        property_values_list = [
            ["QualityOfCare:Seed_1",
             "QualityOfCare:Susceptible_1"]]
        values_list = build_values_from_property_values(property_values_list)

        self.assertTrue(values_list == property_values_list[0])

    def test_build_values_from_property_values_2(self):
        property_values_list = [
            ["QualityOfCare:Seed_1",
             "QualityOfCare:Susceptible_1"],
            ["Risk:High",
             "Risk:Low"]]
        values_list = build_values_from_property_values(property_values_list)

        expected_values_list = ['QualityOfCare:Seed_1,Risk:High',
                                'QualityOfCare:Seed_1,Risk:Low',
                                'QualityOfCare:Susceptible_1,Risk:High',
                                'QualityOfCare:Susceptible_1,Risk:Low']
        self.assertTrue(values_list == expected_values_list)

    def test_build_values_from_property_values_3(self):
        property_values_list = [
            ["QualityOfCare:Seed_1",
             "QualityOfCare:Susceptible_1"],
            ["Risk:High",
             "Risk:Low"],
            ["AgeBin:0-50",
             "AgeBin:51-99"]]
        values_list = build_values_from_property_values(property_values_list)
        expected_values_list = ['QualityOfCare:Seed_1,Risk:High,AgeBin:0-50', 'QualityOfCare:Seed_1,Risk:High,AgeBin:51-99',
                                'QualityOfCare:Seed_1,Risk:Low,AgeBin:0-50', 'QualityOfCare:Seed_1,Risk:Low,AgeBin:51-99',
                                'QualityOfCare:Susceptible_1,Risk:High,AgeBin:0-50', 'QualityOfCare:Susceptible_1,Risk:High,AgeBin:51-99',
                                'QualityOfCare:Susceptible_1,Risk:Low,AgeBin:0-50', 'QualityOfCare:Susceptible_1,Risk:Low,AgeBin:51-99']
        self.assertTrue(values_list == expected_values_list)

    def test_build_channel_string_for_property(self):
        property_list = [
                    {
                        "Property": "QualityOfCare",
                        "Values": [
                            "Seed_1",
                            "Susceptible_1"
                        ],
                        "Initial_Distribution": [ 0.1, 0.9],
                        "Transitions": [],
                        "Matrix": [
                                [0.0, 0.26],
                                [0.0, 0.0]
                            ],
                        "Route": "Contact"
                    },
                    {
                        "Property": "Risk",
                        "Values": [
                            "High",
                            "Low"
                        ],
                        "Initial_Distribution": [ 0.1, 0.9],
                        "Transitions": [],
                        "Matrix": [
                                [0.0, 1.34],
                                [0.0, 0.0]
                            ],
                        "Route": "Contact"
                    },
                    {
                        "Property": "AgeBin",
                        "Values": [
                            "0-50",
                            "51-99"
                        ],
                        "Initial_Distribution": [0.1, 0.9],
                        "Transitions": [],
                        "Matrix": [
                            [0.0, 1.34],
                            [0.0, 0.0]
                        ],
                        "Route": "Contact"
                    }
                ]
        property_channels = build_channel_string_for_property(property_list, channels)
        expected_property_channels = ['Infected:QualityOfCare:Seed_1,Risk:High,AgeBin:0-50',
                                      'Infected:QualityOfCare:Seed_1,Risk:High,AgeBin:51-99',
                                      'Infected:QualityOfCare:Seed_1,Risk:Low,AgeBin:0-50',
                                      'Infected:QualityOfCare:Seed_1,Risk:Low,AgeBin:51-99',
                                      'Infected:QualityOfCare:Susceptible_1,Risk:High,AgeBin:0-50',
                                      'Infected:QualityOfCare:Susceptible_1,Risk:High,AgeBin:51-99',
                                      'Infected:QualityOfCare:Susceptible_1,Risk:Low,AgeBin:0-50',
                                      'Infected:QualityOfCare:Susceptible_1,Risk:Low,AgeBin:51-99',
                                      'New Infections:QualityOfCare:Seed_1,Risk:High,AgeBin:0-50',
                                      'New Infections:QualityOfCare:Seed_1,Risk:High,AgeBin:51-99',
                                      'New Infections:QualityOfCare:Seed_1,Risk:Low,AgeBin:0-50',
                                      'New Infections:QualityOfCare:Seed_1,Risk:Low,AgeBin:51-99',
                                      'New Infections:QualityOfCare:Susceptible_1,Risk:High,AgeBin:0-50',
                                      'New Infections:QualityOfCare:Susceptible_1,Risk:High,AgeBin:51-99',
                                      'New Infections:QualityOfCare:Susceptible_1,Risk:Low,AgeBin:0-50',
                                      'New Infections:QualityOfCare:Susceptible_1,Risk:Low,AgeBin:51-99',
                                      'Statistical Population:QualityOfCare:Seed_1,Risk:High,AgeBin:0-50',
                                      'Statistical Population:QualityOfCare:Seed_1,Risk:High,AgeBin:51-99',
                                      'Statistical Population:QualityOfCare:Seed_1,Risk:Low,AgeBin:0-50',
                                      'Statistical Population:QualityOfCare:Seed_1,Risk:Low,AgeBin:51-99',
                                      'Statistical Population:QualityOfCare:Susceptible_1,Risk:High,AgeBin:0-50',
                                      'Statistical Population:QualityOfCare:Susceptible_1,Risk:High,AgeBin:51-99',
                                      'Statistical Population:QualityOfCare:Susceptible_1,Risk:Low,AgeBin:0-50',
                                      'Statistical Population:QualityOfCare:Susceptible_1,Risk:Low,AgeBin:51-99']
        self.assertTrue(property_channels == expected_property_channels)


if __name__ == "__main__":
    unittest.main()

