#!/usr/bin/python

import json
import dtk_sft
import os
import pandas as pd
import csv

"""
Diagnostics_MDR:
What we are testing is three outcomes Positive Event, Negative Event, or Dropout Event
(all of which can be user specified in campaign).

Positive proportion  = Test Sensitivity * Treatment_Proportion
Negative Proportion = (1-Test_Sensitivity) * Treatment_Porportion
Dropout = 1- Treatment_Proportion.
"""
# campaign parameter
KEY_BASE_SENSITIVITY = "Base_Sensitivity"
KEY_TREATMENT_FRACTION = "Treatment_Fraction"
KEY_TREATMENT_FRACTION_NEGATIVE_DIAGNOSIS = "Treatment_Fraction_Negative_Diagnosis"
KEY_MDR_POSITIVE = "TBMDRTestPositive"
KEY_MDR_NEGATIVE = "TBMDRTestNegative"
KEY_MDR_DEFAULT = "TBMDRTestDefault"
# config parameter
KEY_CONFIG_NAME = "Config_Name"
KEY_SIMULATION_TIMESTEP = "Simulation_Timestep"
# reporter constant
KEY_REPORTER_TIME = "Time"
KEY_REPORTER_EVENT_NAME = "Event_Name"
# other constant
KEY_INFECTED = "infected"
KEY_STAT_POP = "Stat Pop"

matches = ["Update(): Time: ",
           "StatPop: ",
           "Infected: ",
           "MDR test result is "
           ]

def load_emod_parameters(config_filename="config.json", debug=False):
    """reads config file and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_CONFIG_NAME, etc., keys (e.g.)
    """
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {}
    param_obj[KEY_CONFIG_NAME] = cdj[KEY_CONFIG_NAME]
    if debug:
        with open("DEBUG_param_object.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj

def load_campaign_file(campaign_filename="campaign.json", debug=False):
    """reads campaign file

    :param campaign_filename: campaign.json file
    :returns: campaign_obj structure, dictionary with KEY_BASE_SENSITIVITY, KEY_TREATMENT_FRACTION, etc., keys (e.g.)
    """
    with open(campaign_filename) as infile:
        cf = json.load(infile)
    campaign_obj = {}
    base_sensitivity = cf["Events"][1]["Event_Coordinator_Config"]["Intervention_Config"]["Actual_IndividualIntervention_Config"][KEY_BASE_SENSITIVITY]
    treatment_fraction = cf["Events"][1]["Event_Coordinator_Config"]["Intervention_Config"]["Actual_IndividualIntervention_Config"][KEY_TREATMENT_FRACTION]
    treatment_fraction_negative_diagnosis = cf["Events"][1]["Event_Coordinator_Config"]["Intervention_Config"]["Actual_IndividualIntervention_Config"][KEY_TREATMENT_FRACTION_NEGATIVE_DIAGNOSIS]
    campaign_obj[KEY_BASE_SENSITIVITY] = base_sensitivity
    campaign_obj[KEY_TREATMENT_FRACTION] = treatment_fraction
    campaign_obj[KEY_TREATMENT_FRACTION_NEGATIVE_DIAGNOSIS] = treatment_fraction_negative_diagnosis

    if debug:
        with open("DEBUG_campaign_object.json", 'w') as outfile:
            json.dump(campaign_obj, outfile, indent=4)

    return campaign_obj

def parse_output_file(output_filename="test.txt", debug=False):
    """
    creates a dictionary to store stat populations, infected population, and MDR test result for each time step
    :param output_filename: file to parse (test.txt)
    :return:                output_dict
    """
    filtered_lines = []
    with open(output_filename) as logfile:
        for line in logfile:
            if dtk_sft.has_match(line,matches):
                filtered_lines.append(line)
    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    # initialize variables
    time_step = 0
    infected = 0
    statpop = 0
    simulation_timestep = 1
    mdr_positive = 0
    mdr_negative = 0
    output_dict = {}
    for line in filtered_lines:
        if matches[0] in line:
            output_dict[time_step] = {KEY_STAT_POP:statpop, KEY_INFECTED:infected, KEY_MDR_POSITIVE:mdr_positive,
                                 KEY_MDR_NEGATIVE:mdr_negative}
            infected = dtk_sft.get_val(matches[2], line)
            statpop = dtk_sft.get_val(matches[1], line)
            time_step += simulation_timestep
            mdr_positive = 0
            mdr_negative = 0
        if matches[3] in line:
            mdr_test_result = dtk_sft.get_val(matches[3], line)
            if int(mdr_test_result)==1:
                mdr_positive += 1
            else:
                mdr_negative += 1
    res_path = r'./mdr_test_result_from_logging.json'
    with open(res_path, "w") as file:
        json.dump(output_dict, file, indent=4)
    return output_dict

def parse_report_file(output_folder="output", report_filename="ReportEventRecorder.csv", debug=False):
    """
    creates a dictionary to store MDR test result for each time step
    :param report_filename: file to parse (ReportEventRecorder.csv)
    :return:                report_dict
    """
    report_dict = {}
    with open(os.path.join(output_folder, report_filename), newline='') as csvfile:
        reader = csv.reader(csvfile)
        header = next(reader)
        column = {}
        for h in header:
            column[h] = []
        for line in reader:
            for h, value in zip(header, line):
                column[h].append(value)
    for i in range(len(column[KEY_REPORTER_TIME])):
        t = column[KEY_REPORTER_TIME][i]
        test_result = str(column[KEY_REPORTER_EVENT_NAME][i])
        if t not in report_dict:
            report_dict[t] = {KEY_MDR_POSITIVE: 0, KEY_MDR_NEGATIVE: 0, KEY_MDR_DEFAULT: 0}
        if test_result == KEY_MDR_POSITIVE:
            report_dict[t][KEY_MDR_POSITIVE] += 1
        elif test_result == KEY_MDR_NEGATIVE:
            report_dict[t][KEY_MDR_NEGATIVE] += 1
        else:
            report_dict[t][KEY_MDR_DEFAULT] += 1
    res_path = r'./mdr_test_result_from_report.json'
    with open(res_path, "w") as file:
        json.dump(report_dict, file, indent=4)
    return report_dict

def create_report_file(param_obj, campaign_obj, output_dict, report_dict, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[KEY_CONFIG_NAME]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        sensitivity = campaign_obj[KEY_BASE_SENSITIVITY]
        treatment_fraction = campaign_obj[KEY_TREATMENT_FRACTION]
        treatment_fraction_negative_diagnosis = campaign_obj[KEY_TREATMENT_FRACTION_NEGATIVE_DIAGNOSIS]
        proportions = [sensitivity * treatment_fraction, (1.0 - sensitivity) * treatment_fraction_negative_diagnosis,
                       (1.0 - sensitivity) * (1.0 - treatment_fraction_negative_diagnosis) + sensitivity* (1.0 - treatment_fraction)]
        total_proportion = sum(proportions)
        positive = []
        negative = []
        default = []
        total = []
        for t in report_dict:
            value_to_test = [report_dict[t][KEY_MDR_POSITIVE], report_dict[t][KEY_MDR_NEGATIVE], report_dict[t][KEY_MDR_DEFAULT]]
            positive.append(value_to_test[0])
            negative.append(value_to_test[1])
            default.append(value_to_test[2])
            total.append(int(sum(value_to_test)/total_proportion))
            outfile.write("Run Chi-squared test at time step {}.\n".format(t))
            result = dtk_sft.test_multinomial(dist=value_to_test, proportions=proportions, report_file=outfile)
            if not result:
                success = False
                outfile.write(
                    "BAD: At timestep {0}, the Chi-squared test failed.\n".format(t))

        dtk_sft.plot_data(positive, dist2=total, label1="TBMDRTestPositive", label2="Total tested",
                                   title="MDR Test positive vs. total, positive proportion = {}".format(
                                       proportions[0]),
                                   xlabel="time step", ylabel="# of individuals", category='MDR_Test_positive_vs_total',
                                   show=True, line=False)
        dtk_sft.plot_data(negative, dist2=total, label1="TBMDRTestNegative", label2="Total tested",
                                   title="MDR Test negative vs. total, negative proportion = {}".format(
                                       proportions[1]),
                                   xlabel="time step", ylabel="# of individuals", category='MDR_Test_negative_vs_total',
                                   show=True, line=False)
        dtk_sft.plot_data(default, dist2=total, label1="TBMDRTestDefault", label2="Total tested",
                                   title="MDR Test default vs. total, default proportion = {}".format(
                                       proportions[2]),
                                   xlabel="time step", ylabel="# of individuals", category='MDR_Test_default_vs_total',
                                   show=True, line=False)
        outfile.write(dtk_sft.format_success_msg(success))
    if debug:
        print( "SUMMARY: Success={0}\n".format(success) )
    return success

def application( output_folder="output", stdout_filename="test.txt", insetchart_name="InsetChart.json",
                 config_filename="config.json", campaign_filename="campaign.json",
                 report_name=dtk_sft.sft_output_filename,
                 debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "insetchart_name: " + insetchart_name+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "campaign_filename: " + campaign_filename + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )

    dtk_sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename, debug)
    campaign_obj = load_campaign_file(campaign_filename, debug)
    output_dict = parse_output_file(stdout_filename, debug)
    report_dict = parse_report_file(debug=debug)
    create_report_file(param_obj, campaign_obj, output_dict, report_dict, report_name, debug)

if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-j', '--jsonreport', default="InsetChart.json", help="Json report to load (InsetChart.json)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-C', '--campaign', default="campaign.json", help="campaign name to load (campaign.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout, insetchart_name=args.jsonreport,
                config_filename=args.config, campaign_filename=args.campaign,
                report_name=args.reportname, debug=False)
