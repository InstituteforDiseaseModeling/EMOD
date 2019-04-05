#!/usr/bin/python

import dtk_test.dtk_sft as sft
import math
import pandas as pd
from dtk_test.dtk_General_Support import ConfigKeys, CampaignKeys, DemographicsKeys, InsetKeys
import numpy as np
import dtk_test.dtk_General_Support as General_Support
import dtk_test.dtk_HINT_Support as HINT_Support
import json
import os

"""

This SFT is testing environmental feature 2.6 Interventions EnvironmentalDiagnostic:

"The EnvironmentalDiagnostic class is a node-level intervention that samples the environment, compares this value to a 
threshold, and then broadcasts either a positive or negative node event. The user will need the ability to optionally 
indicate an IP in case IP's are used to model different locations.
Environment_IP_Key_Value
    If provided, this individual property key-value pair will indicate a specific transmission “pool”. Typically, 
    this will be used to identify a location.
    If not provided or empty, then the sample is to be expected for the entire node.
    This is an IP key-value pair such that we have “key:value”.
Sample_Threshold
    If the sample is greater than (NOT equal to) this threshold, then a positive finding will have been found. This 
    does not include values equal to the threshold so that the threshold can be set at zero. If the value is zero, 
    the user is just looking for any deposit in the transmission pool.
    Range: 0 to FLT_MAX
    Default = 0
Base_Sensitivity
    Modified from SimpleDiagnostic - The sensitivity of the diagnostic. This sets the proportion of the time that the 
    environment with the condition being tested receives a positive diagnostic test.
    When set to 1, the diagnostic accurately reflects the condition. When set to 0, the diagnosis always returns a 
    false-negative.
    Range: 0 to 1
    Default = 1
Base_Specificity
    Modified from SimpleDiagnostic - This sets the proportion of the time that the environment without the condition 
    being tested receive a negative diagnostic test. When set to 1, the diagnostic always accurately reflects the lack 
    of having the condition. When set to zero, then environments that do not have the condition always receive a 
    false-positive diagnostic test."
    1.0 => no false positives, 0 => positive result always returned whether it is true or not
    Range: 0 to 1
    Default = 1
Positive_Diagnostic_Event
    If the sample was greater than the threshold, this node-level event will be broadcast for this node.
    This cannot be empty string or NoTrigger.
Negative_Diagnostic_Event
    If the sample was less than or equal to the threshold, this node-level event will be broadcast for this node.
    If the string is empty or NoTrigger, then no event will be distributed."

Unlike the tests in the subfolder, in this test we turn on all features in EnvironmentalDiagnostic.

Test data is loaded from StdOut.txt, PropertyReportEnvironmental.json and ReportEventRecorder.csv.

Suggested sweep parameters: Base_Infectivity, Sample_Threshold, Base_Sensitivity, Base_Specificity

"""

channels = [InsetKeys.ChannelsKeys.Infected,
            "New Infections By Route (ENVIRONMENT)",
            "New Infections By Route (CONTACT)",
            "Contagion (Environment)",
            "Contagion (Contact)",
            InsetKeys.ChannelsKeys.Statistical_Population]

config_keys = [ConfigKeys.Config_Name, ConfigKeys.Simulation_Timestep,
               ConfigKeys.Simulation_Duration, ConfigKeys.Base_Infectivity,
               ConfigKeys.Demographics_Filenames,
               ConfigKeys.Run_Number]

matches = ["Update(): Time: ",
           "EnvironmentalDiagnostic tested ",
           "sample = ",
           "IP="
           ]

class ReportColumn:
    time = "Time"
    event = "NodeEventName"
    negative = "Negative_Event_Node"
    positive = "Positive_Event_Node"
    counts = "Test result counts"

class Stdout:
    test_positive = "test positive"
    test_negative = "test negative"
    test_default = "test default"
    stat_pop = "StatPop: "
    infected = "Infected: "
    sample = "Environmental sample"
    ip_value = "IP"


def load_campaign_file(campaign_filename="campaign.json", debug=False):
    """
    reads campaign file
    :param campaign_filename: campaign.json file
    :returns: campaign_obj structure, dictionary with Start_Day, etc., keys (e.g.)
    """
    with open(campaign_filename) as infile:
        cf = json.load(infile)
    campaign_obj = {}
    events = cf[CampaignKeys.Events]
    for event in events:
        print( event[CampaignKeys.Event_Coordinator_Config][CampaignKeys.Intervention_Config].keys() )
        intervention_config = None
        if "Actual_NodeIntervention_Config" in event[CampaignKeys.Event_Coordinator_Config][CampaignKeys.Intervention_Config]: 
            intervention_config = event[CampaignKeys.Event_Coordinator_Config][CampaignKeys.Intervention_Config]["Actual_NodeIntervention_Config"]
        else:
            intervention_config = event[CampaignKeys.Event_Coordinator_Config][CampaignKeys.Intervention_Config]
        intervention_class = intervention_config[CampaignKeys.class_key]
        if intervention_class == CampaignKeys.InterventionClassKeys.EnvironmentalDiagnostic:
            campaign_obj = intervention_config

    if debug:
        with open("DEBUG_campaign_object.json", 'w') as outfile:
            json.dump(campaign_obj, outfile, indent=4)

    return campaign_obj


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
    time_step = index = envi_sample = 0
    stdout_df = pd.DataFrame(columns=[ConfigKeys.Simulation_Timestep, Stdout.stat_pop, Stdout.infected,
                                      Stdout.test_positive, Stdout.test_negative, Stdout.test_default,
                                      Stdout.sample, Stdout.ip_value])
    stdout_df.index.name = 'index'
    stat_pop = infected = None
    ip_value = ""
    test_positive = test_negative = test_default = 0
    for line in filtered_lines:
        if matches[0] in line:
            stdout_df.loc[index] = [time_step, stat_pop, infected, test_positive, test_negative,
                                    test_default, envi_sample, ip_value]
            index += 1
            time_step += simulation_timestep
            infected = int(sft.get_val(Stdout.infected, line))
            stat_pop = int(sft.get_val(Stdout.stat_pop, line))
            test_positive = test_negative = test_default = 0

        elif matches[1] in line:
            test_result = sft.get_char(matches[1], line)
            if 'positive' in test_result.lower():
                test_positive += 1
            elif 'negative' in test_result.lower():
                test_negative += 1
            else:
                test_default += 1
        else:
            if matches[2] in line:
                envi_sample = float(sft.get_val(matches[2], line))
            # matches[2] and matches[3] may and may not be in the same line
            if matches[3] in line:
                # ip_value = str(sft.get_char(matches[3], line))
                 for s in line.split():
                     if matches[3] in s:
                         ip_value = s.replace(matches[3], '')


    if debug:
        res_path = r'./DEBUG_filtered_from_logging.csv'
        stdout_df.to_csv(res_path)
    return stdout_df


def parse_report_event_recorder(output_folder="output", report_event_recorder="ReportEventRecorder.csv", debug=False):
    report_df = pd.read_csv(os.path.join(output_folder, report_event_recorder))
    success = True
    try:
        filtered_df = report_df[
            [ReportColumn.time, ReportColumn.event]]
        if debug:
            with open("DEBUG_EventRecorder_dataframe.csv", "w") as outfile:
                filtered_df.to_csv(outfile, header=True)
        return filtered_df, success
    except Exception as ex:
        success = False
        print("Failed to parse report file, get exception: " + str(ex))
        return ex, success


def create_report_file(param_obj, campaign_obj, property_obj, property_df, stdout_df, report_name, debug):
    with open(report_name, "w") as outfile:
        for name, param in param_obj.items():
            outfile.write("{0} = {1}\n".format(name, param))
        success = True
        sample_threshold = float(campaign_obj[CampaignKeys.EnvironmentalDiagnosticKeys.Sample_Threshold])
        base_sensitivity = float(campaign_obj[CampaignKeys.EnvironmentalDiagnosticKeys.Base_Sensitivity])
        base_specificity = float(campaign_obj[CampaignKeys.EnvironmentalDiagnosticKeys.Base_Specificity])
        ip_key_value = campaign_obj[CampaignKeys.EnvironmentalDiagnosticKeys.Environment_IP_Key_Value]
        outfile.write("{0} = {1}\n".format(CampaignKeys.EnvironmentalDiagnosticKeys.Sample_Threshold, sample_threshold))
        outfile.write("{0} = {1}\n".format(CampaignKeys.EnvironmentalDiagnosticKeys.Base_Sensitivity, base_sensitivity))
        outfile.write("{0} = {1}\n".format(CampaignKeys.EnvironmentalDiagnosticKeys.Base_Specificity, base_specificity))
        outfile.write("{0} = {1}\n".format(CampaignKeys.EnvironmentalDiagnosticKeys.Environment_IP_Key_Value,
                                           ip_key_value))

        # making sure sample_threshold is 0
        if sample_threshold:
            success = False
            outfile.write("BAD: {0} should be 0 in this test, got {1} from compaign file. Please fix the test.\n"
                          "".format(CampaignKeys.EnvironmentalDiagnosticKeys.Sample_Threshold, sample_threshold))
        if base_specificity != 1:
            outfile.write("WARNING: the {0} is {1}, expected value is 1.\n".format(
                CampaignKeys.EnvironmentalDiagnosticKeys.Base_Specificity, base_specificity))
        if base_sensitivity != 1:
            outfile.write("WARNING: the {0} is {1}, expected value is 1.\n".format(
                CampaignKeys.EnvironmentalDiagnosticKeys.Base_Sensitivity, base_sensitivity))

        duration = param_obj[ConfigKeys.Simulation_Duration]

        positive_list = []
        negative_list = []

        infected_ip_group_list = property_df[[c for c in property_df.columns if channels[0] in c]]
        stat_pop_ip_group_list = property_df[[c for c in property_df.columns if channels[-1] in c]]

        # when both base_sensitivity and base_specificity are 1, the diagnostic is 100% accurate, so the tolerance is 0
        tolerance = 0 if base_sensitivity == 1 and base_specificity == 1 else 5e-2

        for t in range(1, duration):
            infected = stdout_df[Stdout.infected].iloc[t]
            stat_pop = stdout_df[Stdout.stat_pop].iloc[t]
            test_positive = stdout_df[Stdout.test_positive].iloc[t]
            test_negative = stdout_df[Stdout.test_negative].iloc[t]
            test_default = stdout_df[Stdout.test_default].iloc[t]

            infected_ip_group = infected_ip_group_list.iloc[t][0]
            stat_pop_ip_group = stat_pop_ip_group_list.iloc[t][0]
            if stat_pop == stat_pop_ip_group or infected == infected_ip_group:
                success = False
                outfile.write("BAD: at time step {0} the total stat_pop = {1} and total infect = {2}, we got "
                              "stat_pop_ip_group = {3} and infected_ip_group = {4} in group {5}, we expect to "
                              "see less stat_pop and infected individual in the IP group , this is not a valid test "
                              "for Environment_IP_Key_Value, please check the test condition.\n".format(t, stat_pop,
                               infected, stat_pop_ip_group, infected_ip_group, ip_key_value))

            susceptible = stat_pop_ip_group - infected_ip_group
            message = "BAD: at time {0}, group {1} has infected individuals = {2} and susceptible individuals = {3},"
            " expected {4} individuals receive a {5} test result, got {6} from logging.\n"

            # positive = real positive + false positive
            expected_test_positive = infected_ip_group * base_sensitivity + susceptible * (1.0 - base_specificity)
            if math.fabs(test_positive - expected_test_positive) > tolerance * expected_test_positive:
                success = False
                outfile.write(message.format(
                    t, ip_key_value, infected_ip_group, susceptible, expected_test_positive, "positive", test_positive))

            # negative = false negative + real negative
            expected_test_negative = infected_ip_group * (1.0 - base_sensitivity) + susceptible * base_specificity
            if math.fabs(test_negative - expected_test_negative) > tolerance * expected_test_negative:
                success = False
                outfile.write(message.format(
                    t, ip_key_value, infected_ip_group, susceptible, expected_test_negative, "negative", test_negative))

            # no test default
            expected_test_default = 0
            if test_default != expected_test_default:
                success = False
                outfile.write(message.format(
                        t, ip_key_value, infected_ip_group, susceptible, expected_test_default, "default", test_default))

            positive_list.append([test_positive, expected_test_positive])
            negative_list.append([test_negative, expected_test_negative])
        sft.plot_data(np.array(positive_list)[:, 0], np.array(positive_list)[:, 1],
                          label1="Actual",
                          label2="Expected",
                          title="Test Positive\n Group {}".format(ip_key_value), xlabel="Day",
                          ylabel="Positive count",
                          category='Test_Positive', overlap=True, alpha=0.5)
        sft.plot_data(np.array(negative_list)[:, 0], np.array(negative_list)[:, 1],
                          label1="Actual",
                          label2="Expected",
                          title="Test Negative\n Group {}".format(ip_key_value), xlabel="Day",
                          ylabel="Negative count",
                          category='Test_Negative', overlap=True, alpha=0.5)
        outfile.write(sft.format_success_msg(success))
        if debug:
            print(sft.format_success_msg(success))
        return success


def application( output_folder="output", stdout_filename="test.txt",
                 property_report_name="PropertyReportEnvironmental.json",
                 config_filename="config.json", campaign_filename="campaign.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename+ "\n")
        print("property_report_name: " + property_report_name+ "\n")
        print("config_filename: " + config_filename + "\n")
        print("campaign_filename: " + campaign_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done(stdout_filename)

    config_obj = General_Support.load_config_parameters(config_filename, config_keys, debug)
    demo_overlay_file = config_obj[ConfigKeys.Demographics_Filenames][-1]
    campaign_obj = load_campaign_file(campaign_filename, debug)

    demo_path = "Assets" if stdout_filename == "StdOut.txt" else ""
    property_obj = HINT_Support.load_demo_overlay_file(demo_overlay_file, demo_path, debug)[0]
    ip_key_value = campaign_obj[CampaignKeys.EnvironmentalDiagnosticKeys.Environment_IP_Key_Value]
    property_keys = []
    for channel in channels:
        property_keys.append("{0}:{1}".format(channel, ip_key_value))

    property_df = HINT_Support.parse_property_report_json(property_report_name, output_folder, property_keys, debug)
    stdout_df = parse_stdout_file(stdout_filename, config_obj[ConfigKeys.Simulation_Timestep], debug)
    create_report_file(config_obj, campaign_obj, property_obj, property_df, stdout_df, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-p', '--propertyreport', default="PropertyReportEnvironmental.json",
                        help="Property report to load (PropertyReportEnvironmental.json)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-C', '--campaign', default="campaign.json", help="campaign name to load (campaign.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                property_report_name=args.propertyreport,
                config_filename=args.config, campaign_filename=args.campaign,
                report_name=args.reportname, debug=args.debug)

