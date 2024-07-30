#!/usr/bin/python
if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append(str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )


import os.path as path
import dtk_test.dtk_sft as sft
import matplotlib.pyplot as plt
import pandas as pd
import dtk_test.dtk_STI_Support as sti_s
import dtk_test.dtk_ICP_Support as icp_s

"""
InterventionForCurrentPartners/NO_PRIORITIZATION
This test is testing the InterventionForCurrentPartners with Prioritize_Partners_By set to NO_PRIORITIZATION. All 
partners should be contacted no matter what value Maximum_Partners is set to.

This test is configured with concurrent relationships and long relationship duration for all types. Multiples ICP 
intervention were distributed at day 30, 60 and 90 with Maximum_Partners set to 0, 1 and 100 respectively.

Data for test is loaded from ReportEventRecorder.csv, RelationshipStart.csv and RelationshipEnd.csv. 

It performs the following tests:
1. Load campaign.json for parameters to test and check if it has the correct setup.
2. load data from RelationshipEnd.csv and make sure it's empty
3. Load RelationshipStart.csv, collect how many relationships each male individual has at the times when ICP happens.
4. load ReportEventRecorder.csv and make sure male individuals from step 3 received the exact amount of intervention 
    event for each ICP intervention.

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
        campaign_obj = icp_s.load_campaigns(campaign_filename)
        if len(campaign_obj) != 3:
            output_report_file.write(f"WARNING: we are expecting 3 campaign events got {len(campaign_obj)}, please"
                                     f" check the test.\n")
        for event in campaign_obj.values():
            prioritize_by = event[icp_s.Constant.prioritize_partners_by]
            if prioritize_by != icp_s.Events.NO_PRIORITIZATION:
                succeed = False
                output_report_file.write(f"BAD: {icp_s.Constant.prioritize_partners_by} is {prioritize_by} expected "
                                         f"{icp_s.Events.NO_PRIORITIZATION}, "
                                         f"please check the test.\n")
            min_duration = event[icp_s.Constant.minimum_duration_years]
            if min_duration != 0:
                output_report_file.write(f"WARNING: {icp_s.Constant.minimum_duration_years} is {min_duration} expected 0, "
                                         f"please check the test.\n")

        # load data from RelationshipStart.csv
        start_df = sti_s.parse_relationship_start_report(report_path=output_folder,
                                                         report_filename=relationship_start_report)

        female_id, female_id_0, female_id_1 = [], [], []
        event_count, event_count_0, event_count_1 = {}, {}, {}
        relationship_count = relationship_count_0 = relationship_count_1 = 0
        start_days = sorted(campaign_obj.keys())
        for index, row in start_df.iterrows():
            time = row[sti_s.ReportHeader.rel_start_time]
            if time == start_days[0]:
                female_id_0 = female_id[:]
                relationship_count_0 = relationship_count
                event_count_0 = event_count.copy()
            elif time == start_days[1]:
                female_id_1 = female_id[:]
                relationship_count_1 = relationship_count
                event_count_1 = event_count.copy()
            elif time > start_days[-1]:
                break
            f_id = row[sti_s.ReportHeader.b_ID]
            m_id = row[sti_s.ReportHeader.a_ID]
            if m_id not in event_count:
                event_count[m_id] = 1
            else:
                event_count[m_id] += 1
            relationship_count += 1
            if f_id not in female_id:
                female_id.append(f_id)

        # load data from RelationshipEnd.csv
        end_df = pd.read_csv(path.join(output_folder, relationship_end_report))
        if not end_df.empty:
            output_report_file.write(f"WARNING: {relationship_end_report} is not empty, please check your test.\n")

        # load data from ReportEventRecorder.csv
        report_df = pd.read_csv(path.join(output_folder, report_event_recorder))
        output_report_file.write(f"{len(report_df)} events are broadcast.\n")
        base_year = sft.get_config_parameter(config_filename=config_filename, parameters=[icp_s.Constant.base_year])[0]
        report_df_dict = {}
        for i in range(len(start_days)):
            start_day = start_days[i]
            start_year = round(start_day / sft.DAYS_IN_YEAR, 2) + base_year
            female_id_i = [female_id_0, female_id_1, female_id][i]
            relationship_count_i = [relationship_count_0, relationship_count_1, relationship_count][i]
            event_count_i = [event_count_0, event_count_1, event_count][i]
            broadcast_event = campaign_obj[start_day][icp_s.Constant.broadcast_event]
            output_report_file.write(f"at day {start_day} year {start_year}, there are {len(female_id_i)} females have "
                                     f"{relationship_count_i} relationships, each will have all partners receive "
                                     f"the {broadcast_event} event.\n")
            report_df_i = report_df[report_df[icp_s.Constant.year] == start_year]
            report_df_dict[start_year] = report_df_i
            if any(report_df_i[icp_s.Constant.event_name] != broadcast_event):
                succeed = False
                output_report_file.write(f"BAD: expected only {broadcast_event} event at day {start_day} year "
                                         f"{start_year}, got other event in {report_event_recorder}, test failed.\n")

            msg = f"expected {relationship_count_i} {broadcast_event} events at day {start_day} year {start_year}, " \
                f"got {len(report_df_i)}.\n"
            if relationship_count_i != len(report_df_i):
                succeed = False
                output_report_file.write("BAD: " + msg)
            else:
                output_report_file.write("GOOD: " + msg)

            count_i = report_df_i[icp_s.Constant.individual_id].value_counts().to_dict()
            for ind_id, count in event_count_i.items():
                if ind_id not in count_i:
                    succeed = False
                    output_report_file.write(f"BAD: at day {start_day} year {start_year} male {ind_id} should receive"
                                             f" {count} events but he doesn't receive any.\n")
                elif count != count_i[ind_id]:
                    succeed = False
                    output_report_file.write(
                        f"BAD: at day {start_day} year {start_year} male {ind_id} should receive {count} events "
                        f"but he receives {count_i[ind_id]} events.\n")

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

