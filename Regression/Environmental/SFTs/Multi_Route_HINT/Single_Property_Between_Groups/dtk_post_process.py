#!/usr/bin/python

if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as sft
import math
import pandas as pd
import dtk_test.dtk_General_Support as General_Support
from dtk_test.dtk_General_Support import ConfigKeys, CampaignKeys, DemographicsKeys, InsetKeys
import numpy as np
import dtk_test.dtk_HINT_Support as hint_support

"""

This SFT is testing environmental feature 2.5 Multi-route HINT.

Similar to Environmental\SFTs\Multi_Route_HINT\Single_Property_Between_Groups, this SFT is testing multiple routes HINT, 
but it's testing transmission between one group to another group. Please see test comment in 
Environmental\SFTs\Multi_Route_HINT\Single_Property_Within_Group for more details.

This test validates 7 things:

1. Simulation configuration preconditions for the test are met.
2. For group QualityOfCare:Susceptible:
    2.1 Total contagion for contact route should be Base_Infectivity * number of infected in group QualityOfCare:Seed * 
        multiplier in HINT matrix / total population
    2.2 Total contagion for environmental route should be Base_Infectivity * number of infected in group QualityOfCare:Seed * 
        multiplier in HINT matrix / population of group QualityOfCare:Susceptible
3. New infections should be within 5% of (susceptibles * new infection probability) for 95% of timesteps
4. Total new infections for both routes should be within 5% of tolerance compared to expected value.
5. For group QualityOfCare:Seed, contagions should be zero for all time steps.
6. InsetChart New Infections for each route should equal to the sum of all new infections for that route across all groups
7. Total contagion population from InsetChart should be euqal to the sum of total contagion population across all groups

"""

channels = [InsetKeys.ChannelsKeys.Infected,
            InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT,
            InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT,
            "Contagion (Environment)",
            "Contagion (Contact)",
            InsetKeys.ChannelsKeys.Statistical_Population]

inset_channels = [InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT,
                  InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT,
                  InsetKeys.ChannelsKeys.Environmental_Contagion_Population,
                  InsetKeys.ChannelsKeys.Contact_Contagion_Population]

config_keys = [ConfigKeys.Config_Name,
               ConfigKeys.Simulation_Timestep,
               ConfigKeys.Simulation_Duration,
               ConfigKeys.Base_Infectivity,
               ConfigKeys.Run_Number,
               ConfigKeys.Demographics_Filenames,
               ConfigKeys.Enable_Heterogeneous_Intranode_Transmission,
               ConfigKeys.Node_Contagion_Decay_Rate]

matches = ["Update(): Time: ",
           "total contagion = ",
           "group_id = "
           ]

routes = [
    "environmental",
    "contact"
]


class Stdout:
    prob = "prob="
    stat_pop = "StatPop: "
    infected = "Infected: "
    contagion = "Contagion"
    group_id = "group_id"


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
    stdout_df = pd.DataFrame(columns=[ConfigKeys.Simulation_Timestep,
                                      Stdout.stat_pop,
                                      Stdout.infected,
                                      Stdout.group_id,
                                      Stdout.contagion,
                                      Stdout.prob])
    stdout_df.index.name = 'index'
    contagion = prob = group_id = None
    for line in filtered_lines:
        if matches[0] in line:
            stat_pop = int(sft.get_val(Stdout.stat_pop, line))
            infected = int(sft.get_val(Stdout.infected, line))
            stdout_df.loc[index] = [time_step,
                                    stat_pop,
                                    infected,
                                    group_id,
                                    contagion,
                                    prob]
            index += 1
            time_step += simulation_timestep

        elif matches[1] in line:
            contagion = float(sft.get_val(matches[1], line))
            prob = float(sft.get_val(Stdout.prob, line))
            group_id = int(sft.get_val(matches[2], line))

    if debug:
        res_path = r'./DEBUG_filtered_from_logging.csv'
        stdout_df.to_csv(res_path)
    return stdout_df

def create_report_file(param_obj, campaign_obj, stdout_df, property_df, property_obj, inset_chart_obj,
                       insetchart_name, property_report_name, report_name, debug):
    with open(report_name, "w") as sft_report_file :
        config_name = param_obj[ConfigKeys.Config_Name]
        base_infectivity = param_obj[ConfigKeys.Base_Infectivity]
        sft_report_file.write("Config_name = {}\n".format(config_name))
        sft_report_file.write("{0} = {1} {2} = {3}\n".format(
            ConfigKeys.Base_Infectivity, base_infectivity,
            ConfigKeys.Run_Number, param_obj[ConfigKeys.Run_Number]))

        success = True

        sft_report_file.write("Test 1: checking test conditions/setup in config.json and campaign.json:\n")
        if int(param_obj[ConfigKeys.Enable_Heterogeneous_Intranode_Transmission]) != 1:
            success = False
            sft_report_file.write("BAD: HINT is not enabled, please check the test.\n")
        else:
            if float(param_obj[ConfigKeys.Node_Contagion_Decay_Rate]) != 1:
                success = False
                sft_report_file.write("BAD: Expect Environmental contagion decay 100%, but Node_Contagion_Decay_Rate = {}"
                              ", please check the test.\n".format(param_obj[ConfigKeys.Node_Contagion_Decay_Rate]))

            if not all(x == 0 for x in campaign_obj[CampaignKeys.Start_Day]):
                success = False
                sft_report_file.write("BAD: All intervention should start at day 0, please check campaign.json.\n")

            if not all(x == 0.05 for x in campaign_obj[CampaignKeys.Demographic_Coverage]):
                success = False
                sft_report_file.write("BAD: {} should be 0.05, please check campaign.json.\n".format(
                    CampaignKeys.Demographic_Coverage))

            expected_property_restrictions = []

            property_values = property_obj[DemographicsKeys.PropertyKeys.Values]
            seed_groups = [value for value in property_values if 'seed' in value.lower()]
            for value in seed_groups:
                expected_property_restrictions.append(["{0}:{1}".format(
                    property_obj[DemographicsKeys.PropertyKeys.Property], value)])
            if campaign_obj[CampaignKeys.Property_Restrictions] != expected_property_restrictions:
                success = False
                sft_report_file.write(
                    "BAD: {0} should be {1}, got {2} in campaign.json, please check campaign.json.\n".format(
                        CampaignKeys.Property_Restrictions, expected_property_restrictions,
                        campaign_obj[CampaignKeys.Property_Restrictions]))
        sft_report_file.write("Test 1: campaign and config met preconditions: {}.\n".format(success))

        if success:
            sft_report_file.write("Test 2: Testing contagion and probability for each Routes for every time step:\n")
            sft_report_file.write("Test 3: Testing New Infection channel from property report based on transmission matrix "
                          "for every timestep:\n")
            sft_report_file.write("Test 4: testing total new infection by routes match the total expected new infections"
                          "for each group.\n".format(
                InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT,
                InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT, insetchart_name, property_report_name))
            sft_report_file.write("Test 2, 3 and 4 and running at the same time:\n")
            result_2 = result_3 = result_4 = True
            stat_pop = stdout_df[Stdout.stat_pop]
            duration = param_obj[ConfigKeys.Simulation_Duration]
            transmission_matrix_e = property_obj[DemographicsKeys.PropertyKeys.TransmissionMatrix][routes[0]]
            transmission_matrix_c = property_obj[DemographicsKeys.PropertyKeys.TransmissionMatrix][routes[1]]
            for seed_group in seed_groups:
                # get the name of susceptible group, there should be only one susceptible group based on the query
                susceptible_group = [value for value in property_obj[DemographicsKeys.PropertyKeys.Values
                ] if "susceptible" in value.lower()][0]
                # get all the column names that contains the seed group
                seed_cols = [c for c in property_df.columns if seed_group in c]
                # get all the column names that contains the susceptible group
                susceptible_cols = [c for c in property_df.columns if susceptible_group in c]

                # test data for seed group
                infected_seed = property_df[[c for c in seed_cols if channels[0] in c]]
                contagion_seed_e = property_df[[c for c in seed_cols if channels[3] in c]]
                contagion_seed_c = property_df[[c for c in seed_cols if channels[4] in c]]

                # test data for susceptible group
                infected_sus = property_df[[c for c in susceptible_cols if channels[0] in c]]
                new_infection_sus_e = property_df[[c for c in susceptible_cols if channels[1] in c]]
                new_infection_sus_c = property_df[[c for c in susceptible_cols if channels[2] in c]]
                contagion_sus_e = property_df[[c for c in susceptible_cols if channels[3] in c]]
                contagion_sus_c = property_df[[c for c in susceptible_cols if channels[4] in c]]
                population_sus = property_df[[c for c in susceptible_cols if channels[5] in c]]

                expected_new_infection_list_e = []
                expected_new_infection_list_c = []
                contagion_list_e = [[contagion_sus_e.iloc[0][0], 0]]
                contagion_list_c = [[contagion_sus_c.iloc[0][0], 0]]

                failed_count_e = 0
                failed_count_c = 0
                with open("DEBUG_binomial_test_{}.txt".format(susceptible_group), 'w') as group_file:
                    for t in range(duration - 1):
                        infectivity_seed = base_infectivity * infected_seed.iloc[t][0]

                        # calculate contagion of susceptible group
                        # environmental contagion decay 100% at each time step so it works as contact contagion
                        # "contact" route will be normalized by total population
                        # "environmental" route will be normalized by group population
                        calculated_contagion_e = infectivity_seed * \
                                                 transmission_matrix_e[property_values.index(seed_group)][
                            property_values.index(susceptible_group)] / population_sus.iloc[t][0]

                        calculated_contagion_c = infectivity_seed * \
                                                 transmission_matrix_c[property_values.index(seed_group)][
                            property_values.index(susceptible_group)] / stat_pop[t]

                        # get contagion of susceptible group from property report
                        actual_contagion_e = contagion_sus_e.iloc[t + 1][0]
                        actual_contagion_c = contagion_sus_c.iloc[t + 1][0]

                        contagion_list_e.append([actual_contagion_e, calculated_contagion_e])
                        contagion_list_c.append([actual_contagion_c, calculated_contagion_c])

                        if math.fabs(calculated_contagion_e - actual_contagion_e) > 5e-2 * calculated_contagion_e:
                            result_2 = success = False
                            sft_report_file.write("    BAD: at time step {0}, for group {1} route {2}, the contagion is {3}, "
                                          "expected {4}.\n".format(t + 1, susceptible_group, routes[0], actual_contagion_e,
                                                                   calculated_contagion_e
                            ))

                        if math.fabs(calculated_contagion_c - actual_contagion_c) > 5e-2 * calculated_contagion_c:
                            result_2 = success = False
                            sft_report_file.write("    BAD: at time step {0}, for group {1} route {2}, the contagion is {3}, "
                                          "expected {4}.\n".format(t + 1, susceptible_group, routes[1], actual_contagion_c,
                                                                   calculated_contagion_c
                            ))

                        # calculate infection probability based on contagion
                        calculated_prob_e = 1.0 - math.exp(-1 * calculated_contagion_e *
                                                           param_obj[ConfigKeys.Simulation_Timestep])
                        calculated_prob_c = 1.0 - math.exp(-1 * calculated_contagion_c *
                                                           param_obj[ConfigKeys.Simulation_Timestep])

                        # calculate expected new infection for susceptible group
                        susceptible_population_c = population_sus.iloc[t][0] - infected_sus.iloc[t][0]
                        expected_new_infection_c = susceptible_population_c * calculated_prob_c

                        # Superinfection is not supported, so individuals may not be infected from both the environmental and the contact route.
                        susceptible_population_e = susceptible_population_c - new_infection_sus_c.iloc[t + 1][0]
                        expected_new_infection_e = susceptible_population_e * calculated_prob_e

                        expected_new_infection_list_e.append(expected_new_infection_e)
                        expected_new_infection_list_c.append(expected_new_infection_c)

                        actual_new_infection_e = new_infection_sus_e.iloc[t + 1][0]
                        actual_new_infection_c = new_infection_sus_c.iloc[t + 1][0]

                        # run the same test for both routes
                        failed_count_e = hint_support.test_new_infections(
                            expected_new_infection_e, actual_new_infection_e, calculated_prob_e, failed_count_e,
                            routes[0], t + 1, susceptible_group, sft_report_file , susceptible_population_e, group_file)

                        failed_count_c = hint_support.test_new_infections(
                            expected_new_infection_c, actual_new_infection_c, calculated_prob_c, failed_count_c,
                            routes[1], t + 1, susceptible_group, sft_report_file , susceptible_population_c, group_file)

                # make sure the seed group has no contagion to itself
                message_template = "{0}: (route {1}) contagion pool for seed group is {2}, expected 0 contagion, " \
                                   "test is {3}.\n"
                for contagion_seed, route in [(contagion_seed_e, routes[0]),
                                              (contagion_seed_c, routes[1])]:
                    contagion_sum = contagion_seed.values.sum()
                    if contagion_sum:
                        result_2 = success = False
                        sft_report_file.write(message_template.format("BAD", route, contagion_sum, "failed"))
                    else:
                        sft_report_file.write(message_template.format("GOOD", route, contagion_sum, "passed"))

                message_template = "{0}: (route {1}) binomial test for {2} failed {3} times within {4} total " \
                                   "timesteps, which is {5}% fail rate, test is {6}. Please see " \
                                   "'DEBUG_binomial_test_{7}.txt' for details.\n"
                for failed_count, new_infection_sus, route in [(failed_count_e, new_infection_sus_e, routes[0]),
                                            (failed_count_c, new_infection_sus_c, routes[1])]:
                    if failed_count / duration > 5e-2:
                        result_3 = success = False
                        sft_report_file.write(message_template.format("BAD", route, new_infection_sus.columns.values,
                                                              failed_count, duration, (failed_count / duration) * 100,
                                                              "failed", susceptible_group))
                    else:
                        sft_report_file.write(message_template.format("GOOD", route, new_infection_sus.columns.values,
                                                              failed_count, duration, (failed_count / duration) * 100,
                                                              "passed", susceptible_group))

                # check the total new infection
                # plot actual and expected values for contagion and new infeciton for each route and each group
                message = "{0}: for route {1} and group {2}, the total new infection is {3}, expected {4}.\n"
                for contagion_list, new_infection_sus, expected_new_infection_list, route in \
                        [(contagion_list_e, new_infection_sus_e, expected_new_infection_list_e, routes[0]),
                         (contagion_list_c, new_infection_sus_c, expected_new_infection_list_c, routes[1])]:
                    if math.fabs(new_infection_sus.ix[1:, 0].sum() - sum(expected_new_infection_list)) > \
                            5e-2 * sum(expected_new_infection_list):
                        result_4 = success = False
                        sft_report_file.write(message.format("BAD", route, susceptible_group,
                                                             new_infection_sus.ix[1:, 0].sum(),
                                                            sum(expected_new_infection_list)))
                    else:
                        sft_report_file.write(message.format("GOOD", route, susceptible_group,
                                                             new_infection_sus.ix[1:, 0].sum(),
                                                             sum(expected_new_infection_list)))

                    sft.plot_data(np.array(contagion_list)[:, 0], np.array(contagion_list)[:, 1],
                                      label1=property_report_name, label2="calculated contagion",
                                      title="contagion\nroute {0}, group {1}".format(route, susceptible_group),
                                      xlabel='day', ylabel='contagion',
                                      category="contagion_{0}_{1}".format(susceptible_group, route),
                                      line=True, alpha=0.5, overlap=True, sort=False)

                    sft.plot_data(new_infection_sus.ix[1:, 0].tolist(), expected_new_infection_list,
                                      label1=property_report_name,
                                      label2="expected new infection",
                                      title="new infections\nroute {0}, group {1}".format(route, susceptible_group),
                                      xlabel='day', ylabel='new_infections',
                                      category="new_infections_{0}_{1}".format(susceptible_group, route),
                                      line=False, alpha=0.5, overlap=True, sort=False)

            sft_report_file.write("Test 2: result is: {}.\n".format(result_2))
            sft_report_file.write("Test 3: result is: {}.\n".format(result_3))
            sft_report_file.write("Test 4: result is: {}.\n".format(result_4))

            sft_report_file.write("Test 5: testing {0} and {1} in {2} and data collected form {3} for every time step.\n".format(
                InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT,
                InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT, insetchart_name, property_report_name))
            result_5 = hint_support.compare_new_infections_channels(inset_channels[:2], property_df, duration,
                                                                    inset_chart_obj, sft_report_file , insetchart_name,
                                                                    property_report_name)
            if not result_5:
                success = False
            sft_report_file.write("Test 5: result is: {}.\n".format(result_5))

            sft_report_file.write("Test 6: testing {0} and {1} in {2} and data collected form {3} for every time step.\n".format(
                InsetKeys.ChannelsKeys.Environmental_Contagion_Population,
                InsetKeys.ChannelsKeys.Contact_Contagion_Population, insetchart_name, property_report_name))
            result_6 = hint_support.compare_contagion_channels(inset_channels[2:], channels[3:5], property_df, duration,
                                                               inset_chart_obj, sft_report_file , insetchart_name,
                                                               property_report_name)
            if not result_6:
                success = False
            sft_report_file.write("Test 6: result is: {}.\n".format(result_6))

        sft_report_file.write(sft.format_success_msg(success))
    if debug:
        print(sft.format_success_msg(success))
    return success


def application(output_folder="output", stdout_filename="test.txt",
                property_report_name="PropertyReportEnvironmental.json",
                insetchart_name="InsetChart.json",
                config_filename="config.json", campaign_filename="campaign.json",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename + "\n")
        print("property_report_name: " + property_report_name + "\n")
        print("insetchart_name: " + insetchart_name + "\n")
        print("config_filename: " + config_filename + "\n")
        print("campaign_filename: " + campaign_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done(stdout_filename)

    config_obj = hint_support.load_config_parameters(config_filename, config_keys, debug)
    campaign_obj = hint_support.load_campaign_file(campaign_filename, debug)
    stdout_df = parse_stdout_file(stdout_filename, config_obj[ConfigKeys.Simulation_Timestep], debug)

    demo_path = "Assets" if stdout_filename == "StdOut.txt" else ""
    property_list = hint_support.load_demo_mr_overlay_file(
        config_obj[ConfigKeys.Demographics_Filenames][1], demo_path, debug)
    property_keys = hint_support.build_channel_string_for_property(property_list, channels, debug)
    property_df = hint_support.parse_property_report_json(property_report_name, output_folder, property_keys, debug)
    property_obj = property_list[0] # this test only has one property object

    inset_chart_obj = General_Support.parse_inset_chart(output_folder, insetchart_name, inset_channels, debug)

    create_report_file(config_obj, campaign_obj, stdout_df, property_df, property_obj, inset_chart_obj,
                       insetchart_name, property_report_name, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-p', '--propertyreport', default="PropertyReportEnvironmental.json",
                        help="Property report to load (PropertyReportEnvironmental.json)")
    parser.add_argument('-j', '--insetchart', default="InsetChart.json",
                        help="json report to load (InsetChart.json)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-C', '--campaign', default="campaign.json", help="campaign name to load (campaign.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                property_report_name=args.propertyreport, insetchart_name=args.insetchart,
                config_filename=args.config, campaign_filename=args.campaign,
                report_name=args.reportname, debug=args.debug)

