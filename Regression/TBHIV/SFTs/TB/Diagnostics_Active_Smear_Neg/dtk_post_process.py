#!/usr/bin/python

import json
import dtk_test.dtk_sft as sft
import os
import csv

"""
Diagnostics_Active_Smear_Neg:
What we are testing is two outcomes Positive Event and Negative event.

TBTestPositive  = (1 - Base_Specificity) * Treatment_Proportion
TBTestNegative  = Base_Specificity * Treatment_Proportion

"""
KEY_BASE_SPECIFICITY = "Base_Specificity"
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
    base_specificity = cf["Events"][1]["Event_Coordinator_Config"]["Intervention_Config"]["Actual_IndividualIntervention_Config"][KEY_BASE_SPECIFICITY]
    treatment_fraction = cf["Events"][1]["Event_Coordinator_Config"]["Intervention_Config"]["Actual_IndividualIntervention_Config"][KEY_TREATMENT_FRACTION]
    start_day = cf["Events"][0][KEY_START_DAY]
    campaign_obj[KEY_START_DAY] = start_day
    campaign_obj[KEY_BASE_SPECIFICITY] = base_specificity
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
            if sft.has_match(line,matches):
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
            infected = sft.get_val(matches[2], line)
            statpop = sft.get_val(matches[1], line)
            time_step += simulation_timestep
            positive = 0
            negative = 0
            default = 0
        if matches[3] in line:
            result = int(sft.get_val(matches[3], line))
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
        if test_result == KEY_POSITIVE:
            report_dict[t][KEY_POSITIVE] += 1
        elif test_result == KEY_NEGATIVE:
            report_dict[t][KEY_NEGATIVE] += 1
        else:
            report_dict[t][KEY_DEFAULT] += 1
    res_path = r'./tb_test_result_from_report.json'
    with open(res_path, "w") as file:
        json.dump(report_dict, file, indent=4)
    return report_dict

def create_report_file(param_obj, campaign_obj, output_dict, report_dict, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[KEY_CONFIG_NAME]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        specificity = campaign_obj[KEY_BASE_SPECIFICITY]
        treatment_fraction = campaign_obj[KEY_TREATMENT_FRACTION]
        prob = (1.0 - specificity) * treatment_fraction
        binomial_test_count = 0
        positive = []
        total = []
        if not len(report_dict):
            success = False
            outfile.write(sft.sft_no_test_data)

        point_fail = 0
        point_tolerance = 0.3

        for t in report_dict:
            num_success = report_dict[t][KEY_POSITIVE]
            num_trials = report_dict[t][KEY_NEGATIVE] + num_success
            positive.append(num_success)
            total.append(num_trials)
            if num_trials * prob < 5 or num_trials * (1 - prob) < 5:
                outfile.write("At timestep {0}, there is not enough sample size : mean = {1}, sample size - mean = {2}"
                              ".\n".format(t, num_trials * prob, num_trials * ( 1 - prob)))
            else:
                result = sft.test_binomial_95ci(num_success, num_trials, prob, report_file=outfile, category="TB positive")
                outfile.write("At timestep {0}, the binomial 95% test result is {1}.\n".format(t, result))
                binomial_test_count += 1
                if not result:
                    point_fail += 1
        if not binomial_test_count:
            success = False 
            outfile.write("FAIL: There is not enough sample size for binomial test in every time step, please fix the test.\n")

        point_test_message = f"Detected failures: {point_fail} trials: {binomial_test_count} tolerance: {point_tolerance}.\n"
        if point_fail / binomial_test_count > point_tolerance:
            success = False
            outfile.write(f"FAIL: {point_test_message}")
        else:
            outfile.write(f"PASS: {point_test_message}")

        outfile.write("BIG TEST: Testing the total of diagnoses across the simulation...\n")
        total_result = sft.test_binomial_95ci(num_success=sum(positive),
                                                  num_trials=sum(total),
                                                  prob=prob,
                                                  report_file=outfile,
                                                  category="Total active smear neg test positive")
        if not total_result:
            success=False
            outfile.write("FAIL: the total test failed, see line above.\n")

        sft.plot_data(positive, dist2=total, label1="TBTestPositive", label2="Total tested",
                                title="Test positive vs. total, positive proportion = {}".format(prob),
                                xlabel="time step", ylabel="# of individuals", category='Test_positive_vs_total',
                                show=True, line=False)
        # When Treatment_fraction is set to 1, the report should match debug log. Here is the test for it:
        for t in output_dict:
            log_positive = output_dict[t][KEY_POSITIVE]
            log_negative = output_dict[t][KEY_NEGATIVE]
            match_error = 0
            if log_negative or log_positive:
                report_positive = report_dict[t][KEY_POSITIVE]
                report_negative = report_dict[t][KEY_NEGATIVE]
                if report_positive != log_positive:
                    match_error += 1
                    success = False
                    outfile.write("BAD: at time step {0} the TBTestPositive is {1} from ReportEventRecorder.csv and {2} from"
                                  "debug logging.\n".format(t, report_positive, log_positive))
                if report_negative != log_negative:
                    match_error += 1
                    success = False
                    outfile.write("BAD: at time step {0} the TBTestNegative is {1} from ReportEventRecorder.csv and {2} from"
                                  "debug logging.\n".format(t, report_negative, log_negative))
            else:
                if t in report_dict:
                    report_positive = report_dict[t][KEY_POSITIVE]
                    report_negative = report_dict[t][KEY_NEGATIVE]
                    match_error += 1
                    success = False
                    outfile.write("BAD: at time step {0} the TBTestPositive and TBTestNegative are {1} and {2} from "
                                  "ReportEventRecorder.csv and {2} and {3} from debug logging. They should be matched\n"
                                  "".format(t, report_positive, report_negative, log_positive, log_negative))
        if not match_error:
            outfile.write("GOOD: The ReportEventRecorder.csv matches the debug logging.\n")
        else:
            outfile.write("BAD: The ReportEventRecorder.csv doesn't match the debug logging.\n")

        outfile.write(sft.format_success_msg(success))
    if debug:
        print( "SUMMARY: Success={0}\n".format(success) )
    return success

def application( output_folder="output", stdout_filename="test.txt", insetchart_name="InsetChart.json",
                 config_filename="config.json", campaign_filename="campaign.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "insetchart_name: " + insetchart_name+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "campaign_filename: " + campaign_filename + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )

    sft.wait_for_done()
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
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout, insetchart_name=args.jsonreport,
                config_filename=args.config, campaign_filename=args.campaign,
                report_name=args.reportname, debug=False)
