#!/usr/bin/python
import os
if __name__ == '__main__':
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_ARTMortalityTable_Support as amt_s
from dtk_test.dtk_OutputFile import ReportEventRecorder
import numpy as np
"""
This test is testing the HIV survival duration on ARTMortalityTable with different age bins. (Test 1: one 
ARTMortalityTable intervention with different mortality rates including 0 and 1)

Data to test: ReportEventRecorder.csv

main test: test whether the survival duration on ARTMortalityTable(duration after intervention starts) follows an
exponential distribution with rate defined in the MortalityTable. All dimensions except age have only one bin.

Output: scientific_feature_report.txt
        survival_durations_onART_lt-15.png
        survival_durations_onART_15-25.png
        survival_durations_onART_25-35.png
        survival_durations_onART_35-45.png
        survival_durations_onART_45-55.png
        survival_durations_onART_55-65.png
        survival_durations_onART_gt-65.png
        
"""


def create_report_file(event_df, config_filename, report_name):

    succeed = True
    with open(report_name, 'w') as output_report_file:
        # load parameters to test from config
        base_year, simulation_duration, simulation_duration_year, simulation_timestep, campaign_filename = \
            amt_s.load_config(config_filename, output_report_file)

        # load campaign file
        campaign_object = amt_s.load_campaign(campaign_filename)

        # check campaign configuration
        expected_one_bin = [amt_s.Campaign.art_duration_days_bins,
                            amt_s.Campaign.cd4_count_bins]

        res_msg, start_day, ip_name, ip_value, art_mortality_table = amt_s.check_campaign(campaign_object,
                                                                                          campaign_filename,
                                                                                          expected_one_bin,
                                                                                          expected_intervention_count=1)
        for msg in res_msg:
            output_report_file.write(msg)
            if "BAD" in msg:
                succeed = False

        age_years_bins = art_mortality_table[amt_s.Campaign.age_years_bins]
        if len(age_years_bins) == 1:
            output_report_file.write(f"WARNING: {amt_s.Campaign.age_years_bins} only has one element, please test more"
                                     f" than one age bins.\n")

        # prepare test data
        event_df[amt_s.EventReport.age_bin] = event_df[amt_s.EventReport.age].\
            apply(lambda x: amt_s.convert_value_to_bin(x / dtk_sft.DAYS_IN_YEAR, age_years_bins))
        event_death_df = event_df[event_df[amt_s.EventReport.event_name] == amt_s.EventReport.death][[
            amt_s.EventReport.year, amt_s.EventReport.ind_id]]
        event_startART_df = event_df[event_df[amt_s.EventReport.event_name] == amt_s.EventReport.started_art][[
                                                                                          amt_s.EventReport.ind_id,
                                                                                          amt_s.EventReport.age_bin]]
        df_to_test = event_death_df.merge(event_startART_df, on=amt_s.EventReport.ind_id, how="inner")

        # age_bins = df_to_test[amt_s.EventReport.age_bin].unique() # this will skip age bin with rate=0, replace with:
        age_bins = convert_age_bins_to_age_bins_string(age_years_bins)
        mortality_table = art_mortality_table[amt_s.Campaign.mortality_table]
        # main test
        res_msg2 = amt_s.test_survival_time_on_age(df_to_test, age_bins, age_years_bins, mortality_table,
                                                   start_day, base_year, simulation_duration_year)
        for msg in res_msg2:
            output_report_file.write(msg)
            if "BAD" in msg:
                succeed = False

        output_report_file.write(dtk_sft.format_success_msg(succeed))

        return succeed


def convert_age_bins_to_age_bins_string(age_years_bins):
    res = ["lt-" + str(age_years_bins[0])]
    for idx in range(1, len(age_years_bins)):
        res.append(str(age_years_bins[idx -1]) + "-" + str(age_years_bins[idx]))
    res.append("gt-" + str(age_years_bins[-1]))
    return res


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

    event_df = ReportEventRecorder(os.path.join(output_folder, event_report_name)).df

    create_report_file(event_df,  config_filename, report_name)


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

