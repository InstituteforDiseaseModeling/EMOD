#!/usr/bin/python

if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as sft
import math
from dtk_test.dtk_General_Support import ConfigKeys, CampaignKeys
import numpy as np
import dtk_test.dtk_General_Support as General_Support
import dtk_test.dtk_HINT_Support as HINT_Support
import dtk_test.dtk_EnvironmentalDiagnostic_Support as Diagnostic_Support

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


def create_report_file(param_obj, campaign_obj, property_obj, property_df, stdout_df, recorder_obj,
                       report_name, report_event_recorder, stdout_filename, debug):
    with open(report_name, "w") as outfile:
        for name, param in param_obj.items():
            outfile.write("{0} = {1}\n".format(name, param))
        sample_threshold = float(campaign_obj[CampaignKeys.EnvironmentalDiagnosticKeys.Sample_Threshold])
        base_sensitivity = float(campaign_obj[CampaignKeys.EnvironmentalDiagnosticKeys.Base_Sensitivity])
        base_specificity = float(campaign_obj[CampaignKeys.EnvironmentalDiagnosticKeys.Base_Specificity])
        ip_key_value = campaign_obj[CampaignKeys.EnvironmentalDiagnosticKeys.Environment_IP_Key_Value]
        outfile.write("{0} = {1}\n".format(CampaignKeys.EnvironmentalDiagnosticKeys.Sample_Threshold, sample_threshold))
        outfile.write("{0} = {1}\n".format(CampaignKeys.EnvironmentalDiagnosticKeys.Base_Sensitivity, base_sensitivity))
        outfile.write("{0} = {1}\n".format(CampaignKeys.EnvironmentalDiagnosticKeys.Base_Specificity, base_specificity))
        outfile.write("{0} = {1}\n".format(CampaignKeys.EnvironmentalDiagnosticKeys.Environment_IP_Key_Value,
                                           ip_key_value))
        success = recorder_obj[1]

        if not success:
            error_message = recorder_obj[0]
            outfile.write("Failed to parse report file: {0}, get exception: {1}.\n".format(report_event_recorder,
                                                                                           error_message
                                                                                           ))
        else:
            # Throw warning messages for condition checks. Make sure all features are enabled.
            if not sample_threshold:
                outfile.write("WARNING: {0} should not be 0 in this test, got {1} from compaign file. Please fix the test.\n"
                              "".format(CampaignKeys.EnvironmentalDiagnosticKeys.Sample_Threshold, sample_threshold))
            if base_specificity == 1:
                outfile.write("WARNING: the {0} is {1}, expected value is less than 1.\n".format(
                    CampaignKeys.EnvironmentalDiagnosticKeys.Base_Specificity, base_specificity))
            if base_sensitivity == 1:
                outfile.write("WARNING: the {0} is {1}, expected value is less than 1.\n".format(
                    CampaignKeys.EnvironmentalDiagnosticKeys.Base_Sensitivity, base_sensitivity))
            if not ip_key_value:
                outfile.write(
                    "WARNING: {0} should not be empty in this test, got '{1}' from compaign file. Please fix the test.\n"
                    "".format(CampaignKeys.EnvironmentalDiagnosticKeys.Environment_IP_Key_Value, ip_key_value))

            duration = param_obj[ConfigKeys.Simulation_Duration]
            base_infectivity = param_obj[ConfigKeys.Base_Infectivity]

            positive_list = []
            negative_list = []
            positive_event_list = []
            negative_event_list = []

            # get infected and stat_pop channels for the selected IP group from property report
            infected_ip_group_list = property_df[[c for c in property_df.columns if Diagnostic_Support.channels[0] in c]]
            stat_pop_ip_group_list = property_df[[c for c in property_df.columns if Diagnostic_Support.channels[-1] in c]]

            # group by time and event name, then count how many event in each time step and put the value into a new
            # column named: "Test result counts"
            event_df = recorder_obj[0]
            event_df = event_df.groupby([Diagnostic_Support.ReportColumn.time,
                                         Diagnostic_Support.ReportColumn.event]).size().reset_index()
            event_df.rename(columns={0: Diagnostic_Support.ReportColumn.counts}, inplace=True)

            contagion_list = []
            expected_positive_count = expected_negative_count = 0
            for t in range(1, duration):
                # Test 1: make sure we get the correct contagion sample and
                # number of positive and negative results in StdOut.txt
                stdout_t_df = stdout_df[stdout_df[Diagnostic_Support.ConfigKeys.Simulation_Timestep] == t]

                #stdout_next_t_df = stdout_df[stdout_df[Diagnostic_Support.ConfigKeys.Simulation_Timestep] == t + 1]

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
                susceptible = stat_pop_ip_group - infected_ip_group
                message = "BAD: at time {0}, group {1} has infected individuals = {2} and susceptible individuals = {3}," \
                " expected {4} individuals receive a {5} test result, got {6} from logging.\n"

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
                # positive = real positive or false positive
                # negative = false negative or real negative
                if contagion > sample_threshold:
                    expected_test_positive = base_sensitivity
                    expected_test_negative = 1.0 - base_sensitivity
                else:
                    expected_test_positive = 1.0 - base_specificity
                    expected_test_negative = base_specificity

                expected_positive_count += expected_test_positive
                expected_negative_count += expected_test_negative

                # no test default in this intervention
                expected_test_default = 0
                if test_default != expected_test_default:
                    success = False
                    outfile.write(message.format(
                        t, ip_key_value, infected_ip_group, susceptible, expected_test_default,
                        "default", test_default))
                positive_list.append([test_positive, expected_test_positive])
                negative_list.append([test_negative, expected_test_negative])
                # End of Test 1 at this time step

                # Test 2: make sure events reported in ReportEventRecorder.csv and test results from StdOut.txt are matched.
                message = "BAD: at time {0}, {1} records {2} {3} events, got {4} {5} results from {5}.\n"

                # get the positive event count from data frame
                positive_event = event_df[
                    (event_df[Diagnostic_Support.ReportColumn.time] == t) &
                    (event_df[Diagnostic_Support.ReportColumn.event] == Diagnostic_Support.ReportColumn.positive)][
                    Diagnostic_Support.ReportColumn.counts].values
                if len(positive_event):
                    positive_event = positive_event[0]
                else:
                    positive_event = 0

                # StdOut.txt should match ReportEventRecorder.csv
                if test_positive != positive_event:
                    success = False
                    outfile.write(message.format(
                        t, report_event_recorder, positive_event, Diagnostic_Support.ReportColumn.positive, test_positive,
                        Diagnostic_Support.Stdout.test_positive, stdout_filename))

                # get the negative event count from data frame
                negative_event = event_df[
                    (event_df[Diagnostic_Support.ReportColumn.time] == t) &
                    (event_df[Diagnostic_Support.ReportColumn.event] == Diagnostic_Support.ReportColumn.negative)][
                    Diagnostic_Support.ReportColumn.counts].values
                if len(negative_event):
                    negative_event = negative_event[0]
                else:
                    negative_event = 0

                # StdOut.txt should match ReportEventRecorder.csv
                if test_negative != negative_event:
                    success = False
                    outfile.write(message.format(
                        t, report_event_recorder, negative_event, Diagnostic_Support.ReportColumn.negative, test_negative,
                        Diagnostic_Support.Stdout.test_negative, stdout_filename))

                positive_event_list.append(positive_event)
                negative_event_list.append(negative_event)
                # End of Test 2 at this time step

            stdout_sum = stdout_df.sum()

            result = sft.test_multinomial([stdout_sum[Diagnostic_Support.Stdout.test_positive],
                                               stdout_sum[Diagnostic_Support.Stdout.test_negative]],
                                              proportions=[expected_positive_count, expected_negative_count],
                                              report_file=outfile,
                                              prob_flag=False, )

            message = "{0}: the total test positive and negative counts from StdOut.txt are {1} and {2}, expected values" \
                      " are {3} and {4}.\n"

            if result:
                outfile.write(message.format("GOOD", stdout_sum[Diagnostic_Support.Stdout.test_positive],
                                             stdout_sum[Diagnostic_Support.Stdout.test_negative],
                                             expected_positive_count, expected_negative_count))
            else:
                success = False
                outfile.write(message.format("BAD", stdout_sum[Diagnostic_Support.Stdout.test_positive],
                                             stdout_sum[Diagnostic_Support.Stdout.test_negative],
                                             expected_positive_count, expected_negative_count))


            # these two plots are replaced with the scatter with fit line plots
            # sft.plot_data(np.array(positive_list)[:, 0], np.array(positive_list)[:, 1],
            #                   label1="Actual",
            #                   label2="Probability of Positive",
            #                   title="Test Positive\n Group {}".format(ip_key_value), xlabel="Day",
            #                   ylabel="Positive count",
            #                   category='Test_Positive_Probability', overlap=False)
            # sft.plot_data(np.array(negative_list)[:, 0], np.array(negative_list)[:, 1],
            #                   label1="Actual",
            #                   label2="Probability of Negative",
            #                   title="Test Negative\n Group {}".format(ip_key_value), xlabel="Day",
            #                   ylabel="Negative count",
            #                   category='Test_Negative_Probability', overlap=False)

            sft.plot_scatter_fit_line(np.array(positive_list)[:, 0], dist2=np.array(positive_list)[:, 1],
                                          label1="Actual", label2="Probability of Positive",
                                          title="Test Positive\n Group {}".format(ip_key_value),
                                          xlabel="Day",
                                          ylabel="Positive count",
                                          category='Test_Positive_Probability_Scatter_Fit_Line')

            sft.plot_scatter_fit_line(np.array(negative_list)[:, 0], dist2=np.array(negative_list)[:, 1],
                                          label1="Actual", label2="Probability of Negative",
                                          title="Test Negative\n Group {}".format(ip_key_value),
                                          xlabel="Day",
                                          ylabel="Negative count",
                                          category='Test_Negative_Probability_Scatter_Fit_Line')

            sft.plot_data(np.array(positive_list)[:, 0], positive_event_list,
                              label1=stdout_filename,
                              label2=report_event_recorder,
                              title="Test Positive\n Group {}".format(ip_key_value), xlabel="Day",
                              ylabel="Positive count",
                              category='Test_Positive_stdout_vs_event_recorder', overlap=True, alpha=0.5)
            sft.plot_data(np.array(negative_list)[:, 0], negative_event_list,
                              label1=stdout_filename,
                              label2=report_event_recorder,
                              title="Test Negative\n Group {}".format(ip_key_value), xlabel="Day",
                              ylabel="Negative count",
                              category='Test_Negative_stdout_vs_event_recorder', overlap=True, alpha=0.5)
            sft.plot_data(stdout_df[Diagnostic_Support.Stdout.sample].tolist()[1:], contagion_list,
                              label1="Actual",
                              label2="Expected",
                              title="Environmental_Contagion", xlabel="Day",
                              ylabel="Environmental_Contagion",
                              category='Environmental_Contagion', overlap=True, alpha=0.5)
        outfile.write(sft.format_success_msg(success))
        if debug:
            print(sft.format_success_msg(success))
        return success


def application( output_folder="output", stdout_filename="test.txt",
                 property_report_name="PropertyReportEnvironmental.json",
                 report_event_recorder="ReportNodeEventRecorder.csv",
                 config_filename="config.json", campaign_filename="campaign.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename+ "\n")
        print("property_report_name: " + property_report_name+ "\n")
        print("report_event_recorder: " + report_event_recorder+ "\n")
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
    recorder_obj = Diagnostic_Support.parse_report_event_recorder(output_folder, report_event_recorder, debug)
    create_report_file(config_obj, campaign_obj, property_obj, property_df, stdout_df, recorder_obj, report_name,
                       report_event_recorder, stdout_filename, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-p', '--propertyreport', default="PropertyReportEnvironmental.json",
                        help="Property report to load (PropertyReportEnvironmental.json)")
    parser.add_argument('-R', '--report_event_recorder', default="ReportNodeEventRecorder.csv",
                        help="Name of ReportEventRecorder to parse (ReportNodeEventRecorder.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-C', '--campaign', default="campaign.json", help="campaign name to load (campaign.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                property_report_name=args.propertyreport,
                report_event_recorder=args.report_event_recorder,
                config_filename=args.config, campaign_filename=args.campaign,
                report_name=args.reportname, debug=args.debug)

