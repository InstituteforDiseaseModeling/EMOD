#!/usr/bin/python

if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as sft
import math
from dtk_test.dtk_General_Support import ConfigKeys, CampaignKeys
import numpy as np
import dtk_test.dtk_General_Support as General_Support
import dtk_test.dtk_HINT_Support as HINT_Support
import dtk_test.dtk_EnvironmentalDiagnostic_Support as Diagnostic_Support

"""

This SFT is testing environmental feature 2.6 Interventions EnvironmentalDiagnostic: Environment_IP_Key_Value

"If provided, this individual property key-value pair will indicate a specific transmission “pool”. 
Typically, this will be used to identify a location.
If not provided or empty, then the sample is to be expected for the entire node.
This is an IP key-value pair such that we have “key:value”."

In this test we turn off other features in EnvironmentalDiagnostic and only look at Environment_IP_Key_Value.

Test data is loaded from StdOut.txt and PropertyReportEnvironmental.json.

Suggested sweep parameters: Base_Infectivity

"""


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

        # making sure sample_threshold is 0, base_specificity and base_sensitivity both are 1.
        if sample_threshold:
            success = False
            outfile.write("BAD: {0} should be 0 in this test, got {1} from compaign file. Please fix the test.\n"
                          "".format(CampaignKeys.EnvironmentalDiagnosticKeys.Sample_Threshold, sample_threshold))
        if base_specificity != 1:
            success = False
            outfile.write("BAD: the {0} is {1}, expected value is 1.\n".format(
                CampaignKeys.EnvironmentalDiagnosticKeys.Base_Specificity, base_specificity))
        if base_sensitivity != 1:
            success = False
            outfile.write("BAD: the {0} is {1}, expected value is 1.\n".format(
                CampaignKeys.EnvironmentalDiagnosticKeys.Base_Sensitivity, base_sensitivity))

        duration = param_obj[ConfigKeys.Simulation_Duration]
        base_infectivity = param_obj[ConfigKeys.Base_Infectivity]

        positive_list = []
        negative_list = []

        infected_ip_group_list = property_df[[c for c in property_df.columns if Diagnostic_Support.channels[0] in c]]
        stat_pop_ip_group_list = property_df[[c for c in property_df.columns if Diagnostic_Support.channels[-1] in c]]

        # when both base_sensitivity and base_specificity are 1, the diagnostic is 100% accurate, so the tolerance is 0
        # tolerance = 0 if base_sensitivity == 1 and base_specificity == 1 else 5e-2

        contagion_list = []
        contagion = 0
        for t in range(1, duration):
            stdout_t_df = stdout_df[stdout_df[Diagnostic_Support.ConfigKeys.Simulation_Timestep] == t]
            infected = stdout_t_df[Diagnostic_Support.Stdout.infected].iloc[0]
            stat_pop = stdout_t_df[Diagnostic_Support.Stdout.stat_pop].iloc[0]
            test_positive = stdout_t_df[Diagnostic_Support.Stdout.test_positive].iloc[0]
            test_negative = stdout_t_df[Diagnostic_Support.Stdout.test_negative].iloc[0]
            test_default = stdout_t_df[Diagnostic_Support.Stdout.test_default].iloc[0]
            envi_sample = stdout_t_df[Diagnostic_Support.Stdout.sample].iloc[0]
            ip = stdout_t_df[Diagnostic_Support.Stdout.ip_value].iloc[0]

            infected_ip_group = infected_ip_group_list.iloc[t - 1][0]
            stat_pop_ip_group = stat_pop_ip_group_list.iloc[t - 1][0]
            if stat_pop == stat_pop_ip_group and infected == infected_ip_group:
                success = False
                outfile.write("BAD: at time step {0} the total stat_pop = {1} and total infect = {2}, we got "
                              "stat_pop_ip_group = {3} and infected_ip_group = {4} in group {5}, we expect to "
                              "see less stat_pop and infected individual in the IP group , this is not a valid test "
                              "for Environment_IP_Key_Value, please check the test condition.\n".format(t, stat_pop,
                               infected, stat_pop_ip_group, infected_ip_group, ip_key_value))
            if ip_key_value != ip:
                success = False
                outfile.write("BAD: at time step {0}, IP={1} from StdOut.txt, expected IP={2}.\n".format(
                    t, ip, ip_key_value))

            message = "BAD: at time step {0}, total infected individuals = {1} and total susceptible individuals = {2}," \
            " expected {3} {4} test result, got {5} from logging.\n"
            susceptible = stat_pop_ip_group - infected_ip_group

            # calculated environmental contagion
            contagion = base_infectivity * infected_ip_group / stat_pop_ip_group
            contagion_list.append(contagion)
            if math.fabs(contagion - envi_sample) > envi_sample * 1e-2:
                success = False
                outfile.write(
                    "BAD: at time step {0} the environmental sample for IP group {1} is {2}, expected value is {3}"
                    ".\n".format(
                        t, ip_key_value, envi_sample, contagion
                    ))
            if contagion > sample_threshold:
                expected_test_positive = 1
                expected_test_negative = 0
            else:
                expected_test_positive = 0
                expected_test_negative = 1

            # tolerance is 0 in this test, so test result should exactly match expected value.
            if test_positive != expected_test_positive:
                success = False
                outfile.write(message.format(
                    t, infected_ip_group, susceptible, expected_test_positive, "positive", test_positive))
            if test_negative != expected_test_negative:
                success = False
                outfile.write(message.format(
                    t, infected_ip_group, susceptible, expected_test_negative, "negative", test_negative))

            expected_test_default = 0
            if test_default != expected_test_default:
                success = False
                outfile.write(message.format(
                    t, infected_ip_group, susceptible, expected_test_default, 0, "default", test_default))

            positive_list.append([test_positive, expected_test_positive])
            negative_list.append([test_negative, expected_test_negative])

        sft.plot_data(stdout_df[Diagnostic_Support.Stdout.sample].tolist()[1:], contagion_list,
                          label1="Actual",
                          label2="Expected",
                          title="Environmental_Contagion", xlabel="Day",
                          ylabel="Environmental_Contagion",
                          category='Environmental_Contagion', overlap=True, alpha=0.5)

        sft.plot_data(np.array(positive_list)[:, 0], np.array(positive_list)[:, 1],
                          label1="Actual",
                          label2="Expected",
                          title="Test Positive\n infectivity = {0}, threshold = {1}".format(
                              base_infectivity, sample_threshold), xlabel="Day",
                          ylabel="Positive count",
                          category='Test_Positive', overlap=True, alpha=0.5)
        sft.plot_data(np.array(negative_list)[:, 0], np.array(negative_list)[:, 1],
                          label1="Actual",
                          label2="Expected",
                          title="Test Negative\n infectivity = {0}, threshold = {1}".format(
                              base_infectivity, sample_threshold), xlabel="Day",
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

    config_obj = General_Support.load_config_parameters(config_filename, Diagnostic_Support.config_keys, debug)
    demo_overlay_file = config_obj[ConfigKeys.Demographics_Filenames][-1]
    campaign_obj = Diagnostic_Support.load_campaign_file(campaign_filename, debug)

    demo_path = "Assets" if stdout_filename == "StdOut.txt" else ""
    property_obj = HINT_Support.load_demo_mr_overlay_file(demo_overlay_file, demo_path, debug)[0]
    ip_key_value = campaign_obj[CampaignKeys.EnvironmentalDiagnosticKeys.Environment_IP_Key_Value]
    property_keys = []
    for channel in Diagnostic_Support.channels:
        property_keys.append("{0}:{1}".format(channel, ip_key_value))

    property_df = HINT_Support.parse_property_report_json(property_report_name, output_folder, property_keys, debug)
    stdout_df = Diagnostic_Support.parse_stdout_file(stdout_filename, config_obj[ConfigKeys.Simulation_Timestep], debug)
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

