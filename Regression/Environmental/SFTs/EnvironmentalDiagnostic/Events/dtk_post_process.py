#!/usr/bin/python

if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as sft
from dtk_test.dtk_General_Support import ConfigKeys, CampaignKeys
import numpy as np
import dtk_test.dtk_General_Support as General_Support
import dtk_test.dtk_EnvironmentalDiagnostic_Support as Diagnostic_Support

"""

This SFT is testing environmental feature 2.6 Interventions EnvironmentalDiagnostic: Positive_Diagnostic_Event and
Negative_Diagnostic_Event.

"Positive_Diagnostic_Event
    If the sample was greater than the threshold, this node-level event will be broadcast for this node.
    This cannot be empty string or NoTrigger.
Negative_Diagnostic_Event
    If the sample was less than or equal to the threshold, this node-level event will be broadcast for this node.
    If the string is empty or NoTrigger, then no event will be distributed."

In this test we turn off other features in EnvironmentalDiagnostic and only look at Positive_Diagnostic_Event and
Negative_Diagnostic_Event.

Test data is loaded from StdOut.txt and ReportEventRecorder.csv.

Suggested sweep parameters: Base_Infectivity

"""


def create_report_file(param_obj, campaign_obj, stdout_df, recorder_obj, report_name, report_event_recorder, debug):
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
            # the following condition checks throw warning messages instead of fail the test.
            if sample_threshold:
                outfile.write("WARNING: {0} should be 0 in this test, got {1} from compaign file. Please fix the test.\n"
                              "".format(CampaignKeys.EnvironmentalDiagnosticKeys.Sample_Threshold, sample_threshold))
            if base_specificity != 1:
                outfile.write("WARNING: the {0} is {1}, expected value is 1.\n".format(
                    CampaignKeys.EnvironmentalDiagnosticKeys.Base_Specificity, base_specificity))
            if base_sensitivity != 1:
                outfile.write("WARNING: the {0} is {1}, expected value is 1.\n".format(
                    CampaignKeys.EnvironmentalDiagnosticKeys.Base_Sensitivity, base_sensitivity))
            if ip_key_value:
                outfile.write(
                    "WARNING: {0} should be empty in this test, got {1} from compaign file. Please fix the test.\n"
                    "".format(CampaignKeys.EnvironmentalDiagnosticKeys.Environment_IP_Key_Value, ip_key_value))

            # group by time and event name, then count how many event in each time step and put the value into a new
            # column named: "Test result counts"
            event_df = recorder_obj[0]
            event_df = event_df.groupby([Diagnostic_Support.ReportColumn.time, Diagnostic_Support.ReportColumn.event]).size().reset_index()
            event_df.rename(columns={0: Diagnostic_Support.ReportColumn.counts}, inplace=True)

            #event_df[ReportColumn.counts] = event_df.groupby(ReportColumn.time)[ReportColumn.event].transform('count')

            duration = param_obj[ConfigKeys.Simulation_Duration]
            positive_list = []
            negative_list = []
            for t in range(1, duration):
                # get values of how many test positive and test negative for each time step from stdout file
                stdout_t_df = stdout_df[stdout_df[Diagnostic_Support.ConfigKeys.Simulation_Timestep] == t]
                test_positive = stdout_t_df[Diagnostic_Support.Stdout.test_positive].iloc[0]
                test_negative = stdout_t_df[Diagnostic_Support.Stdout.test_negative].iloc[0]

                message = "BAD: at time {0}, {1} records {2} {3}, got {4} {5} result from logging.\n"

                # get the positive event count from data frame
                positive_event = event_df[
                    (event_df[Diagnostic_Support.ReportColumn.time] == t) &
                    (event_df[Diagnostic_Support.ReportColumn.event] == Diagnostic_Support.ReportColumn.positive)] \
                [Diagnostic_Support.ReportColumn.counts].values
                if len(positive_event):
                    positive_event = positive_event[0]
                else:
                    positive_event = 0

                # StdOut.txt should match ReportEventRecorder.csv
                if test_positive != positive_event:
                    success = False
                    outfile.write(message.format(
                        t, report_event_recorder, positive_event, Diagnostic_Support.ReportColumn.positive, test_positive,
                        Diagnostic_Support.Stdout.test_positive))

                # get the negative event count from data frame
                negative_event = event_df[(event_df[Diagnostic_Support.ReportColumn.time] == t) &
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
                        Diagnostic_Support.Stdout.test_negative))

                positive_list.append([test_positive, positive_event])
                negative_list.append([test_negative, negative_event])
            sft.plot_data(np.array(positive_list)[:, 0], np.array(positive_list)[:, 1],
                              label1="Stdout",
                              label2=report_event_recorder,
                              title="Test Positive vs. {}".format(Diagnostic_Support.ReportColumn.positive),
                              xlabel="Day",
                              ylabel="Positive count",
                              category='Test_Positive', overlap=True, alpha=0.5)
            sft.plot_data(np.array(negative_list)[:, 0], np.array(negative_list)[:, 1],
                              label1="Stdout",
                              label2=report_event_recorder,
                              title="Test Negative vs. {}".format(Diagnostic_Support.ReportColumn.negative),
                              xlabel="Day",
                              ylabel="Negative count",
                              category='Test_Negative', overlap=True, alpha=0.5)
        outfile.write(sft.format_success_msg(success))
        if debug:
            print(sft.format_success_msg(success))
        return success


def application( output_folder="output", stdout_filename="test.txt", report_event_recorder="ReportNodeEventRecorder.csv",
                 config_filename="config.json", campaign_filename="campaign.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename+ "\n")
        print("report_event_recorder: " + report_event_recorder+ "\n")
        print("config_filename: " + config_filename + "\n")
        print("campaign_filename: " + campaign_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done(stdout_filename)

    config_obj = General_Support.load_config_parameters(config_filename, Diagnostic_Support.config_keys, debug)
    campaign_obj = Diagnostic_Support.load_campaign_file(campaign_filename, debug)
    stdout_df = Diagnostic_Support.parse_stdout_file(stdout_filename, config_obj[ConfigKeys.Simulation_Timestep], debug)
    recorder_obj = Diagnostic_Support.parse_report_event_recorder(output_folder, report_event_recorder, debug)
    create_report_file(config_obj, campaign_obj, stdout_df, recorder_obj, report_name, report_event_recorder, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-R', '--report_event_recorder', default="ReportNodeEventRecorder.csv",
                        help="Name of ReportEventRecorder to parse (ReportNodeEventRecorder.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-C', '--campaign', default="campaign.json", help="campaign name to load (campaign.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                report_event_recorder = args.report_event_recorder,
                config_filename=args.config, campaign_filename=args.campaign,
                report_name=args.reportname, debug=args.debug)

