import json
import dtk_test.dtk_sft as dtk_sft
import math
import pandas as pd
from dtk_test.dtk_malaria_summary_support import summary_channel_to_pandas
import json
import numpy as np

"""
This tests opens custom_reports.json file and gathers pertinent to it information, 
it then gathers new infections data via logging and compares it to the new infections data
that was produced in the reports. The data is grouped as in the reports. 

"""

key_config_name = "Config_Name"
key_simulation_duration = "Simulation_Duration"
key_new_infections = "New Infections by Age Bin"
key_name = "Name"
key_age_bins = "Age_Bins"
key_start_day = "Start_Day"
key_reporting_interval = "Reporting_Interval"
key_ind_prop = "Must_Have_IP_Key_Value"
key_max_reports = "Max_Number_Reports"
key_nodes = "Nodes"


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
        nodes = report["Node_IDs_Of_Interest"]
        report_dict = {key_name: report_filename,
                       key_age_bins: report[key_age_bins],
                       key_start_day: report[key_start_day],
                       key_reporting_interval: report[key_reporting_interval],
                       key_ind_prop: report[key_ind_prop],
                       key_max_reports: report[key_max_reports],
                       key_nodes: nodes}
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


def summary_channel_data(filepath=None, channel=key_new_infections):
    with open(filepath, 'r') as json_file:
        data = json.load(json_file)

    # Get channels by age and time series
    channel_series = summary_channel_to_pandas(data, channel)
    return channel_series


def parse_output_file(output_filename="test.txt", report_dict=None, sim_duration=None, debug=False):
    """
    Looks for line "NodeID = 3 human of age 6.016800 got new infection" and records if the information matches
    the parameters needed (can be filtered by nodeID)

    Args:
        output_filename: the logging file
        report_dict: relevant parameters from the custom_reports.json file
        debug: if True saves off a file with just the lines of interest and saves the final dataframe

    Returns:
    Dataframe organized in the way the report_dict parameters ask
    """
    filtered_lines = []
    time = 0
    total = []
    age_bins = report_dict[key_age_bins]
    start_time = report_dict[key_start_day]
    reporting_interval = report_dict[key_reporting_interval]
    nodes = report_dict[key_nodes]
    ind_property = report_dict[key_ind_prop]
    new_inf = {}
    max_reports = report_dict[key_max_reports]
    for age_bin in age_bins:  # initializing
        new_inf[age_bin] = 0
    with open(output_filename) as logfile:
        for line in logfile:
            if time > start_time + reporting_interval * max_reports:
                break
            elif "Time:" in line:
                time = int(float(dtk_sft.get_val("Time: ", line)))
                filtered_lines.append(line)
                if (time > start_time and not (time - start_time) % reporting_interval) or time == sim_duration:
                    for key in new_inf:
                        total.append([time, key, new_inf[key]])  # recording
                        new_inf[key] = 0  # resetting
            elif "human of age" in line and time >= start_time:
                filtered_lines.append(line)
                age = float(dtk_sft.get_val("human of age ", line))
                node = int(dtk_sft.get_val("NodeID = ", line))
                if not nodes or node in nodes:
                    if not ind_property or ind_property in line:
                        for age_bin in age_bins:
                            if age <= age_bin:
                                new_inf[age_bin] += 1
                                break

    new_infections_df = pd.DataFrame(total, columns=['Time', 'Age Bin', key_new_infections])

    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)
        new_infections_df.to_csv("DEBUG_new_infections_df.csv")

    return new_infections_df


def create_report_file(stdout_filename, param_obj, reports_dict, report_name, debug):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj[key_config_name]} : \n")
        for report in reports_dict:
            msr = summary_channel_data("output/" + report[key_name])
            msr_df = msr.to_frame().reset_index()
            new_infections_df = parse_output_file(stdout_filename, report, param_obj[key_simulation_duration], debug=debug)
            if not msr_df[key_new_infections].astype(np.int64).equals(new_infections_df[key_new_infections]):
                success = False
                outfile.write(f"BAD:The data in {report[key_name]} does not match the data from logging.\n")
                msr_df.to_csv(f"DEBUG_from_report_{report[key_name]}.csv")
                new_infections_df.to_csv(f"DEBUG_from_stdout_{report[key_name]}.csv")
            else:
                outfile.write(f"GOOD:The data in {report[key_name]} matches the data from logging.\n")

        outfile.write(dtk_sft.format_success_msg(success))


def application(output_folder="output", stdout_filename="test.txt",
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
    create_report_file(stdout_filename, param_obj, reports_list, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-cu', '--custom', default="custom_reports.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                config_filename=args.config, report_name=args.reportname, debug=args.debug)
