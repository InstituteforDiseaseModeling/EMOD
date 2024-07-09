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
InterventionForCurrentPartners\SHORTER_TIME_IN_RELATIONSHIP
This test is testing the InterventionForCurrentPartners with Prioritize_Partners_By set to SHORTER_TIME_IN_RELATIONSHIP.

This test is configured with concurrent relationships and long relationship duration for all types. The ICP 
intervention in campaign.json has Maximum_Partners set to 1 and min_duration set to 0.

Data for test is loaded from ReportEventRecorder.csv, RelationshipStart.csv and RelationshipEnd.csv. 

It performs the following tests:
1. Load campaign.json for parameters to test and check if it has the correct setup.
2. load data from RelationshipEnd.csv and make sure it's empty
3. Load RelationshipStart.csv from bottom to top, collect the male ids which have the latest relationship for each 
    female individual.
4. load ReportEventRecorder.csv and perform:
    4.1: make sure the correct number of event is broadcast.
    4.2: if an female individual only has one male partner with shortest duration, add 1 expected event for the male id.
    4.3: if she has more than one male partners with shortest duration, add 1 potential event for every male ids.
    4.4: make sure male ids have expected event received the event
    4.5: make sure individual who received the event doesn't have expected event, he should have potential event.
    4.6: for the male ids in step 4.4, make sure they have at least one shortest relationship for at least one female 
         individual.

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
        if max_partners != 1:
            output_report_file.write(f"WARNING: {icp_s.Constant.maximum_partners} is {max_partners} expected 1, "
                                     f"please check the test.\n")
        if prioritize_by != icp_s.Events.SHORTER_TIME_IN_RELATIONSHIP:
            succeed = False
            output_report_file.write(f"BAD: {icp_s.Constant.prioritize_partners_by} is {prioritize_by} expected "
                                     f"{icp_s.Events.SHORTER_TIME_IN_RELATIONSHIP}, "
                                     f"please check the test.\n")
        if min_duration != 0:
            output_report_file.write(f"WARNING: {icp_s.Constant.minimum_duration_years} is {min_duration} expected 0, "
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
        protential_male_ids = []
        female_id = []
        count_id_to_ckeck = 0
        for index, row in start_df[::-1].iterrows():  # from bottom to top
            time = row[sti_s.ReportHeader.rel_start_time]
            if time > start_day:
                continue
            f_id = row[sti_s.ReportHeader.b_ID]
            if f_id not in female_id:
                female_id.append(f_id)
                m_id = list(start_df[(start_df[sti_s.ReportHeader.b_ID] == f_id) &
                                     (start_df[sti_s.ReportHeader.rel_start_time] == time)][
                                sti_s.ReportHeader.a_ID])
                if len(m_id) > 1:
                    count_id_to_ckeck += 1
                    protential_male_ids.extend(m_id)
                else:
                    male_id.extend(m_id)
        output_report_file.write(f"{len(female_id)} female individuals have duration longer than {min_duration} year "
                                 f"when campaign event starts at day {start_day}.\n")
        output_report_file.write(f"{len(set(male_id + protential_male_ids))} male individuals have shortest duration "
                                 f"with these females .\n")

        # load data from RelationshipEnd.csv
        end_df = pd.read_csv(path.join(output_folder, relationship_end_report))
        if not end_df.empty:
            output_report_file.write(f"WARNING: {relationship_end_report} is not empty, please check your test.\n")

        # load data from ReportEventRecorder.csv
        id_to_check = []
        report_df = pd.read_csv(path.join(output_folder, report_event_recorder))
        # check expected number of broadcast event
        if len(report_df) != len(set(female_id)):
            succeed = False
            output_report_file.write(f"BAD: {len(report_df)} events are broadcast, expected {len(set(female_id))}.\n")
        else:
            output_report_file.write(f"GOOD: {len(report_df)} events are broadcast, expected {len(set(female_id))}.\n")

        # for female_partner_id in female_id:
        #     relationship = df[df[sti_s.ReportHeader.b_ID] == female_partner_id]
        #     male_partner_id = relationship[relationship[sti_s.ReportHeader.rel_start_time] ==
        #     relationship[sti_s.ReportHeader.rel_start_time].min()][sti_s.ReportHeader.a_ID]#.iloc(0)

        # check any male ids that receive the event and has no shortest duration relationship
        for index, row in report_df.iterrows():
            id = row["Individual_ID"]
            if id not in male_id:
                print(f"ID to check: {id}.")
                id_to_check.append(id)
            else:
                male_id.remove(id)

        if len(id_to_check) != count_id_to_ckeck:
            succeed = False
            output_report_file.write(f"BAD: expected {count_id_to_ckeck} ids in "
                                     f"id_to_check, got {len(id_to_check)}.\n")
        else:
            output_report_file.write(f"GOOD: expected {count_id_to_ckeck} ids in "
                                     f"id_to_check, got {len(id_to_check)}.\n")

        if not len(id_to_check):
            output_report_file.write("WARNING: id_to_check is empty, please check the test.\n")

        # check if these male ids have the latest relationship of at least one of his partners.
        for ind_id in id_to_check:
            if ind_id not in protential_male_ids:
                succeed = False
                output_report_file.write(f"BAD: {ind_id} is not expected to receive any event based on "
                                         f"{icp_s.Events.SHORTER_TIME_IN_RELATIONSHIP}.\n")
            else:
                protential_male_ids.remove(ind_id)

            found_valid_relationship = False
            relation_to_check = start_df[start_df[sti_s.ReportHeader.a_ID] == ind_id]
            for index, row in relation_to_check.iterrows():
                female_partner_id = row[sti_s.ReportHeader.b_ID]
                time = row[sti_s.ReportHeader.rel_start_time]
                female_relationship = start_df[start_df[sti_s.ReportHeader.b_ID] == female_partner_id]
                if female_relationship[sti_s.ReportHeader.rel_start_time].max() == time:
                    found_valid_relationship = True
                    break
            if not found_valid_relationship:
                succeed = False
                output_report_file.write(f'BAD: female parters of male {ind_id} have relationship after day {time}, '
                                         f'we should target other male id instead of {ind_id}\n')
            else:
                output_report_file.write(f'GOOD: female parter {female_partner_id} of male {ind_id} have no '
                                         f'relationship after day {time}, we target male id {ind_id} based on '
                                         f'{icp_s.Events.SHORTER_TIME_IN_RELATIONSHIP}.\n')
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

