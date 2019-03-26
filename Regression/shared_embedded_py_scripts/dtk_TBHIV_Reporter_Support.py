#!/usr/bin/python

import json
import dtk_test.dtk_sft as sft
import math
import numpy as np
import pandas as pd
import os

"""
This is a support library for TBHIV Reporter SFTs

"""

class Config:
    config_name = "Config_Name"
    simulation_timestep = "Simulation_Timestep"
    duration = "Simulation_Duration"

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

def parse_output_file(matches, file_name, output_filename="test.txt", simulation_timestep=1, debug=False):
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
        res_path = r'./DEBUG_{}.json'.format(file_name)
        with open(res_path, "w") as file:
            json.dump(output_dict, file, indent=4)
    return output_dict

def parse_custom_reporter( report_column_list, reporter_path="output", reporter_filename="Report_TBHIV_ByAge.csv", debug=False):
    report_df = pd.read_csv(os.path.join(reporter_path,reporter_filename))
    filtered_df = report_df[report_column_list]

    if debug:
        with open("DEBUG_reporter_dataframe.csv","w") as outfile:
            filtered_df.to_csv(outfile,header=True)
    return filtered_df

def create_report_file_incidence(column_to_test, column_year, param_obj, output_dict, reporter_df, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[Config.config_name]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        simulation_duration = param_obj[Config.duration]
        # timestep = param_obj[Config.simulation_timestep]
        if not len(output_dict):
            success = False
            outfile.write(sft.sft_no_test_data)
        else:
            outfile.write("Group the {} by year and get the sum for all age bins:\n".format(column_to_test))
            # the year column becomes the index of the groupby_df
            groupby_df = reporter_df.groupby(column_year).sum()

            if debug:
                with open("DEBUG_groupby_dataframe.csv", "w") as groupby_file:
                    groupby_df.to_csv(groupby_file, header=True)

            if not groupby_df[column_to_test].sum():
                success = False
                outfile.write("BAD: there in no {} in the test, please check the test.\n".format(column_to_test))

            else:
                outfile.write("Testing the {} count with log_valid for all year buckets:\n".format(column_to_test))
                i = incidence_count = 0
                years = groupby_df.index.values
                incidence_counts = []

                for t in output_dict:
                    if i < len(years):
                        year = years[i]
                        if t <= round(year * sft.DAYS_IN_YEAR):
                            incidence_count += output_dict[t]
                        else:
                            reporter_sum = int(groupby_df[groupby_df.index==year][column_to_test])
                            incidence_counts.append(incidence_count)
                            if incidence_count != reporter_sum:
                                success = False
                                outfile.write("BAD: in year {0} the {1} count get from reporter is {2}, while test.txt reports"
                                              " {3} cases.\n".format(year, column_to_test, reporter_sum, incidence_count))
                            incidence_count = output_dict[t]
                            i += 1
                    else:
                        break

                sft.plot_data(incidence_counts, dist2=np.array(groupby_df[column_to_test]), label1="reporter",
                                           label2="log_valid", title=str(column_to_test),
                                           xlabel="every half year", ylabel=str(column_to_test), category=str(column_to_test),
                                           show=True, line=False, alpha=0.8, overlap=True)

                outfile.write("Testing whether the reporter year matches the simulation duration:\n")
                if i != len(years):
                    success = False
                    outfile.write("BAD: the reporter has data up to year {0} but the simulation duration is {1}, we are expecting "
                                  "not more than year {2} from reporter.".format(max(years), simulation_duration,
                                                                           math.floor(simulation_duration/180)))
                if simulation_duration > round(max(years) * sft.DAYS_IN_YEAR) + 180:
                    success = False
                    outfile.write("BAD: the reporter has data up to year {0} but the simulation duration is {1}, we are expecting "
                                  "data after year {0} from reporter.".format(max(years), simulation_duration))


        outfile.write(sft.format_success_msg(success))

        if debug:
            print( "SUMMARY: Success={0}\n".format(success) )
        return success

def create_report_file_prevalence(column_to_test, column_year, param_obj, output_dict, reporter_df, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[Config.config_name]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        # simulation_duration = param_obj[Config.duration]
        # timestep = param_obj[Config.simulation_timestep]
        if not len(output_dict):
            success = False
            outfile.write(sft.sft_no_test_data)

        outfile.write("Group the {} prevalence by year and get the sum for all age bins:\n".format(column_to_test))
        # the year column becomes the index of the groupby_df
        groupby_df = reporter_df.groupby(column_year).sum()

        if not groupby_df[column_to_test].sum():
            success = False
            outfile.write("BAD: there in no {} prevalence in the test, please check the test.\n".format(column_to_test))

        if debug:
            with open("DEBUG_groupby_dataframe.csv", "w") as groupby_file:
                groupby_df.to_csv(groupby_file, header=True)

        outfile.write("Testing the {} prevalence count with log_valid for all year buckets:\n".format(column_to_test))
        i = prevalence_sum = 0
        years = groupby_df.index.values
        prevalence_counts = []
        prevalence_at_last_time_step = {}

        for t in output_dict:
            if i < len(years):
                year = years[i]
                if t <= round(year * sft.DAYS_IN_YEAR):
                    prevalence_sum += output_dict[t]
                    if t == round(year * sft.DAYS_IN_YEAR):
                        prevalence_at_last_time_step[t] = output_dict[t]
                else:
                    reporter_sum = int(groupby_df[groupby_df.index==year][column_to_test])
                    prevalence = prevalence_sum / (years[0] * sft.DAYS_IN_YEAR)
                    prevalence_counts.append(prevalence)
                    # uncomment the following lines to test average prevalence
                    # if prevalence != reporter_sum:
                    #     success = False
                    #     outfile.write("BAD: in year {0} the HIV prevalence count get from reporter is {1}, while test.txt reports"
                    #                   " {2} cases.\n".format(year, reporter_sum, prevalence))
                    if prevalence_at_last_time_step[sorted(prevalence_at_last_time_step.keys())[-1]] != reporter_sum:
                        success = False
                        outfile.write("BAD: in year {0} the {1} prevalence count get from reporter is {2}, while test.txt reports"
                                      " {3} cases at the last time step of this report time window.\n".format(
                            year, column_to_test,reporter_sum, prevalence_at_last_time_step[sorted(prevalence_at_last_time_step.keys())[-1]]))
                    prevalence_sum = output_dict[t]
                    i += 1
            else:
                break
        # uncomment the following lines to plot average prevalence from logging and prevalence from reporter
        # sft.plot_data(prevalence_counts, dist2=np.array(groupby_df[ReportColumn.HIV]), label1="log_valid_on_average",
        #                            label2="reporter", title="HIV prevalence",
        #                            xlabel="every half year", ylabel="HIV prevalence", category='HIV_prevalence',
        #                            show=True, line=True, alpha=0.8, overlap=True)
        sft.plot_data([prevalence_at_last_time_step[key] for key in sorted(prevalence_at_last_time_step.keys())],
                          dist2=np.array(groupby_df[column_to_test]), label1="log_valid",
                          label2="reporter", title="{} prevalence at last timestep of each report time window".format(column_to_test),
                          xlabel="every half year", ylabel="{} prevalence".format(column_to_test),
                          category='{}_prevalence_last_time_step'.format(column_to_test),
                          show=True, line=True, alpha=0.8, overlap=True)
        if debug:
            with open('DEBUG_prevalence_at_last_time_step.json','w') as file:
                json.dump(prevalence_at_last_time_step, file, indent=4)
        outfile.write(sft.format_success_msg(success))

    if debug:
        print( "SUMMARY: Success={0}\n".format(success) )
    return success