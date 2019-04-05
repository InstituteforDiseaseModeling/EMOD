#!/usr/bin/python

import json
import os.path as path
import dtk_test.dtk_sft as sft
import math
import pandas as pd


"""
This is a test for TyphoidCarrierClear intervention. 
We verify that the day after intervention is administered, all of the people who were chronic 
before are no longer chronic and only those people. Verifies that sick people who are not chronic are still sick.
Ignores people who go chronic on the day of campaign as those people are not cleared. 
This is not a statistical test.
This is not testing Clearance_Rate as it's not used.
This is not reading data out of campaign.json.

Intervention spec:
This individual-targeted intervention immediately clears an infected individual’s Typhoid infection (and expires). 
Note that despite its name it will clear Typhoid infections of all types. 
And it takes a Clearance_Rate parameter but that is for “future expansion” and is not currently used.

Sweep suggestion:
Run_Number, Population size 

"""


def load_config_file(config_filename="config.json",
                     config_location=".",
                     debug=False):
    with open(path.join(config_location, config_filename)) as infile:
        config_json = json.load(infile)
        config_parameters = config_json["parameters"]

    config_object = {"Config_Name": config_parameters["Config_Name"]}

    if debug:
        with open('DEBUG_config_object.json', 'w') as outfile:
            json.dump(config_object, outfile, indent=4)

    return config_object


def parse_json_report(output_folder="output", propertyreport_name="PropertyReport.json", debug=False):
    """
        This is not used, this is a place holder in case we want to expand the test when/if the functionality is
        expanded.
    """
    propertyreport_path = path.join(output_folder, propertyreport_name)
    with open(propertyreport_path) as infile:
        icj = json.load(infile)["Channels"]

    if debug:
        with open("data_PropertyReport.json", "w") as outfile:
            json.dump(report_data_obj, outfile, indent=4, sort_keys= True)

    return report_data_obj


def parse_output_file(output_filename="test.txt", debug=False):
    """
        Creates data frame containing lists of prepat,sublicinical, chronic, and acute individual from the
        output file
    Args:
        output_filename: file from which we get the data
        debug: flag that optionally puts the collected data into a csv file when true

    Returns:
        The data frame it creates
    """
    "state_to_report for individual 72 = SUS"
    time_step = 0
    campaign_day = 50
    state_chr = []
    state_acu = []
    state_sub = []
    state_pre = []
    state_sus = []
    pre_cumulative = []
    sub_cumulative = []
    chr_cumulative = []
    acu_cumulative = []
    sus_cumulative = []
    time = "Time: "
    state_report = "state_to_report for individual "
    throw_out = False  # indicator that individual's state just changed, we don't want to keep track of these
    with open(output_filename) as logfile:
        for line in logfile:
            if time in line:
                time_step = int(float(sft.get_val(time, line)))
                pre_cumulative.append(state_pre)
                sub_cumulative.append(state_sub)
                chr_cumulative.append(state_chr)
                acu_cumulative.append(state_acu)
                sus_cumulative.append(state_sus)
                state_chr = []
                state_acu = []
                state_sub = []
                state_pre = []
                state_sus = []
            elif ", sex" in line:
                throw_out = True  # means the following line will have individual that just changed states,
                # we want to ignore them as then are not affected by the intervention
            elif state_report in line:
                individual = int(sft.get_val(state_report, line))
                state = sft.get_char(" = ", line)
                if state == "PRE":
                    state_pre.append(individual)
                elif state == "ACU":
                    state_acu.append(individual)
                elif state == "SUB":
                    state_sub.append(individual)
                elif state == "CHR":
                    if time_step == campaign_day and throw_out:
                        pass
                    else:
                        state_chr.append(individual)
                elif state == "SUS":
                    state_sus.append(individual)
                else:
                    print(line)
                    raise ValueError("Found an unknown state - {} at Time: {}.\n".format(state, time_step))
                throw_out = False  # clearing the flag for the following lines

    # the columns are in order of disease progression, so looking at the data frame you can actually see people
    # go from one list to another between time steps
    states_df = pd.DataFrame.from_dict({"PRE": pre_cumulative, "SUB": sub_cumulative,
                                       "ACU": acu_cumulative, "CHR": chr_cumulative, "SUS": sus_cumulative})
    if debug:
        states_df.to_csv("states_df.csv")

    return states_df


def create_report_file(states_df, config_obj, report_name):
    with open(report_name, "w") as outfile:
        outfile.write("{}\n".format(config_obj["Config_Name"]))
        success = True
        # ideally we would get this from campaign.json
        campaign_day = 50
        campaign_day_plus = campaign_day + 1
        # checking that all chronic people that were sick on day of campaign have been cured,
        # there should be no overlapping ids
        common_ids = set(states_df.at[campaign_day, "CHR"]) & set(states_df.at[campaign_day_plus, "CHR"])
        if common_ids:
            success = False
            outfile.write("BAD: The following individuals were chronic and should've been cured, "
                          "but they are still sick the day after the campaign:\n")
            for ind in common_ids:
                outfile.write("{} ".format(ind))
            outfile.write("\n")
        else:
            outfile.write("GOOD: Everyone who was chronic on the day of campaign has been cured the next day.\n")

        # checking that none of the of the non-chronic sick people was cured
        sick_campaign_day = set(states_df.at[campaign_day, "PRE"] + states_df.at[campaign_day, "ACU"]
                                + states_df.at[campaign_day, "SUB"])
        # chronic and susceptible added in case someone went chronic after the campaign or healed on their own
        sick_campaign_day_plus = set(states_df.at[campaign_day_plus, "PRE"] + states_df.at[campaign_day_plus, "ACU"]
                                     + states_df.at[campaign_day_plus, "SUB"] + states_df.at[campaign_day_plus, "SUS"]
                                     + states_df.at[campaign_day_plus, "CHR"])
        # verifying no one else got cured by checking that those still sick are still sick (or went chronic or healed)
        if not sick_campaign_day.issubset(sick_campaign_day_plus):
            outfile.write("BAD: List of those non-chronically sick the day before is not a subset of people sick the "
                          "following day. Please check the logs for following ids: \n")
            for individual in sick_campaign_day.difference(sick_campaign_day_plus):
                outfile.write("{} ".format(str(individual)))
            outfile.write("\n")
        else:
            outfile.write("GOOD: People who were not chronic were not cured by the intervention.\n")

        outfile.write(sft.format_success_msg(success))


def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json", campaign_filename="campaign.json",
                 propertyreport_name="PropertyReportTyphoid.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename+ "\n")
        print("config_filename: " + config_filename + "\n")
        print("campaign_filename: " + campaign_filename + "\n")
        print("propertyreport_name: " + propertyreport_name + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    # report_data_obj = parse_json_report(output_folder, propertyreport_name, debug)
    config_obj = load_config_file(config_filename, debug=debug)
    states_df = parse_output_file(stdout_filename, debug=debug)
    create_report_file(states_df, config_obj, report_name)


if __name__ == "__main__":
    # execute only if run as a script
    application("output")
