#!/usr/bin/python

import json
import dtk_test.dtk_sft as sft
import math
import numpy as np
import pandas as pd
import os

"""
Reporter: Incidence

This SFT is testing the Incidence column in the Report_TBHIV_ByAge.csv match the count for TB in Active Symptomatic 
state in stdout.txt.

This test has simulation timestep > 1.

"""


class Config:
    config_name = "Config_Name"
    simulation_timestep = "Simulation_Timestep"
    duration = "Simulation_Duration"


class ReportColumn:
    year = "Year"
    incidence = " Incidence"
    agebin = " AgeBin"


matches = ["Update(): Time: ",
           "moved from Active Presymptomatic to Active Symptomatic"
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

def parse_output_file(output_filename="test.txt", simulation_timestep=1, debug=False):
    """
    creates a dictionary to store filtered information for each time step
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
    output_dict = {}
    for line in filtered_lines:
        if matches[0] in line:
            time_step += simulation_timestep
            output_dict[time_step] = 0
        elif matches[1] in line: # this individual is Symptomatic active
            output_dict[time_step] += 1

    if debug:
        res_path = r'./DEBUG_incidence.json'
        with open(res_path, "w") as file:
            json.dump(output_dict, file, indent=4)
    return output_dict

def parse_custom_reporter(reporter_path="output", reporter_filename="output/Report_TBHIV_ByAge.csv", debug=False):
    report_df = pd.read_csv(os.path.join(reporter_path, reporter_filename))
    filtered_df = report_df[[ReportColumn.year, ReportColumn.agebin, ReportColumn.incidence]]

    if debug:
        with open("DEBUG_reporter_dataframe.csv","w") as outfile:
            filtered_df.to_csv(outfile,header=True)
    return filtered_df

def create_report_file(param_obj, output_dict, reporter_df, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[Config.config_name]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        simulation_duration = param_obj[Config.duration]
        timestep = param_obj[Config.simulation_timestep]
        if not len(output_dict):
            success = False
            outfile.write(sft.sft_no_test_data)
        else:
            outfile.write("Group the incidence by year and get the sum for all age bins:\n")
            # the year column becomes the index of the groupby_df
            groupby_df = reporter_df.groupby(ReportColumn.year).sum()

            if debug:
                with open("DEBUG_groupby_dataframe.csv", "w") as groupby_file:
                    groupby_df.to_csv(groupby_file, header=True)

            expected_max_time_step = math.floor(simulation_duration / timestep) * timestep
            if simulation_duration <= 180 or expected_max_time_step <= 180:
                success = False
                outfile.write("BAD: the simulation duration is too short, please increase the duration.\n")

            elif not groupby_df[ReportColumn.incidence].sum():
                success = False
                outfile.write("BAD: there in no TB incidence in the test, please check the test.\n")

            else:
                outfile.write("Testing the incidence count with log_valid for all year buckets:\n")
                i = incidence_count = 0
                years = groupby_df.index.values
                incidence_counts = []

                for t in output_dict:
                    if i < len(years):
                        year = years[i]
                        if t <= round(year * sft.DAYS_IN_YEAR):
                            incidence_count += output_dict[t]
                        else:
                            reporter_sum = int(groupby_df[groupby_df.index==year][ReportColumn.incidence])
                            incidence_counts.append(incidence_count)
                            if incidence_count != reporter_sum:
                                success = False
                                outfile.write("BAD: in year {0} the incidence count get from reporter is {1}, while test.txt reports"
                                              " {2} cases.\n".format(year, reporter_sum, incidence_count))
                            incidence_count = output_dict[t]
                            i += 1
                    else:
                        break

                sft.plot_data(incidence_counts, dist2=np.array(groupby_df[ReportColumn.incidence]), label1="reporter",
                                           label2="log_valid", title="incidence",
                                           xlabel="every half year", ylabel="incidence", category='incidence',
                                           show=True, line=False, alpha=0.8)

                outfile.write("Testing whether the time step in log mathces the simulation duration:\n")
                max_time_step = max(output_dict.keys())
                if  max_time_step != expected_max_time_step :
                    success = False
                    outfile.write("BAD: the last time step in simulation is {0}, expected {1}."
                                  "\n".format(max_time_step, expected_max_time_step))

                outfile.write("Testing whether the reporter year matches the simulation duration:\n")
                if i != len(years):
                    success = False
                    outfile.write("BAD: the reporter has data up to year {0} but the simulation duration is {1}, we are expecting "
                                  "not more than year {2} from reporter.\n".format(max(years), simulation_duration,
                                                                           math.floor(simulation_duration/sft.DAYS_IN_YEAR)))
                    outfile.write("i={0}, len(years)={1}\n".format(i, len(years)))
                if simulation_duration > round(max(years) * sft.DAYS_IN_YEAR) + 180:
                    success = False
                    outfile.write("BAD: the reporter has data up to year {0} but the simulation duration is {1}, we are expecting "
                                  "data after year {0} from reporter.\n".format(max(years), simulation_duration))

        outfile.write(sft.format_success_msg(success))

        if debug:
            print( "SUMMARY: Success={0}\n".format(success) )
        return success

def application( output_folder="output", stdout_filename="test.txt", reporter_filename="Report_TBHIV_ByAge.csv",
                 config_filename="config.json",
                 report_name=sft.sft_output_filename,
                 debug=True):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "reporter_filename: " + reporter_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )

    sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename, debug)
    output_dict = parse_output_file(stdout_filename, param_obj[Config.simulation_timestep], debug)
    reporter_df = parse_custom_reporter(reporter_path=output_folder, reporter_filename=reporter_filename, debug=debug)
    create_report_file(param_obj, output_dict, reporter_df, report_name, debug)

if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-j', '--reportername', default="Report_TBHIV_ByAge.csv", help="reporter to test(Report_TBHIV_ByAge.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', default=False, help="Debug = True or False")
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout, reporter_filename=args.reportername,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)
