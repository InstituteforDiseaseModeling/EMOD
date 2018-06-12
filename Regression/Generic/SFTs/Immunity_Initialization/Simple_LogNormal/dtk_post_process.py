#!/usr/bin/python

import dtk_sft as sft
import dtk_Immunity_Initialization_Support as dtk_iis


"""
These tests operate thusly:
1. Load a config file with a demographics overlay
 that declares a particular immunity profile
2. Load a campaign file that gives an immunity respecting
 outbreak individual to the entire population
3. Capture the population and New Infections on the day
 of the outbreak event
4. Compare the number of new infections to the expected 
 new infections
5. Fail if the numbers are off by more than 2%

NOTE: Always make sure that the expected immunity isn't close to 50%
"""



def application(output_folder="output", config_filename="config.json",
                jsonreport_name="InsetChart.json",
                report_name=sft.sft_output_filename, debug=False):
    if debug:
        print("output_folder: " + output_folder + "\n")
        print("config_filename: " + config_filename + "\n")
        print("insetchart_name: " + jsonreport_name + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    config_object = dtk_iis.load_config_file(config_filename=config_filename,
                                             debug=debug)
    demographics_overlay_name = config_object[dtk_iis.ConfigKeys.DEMOGRAPHIC_FILENAMES][-1]
    campaign_name = config_object[dtk_iis.ConfigKeys.CAMPAIGN_FILENAME]
    test_config_name = config_object[dtk_iis.ConfigKeys.CONFIG_NAME]

    campaign_object = dtk_iis.load_campaign_file(campaign_name, debug)
    outbreak_day = campaign_object[dtk_iis.CampaignKeys.START_DAY]

    demographics_object = dtk_iis.load_demographics_file(
        demographics_filename=demographics_overlay_name,
        immunity_initialization_type=config_object[dtk_iis.ConfigKeys.IMM_DIST_TYPE],
        debug=debug)
    average_immunity = demographics_object[dtk_iis.DemographicFileKeys.KEY_AVERAGE_IMMUNITY]

    sft.start_report_file(report_name, test_config_name)

    report_data_object = dtk_iis.parse_json_report(output_folder=output_folder,
                                                   report_name=jsonreport_name,
                                                   debug=debug)

    statistical_population_channel = report_data_object[dtk_iis.InsetChannels.STATISTICAL_POPULATION]
    new_infections_channel = report_data_object[dtk_iis.InsetChannels.NEW_INFECTIONS]

    expected_infections_obj = dtk_iis.get_expected_infections(statistical_population_channel,
                                                              average_immunity,
                                                              campaign_object,
                                                              debug=debug)

    actual_infections = dtk_iis.get_actual_infections(new_infections_channel,
                                              outbreak_day)

    dtk_iis.create_report_file(expected_infections_obj, actual_infections,
                               outbreak_day, report_name,
                               debug=debug)



if __name__ == "__main__":
    # execute only if run as a script
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-j', '--jsonreport', default="InsetChart.json", help="Json report to load (InsetChart.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    args = parser.parse_args()

    application(output_folder=args.output, config_filename=args.config,
                jsonreport_name=args.jsonreport,
                report_name=args.reportname, debug=True)
