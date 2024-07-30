#!/usr/bin/python
import os
if __name__ == '__main__':
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_HIVDeath_Support as hiv_s
from dtk_test.dtk_StdOut import StdOut, SearchType
from dtk_test.dtk_OutputFile import ReportEventRecorder

import math
import numpy as np


"""
This test is testing the HIV symptomatic to death and infection to death durations. (testing both duration counters are 
honored.)

Data to test: StdOut debug logging and ReportEventRecorder.csv

main test: test symptomatic to death duration counters and infection to death duration counters are honored. 
If symptomatic to death counter is less than the infection to death counter for an individual, then the infection to 
death counter is used as the symptomatic to death counter. The simulation runs with 150+ population for 33 years. 

            
Output: scientific_feature_report.txt
        Duration(StdOut).png
        years_of_death.png
        years_of_symptomatic.png
        
"""


# filters for different counters
filter_string_list_1 = ["Update(): Time:",
                        "days_between_sympotmatic_and_death"]


load_df_param_1 = [[hiv_s.Constant.ind_id, hiv_s.Constant.total_duration,
                    hiv_s.Constant.symptomatic_duration, hiv_s.Constant.time_of_infection],
                   ["Individual id=", "days_until_death_by_HIV=",
                    "days_between_sympotmatic_and_death=", "Time_of_Infection="],
                   [SearchType.VAL, SearchType.VAL, SearchType.VAL, SearchType.VAL]]


# filters for fast forward duration
filter_string_list_2 = ["Update(): Time:",
                        "m_infection_stage"]


load_df_param_2 = [[hiv_s.Constant.ind_id, hiv_s.Constant.current_duration],
                   ["Individual ", "duration = "],
                   [SearchType.VAL, SearchType.VAL]]


def create_report_file(duration_df, event_df, fast_forward_duration_df, total_pop, config_filename, report_name):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        # Load parameters to test from config
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")
        simulation_duration_year = dtk_sft.get_simulation_duration(config_filename) / dtk_sft.DAYS_IN_YEAR

        cp = hiv_s.ConfigParam
        variables = [value for key, value in cp.__dict__.items() if key[:1] != '_']
        variable_values = dtk_sft.get_config_parameter(config_filename, variables)
        base_year = variable_values[variables.index(cp.base_year)]
        for i, variable_value in enumerate(variable_values):
            output_report_file.write(f"\t {variables[i]} = {variable_value}\n")
        output_report_file.write(f"Simulation_Duration is {simulation_duration_year} years.\n")

        # Merge test data from different source:
        # merge duration df with fast forward duration, only keep times that we are interested with inner join
        duration_df = duration_df.merge(fast_forward_duration_df, how='inner', on=["Time", hiv_s.Constant.ind_id])
        # drop duplicated rows from from fast_forward_duration_df
        duration_df = duration_df.drop_duplicates(subset=["Time", hiv_s.Constant.ind_id], keep="last")
        # calculate the latent_duration for current time step from data collected
        duration_df[hiv_s.Constant.latent_duration] = duration_df[hiv_s.Constant.total_duration] - \
                                                      duration_df[hiv_s.Constant.symptomatic_duration] - \
                                                      duration_df[hiv_s.Constant.current_duration]

        # Check test condition:
        # If an individual has a total duration less than the duration draw from the weibull distribution under
        # test, the actual symptomatic to death duration is replace by the total duration. This is one of the test case
        # that we want to test.
        non_positive_count = sum(n <= 0 for n in duration_df[hiv_s.Constant.latent_duration].values.tolist())
        if not non_positive_count:
            output_report_file.write(f"WARNING: all individuals have latent duration > 0). We want to test cases that"
                                     f"have latent duration <= 0, please check the test.\n")
        else:
            output_report_file.write(f"{non_positive_count} out of {total_pop} individuals should have symptomatic "
                                     f"presentation occur immediately upon infection(latent duration <= 0). We will "
                                     f"test it later.\n")

        # Plot all type durations in the test:
        hiv_s.plot_durations(duration_df)

        # Main test:
        # Prepare test data:
        # get actual_latent_duration, convert all negative values to 0 for latent duration
        duration_df[hiv_s.Constant.actual_latent_duration] = np.where(duration_df[hiv_s.Constant.latent_duration] < 0,
                                                             0, duration_df[hiv_s.Constant.latent_duration])
        # calculate actual time of symptomatic and time of death
        duration_df[hiv_s.Constant.time_of_symptomatic] = duration_df[hiv_s.Constant.time_of_infection] + \
                                                    duration_df[hiv_s.Constant.actual_latent_duration]
        duration_df[hiv_s.Constant.time_of_death] = duration_df[hiv_s.Constant.time_of_infection] + \
                                              duration_df[hiv_s.Constant.total_duration] - \
                                              duration_df[hiv_s.Constant.current_duration]  # duration is fast forwarded
        # convert them into simulation year format
        duration_df[hiv_s.Constant.year_of_symptomatic] = round(duration_df[hiv_s.Constant.time_of_symptomatic] /
                                                          dtk_sft.DAYS_IN_YEAR + base_year, 2)
        duration_df[hiv_s.Constant.year_of_death] = round(duration_df[hiv_s.Constant.time_of_death] /
                                                    dtk_sft.DAYS_IN_YEAR + base_year, 2)

        years_of_symptomatic, actual_years_of_symptomatic = [], []
        years_of_death, actual_years_of_death = [], []
        result_symptomatic = result_death = True
        for idx, row in duration_df.iterrows():
            # collect expected data from duration_df(stdout)
            ind_id = int(row.loc[hiv_s.Constant.ind_id])
            year_of_symptomatic = row.loc[hiv_s.Constant.year_of_symptomatic]
            year_of_death = row.loc[hiv_s.Constant.year_of_death]
            # collect test data from ReportEventRecorder.csv
            actual_year_of_symptomatic = event_df[
                (event_df[hiv_s.EventReport.ind_id] == ind_id) &
                (event_df[hiv_s.EventReport.event_name] == hiv_s.EventReport.symptomatic)][
                hiv_s.EventReport.year]
            actual_year_of_symptomatic = actual_year_of_symptomatic.iloc[0] if not actual_year_of_symptomatic.empty else 0
            actual_year_of_death = event_df[(event_df[hiv_s.EventReport.ind_id] == ind_id) & \
                                            (event_df[hiv_s.EventReport.event_name] == hiv_s.EventReport.death)][
                hiv_s.EventReport.year]
            actual_year_of_death = actual_year_of_death.iloc[0] if not actual_year_of_death.empty else 0
            # save them for plotting later
            years_of_symptomatic.append(year_of_symptomatic)
            years_of_death.append(year_of_death)
            actual_years_of_symptomatic.append(actual_year_of_symptomatic)
            actual_years_of_death.append(actual_year_of_death)

            # testing the actual time of symptomatic
            if year_of_symptomatic < simulation_duration_year + base_year and \
                    round(math.fabs(year_of_symptomatic - actual_year_of_symptomatic), 2) > 1e-2:
                # use round(, 2) to avoid Floating Point limitation in Python
                result_symptomatic = False
                if actual_year_of_symptomatic == 0:
                    output_report_file.write(f"BAD: individual={ind_id} is expected to become symptomatic at year "
                                             f"{year_of_symptomatic}, but it didn't' receive the "
                                             f"{hiv_s.EventReport.symptomatic} event before simulation end at year "
                                             f"{simulation_duration_year + base_year}.\n")
                else:
                    output_report_file.write(f"BAD: individual={ind_id} is expected to become symptomatic at year "
                                             f"{year_of_symptomatic}, but it received the {hiv_s.EventReport.symptomatic} "
                                             f"event at year {actual_year_of_symptomatic}.\n")
            # testing the actual time of death
            if year_of_death < simulation_duration_year + base_year and \
                    round(math.fabs(year_of_death - actual_year_of_death), 2) > 1e-2:
                # use round(, 2) to avoid Floating Point limitation in Python
                result_death = False
                if actual_year_of_death == 0:
                    output_report_file.write(f"BAD: individual={ind_id} is expected to die at year {year_of_death}, "
                                             f"but it didn't receive the {hiv_s.EventReport.death} event before simulation "
                                             f"end at year {simulation_duration_year + base_year}.\n")
                else:
                    output_report_file.write(f"BAD: individual={ind_id} is expected to die at year {year_of_death}, "
                                             f"but it received the {hiv_s.EventReport.death} event at year "
                                             f"{actual_year_of_death}.\n")

        if not result_symptomatic:
            succeed = False
        else:
            output_report_file.write(f"GOOD: year_of_symptomatic matches the actual_year_of_symptomatic for all "
                                     f"{total_pop} individuals.\n")
        if not result_death:
            succeed = False
        else:
            output_report_file.write(f"GOOD: year_of_death matches the actual_year_of_death for all "
                                     f"{total_pop} individuals.\n")

        output_report_file.write(dtk_sft.format_success_msg(succeed))

        # plot test data with expected data
        dtk_sft.plot_data(years_of_symptomatic, actual_years_of_symptomatic,
                          sort=False, category="years_of_symptomatic", overlap=True,
                          label1="expected years_of_symptomatic",label2="actual_years_of_symptomatic",
                          xlabel="Data Points", ylabel="Years",
                          title="years_of_symptomatic")
        dtk_sft.plot_data(years_of_death, actual_years_of_death,
                          sort=False, category="years_of_death", overlap=True,
                          label1="expected years_of_death", label2="actual_years_of_death",
                          xlabel="Data Points", ylabel="Years",
                          title="years_of_death")

        return succeed


def application(output_folder="output", stdout_filename="test.txt",
                event_report_name="ReportEventRecorder.csv",
                config_filename="config.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("event_report_name: " + event_report_name + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done(stdout_filename)
    stdout = StdOut(stdout_filename, filter_string_list_1, load_df_param_1)
    duration_df = stdout.df
    duration_df = duration_df.astype({load_df_param_1[0][0]: 'int64'})

    total_pop = 0
    for line in stdout.filtered_lines:
        if "Time: 1" in line:
            total_pop = int(dtk_sft.get_val("StatPop: ", line))
            break
    if total_pop <= 0:
        raise ValueError(f"total population is {total_pop}, please check the test.")

    stdout_2 = StdOut(stdout_filename, filter_string_list_2, load_df_param_2)
    fast_forward_duration_df = stdout_2.df
    fast_forward_duration_df = fast_forward_duration_df.astype({load_df_param_2[0][0]: 'int64'})

    event_df = ReportEventRecorder(os.path.join(output_folder, event_report_name)).df

    create_report_file(duration_df, event_df, fast_forward_duration_df, total_pop, config_filename, report_name)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-e', '--event_report', default="ReportEventRecorder.csv",
                        help="Report Event Recorder to parse (ReportEventRecorder.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                event_report_name=args.event_report,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

