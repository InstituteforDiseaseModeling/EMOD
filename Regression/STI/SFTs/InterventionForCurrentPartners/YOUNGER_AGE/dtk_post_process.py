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
InterventionForCurrentPartners\YOUNGER_AGE
This test is testing the InterventionForCurrentPartners with Prioritize_Partners_By set to YOUNGER_AGE.

This test is configured with concurrent relationships and long relationship duration for all types. The ICP 
intervention in campaign.json has Maximum_Partners set to 1 and min_duration set to 0.

Data for test is loaded from ReportEventRecorder.csv, RelationshipStart.csv and RelationshipEnd.csv. 

It performs the following tests:
1. Load campaign.json for parameters to test and check if it has the correct setup.
2. load data from RelationshipEnd.csv and make sure it's empty
3. Load RelationshipStart.csv, collect relationships table for female id with her youngest partner.
4. load ReportEventRecorder.csv and:
    4.1: check number of broadcast event
    4.2: check if all female individuals' youngest partner received the exact number of event
    4.3: make sure no male individuals who should not receive the event received it
5. Plot histogram for age of male ids who received the event.

Output:
    scientific_feature_report.txt
    Age.png
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
        if max_partners != 1:
            output_report_file.write(f"WARNING: {icp_s.Constant.maximum_partners} is {max_partners} expected 1, "
                                     f"please check the test.\n")
        if prioritize_by != icp_s.Events.YOUNGER_AGE:
            succeed = False
            output_report_file.write(f"BAD: {icp_s.Constant.prioritize_partners_by} is {prioritize_by} expected "
                                     f"{icp_s.Events.YOUNGER_AGE}, "
                                     f"please check the test.\n")
        if min_duration != 0:
            output_report_file.write(f"WARNING: {icp_s.Constant.minimum_duration_years} is {min_duration} expected 0, "
                                     f"please check the test.\n")
        if debug:
            output_report_file.write(f"{icp_s.Constant.start_day} is {start_day}.\n")
            output_report_file.write(f"{icp_s.Constant.minimum_duration_years} is {min_duration}.\n")
            output_report_file.write(f"{icp_s.Constant.maximum_partners} is {max_partners}.\n")
            output_report_file.write(f"{icp_s.Constant.prioritize_partners_by} is {prioritize_by}.\n")

        # load data from RelationshipEnd.csv
        #end_df = pd.read_csv(path.join(output_folder, relationship_end_report))
        #if not end_df.empty:
        #    output_report_file.write(f"WARNING: {relationship_end_report} is not empty, please check your test.\n")

        # load data from RelationshipStart.csv
        start_df = sti_s.parse_relationship_start_report(report_path=output_folder,
                                                         report_filename=relationship_start_report)

        relationships = {}  # relationships dictionary for female_id: male_id(youngest age)
        age_map = {}  # age dictionary for male ids
        for index, row in start_df.iterrows():
            time = row[sti_s.ReportHeader.rel_start_time]
            if time > start_day:
                break
            f_id = row[sti_s.ReportHeader.b_ID]
            m_id = row[sti_s.ReportHeader.a_ID]
            age = row[sti_s.ReportHeader.a_age] * sft.DAYS_IN_YEAR
            
            if m_id not in age_map:
                age_map[m_id] = (time, age)
            if f_id not in relationships:
                relationships[f_id] = m_id  # [m_id]
            else:
                youngest_male = relationships[f_id]  # [0]
                youngest_male_age = age_map[youngest_male][1] + time - age_map[youngest_male][0]
                
                if age < youngest_male_age:
                    relationships[f_id] = m_id  # [m_id]
                elif age == youngest_male_age:
                    output_report_file.write(f"WARNING: male {m_id} and {youngest_male} are both the youngest male "
                                             f"partner for female {f_id} at day {time}. We assume intervention will be"
                                             f" send to {youngest_male}.\n")
                #     relationships[f_id].append(m_id)
        output_report_file.write(f"{len(relationships)} female individuals in relationship at day {start_day}.\n")

        # load data from ReportEventRecorder.csv
        report_df = pd.read_csv(path.join(output_folder, report_event_recorder))
        # check number of event broadcast.
        if len(report_df) != len(relationships):
            succeed = False
            output_report_file.write(
                f"BAD: {len(report_df)} events are broadcast, expected {len(relationships)}.\n")
        else:
            output_report_file.write(
                f"GOOD: {len(report_df)} events are broadcast, expected {len(relationships)}.\n")

        # check if all female individuals' youngest partner received the exact number of event
        for index, row in report_df.iterrows():
            id = row["Individual_ID"]
            if id not in relationships.values():
                succeed = False
                output_report_file.write(f"BAD: male {id} should not receive event.\n")
            else:
                for f_id, m_id in relationships.items():
                    if m_id == id:
                        del relationships[f_id]
                        break
        if len(relationships):
            succeed = False
            output_report_file.write(f"BAD: these male ids are expected to receive the event based on "
                                     f"{icp_s.Events.YOUNGER_AGE} priority but they didn't receive it or receive less "
                                     f"than expected # of events: {relationships}.\n")
        else:
            output_report_file.write(f"GOOD: all male ids that are expected to receive the event based on "
                                     f"{icp_s.Events.YOUNGER_AGE} priority have received the expected # of events.\n")

        # add plotting
        report_df['Age'] = report_df['Age'] / sft.DAYS_IN_YEAR
        report_df.plot(y='Age', kind='hist')
        plt.title("Age-hist")
        plt.savefig("Age.png")
        if sft.check_for_plotting():
            plt.show()
        plt.close()

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

