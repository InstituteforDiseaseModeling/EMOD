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

from scipy import stats
import math


"""
This test is testing the HIV symptomatic to death duration. Test 1: testing the actual duration.

Upon infection or ART discontinuation, the individuals draw from the configured Weibull distribution to
determine the time between symptomatic presentation and untreated AIDS-related death. The duration
is subtracted from the AIDS-related death date(described in the previous scenario) to determine the time
of symptomatic presentation. Small values lead to symptomatic presentation close to the time of
AIDS-related death. Large values lead to symptomatic presentation well before AIDS-related death. If the
drawn time is longer than the total survival time, then the total survival time isused (i.e.,symptomatic
presentation occurs immediately upon infection). The date of symptomatic presentation has no direct
impact on clinical progression; however, it can be used to configure health-seeking behavior

Data to test: StdOut debug logging and ReportEventRecorder.csv

main test 1: test whether disease death is reported correctly in ReportEventRecorder.csv.
main test 2: test actual symptomatic to death durations is drawn from the expected weibull distribution.

Parameters to test:
            "Days_Between_Symptomatic_And_Death_Weibull_Heterogeneity"
            "Days_Between_Symptomatic_And_Death_Weibull_Scale"
            
Output: scientific_feature_report.txt
        Died with HIV count(StdOut).png
        Died with HIV(StdOut).png
        symptomatic_to_death_durations.png
        weibull_cdf.png
        
"""


filter_string_list = ["Update(): Time:",
                      "(HIV) destructor"]

load_df_param = [["Died with HIV"], ["[IndividualHIV] "], [SearchType.VAL]]


def create_report_file(death_df, death_count_df, total_pop, event_df, config_filename, event_report_name, report_name):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        # load parameters to test from config
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")
        simulation_duration_year = dtk_sft.get_simulation_duration(config_filename) / dtk_sft.DAYS_IN_YEAR

        sym_to_death_heterogeneity, sym_to_death_scale, base_year = dtk_sft.get_config_parameter(config_filename,
                                                                                 [hiv_s.ConfigParam.Sym_to_Death_H,
                                                                                  hiv_s.ConfigParam.Sym_to_Death_S,
                                                                                  hiv_s.ConfigParam.base_year])
        output_report_file.write(f"{hiv_s.ConfigParam.Sym_to_Death_H} = {sym_to_death_heterogeneity}\n")
        output_report_file.write(f"{hiv_s.ConfigParam.Sym_to_Death_S} = {sym_to_death_scale}\n")

        # main test 1: make sure the disease death is reported correctly in ReportEventRecorder.csv
        # data from ReportEventRecorder.csv
        death_event_df = event_df[event_df[hiv_s.EventReport.event_name] == hiv_s.EventReport.death]
        death_event_count_df = death_event_df.groupby(hiv_s.EventReport.year).size().\
            reset_index(name='count').sort_values(by=[hiv_s.EventReport.year])
        # data from debug logging
        death_count_df[hiv_s.EventReport.year] = round(death_count_df["Time"]/dtk_sft.DAYS_IN_YEAR + base_year, 2)
        death_count_df = death_count_df.groupby(hiv_s.EventReport.year).sum().\
            reset_index().sort_values(by=[hiv_s.EventReport.year])

        result = death_count_df[[hiv_s.EventReport.year, 'count']].equals(death_event_count_df)
        msg = f"compare {hiv_s.EventReport.death} event from {event_report_name} with debug logging for " \
            f"{load_df_param[0][0]} from stdout, result is: {result}.\n"
        if not result:
            succeed = False
            output_report_file.write("BAD: " + msg)
            output_report_file.write("Please see DEBUG_death_count_df.csv and DEBUG_death_event_count_df.csv for data"
                                     " under test.\n")
            death_count_df.to_csv("DEBUG_death_count_df.csv")
            death_event_count_df.to_csv("DEBUG_death_event_count_df.csv")
        else:
            output_report_file.write("GOOD: " + msg)

        # main test 2
        # collect the symptomatic to death duration from stdout file and ReportEventRecorder.csv
        infection_to_symptomatic_durations = []
        infection_to_death_durations = []
        symptomatic_to_death_durations = []

        symptomatic_df = event_df[event_df[hiv_s.EventReport.event_name] == hiv_s.EventReport.symptomatic]
        for ind_id in symptomatic_df[hiv_s.EventReport.ind_id].unique():
            symptomatic_time = symptomatic_df[(symptomatic_df[hiv_s.EventReport.ind_id] == ind_id) &
                                              (symptomatic_df[hiv_s.EventReport.event_name] == hiv_s.EventReport.symptomatic)
                                              ][hiv_s.EventReport.year]
            symptomatic_time = -1 if symptomatic_time.empty else symptomatic_time.iloc[0]
            death_time = death_df[death_df[load_df_param[0][0]] == ind_id]["Time"]
            death_time = -1 if death_time.empty else death_time.iloc[0]
            if death_time != -1:
                if symptomatic_time == -1:
                    succeed = False
                    output_report_file.write(f"\tBAD: individual {ind_id} die without broadcasting "
                                             f"{hiv_s.EventReport.symptomatic} event.\n")
                infection_to_symptomatic_durations.append((symptomatic_time - base_year) * dtk_sft.DAYS_IN_YEAR)
                infection_to_death_durations.append(death_time)
                symptomatic_to_death_durations.append((death_time - (symptomatic_time - base_year) * dtk_sft.DAYS_IN_YEAR))

        # If too many individuals have a total duration less than the duration draw from the weibull distribution under
        # test, the actual symptomatic to death duration is replace by the total duration. In this case the
        # test_weibull() method may fail the test.
        zero_count = infection_to_symptomatic_durations.count(0)
        if zero_count:
            output_report_file.write(f"WARNING: {zero_count} out of {total_pop} individuals have symptomatic "
                                     f"presentation occur immediately upon infection.\n")

        # size of test data
        output_report_file.write(f"{len(symptomatic_to_death_durations)} out of {total_pop} individuals died in "
                                 f"the simulation(duration = {simulation_duration_year} years).\n")
        result = dtk_sft.test_weibull(symptomatic_to_death_durations, sym_to_death_scale, 1.0/sym_to_death_heterogeneity,
                                      report_file=output_report_file, integer=True)
        if not result:
            succeed = False

        # plotting
        dist_weibull_scipy = stats.weibull_min.rvs(c=1.0/sym_to_death_heterogeneity, loc=0, scale=sym_to_death_scale,
                                                   size=len(symptomatic_to_death_durations))
        dist_weibull_scipy = [math.ceil(x) for x in dist_weibull_scipy]  # actual duration is discrete
        dtk_sft.plot_data(symptomatic_to_death_durations, dist_weibull_scipy,
                          sort=True, category="symptomatic_to_death_durations", overlap=True,
                          label1="symptomatic_to_death_durations",label2="weibull from scipy",
                          xlabel="Data Points", ylabel="Duration",
                          title="symptomatic_to_death_durations")

        dtk_sft.plot_cdf_w_fun(symptomatic_to_death_durations, name="weibull_cdf", cdf_function=stats.weibull_min.cdf,
                               args=(1.0/sym_to_death_heterogeneity, 0, sym_to_death_scale), show=True)

        output_report_file.write(dtk_sft.format_success_msg(succeed))

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
    stdout = StdOut(stdout_filename, filter_string_list, load_df_param)
    death_df = stdout.df
    death_df = death_df.astype({load_df_param[0][0]: 'int64'})
    death_count_df = stdout.count_incidence()
    # plot the test data
    stdout.plot_df()
    stdout.plot_df(death_count_df)
    total_pop = 0
    for line in stdout.filtered_lines:
        if "Time: 1" in line:
            total_pop = int(dtk_sft.get_val("StatPop: ", line))
            break
    if total_pop <= 0:
        raise ValueError(f"total population is {total_pop}, please check the test.")

    event_df = ReportEventRecorder(os.path.join(output_folder, event_report_name)).df

    create_report_file(death_df, death_count_df, total_pop, event_df, config_filename, event_report_name, report_name)


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

