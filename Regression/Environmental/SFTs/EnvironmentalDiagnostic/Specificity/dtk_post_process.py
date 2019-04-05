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
import dtk_test.dtk_General_Support as General_Support
import dtk_test.dtk_EnvironmentalDiagnostic_Support as Diagnostic_Support
import json

"""

This SFT is testing environmental feature 2.6 Interventions EnvironmentalDiagnostic: Base_Specificity

"Modified from SimpleDiagnostic - This sets the proportion of the time that the environment without the condition being 
tested receive a negative diagnostic test. When set to 1, the diagnostic always accurately reflects the lack of having 
the condition. When set to zero, then environments that do not have the condition always receive a false-positive 
diagnostic test.
1.0 => no false positives, 0 => positive result always returned whether it is true or not
Range: 0 to 1
Default = 1"

In this test we turn off other features in EnvironmentalDiagnostic and only look at Base_Specificity.

Test data is loaded from StdOut.txt. 

Suggested sweep parameters: Base_Specificity

"""


# can't use the load_campaign_file() method is the dtk_EnvironmentalDiagnostic_Support library,
# create one for Specificity test.
def load_campaign_file(campaign_filename="campaign.json", debug=False):
    """
    reads campaign file
    :param campaign_filename: campaign.json file
    :returns: intervention_obj, outbreak_obj
    """
    with open(campaign_filename) as infile:
        cf = json.load(infile)
    intervention_obj = {}
    outbreak_obj = {}
    events = cf[CampaignKeys.Events]
    for event in events:
        intervention_config = event[CampaignKeys.Event_Coordinator_Config][CampaignKeys.Intervention_Config]
        intervention_class = intervention_config[CampaignKeys.class_key]
        if intervention_class == CampaignKeys.InterventionClassKeys.EnvironmentalDiagnostic:
            intervention_obj = intervention_config
        elif intervention_class == CampaignKeys.InterventionClassKeys.OutbreakIndividual:
            outbreak_obj = intervention_config

    if debug:
        with open("DEBUG_campaign_object.json", 'w') as outfile:
            json.dump(intervention_obj, outfile, indent=4)

    return intervention_obj, outbreak_obj


def create_report_file(param_obj, intervention_obj, outbreak_obj, stdout_df, report_name, debug):
    with open(report_name, "w") as outfile:
        for name, param in param_obj.items():
            outfile.write("{0} = {1}\n".format(name, param))
        success = True
        sample_threshold = float(intervention_obj[CampaignKeys.EnvironmentalDiagnosticKeys.Sample_Threshold])
        base_sensitivity = float(intervention_obj[CampaignKeys.EnvironmentalDiagnosticKeys.Base_Sensitivity])
        base_specificity = float(intervention_obj[CampaignKeys.EnvironmentalDiagnosticKeys.Base_Specificity])
        ip_key_value = intervention_obj[CampaignKeys.EnvironmentalDiagnosticKeys.Environment_IP_Key_Value]
        outfile.write("{0} = {1}\n".format(CampaignKeys.EnvironmentalDiagnosticKeys.Sample_Threshold, sample_threshold))
        outfile.write("{0} = {1}\n".format(CampaignKeys.EnvironmentalDiagnosticKeys.Base_Sensitivity, base_sensitivity))
        outfile.write("{0} = {1}\n".format(CampaignKeys.EnvironmentalDiagnosticKeys.Base_Specificity, base_specificity))
        outfile.write("{0} = {1}\n".format(CampaignKeys.EnvironmentalDiagnosticKeys.Environment_IP_Key_Value,
                                           ip_key_value))

        # make sure no outbreak in this test, so the sample should not be greater than threshold at all time
        if outbreak_obj:
            success = False
            outfile.write("BAD: Campaign.json should not have {0}, got {1} from the json file.\n".format(
                CampaignKeys.InterventionClassKeys.OutbreakIndividual, outbreak_obj
            ))
        if sample_threshold:
            outfile.write("WARNING: {0} should be 0 in this test, got {1} from compaign file. Please fix the test.\n"
                          "".format(CampaignKeys.EnvironmentalDiagnosticKeys.Sample_Threshold, sample_threshold))
        if base_sensitivity != 1:
            outfile.write("WARNING: the {0} is {1}, expected value is 1.\n".format(
                CampaignKeys.EnvironmentalDiagnosticKeys.Base_Sensitivity, base_sensitivity))
        if ip_key_value:
            success = False
            outfile.write("BAD: {0} should be empty in this test, got {1} from compaign file. Please fix the test.\n"
                          "".format(CampaignKeys.EnvironmentalDiagnosticKeys.Environment_IP_Key_Value, ip_key_value))

        duration = param_obj[ConfigKeys.Simulation_Duration]
        base_infectivity = param_obj[ConfigKeys.Base_Infectivity]
        expected_positive_count = expected_negative_count = 0
        contagion_list = []
        contagion = 0
        for t in range(1, duration):
            stdout_t_df = stdout_df[stdout_df[Diagnostic_Support.ConfigKeys.Simulation_Timestep] == t]
            infected = stdout_t_df[Diagnostic_Support.Stdout.infected].iloc[0]
            stat_pop = stdout_t_df[Diagnostic_Support.Stdout.stat_pop].iloc[0]
            envi_sample = stdout_t_df[Diagnostic_Support.Stdout.sample].iloc[0]

            # calculated environmental contagion
            contagion = base_infectivity * infected /stat_pop
            if math.fabs(contagion - envi_sample) > envi_sample * 1e-2:
                success = False
                outfile.write("BAD: at time step {0} the environmental sample is {1}, expected value is {2}.\n".format(
                    t, envi_sample, contagion
                ))
            if contagion > sample_threshold:
                expected_test_positive = base_sensitivity
                expected_test_negative = 1.0 - base_sensitivity
            else:
                expected_test_positive = 1.0 - base_specificity
                expected_test_negative = base_specificity
            contagion_list.append(contagion)

            expected_positive_count += expected_test_positive
            expected_negative_count += expected_test_negative

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
    intervention_obj, outbreak_obj = load_campaign_file(campaign_filename, debug)
    stdout_df = Diagnostic_Support.parse_stdout_file(stdout_filename, config_obj[ConfigKeys.Simulation_Timestep], debug)
    create_report_file(config_obj, intervention_obj, outbreak_obj, stdout_df, report_name, debug)


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

