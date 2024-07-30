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
"""
This test is testing the HIV survival duration on ARTMortalityTable with different CD4 Count bins. (Test 1: testing 
correct selection on cd4 count bins)

Data to test: ReportEventRecorder.csv

main test: test whether the survival duration on ARTMortalityTable(duration after intervention starts) follows an
exponential distribution with rate defined in the MortalityTable. All dimensions except CD4 Count have only one bin.
The mortality rate calculated with cd4 count is using linear interpolation, since cd4 count for each individual is drawn
from a weibull distribution, each individual will have different mortality rate except the first and last cd4 count bins.
So for the first and last cd4 count bins, we test them with significance level = 5%, like the other SFTs. For the other
cd4 count bins, we test them with a bigger critical value and larger significance level(0.1%).

Output: scientific_feature_report.txt
        PDF_of_Survival_Duration_per_CD4_Bin.png
        CD4_count_density.png
        survival_durations_onART_lt-100.png
        survival_durations_onART_100-200.png
        survival_durations_onART_200-400.png
        survival_durations_onART_gt-400.png
        
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
                            amt_s.Campaign.age_years_bins]

        res_msg, start_day, ip_name, ip_value, art_mortality_table = amt_s.check_campaign(campaign_object,
                                                                                          campaign_filename,
                                                                                          expected_one_bin,
                                                                                          expected_intervention_count=1)
        for msg in res_msg:
            output_report_file.write(msg)
            if "BAD" in msg:
                succeed = False

        cd4_count_bins = art_mortality_table[amt_s.Campaign.cd4_count_bins]

        if len(cd4_count_bins) == 1:
            output_report_file.write(f"WARNING: {amt_s.Campaign.cd4_count_bins} only has one element, please test more "
                                     f"than one cd4 count bin.\n")

        # group the individual into CD4 groups defined by the campaign intervention configuration.
        event_df[amt_s.EventReport.cd4_bin] = event_df[amt_s.EventReport.cd4].\
            apply(lambda x: amt_s.convert_value_to_bin(x, cd4_count_bins))

        # merge cd4 count for each individual at the time ARTMortalityTable starts to death events, prepare the survival
        # durations for testing.
        df_to_test, cd4_bins = amt_s.prepare_df_to_test_by_cd4(event_df, start_day, base_year)

        mortality_table = art_mortality_table[amt_s.Campaign.mortality_table]

        # main test
        res_msg2 = amt_s.test_survival_duration_on_cd4(df_to_test, cd4_bins, mortality_table, cd4_count_bins,
                                                       simulation_duration_year)
        for msg in res_msg2:
            output_report_file.write(msg)
            if "BAD" in msg:
                succeed = False

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

