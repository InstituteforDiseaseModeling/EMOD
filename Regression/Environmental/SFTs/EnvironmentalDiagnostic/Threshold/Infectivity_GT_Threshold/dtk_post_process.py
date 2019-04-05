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
import dtk_test.dtk_EnvironmentalDiagnostic_Support as Diagnostic_Support

"""

This SFT is testing environmental feature 2.6 Interventions EnvironmentalDiagnostic: Sample_Threshold

In this test we have Base_Infectivity > Sample_Threshold

"If the sample is greater than (NOT equal to) this threshold, then a positive finding will have been found. This does 
not include values equal to the threshold so that the threshold can be set at zero. If the value is zero, the user is 
just looking for any deposit in the transmission pool.
Range: 0 to FLT_MAX
Default = 0"

In this test we turn off other features in EnvironmentalDiagnostic and only look at Sample_Threshold.

Test data is loaded from StdOut.txt. 

Suggested sweep parameters: Sample_Threshold and Base_Infectivity

"""


def create_report_file(param_obj, campaign_obj, stdout_df, report_name, debug):
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

        if base_sensitivity != 1:
            success = False
            outfile.write("BAD: the {0} is {1}, expected value is 1.\n".format(
                CampaignKeys.EnvironmentalDiagnosticKeys.Base_Sensitivity, base_sensitivity))
        if base_specificity != 1:
            success = False
            outfile.write("BAD: the {0} is {1}, expected value is 1.\n".format(
                CampaignKeys.EnvironmentalDiagnosticKeys.Base_Specificity, base_specificity))
        if ip_key_value:
            success = False
            outfile.write("BAD: {0} should be empty in this test, got {1} from compaign file. Please fix the test.\n"
                          "".format(CampaignKeys.EnvironmentalDiagnosticKeys.Environment_IP_Key_Value, ip_key_value))

        duration = param_obj[ConfigKeys.Simulation_Duration]
        base_infectivity = param_obj[ConfigKeys.Base_Infectivity]

        if base_infectivity <= sample_threshold:
            outfile.write("WARNING: {0}({1}) is not greater than {2}({3}), please check the test.\n".format(
                ConfigKeys.Base_Infectivity, base_infectivity,
                CampaignKeys.EnvironmentalDiagnosticKeys.Sample_Threshold, sample_threshold
            ))

        positive_list = []
        negative_list = []
        # tolerance is 0 in this test.
        tolerance = 0 if base_sensitivity == 1 and base_specificity == 1 else 5e-2
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

            susceptible = stat_pop - infected
            message = "BAD: at time {0}, total infected individuals = {1} and total susceptible individuals = {2}," \
            " expected {3} {4} test result, got {5} from logging.\n"

            # calculated environmental contagion
            contagion = base_infectivity * infected /stat_pop
            if math.fabs(contagion - envi_sample) > envi_sample * 1e-2:
                success = False
                outfile.write("BAD: at time step {0} the environmental sample is {1}, expected value is {2}.\n".format(
                    t, envi_sample, contagion
                ))
            if contagion > sample_threshold:
                expected_test_positive = 1
                expected_test_negative = 0
            else:
                expected_test_positive = 0
                expected_test_negative = 1

            contagion_list.append(contagion)

            # tolerance is 0 in this test, so we are looking for a 100% match in this test.
            if math.fabs(test_positive - expected_test_positive) > tolerance * expected_test_positive:
                success = False
                outfile.write(message.format(
                    t, infected, susceptible, expected_test_positive, "positive", test_positive))
            if math.fabs(test_negative - expected_test_negative) > tolerance * expected_test_negative:
                success = False
                outfile.write(message.format(
                    t, infected, susceptible, expected_test_negative, "negative", test_negative))

            expected_test_default = 0
            if test_default != expected_test_default:
                success = False
                outfile.write(message.format(
                        t, infected, susceptible, expected_test_default, "default", test_default))

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
                 config_filename="config.json", campaign_filename="campaign.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename+ "\n")
        print("config_filename: " + config_filename + "\n")
        print("campaign_filename: " + campaign_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done(stdout_filename)

    config_obj = General_Support.load_config_parameters(config_filename, Diagnostic_Support.config_keys, debug)
    campaign_obj = Diagnostic_Support.load_campaign_file(campaign_filename, debug)
    stdout_df = Diagnostic_Support.parse_stdout_file(stdout_filename, config_obj[ConfigKeys.Simulation_Timestep], debug)
    create_report_file(config_obj, campaign_obj, stdout_df, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-C', '--campaign', default="campaign.json", help="campaign name to load (campaign.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                config_filename=args.config, campaign_filename=args.campaign,
                report_name=args.reportname, debug=args.debug)

