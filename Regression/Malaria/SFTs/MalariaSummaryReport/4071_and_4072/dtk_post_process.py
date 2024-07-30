import json
import dtk_test.dtk_sft as dtk_sft
import math
import pandas as pd
from dtk_test.dtk_malaria_summary_support import summary_channel_to_pandas
import json
import numpy as np

"""
This verifies that when start_day + reporting_interval falls on the last time step, an extraneous report section with
no data is not created. 
"""

key_config_name = "Config_Name"
key_simulation_duration = "Simulation_Duration"
key_average_pop = "Average Population by Age Bin"
key_name = "Name"
key_start_day = "Start_Day"
key_reporting_interval = "Reporting_Interval"
key_max_reports = "Max_Number_Reports"


def load_custom_reports(cr_filename="custom_reports.json", debug=False):
    """
    Loads relevant parameters to verify new infections data.

    Args:
        cr_filename: name of the custom reports file. Default: "custom_reports.json"
        debug: when true, the parameters get written out as a json.

    Returns:
    Dictionary with relevant parameters.
    """
    with open(cr_filename) as infile:
        crj = json.load(infile)["Reports"]

    reports = []
    for report in crj:
        report_filename = report["class"] + "_" + report["Filename_Suffix"] + ".json"
        report_dict = {key_name: report_filename,
                       key_start_day: report[key_start_day],
                       key_reporting_interval: report[key_reporting_interval],
                       key_max_reports: report[key_max_reports]}
        reports.append(report_dict)

    if debug:
        with open("DEBUG_custom_report_params.json", 'w') as outfile:
            json.dump(reports, outfile, indent=4)

    return reports


def load_emod_parameters(config_filename="config.json", debug=False):
    """
    Reads the config filename and takes out relevant parameters.
    Args:
        config_filename: config filename
        debug: when true, the parameters get written out as a json.

    Returns:

    """

    """reads config file and populates params_obj
    :param config_filename: name of config file (config.json)
    :param debug: write out parsed data on true
    :returns param_obj:     dictionary with KEY_CONFIG_NAME, etc., keys (e.g.)
    """
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {key_config_name: cdj[key_config_name],
                 key_simulation_duration: cdj[key_simulation_duration]}

    if debug:
        with open("DEBUG_emod_params.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj


def create_report_file(param_obj, reports_dict, report_name, debug):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj[key_config_name]} : \n")
        for report in reports_dict:
            report_file = "output/" + report[key_name]
            outfile.write(f"\nThis is for {report[key_name]}:\n")
            with open(report_file, 'r') as json_file:
                data = json.load(json_file)
            time_of_report = data["DataByTime"]["Time Of Report"]
            average_pop_channel = data["DataByTimeAndAgeBins"][key_average_pop]
            start_day = report[key_start_day]
            reporting_interval = report[key_reporting_interval]
            max_reports = report[key_max_reports]
            simulation_duration = param_obj[key_simulation_duration]
            time_of_report_theoretical = []
            for x in range(max_reports):
                report_time = start_day + (x + 1) * reporting_interval
                if report_time <= simulation_duration:
                    time_of_report_theoretical.append(report_time)
                else:
                    break

            def bad_time_report():
                outfile.write(f"BAD: Expected 'Time of Report' does not match the one from {report[key_name]}.\n")
                outfile.write("Expected: \n")
                for time in time_of_report_theoretical:
                    outfile.write(f" {time} ")
                outfile.write("\n")
                outfile.write(f"From {report[key_name]}:\n")
                for time in time_of_report:
                    outfile.write(f" {time} ")
                outfile.write("\n")
            # compare times of the report in the report to theoretical
            if len(time_of_report_theoretical) != len(time_of_report):
                success = False
                bad_time_report()
            else:
                for x in range(len(time_of_report_theoretical)):
                    if time_of_report_theoretical[x] != time_of_report[x]:
                        success = False
                        bad_time_report()

            if len(time_of_report_theoretical) != len(average_pop_channel):
                success = False
                outfile.write(f"BAD: We expected {len(time_of_report_theoretical)} entrees in the {key_average_pop} "
                              f"channel, but got {len(average_pop_channel)}.\n")
            if sum(average_pop_channel[-1]) == 0:
                success = False
                outfile.write(f"BAD: We were expecting non-zero numbers in the last entree of {key_average_pop} "
                              f"channel, but got zeroes. Something's wrong. \n")
            else:
                outfile.write("GOOD: The times of the report match and there is data present in the last entree for the "
                              f"{key_average_pop} channel.\n")

        outfile.write(dtk_sft.format_success_msg(success))


def application(output_folder="output",
                config_filename="config.json", custom_reports="custom_reports.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done()
    reports_list = load_custom_reports(custom_reports)
    param_obj = load_emod_parameters(config_filename, debug)
    create_report_file( param_obj, reports_list, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-cu', '--custom', default="custom_reports.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output,
                config_filename=args.config, report_name=args.reportname, debug=args.debug)
