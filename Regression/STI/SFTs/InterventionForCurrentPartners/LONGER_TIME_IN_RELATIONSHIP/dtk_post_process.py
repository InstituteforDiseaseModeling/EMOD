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
InterventionForCurrentPartners\LONGER_TIME_IN_RELATIONSHIP
This test is testing the InterventionForCurrentPartners with Prioritize_Partners_By set to LONGER_TIME_IN_RELATIONSHIP.

This test is configured with concurrent relationships and long relationship duration for all types. The ICP 
intervention in campaign.json has Maximum_Partners set to 1.

Data for test is loaded from ReportEventRecorder.csv, RelationshipStart.csv and RelationshipEnd.csv. 

It performs the following tests:
1. Load campaign.json for parameters to test and check if it has the correct setup.
2. load data from RelationshipEnd.csv and make sure it's empty
3. Load RelationshipStart.csv, from beginning of the simulation until time > start_day - min_duration * sft.DAYS_IN_YEAR,
    collect the male ids which have the longest relationship.
4. load ReportEventRecorder.csv and make sure the male ids from step 3 received the intervention.
5. For ids in ReportEventRecorder.csv but not in male ids from step 3, make sure they have one longest 
    relationship for at least one female individual(the relationship is not started at day 0 though).

Output:
    scientific_feature_report.txt

"""


def create_report_file(output_folder, report_event_recorder, relationship_start_report, relationship_end_report,
                       config_filename, output_report_name, debug):
    succeed = True
    with open(output_report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {sft.get_config_name(config_filename)}\n")

        # load campaign
        campaign_filename = sft.get_config_parameter(config_filename=config_filename,
                                                     parameters=icp_s.Constant.campaign_filename)[0]
        start_day, min_duration, max_partners, prioritize_by = icp_s.load_campaign(campaign_filename)
        # check if test has the correct setup
        if max_partners != 1:
            output_report_file.write(f"WARNING: {icp_s.Constant.maximum_partners} is {max_partners} expected 1, "
                                     f"please check the test.\n")
        if prioritize_by != icp_s.Events.LONGER_TIME_IN_RELATIONSHIP:
            succeed = False
            output_report_file.write(f"BAD: {icp_s.Constant.prioritize_partners_by} is {prioritize_by} expected "
                                     f"{icp_s.Events.LONGER_TIME_IN_RELATIONSHIP}, "
                                     f"please check the test.\n")
        if debug:
            output_report_file.write(f"{icp_s.Constant.start_day} is {start_day}.\n")
            output_report_file.write(f"{icp_s.Constant.minimum_duration_years} is {min_duration}.\n")
            output_report_file.write(f"{icp_s.Constant.maximum_partners} is {max_partners}.\n")
            output_report_file.write(f"{icp_s.Constant.prioritize_partners_by} is {prioritize_by}.\n")

        # load data from RelationshipEnd.csv
        end_df = pd.read_csv(path.join(output_folder, relationship_end_report))
        if not end_df.empty:
            output_report_file.write(f"WARNING: {relationship_end_report} is not empty, please check your test.\n")

        # load data from RelationshipStart.csv
        start_df = sti_s.parse_relationship_start_report(report_path=output_folder,
                                                         report_filename=relationship_start_report)
        # collect males id which have the longest relationship
        male_id = []
        female_id = []
        for index, row in start_df.iterrows():
            time = row[sti_s.ReportHeader.rel_start_time]
            if time == 0:
                male_id.append(row[sti_s.ReportHeader.a_ID])
            elif time > start_day - min_duration * sft.DAYS_IN_YEAR:
                break
            female_id.append(row[sti_s.ReportHeader.b_ID])
        output_report_file.write(f"{len(male_id)} relationships started at day 0 with {len(set(male_id))} "
                                 f"male individuals.\n")
        output_report_file.write(f"{len(female_id)} relationship have duration longer than {min_duration} year "
                                 f"when campaign event starts at day {start_day} with {len(set(female_id))} "
                                 f"female individuals.\n")

        # load data from ReportEventRecorder.csv
        id_to_check = []
        report_df = pd.read_csv(path.join(output_folder, report_event_recorder))
        if len(report_df) != len(set(female_id)):
            succeed = False
            output_report_file.write(f"BAD: {len(report_df)} events are broadcast, expected {len(set(female_id))}.\n")
        else:
            output_report_file.write(f"GOOD: {len(report_df)} events are broadcast, expected {len(set(female_id))}.\n")

        # check any male ids that receive the event and has no relationship at day 0
        for index, row in report_df.iterrows():
            id = row["Individual_ID"]
            if id not in male_id:
                print(f"ID to check: {id}.")
                id_to_check.append(id)

        if len(id_to_check) > len(set(female_id)) - len(set(male_id)):
            succeed = False
            output_report_file.write(f"BAD: expected at most {len(set(female_id)) - len(set(male_id))} ids in "
                                     f"id_to_check, got {len(id_to_check)}.\n")
        else:
            output_report_file.write(f"GOOD: expected at most {len(set(female_id)) - len(set(male_id))} ids in "
                                     f"id_to_check, got {len(id_to_check)}.\n")

        if not len(id_to_check):
            output_report_file.write("WARNING: id_to_check is empty, please check the test.\n")

        # check if these male ids have the first relationship of at least one of his partners.
        for ind_id in id_to_check:
            found_valid_relationship = False
            relation_to_check = start_df[start_df[sti_s.ReportHeader.a_ID] == ind_id]
            for index, row in relation_to_check.iterrows():
                female_partner_id = row[sti_s.ReportHeader.b_ID]
                time = row[sti_s.ReportHeader.rel_start_time]
                female_relationship = start_df[start_df[sti_s.ReportHeader.b_ID] == female_partner_id]
                if female_relationship[sti_s.ReportHeader.rel_start_time].min() == time:
                    found_valid_relationship = True
                    break
            if not found_valid_relationship:
                succeed = False
                output_report_file.write(f'BAD: female parters of male {ind_id} have relationship before day {time}, '
                                         f'we should target other male id instead of {ind_id}\n')
            else:
                output_report_file.write(f'GOOD: female parter {female_partner_id} of male {ind_id} have no '
                                         f'relationship before day {time}, we target male id {ind_id} based on '
                                         f'{icp_s.Events.LONGER_TIME_IN_RELATIONSHIP}.\n')
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

