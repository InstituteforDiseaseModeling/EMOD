#!/usr/bin/python

import json
import dtk_sft
import math
import numpy as np
import pandas as pd
import os

"""
Reporter: TBTestNegative and TBTestDefault

This SFT is testing the TBTestNegative and TBTestDefault column in the Report_TBHIV_ByAge.csv match the count for 
TB test result in stdout.txt.

"""


class Config:
    config_name = "Config_Name"
    simulation_timestep = "Simulation_Timestep"
    duration = "Simulation_Duration"


class ReportColumn:
    year = "Year"
    negative = " TBTestNegative"
    default = " TBTestDefault"
    positive = " TBTestPositive"
    agebin = " AgeBin"
    seek200 = " Seek200"


class Campaign:
    events = "Events"
    event_co_config = "Event_Coordinator_Config"
    int_config = "Intervention_Config"
    actual_indint_config = "Actual_IndividualIntervention_Config"
    sensitivity = "Base_Sensitivity"
    treatment = "Treatment_Fraction"
    start_day = "Start_Day"


matches = ["Update(): Time: ",
           "result is ",
           "got the test but defaulted"
           ]


def load_emod_parameters(config_filename="config.json", debug=False):
    """reads config file and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with Config.config_name, etc., keys (e.g.)
    """
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {}
    param_obj[Config.config_name] = cdj[Config.config_name]
    param_obj[Config.simulation_timestep] = cdj[Config.simulation_timestep]
    param_obj[Config.duration] = cdj[Config.duration]
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
    base_sensitivity = cf[Campaign.events][1][Campaign.event_co_config][Campaign.int_config][Campaign.actual_indint_config][Campaign.sensitivity]
    treatment_fraction = cf[Campaign.events][1][Campaign.event_co_config][Campaign.int_config][Campaign.actual_indint_config][Campaign.treatment]
    start_day = cf[Campaign.events][0][Campaign.start_day]
    campaign_obj[Campaign.start_day] = start_day
    campaign_obj[Campaign.sensitivity] = base_sensitivity
    campaign_obj[Campaign.treatment] = treatment_fraction

    if debug:
        with open("DEBUG_campaign_object.json", 'w') as outfile:
            json.dump(campaign_obj, outfile, indent=4)

    return campaign_obj

def parse_output_file(output_filename="test.txt", simulation_timestep=1, debug=False):
    """
    creates a dictionary to store filtered information for each time step
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
    positive = 0
    negative = 0
    default = 0
    output_df = pd.DataFrame(columns=[ReportColumn.negative, ReportColumn.default, ReportColumn.positive])
    output_df.index.name = Config.simulation_timestep
    for line in filtered_lines:
        if matches[0] in line:
            output_df.loc[time_step] = pd.Series({ReportColumn.positive: positive,
                                                  ReportColumn.negative: negative, ReportColumn.default: default})
            time_step += simulation_timestep
            positive = 0
            negative = 0
            default = 0
        if matches[1] in line:
            result = int(dtk_sft.get_val(matches[1], line))
            if result:
                positive += 1
            else:
                negative += 1
        if matches[2] in line:
            default += 1
    res_path = r'./DEBUG_tb_test_result_from_logging.csv'
    with open(res_path, "w") as file:
        output_df.to_csv(file)
    return output_df

def parse_custom_reporter(reporter_path="output", reporter_filename="output/Report_TBHIV_ByAge.csv", debug=False):
    report_df = pd.read_csv(os.path.join(reporter_path, reporter_filename))
    success = True
    try:
        filtered_df = report_df[[ReportColumn.year, ReportColumn.agebin, ReportColumn.negative, ReportColumn.default, ReportColumn.seek200]]
        if debug:
            with open("DEBUG_reporter_dataframe.csv", "w") as outfile:
                filtered_df.to_csv(outfile, header=True)
        return filtered_df, success
    except Exception as ex:
        success = False
        print ("Failed to parse report file, get exception: " + str(ex))
        return ex, success


def create_report_file(param_obj, campaign_obj, output_df, reporter, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[Config.config_name]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        simulation_duration = param_obj[Config.duration]
        timestep = param_obj[Config.simulation_timestep]
        if not len(output_df):
            success = False
            outfile.write(dtk_sft.sft_no_test_data)
        # reporter[1] is a boolean.
        # True means parse_custom_reporter succeed and reporter[0] is a dataframe collected from the csv report.
        # False measn parse_custom_reporter failed and reporter[0] is an error message
        elif not reporter[1]:
            success = False
            outfile.write("BAD: failed to parse report, get exception : {}.\n".format(reporter[0]))
        else:
            outfile.write("GOOD: parse report successfully.\n")
            reporter_df = reporter[0]
            outfile.write("Group the custom envent column by year and get the sum for all age bins:\n")
            # the year column becomes the index of the groupby_df
            groupby_df = reporter_df.groupby(ReportColumn.year).sum()

            if debug:
                with open("DEBUG_groupby_dataframe.csv", "w") as groupby_file:
                    groupby_df.to_csv(groupby_file, header=True)

            outfile.write("Checking whether we have enough test data:\n")
            expected_max_time_step = math.floor(simulation_duration / timestep) * timestep
            if simulation_duration <= 180 or expected_max_time_step <= 180:
                success = False
                outfile.write("BAD: the simulation duration is too short, please increase the duration.\n")
            elif (not groupby_df[ReportColumn.negative].sum()) and (not groupby_df[ReportColumn.default].sum()):
                success = False
                outfile.write("BAD: there in no {0} and {1} in the test, please check the test.\n".format(ReportColumn.negative,
                                                                                                          ReportColumn.default))
            else:
                outfile.write("Checking more test condition:\n")
                treatment_fraction = float(campaign_obj[Campaign.treatment])
                outfile.write("{0} in Campaign.json is {1}.\n".format(Campaign.treatment,treatment_fraction))
                if treatment_fraction == 1:
                    test_treatment_only = False
                    outfile.write("Testing the {0} and {1} count with log_valid for all year buckets:\n".format(
                        ReportColumn.negative,
                        ReportColumn.default))

                else:
                    test_treatment_only = True
                    outfile.write("Testing the {0} count with log_valid for all year buckets:\n".format(
                        ReportColumn.default))

                i = default_count = negative_count =0
                years = groupby_df.index.values
                negative_counts = []
                default_counts = []

                for t in output_df.index.values.tolist():
                    if i < len(years):
                        year = years[i]
                        if t <= round(year * dtk_sft.DAYS_IN_YEAR):
                            default_count += output_df.loc[t][ReportColumn.default]
                            negative_count += output_df.loc[t][ReportColumn.negative]
                        else:
                            reporter_negative_sum = int(groupby_df[groupby_df.index==year][ReportColumn.negative])
                            reporter_default_sum = int(groupby_df[groupby_df.index==year][ReportColumn.default])
                            negative_counts.append(negative_count)
                            default_counts.append(default_count)

                            if not test_treatment_only:
                                if negative_count != reporter_negative_sum:
                                    success = False
                                    outfile.write("BAD: in year {0} the {1} count get from reporter is {2}, while "
                                                  "test.txt reports {3} cases.\n".format(year, ReportColumn.negative,
                                                                                         reporter_negative_sum,
                                                                                         negative_count))
                            if default_count != reporter_default_sum:
                                success = False
                                outfile.write("BAD: in year {0} the {1} count get from reporter is {2}, while test.txt reports"
                                              " {3} cases.\n".format(year, ReportColumn.default, reporter_default_sum, default_count))
                            default_count = output_df.loc[t][ReportColumn.default]
                            negative_count = output_df.loc[t][ReportColumn.negative]
                            i += 1
                    else:
                        break

                if not test_treatment_only:
                    dtk_sft.plot_data(negative_counts, dist2=np.array(groupby_df[ReportColumn.negative]), label1="reporter",
                                    label2="log_valid", title=ReportColumn.negative,
                                    xlabel="every half year", ylabel=ReportColumn.negative, category=ReportColumn.negative,
                                    show=True, line=True, alpha=0.8, overlap=True)
                dtk_sft.plot_data(default_counts, dist2=np.array(groupby_df[ReportColumn.default]), label1="reporter",
                                           label2="log_valid", title=ReportColumn.default,
                                           xlabel="every half year", ylabel=ReportColumn.default, category=ReportColumn.default,
                                           show=True, line=True, alpha=0.8, overlap=True)

                outfile.write("Testing whether the time step in log mathces the simulation duration:\n")
                max_time_step = max(output_df.index.values)
                if  abs(max_time_step - expected_max_time_step) > 1 :
                    success = False
                    outfile.write("BAD: the last time step in simulation is {0}, expected {1}."
                                  "\n".format(max_time_step, expected_max_time_step))

                outfile.write("Testing whether the reporter year matches the simulation duration:\n")
                if i != len(years):
                    success = False
                    outfile.write("BAD: the reporter has data up to year {0} but the simulation duration is {1}, we are expecting "
                                  "not more than year {2} from reporter.".format(max(years), simulation_duration,
                                                                           math.floor(simulation_duration/180)))
                if simulation_duration > round(max(years) * dtk_sft.DAYS_IN_YEAR) + 180:
                    success = False
                    outfile.write("BAD: the reporter has data up to year {0} but the simulation duration is {1}, we are expecting "
                                  "data after year {0} from reporter.".format(max(years), simulation_duration))

        outfile.write(dtk_sft.format_success_msg(success))

        if debug:
            print( "SUMMARY: Success={0}\n".format(success) )
        return success

def application( output_folder="output", stdout_filename="test.txt", reporter_filename="Report_TBHIV_ByAge.csv",
                 config_filename="config.json", campaign_filename="campaign.json",
                 report_name=dtk_sft.sft_output_filename,
                 debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "reporter_filename: " + reporter_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "campaign_filename: " + campaign_filename + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )

    dtk_sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename, debug)
    campaign_obj = load_campaign_file(campaign_filename, debug)
    output_df = parse_output_file(stdout_filename, param_obj[Config.simulation_timestep], debug)
    reporter = parse_custom_reporter(reporter_path=output_folder, reporter_filename=reporter_filename, debug=debug)
    create_report_file(param_obj, campaign_obj, output_df, reporter, report_name, debug)

if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-j', '--reportername', default="Report_TBHIV_ByAge.csv", help="reporter to test(Report_TBHIV_ByAge.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-C', '--campaign', default="campaign.json", help="campaign name to load (campaign.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', default=False, help="Debug = True or False")
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout, reporter_filename=args.reportername,
                config_filename=args.config, campaign_filename=args.campaign,
                report_name=args.reportname, debug=args.debug)
