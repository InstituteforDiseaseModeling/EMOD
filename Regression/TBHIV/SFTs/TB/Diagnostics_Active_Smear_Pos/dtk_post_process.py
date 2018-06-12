#!/usr/bin/python

import json
import dtk_sft
import os
import csv
import math

"""
Diagnostics_Active_Smear_Pos:
What we are testing is two outcomes Positive Event, Negative Event, or Dropout Event
(all of which can be user specified in campaign).

Positive proportion  = Test Sensitivity * Treatment_Proportion
Negative Proportion = (1-Test_Sensitivity) * Treatment_Porportion
Dropout = 1- Treatment_Proportion.

"""
KEY_BASE_SENSITIVITY = "Base_Sensitivity"
KEY_TREATMENT_FRACTION = "Treatment_Fraction"
KEY_START_DAY = "Start_Day"
KEY_CONFIG_NAME = "Config_Name"
KEY_SIMULATION_TIMESTEP = "Simulation_Timestep"
KEY_INFECTED = "infected"
KEY_STAT_POP = "Stat Pop"
KEY_POSITIVE = "TBTestPositive"
KEY_NEGATIVE = "TBTestNegative"
KEY_DEFAULT = "TBTestDefault"
KEY_REPORTER_TIME = "Time"
KEY_REPORTER_EVENT_NAME = "Event_Name"

matches = ["Update(): Time: ",
           "StatPop: ",
           "Infected: ",
           "result is ",
           "got the test but defaulted"
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
    start_day = cf["Events"][0][KEY_START_DAY]
    campaign_obj[KEY_START_DAY] = start_day
    campaign_obj[KEY_BASE_SENSITIVITY] = base_sensitivity
    campaign_obj[KEY_TREATMENT_FRACTION] = treatment_fraction

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
    positive = 0
    negative = 0
    default = 0
    output_dict = {}
    for line in filtered_lines:
        if matches[0] in line:
            output_dict[time_step] = {KEY_STAT_POP:statpop, KEY_INFECTED:infected, KEY_POSITIVE:positive,
                                 KEY_NEGATIVE: negative, KEY_DEFAULT: default}
            infected = dtk_sft.get_val(matches[2], line)
            statpop = dtk_sft.get_val(matches[1], line)
            time_step += simulation_timestep
            positive = 0
            negative = 0
            default = 0
        if matches[3] in line:
            result = int(dtk_sft.get_val(matches[3], line))
            if result:
                positive += 1
            else:
                negative += 1
        if matches[4] in line:
            default += 1
    res_path = r'./tb_test_result_from_logging.json'
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
        t = int(column[KEY_REPORTER_TIME][i])
        test_result = str(column[KEY_REPORTER_EVENT_NAME][i])
        if t not in report_dict:
            report_dict[t] = {KEY_POSITIVE: 0, KEY_NEGATIVE: 0, KEY_DEFAULT: 0}
        # if t == 14:
        #     print (report_dict[t])
        if test_result == KEY_POSITIVE:
            report_dict[t][KEY_POSITIVE] += 1
        elif test_result == KEY_NEGATIVE:
            report_dict[t][KEY_NEGATIVE] += 1
        else:
            report_dict[t][KEY_DEFAULT] += 1
        # if t == 14:
        #     print (report_dict[t])
    res_path = r'./tb_test_result_from_report.json'
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
        proportions = [sensitivity * treatment_fraction, (1.0 - sensitivity) * treatment_fraction,
                       1.0 - treatment_fraction]
        positive = []
        negative = []
        default = []
        total = []
        failed_timestep = []
        if not len(report_dict):
            success = False
            outfile.write(dtk_sft.sft_no_test_data)
        for t in report_dict:
            value_to_test = [report_dict[t][KEY_POSITIVE], report_dict[t][KEY_NEGATIVE],
                             report_dict[t][KEY_DEFAULT]]
            positive.append(report_dict[t][KEY_POSITIVE])
            negative.append(report_dict[t][KEY_NEGATIVE])
            default.append(report_dict[t][KEY_DEFAULT])
            total.append(sum(value_to_test))
            outfile.write("Run Chi-squared test at time step {}.\n".format(t))
            result = dtk_sft.test_multinomial(dist=value_to_test, proportions=proportions, report_file=outfile)
            if not result:
                failed_timestep.append(t)
                outfile.write(
                    "Warning: At timestep {0}, the Chi-squared test failed.\n".format(t))
        if len(failed_timestep) > math.ceil(0.05 * len(report_dict)):
            success = False
            outfile.write(
                "BAD: the Chi-squared test failed at timestep {0}.\n".format(', '.join(str(x) for x in failed_timestep)))
        else:
            outfile.write(
                "GOOD: the Chi-squared test failed {} times, less than 5% of the total timestep.\n".format(len(failed_timestep)))

        dtk_sft.plot_data(positive, dist2=total, label1="TBTestPositive", label2="Total tested",
                                   title="Test positive vs. total, positive proportion = {}".format(sensitivity * treatment_fraction),
                                   xlabel="time step", ylabel="# of individuals", category='Test_positive_vs_total',
                                   show=True, line=False)
        dtk_sft.plot_data(negative, dist2=total, label1="TBTestNegative", label2="Total tested",
                                   title="Test negative vs. total, negative proportion = {}".format((1.0 - sensitivity) * treatment_fraction),
                                   xlabel="time step", ylabel="# of individuals", category='Test_negative_vs_total',
                                   show=True, line=False)
        dtk_sft.plot_data(default, dist2=total, label1="TBTestDefault", label2="Total tested",
                                   title="Test default vs. total, default proportion = {}".format(1.0 - treatment_fraction),
                                   xlabel="time step", ylabel="# of individuals", category='Test_default_vs_total',
                                   show=True, line=False)
        # TODO: write test to check if report matches debug logging. Pending on #2279. May not need this part.
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
