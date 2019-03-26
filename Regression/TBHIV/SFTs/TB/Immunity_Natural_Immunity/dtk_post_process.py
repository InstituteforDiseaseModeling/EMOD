#!/usr/bin/python

import json
import dtk_test.dtk_sft as sft
import os
from scipy import stats

"""
Natural_Immunity:

Give out perfect transmission blocking vaccine to everyone over the course of 1 year triggered on individual's birthday.
At the end of 1 year infect everyone with TB. Infected individuals progress to active TB in 2 timesteps, at which point
they are all treated with a perfect TB treatment and they clear their infection.

Following clearance introduce via importation 1 case of active Smear Positive TB.

The 'Post_Infection_Acquisition_Multiplier' = 0.5. Therefore individuals should be half as susceptible as previously

Statistical Test: Cut up time into discrete intervals of length T.  For each of these time bins the expected number of
infections resulting in the interval is Base_Infectivity*T*Post_Infection_Acquisition_Multiplier

Use a chi-squared test with number of time bins - 1, and the expected value on the number of infections resulting in
each bin. (Test is the same as for BCG above).

"""
# campaign parameter
KEY_START_DAY = "Start_Day"
# config parameter
KEY_CONFIG_NAME = "Config_Name"
KEY_BASE_INFECTIVITY = "Base_Infectivity"
KEY_IMMUNITY_ACQUISITION_FACTOR = "Post_Infection_Acquisition_Multiplier"
# insetchart constant
KEY_NEW_INFECTION = "New Infections"

def load_emod_parameters(config_filename="config.json", debug=False):
    """reads config file and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_CONFIG_NAME, etc., keys (e.g.)
    """
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {}
    param_obj[KEY_CONFIG_NAME] = cdj[KEY_CONFIG_NAME]
    param_obj[KEY_BASE_INFECTIVITY] = cdj[KEY_BASE_INFECTIVITY]
    param_obj[KEY_IMMUNITY_ACQUISITION_FACTOR] = cdj[KEY_IMMUNITY_ACQUISITION_FACTOR]
    if debug:
        with open("DEBUG_param_object.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj

def load_campaign_file(campaign_filename="campaign.json", debug=False):
    """reads campaign file

    :param campaign_filename: campaign.json file
    :returns: campaign_obj structure, dictionary with KEY_INITIAL_EFFECT, KEY_START_DAY, etc., keys (e.g.)
    """
    with open(campaign_filename) as infile:
        cf = json.load(infile)
    campaign_obj = {}
    # get start day of import case
    start_day = cf["Events"][-1][KEY_START_DAY]
    campaign_obj[KEY_START_DAY] = start_day

    if debug:
        with open("DEBUG_campaign_object.json", 'w') as outfile:
            json.dump(campaign_obj, outfile, indent=4)

    return campaign_obj

def parse_json_report(insetchart_name="InsetChart.json", output_folder="output", debug=False):
    """
    creates report_data_obj structure with keys
    :param insetchart_name: file to parse (InsetChart.json)
    :param output_folder:
    :return: report_data_obj structure, dictionary with KEY_NEW_INFECTION etc., keys (e.g.)
    """
    insetchart_path = os.path.join(output_folder, insetchart_name)
    with open(insetchart_path) as infile:
        icj = json.load(infile)["Channels"]
    report_data_obj = {}
    new_infection = icj[KEY_NEW_INFECTION]["Data"]
    report_data_obj[KEY_NEW_INFECTION] = new_infection

    if debug:
        with open("DEBUG_data_InsetChart.json", "w") as outfile:
            json.dump(report_data_obj, outfile, indent=4)

    return report_data_obj

def create_report_file(param_obj, campaign_obj, report_data_obj, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[KEY_CONFIG_NAME]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        base_infectivity = param_obj[KEY_BASE_INFECTIVITY]
        start_day = campaign_obj[KEY_START_DAY]
        new_infection = report_data_obj[KEY_NEW_INFECTION]
        immunity_acquisition_factor = param_obj[KEY_IMMUNITY_ACQUISITION_FACTOR]
        # calculate expected number of infections for a time period of 3 months:
        number_of_month = 3
        expected_new_infection = base_infectivity * sft.DAYS_IN_MONTH * number_of_month * immunity_acquisition_factor
        expected = [expected_new_infection] * (sft.MONTHS_IN_YEAR // number_of_month)
        # group new infections for every 3 months:
        value_to_test = []
        if len(new_infection) < start_day + sft.DAYS_IN_YEAR:
            success = False
            outfile.write("BAD: the simulation duration is too short, please make sure it's at least {} days.\n".format(
                start_day + sft.DAYS_IN_YEAR
            ))
        outfile.write("running chi-squared test for expected new infections for {0} {1}-months time bins: \n"
                      "base_infectivity = {2}, immunity_acquisition_factor = {3}.\n".format(sft.MONTHS_IN_YEAR // number_of_month, number_of_month, base_infectivity,
                                                                               immunity_acquisition_factor))
        actual_new_infection = 0
        i = 0
        for t in range(start_day, len(new_infection)):
            actual_new_infection += new_infection[t]
            i += 1
            if not i % (number_of_month * sft.DAYS_IN_MONTH):
                value_to_test.append(actual_new_infection)
                actual_new_infection = 0
        sft.plot_data(value_to_test, dist2=expected, label1="actual_new_infections",
                                   label2="expected_new_infection",
                                   title="actual vs. expected new infection for every {} months".format(number_of_month),
                                   xlabel="every {} months".format(number_of_month), ylabel="# of new infections", category='actual_vs_expected_new_infections',
                                   show=True, line=True)
        result = sft.test_multinomial(dist=value_to_test, proportions=expected, report_file=outfile, prob_flag=False)

        if not result:
            success = False
            outfile.write(
                "BAD: The Chi-squared test for number of new infections in every {} months failed.\n".format(number_of_month))
        else:
            outfile.write(
                "GOOD: The Chi-squared test for number of new infections in every {} months passed.\n".format(
                    number_of_month))
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
    report_data_obj = parse_json_report(insetchart_name, output_folder, debug)
    create_report_file(param_obj, campaign_obj, report_data_obj, report_name, debug)

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
