#!/usr/bin/python

import dtk_test.dtk_sft as sft
import dtk_test.dtk_TBHIV_Reporter_Support as trs
import dtk_test.dtk_TBHIV_Multicore_Support as tms
import json
import math
import numpy as np
import platform

"""
Reporter: Disease Deaths and HIV Deaths

This test is design to run with multinode and multicore setup.

This test also make sure that the following test conditions are met:
    1. multi nodes
    2. multi cores
    3. human migration is happening
    4. If it's not running in Windows, make sure that the stdout file has each core reporting in every time step.

If it's not running in Windows, it test the DiseaseDeaths and HIVDeaths column in the Report_TBHIV_ByAge.csv match the 
count for deaths due to TB and HIV in stdout.txt. It also compare the DiseaseDeaths column in the csv with the 
Disease Deaths channel in InsetChart.json.

If it's in Windows OS, it only test the the DiseaseDeaths column in the csv with the 
Disease Deaths channel in InsetChart.json.

"""


class ReportColumn:
    year = "Year"
    hivdeaths = " HIVDeaths"
    diseasedeaths = " DiseaseDeaths"
    agebin = " AgeBin"


class Config:
    config_name = "Config_Name"
    simulation_timestep = "Simulation_Timestep"
    duration = "Simulation_Duration"
    num_core = "Num_Cores"
    demog_filename = "Demographics_Filenames"

class InsetChart:
    disease_death = "Disease Deaths"

matches = ["Time: ",
           "Rank: ",
           "moved from Active Symptomatic to Death",
           "died from non-TB opportunistic infection"
           ]


# this is for multicore test only
# In Windows, there is an issue that some information get omitted when merging multiple stdout.txt to a single one
# When running this in Windows, it will catch the exception and fail the test.
def parse_output_file(output_filename="test.txt", debug=False):
    """
    creates a dictionary to store filtered information for each time step
    :param output_filename: file to parse (test.txt)
    :return:                output_dict, message
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
    core = 0
    output_dict = {}
    exception_message = None
    for line in filtered_lines:
        try:
            if matches[0] in line:
                #this may raise LookupError
                value = sft.get_val(matches[0], line)
                if debug:
                    print("time value I get is '{}'".format(value))
                # this may raise ValueError
                time_step = int(float(value))

                if matches[1] in line:
                    # this may raise ValueError or LookupError
                    core = int(sft.get_val(matches[1], line))
                else:
                    print (line)
                    raise Exception("at timestep = {0}, {1} and {2) are not in the same line.\n".format(
                        time_step, matches[0], matches[1]))
                if debug:
                    print("core is {}".format(core))

                if time_step not in output_dict:
                    output_dict[time_step] = {core:[0,0]}
                elif core not in output_dict[time_step]:
                    output_dict[time_step][core] = [0,0]
            elif matches[2] in line: # this individual is died from TB Symptomatic active
                output_dict[time_step][core][0] += 1
            elif matches[3] in line: # this individual died from HIV
                output_dict[time_step][core][1] += 1

        except Exception as ex:
            exception_message = "failed to parse {0}, got exception: {1}.".format(output_filename, ex)
            print (exception_message)
            return None, exception_message

    if debug:
        res_path = r'./DEBUG_stdout_parsed.json'
        with open(res_path, "w") as file:
            json.dump(output_dict, file, indent=4)
    return output_dict, exception_message


def create_report_file_incidence(column_diseasedeath, column_hivdeath, column_year, json_obj, param_obj, reporter_df,
                                 migration_df, node_list, stdout_filename, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[trs.Config.config_name]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        simulation_duration = param_obj[Config.duration]
        # timestep = param_obj[Config.simulation_timestep]

        outfile.write("Group the column_to_test by year and get the sum for all age bins:\n")
        # the year column becomes the index of the groupby_df
        groupby_df = reporter_df.groupby(column_year).sum()

        if debug:
            with open("DEBUG_groupby_dataframe.csv", "w") as groupby_file:
                groupby_df.to_csv(groupby_file, header=True)
        outfile.write("checking some test conditions:\n")
        success = tms.check_test_condition(param_obj[Config.num_core], node_list,  migration_df, outfile)

        if platform.system() is 'Windows':
            # in Windows, there is issue when merging multiple stdouts(from different cores) into a single one.
            outfile.write("OS is {0}, let's only compare {1} column in report with {2} channel in insetchart output."
                          "\n".format(platform.system(), column_diseasedeath, InsetChart.disease_death))
            # actual test code is outside the if block, result = compare_report_json()
        # skip the following if OS is Windows
        else:
            outfile.write("OS is {}, let's compare report with insetchart and stdout output.\n".format(platform.system()))
            outfile.write("parse stdout({}) file:\n".format(stdout_filename))
            output_dict, exception_message = parse_output_file( stdout_filename, debug)
            if exception_message:
                success = False
                outfile.write(exception_message+'\n')
                outfile.write("parse stdout file failed, let's still compare report with insetchart output.\n")
                # actual test code is outside the first if block, result = compare_report_json()
            # skip the following if we can't parse the stdout file
            else:
                outfile.write("parse stdout file succeed, let's compare report with both stdout and insetchart output.\n")
                outfile.write("-- compare report with stdout file:\n")
                result1 = True
                if not groupby_df[column_diseasedeath].sum() and not groupby_df[column_hivdeath].sum():
                    success = False
                    outfile.write(
                        "BAD: there in no {0} or {1} in the report, please check the test.\n".format(column_diseasedeath,
                                                                                                   column_hivdeath))
                # skip the following if no interested data in report.
                else:
                    if not len(output_dict):
                        success = False
                        outfile.write(sft.sft_no_test_data)
                        outfile.write("BAD: stdout file has no test data")
                    # skip the following if parsing stdout doesn't throw exception but there is no test data in stdout file.
                    else:
                        outfile.write("Testing the {} count with log_valid for all year buckets:\n".format(column_diseasedeath + " and " + column_hivdeath))
                        test_columns = [column_diseasedeath, column_hivdeath] # two test columns that we are looking at
                        for n in range(len(test_columns)):
                            column_to_test = test_columns[n]
                            i = incidence_count = 0
                            years = groupby_df.index.values
                            incidence_counts = []

                            for t in output_dict:
                                if i < len(years):
                                    year = years[i]
                                    if t <= round(year * sft.DAYS_IN_YEAR):
                                        for core in output_dict[t]:
                                            incidence_count += output_dict[t][core][n]
                                    else: # after the last time step of the reporting window
                                        reporter_sum = int(groupby_df[groupby_df.index==year][column_to_test])
                                        incidence_counts.append(incidence_count) # collected for plot method
                                        if incidence_count != reporter_sum:
                                            success = result1 = False
                                            outfile.write("BAD: in year {0} the {1} count get from reporter is {2}, while test.txt reports"
                                                          " {3} cases.\n".format(year, column_to_test, reporter_sum, incidence_count))
                                        incidence_count = 0 # initialize for next test window
                                        for core in output_dict[t]: # collect the first time step data for each time window
                                            incidence_count += output_dict[t][core][n]
                                        i += 1
                                else:
                                    break

                            sft.plot_data(incidence_counts, dist2=np.array(groupby_df[column_to_test]), label1="log_valid",
                                                       label2="reporter", title=str(column_to_test),
                                                       xlabel="every half year", ylabel=str(column_to_test),
                                                       category=str(column_to_test)+"_log_valid",
                                                       show=True, line=False, alpha=0.8, overlap=True)

                            outfile.write("Testing whether the reporter year matches the simulation duration:\n")
                            if i != len(years):
                                success = result1 = False
                                outfile.write("BAD: the reporter has data up to year {0} but the simulation duration is {1}, "
                                              "we are expecting not more than year {2} from reporter."
                                              "".format(max(years), simulation_duration, math.floor(simulation_duration/180)))

                            if simulation_duration > round(max(years) * sft.DAYS_IN_YEAR) + 180:
                                success = result1 = False
                                outfile.write("BAD: the reporter has data up to year {0} but the simulation duration is {1}, "
                                              "we are expecting data after year {0} from reporter."
                                              "".format(max(years), simulation_duration))
                        outfile.write("compare report with stdout file result is {}.\n".format(result1))

                        outfile.write("-- checking if all cores are reporting in every time step in stdout file:\n")
                        core_list = [int(n) for n in (range(param_obj[Config.num_core]))]
                        result2 = True
                        for t, cores in output_dict.items():
                            cores = list(cores.keys())
                            if core_list != sorted(cores): # compare two list of cores
                                result2 = success = False
                                outfile.write("BAD: at time step {0}, these cores reported to stdout.txt are: {1}, while "
                                              "expected cores are: {2}.\n".format(t, cores, core_list))
                        outfile.write("checking if all cores are reporting in every time step in stdout file: result is "
                                      "{}.\n".format(result2))

                    outfile.write("-- compare {} report with json output(insetchart).\n".format(column_diseasedeath))
                    # actual test code is outside the first if block, result = compare_report_json()


        result = tms.compare_report_json(column_diseasedeath, InsetChart.disease_death, groupby_df, json_obj, outfile)
        if not result:
            success = False
            outfile.write("BAD: report doesn't match insetchart.\n")
        outfile.write("compare {0} in report with json output result is {1}.\n".format(column_diseasedeath, result))
        outfile.write(sft.format_success_msg(success))

        if debug:
            print( "SUMMARY: Success={0}\n".format(success) )
        return success


def application( output_folder="output", stdout_filename="test.txt", reporter_filename="Report_TBHIV_ByAge.csv",
                 inset_chart_filename = "InsetChart.json", migration_report_filename="ReportHumanMigrationTracking.csv",
                 config_filename="config.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "reporter_filename: " + reporter_filename+ "\n" )
        print( "inset_chart_filename: " + inset_chart_filename+ "\n" )
        print( "migration_report_filename: " + migration_report_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )


    sft.wait_for_done()

    param_obj = tms.load_emod_parameters([Config.config_name, Config.simulation_timestep, Config.duration,
                                          Config.num_core, Config.demog_filename],config_filename, debug)

    demog_filename = param_obj[Config.demog_filename][-1]
    node_list = tms.get_node_counts(demog_filename)

    json_obj = tms.parse_json_report([InsetChart.disease_death], inset_chart_filename, output_folder,debug)

    migration_df = tms.load_migration_report(migration_report_filename, output_folder)


    report_column_list = [ReportColumn.year, ReportColumn.agebin, ReportColumn.hivdeaths, ReportColumn.diseasedeaths]
    reporter_df = trs.parse_custom_reporter(report_column_list, reporter_path=output_folder, reporter_filename=reporter_filename, debug=debug)

    create_report_file_incidence(ReportColumn.diseasedeaths, ReportColumn.hivdeaths, ReportColumn.year, json_obj,
                                 param_obj, reporter_df, migration_df, node_list, stdout_filename, report_name, debug)

if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-j', '--reportername', default="Report_TBHIV_ByAge.csv", help="reporter to test(Report_TBHIV_ByAge.csv)")
    parser.add_argument('-i', '--insetchartname', default="InsetChart.json", help="insetchart to test(InsetChart.json)")
    parser.add_argument('-m', '--migrationreport', default="ReportHumanMigrationTracking.csv", help="migration report to test(ReportHumanMigrationTracking.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', default=False, help="Debug = True or False")
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout, reporter_filename=args.reportername,
                inset_chart_filename = args.insetchartname, migration_report_filename=args.migrationreport,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)
