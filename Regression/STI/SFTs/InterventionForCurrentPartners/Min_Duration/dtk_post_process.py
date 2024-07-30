#!/usr/bin/python
if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append(str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )


import os.path as path
import dtk_test.dtk_sft as sft
import pandas as pd
import dtk_test.dtk_STI_Support as sti_s
import dtk_test.dtk_ICP_Support as icp_s

"""
InterventionForCurrentPartners/Min_Duration
This test is testing the Min_Duration in InterventionForCurrentPartners.

Min_Duration: The minimum amount of time (in years) between relationship formation and the current time for the 
              partner to qualify for the intervention.

This test is configured with concurrent relationships and long relationship duration for all types. 

Data for test is loaded from ReportEventRecorder.csv, RelationshipStart.csv and RelationshipEnd.csv. 

It performs the following tests:
1. Load campaign.json for parameters to test and check if it has the correct setup.
2. load data from RelationshipEnd.csv and make sure it's empty
3. Load RelationshipStart.csv, collect how many relationships each male individual has that meet the requirement of 
    Min_Duration at the time when ICP happens.
4. load ReportEventRecorder.csv and make sure male individuals from step 3 received the exact amount of intervention 
    event.

Output:
    scientific_feature_report.txt
"""

def create_report_file(output_folder, report_event_recorder, relationship_start_report, relationship_end_report,
                       config_filename, output_report_name, debug):
    succeed = True
    with open(output_report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {sft.get_config_name(config_filename)}\n")

        #load campaign
        campaign_filename = sft.get_config_parameter(config_filename=config_filename,
                                                     parameters=icp_s.Constant.campaign_filename)[0]
        start_day, min_duration, max_partners, prioritize_by = icp_s.load_campaign(campaign_filename)
        if max_partners != 100:
            output_report_file.write(f"WARNING: {icp_s.Constant.maximum_partners} is {max_partners} expected 100, "
                                     f"please check the test.\n")
        if debug:
            output_report_file.write(f"{icp_s.Constant.start_day} is {start_day}.\n")
            output_report_file.write(f"{icp_s.Constant.minimum_duration_years} is {min_duration}.\n")
            output_report_file.write(f"{icp_s.Constant.maximum_partners} is {max_partners}.\n")
            output_report_file.write(f"{icp_s.Constant.prioritize_partners_by} is {prioritize_by}.\n")

        # load data from RelationshipStart.csv
        start_df = sti_s.parse_relationship_start_report(report_path=output_folder,
                                                         report_filename=relationship_start_report)
        male_id = []
        female_id = []
        for index, row in start_df.iterrows():
            time = row[sti_s.ReportHeader.rel_start_time]
            if time > start_day - min_duration * sft.DAYS_IN_YEAR:
                break
            female_id.append(row[sti_s.ReportHeader.b_ID])
            male_id.append(row[sti_s.ReportHeader.a_ID])

        output_report_file.write(f" {len(female_id)} relationships have duration longer than {min_duration} year when"
                                 f" campaign event starts at day {start_day} with {len(set(female_id))} female "
                                 f"individuals.\n")

        # load data from RelationshipEnd.csv
        end_df = pd.read_csv(path.join(output_folder, relationship_end_report))
        if not end_df.empty:
            output_report_file.write(f"WARNING: {relationship_end_report} is not empty, please check your test.\n")

        # load data from ReportEventRecorder.csv
        id_to_check = []
        report_df = pd.read_csv(path.join(output_folder, report_event_recorder))
        if len(report_df) != len(male_id):
            succeed = False
            output_report_file.write(f"BAD: {len(report_df)} events are broadcast, expected {len(male_id)}.\n")
        else:
            output_report_file.write(f"GOOD: {len(report_df)} events are broadcast, expected {len(male_id)}.\n")

        report_id_count = report_df['Individual_ID'].value_counts().to_dict()
        male_id_count = {}
        for ind_id in set(male_id):
            male_id_count[ind_id] = male_id.count(ind_id)
        for ind_id, count in report_id_count.items():
            if ind_id not in male_id_count:
                succeed = False
                output_report_file.write(f"BAD: male {ind_id} should not receive any event, but he received "
                                         f"{count} events.\n")
            else:
                expected_count = min(male_id_count.pop(ind_id), max_partners)
                if count != expected_count:
                    succeed = False
                    output_report_file.write(f"BAD: male {ind_id} should receive {expected_count} events, but he "
                                             f"received {count} events.\n")
        if len(male_id_count):
            for ind_id, expected_count in male_id_count.items():
                expected_count = min(expected_count, max_partners)
                succeed = False
                output_report_file.write(f"BAD: male {ind_id} should receive {expected_count} events, but he received"
                                         f" 0 event.\n")
        else:
            output_report_file.write("GOOD: all male ids that are expected to receive the event had received at least"
                                     " one event.\n")
        if succeed:
            output_report_file.write("GOOD: all male ids that are expected to receive the event had received the exact"
                                     " # of events.\n")
        output_report_file.write(sft.format_success_msg(succeed))
    return succeed


def application(output_folder="output", stdout_filename="test.txt",
                report_event_recorder="ReportEventRecorder.csv",
                relationship_start_report="RelationshipStart.csv",
                relationship_end_report="RelationshipEnd.csv",
                config_filename="config.json",
                output_report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("report_event_recorder: " + report_event_recorder + "\n")
        print("relationship_start_report: " + relationship_start_report + "\n")
        print("relationship_end_report: " + relationship_end_report + "\n")
        print("config_filename: " + config_filename + "\n")
        print("output_report_name: " + output_report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done(stdout_filename)
    create_report_file(output_folder, report_event_recorder, relationship_start_report, relationship_end_report,
                       config_filename, output_report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-e', '--event_report', default="ReportEventRecorder.csv",
                        help="Report Event Recorder to parse (ReportEventRecorder.csv)")
    parser.add_argument('-S', '--relationship_start_report', default="RelationshipStart.csv",
                        help="RelationshipStart to parse (RelationshipStart.csv)")
    parser.add_argument('-E', '--relationship_end_report', default="RelationshipEnd.csv",
                        help="RelationshipEnd.csv to parse (RelationshipEnd.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                report_event_recorder=args.event_report,
                relationship_start_report=args.relationship_start_report,
                relationship_end_report=args.relationship_end_report,
                config_filename=args.config,
                output_report_name=args.reportname, debug=args.debug)

