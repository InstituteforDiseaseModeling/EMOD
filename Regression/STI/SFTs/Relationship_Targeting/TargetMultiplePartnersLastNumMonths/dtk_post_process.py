#!/usr/bin/python
import os
if __name__ == '__main__':
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_STI_Support as sti_support
import dtk_test.dtk_RelationshipTargeting_Support as rt_support
from dtk_test.dtk_OutputFile import ReportEventRecorder

"""
This test is testing the relationship targeting: HasHadMultiplePartnersInLastNumMonths feature with Num_Months_Type and 
Of_Relationship_Type options.

Here are the explanation of how to determine the "active relationship" that we are targeting:
"Determine if the individual has had more than one relationship in the last “Num” months. This could constitute as 
“high-risk” behavior. The goal is to target people that have had coital acts with more than one person during the 
last X months. This would count current relationships, relationships that started in the last X months, and 
relationships that have ended in the last X months. Basically, the all the relationships that have been active at 
some point during the last X months. NOTE: This only counts unique partners. Two relationships with the same person 
during the time period will count as one."

Test data: RelationshipStart.csv, RelationshipEnd.csv and ReportEventRecorder.csv

Output: scientific_feature_report.txt
        
"""

start_day = 400
update_period = 7

class Event:
    Three_INFORMAL = "3_INFORMAL"
    Six_TRANSITORY = "6_TRANSITORY"
    Nine_NA = "9_NA"
    Twelve_MARITAL = "12_MARITAL"
    Six_COMMERCIAL = "6_COMMERCIAL"
    Three_NA = "3_NA"


def create_report_file(end_df, start_df, event_df, end_report_name, start_report_name, event_report_name,
                       config_filename, output_report_name):
    succeed = True
    with open(output_report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")

        for event in [Event.Three_NA, Event.Three_INFORMAL, Event.Six_COMMERCIAL, Event.Six_TRANSITORY,
                      Event.Nine_NA, Event.Twelve_MARITAL]:
            num_of_month, relationship_type = event.split("_")
            output_report_file.write(f"Testing {event} event:\n")

            output_report_file.write(f"\t1. Collect the relationship pairs that are active in the last {num_of_month} "
                                     f"months for relationship type: {relationship_type} from "
                                     f"{start_report_name} and {end_report_name} .\n")

            # Filter relationship in the last N months

            # 1. Collect the relationships that ended before the last N months
            end_df_n_months = end_df[end_df[rt_support.Constant.rel_actual_end_time] <
                                     start_day - int(num_of_month) * dtk_sft.DAYS_IN_YEAR / 12 - 1]
                                     
            # 2. Collect the relationships that started so far
            start_df_n_months = start_df[start_df[rt_support.Constant.rel_start_time] < start_day]

            # 3. Remove relationship from step 1 from step 2 to get the active relationships in the last N months.
            start_df_n_months = start_df_n_months[~start_df_n_months[rt_support.Constant.rel_id].isin(
                end_df_n_months[rt_support.Constant.rel_id].tolist())]
                
            # 4. Filter by relationship type as needed:
            if relationship_type != "NA":
                start_df_n_months = start_df_n_months[start_df_n_months[rt_support.Constant.rel_type]==
                                                      rt_support.map_relationship_type[relationship_type]]
                                                      
            # 5. Remove relationships that are of the same type by the same two people.
            # We are looking for the number of unique partners, not the starting and stopping
            # of relations with the same person.
            start_df_n_months.drop_duplicates(subset=[rt_support.Constant.rel_type,"A_ID","B_ID"],keep='last',inplace=True)

            # Collect a dictionary for N: list of ids who have N relationships/partners, N is from 1 to maximum number
            # of relationships
            relationship_reverse_counter = rt_support.get_relationship_reverse_counter(start_df_n_months)

            # Collect ids of individual who has multiple partners/relationships.
            target_ids = rt_support.get_ids_based_on_num_relationship(relationship_reverse_counter, 1,
                                                                      rt_support.MyCompareOperation.MoreThan)

            output_report_file.write(f"\t2. Collect the intervention information for {event} from {event_report_name}.\n")
            event_df_n_months = event_df[event_df[ReportEventRecorder.Column.Event_Name.name] == event]
            individual_n_months = set(event_df_n_months[ReportEventRecorder.Column.Individual_ID.name])
            output_report_file.write(f"\t3. Make sure there is {event} event in {event_report_name}:\n")
            if not len(individual_n_months):
                succeed = False
                output_report_file.write(f"\tWARNING: {event} events didn't broadcast at all, please check the test.\n")
            else:
                output_report_file.write(f"\tThere are {len(individual_n_months)} {event} events in {event_report_name}.\n")

            output_report_file.write(f"\t 4. Compare target individual list with actual list of indivduals who receive"
                                     f" {event} event:\n")
            if not rt_support.compare_ids(target_ids, individual_n_months, event, output_report_file):
                succeed = False

        output_report_file.write(dtk_sft.format_success_msg(succeed))

        return succeed


def application( output_folder="output", stdout_filename="test.txt",
                 end_report_name="RelationshipEnd.csv",
                 start_report_name="RelationshipStart.csv",
                 event_report_name="ReportEventRecorder.csv",
                 config_filename="config.json",
                 report_name=dtk_sft.sft_output_filename,
                 debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("end_report_name: " + end_report_name + "\n")
        print("start_report_name: " + start_report_name + "\n")
        print("event_report_name: " + event_report_name + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done(stdout_filename)

    end_df = sti_support.parse_relationship_end_report(report_path=output_folder, report_filename=end_report_name)
    start_df = sti_support.parse_relationship_start_report(report_path=output_folder, report_filename=start_report_name)
    event_df = ReportEventRecorder(os.path.join(output_folder, event_report_name)).df

    create_report_file(end_df, start_df, event_df, end_report_name, start_report_name, event_report_name,
                       config_filename, report_name)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-E', '--end_report', default="RelationshipEnd.csv",
                        help="Relationship end report to parse (RelationshipEnd.csv)")
    parser.add_argument('-S', '--start_report', default="RelationshipStart.csv",
                        help="Relationship start report to parse (RelationshipStart.csv)")
    parser.add_argument('-e', '--event_report', default="ReportEventRecorder.csv",
                        help="Event Recorder report to parse (ReportEventRecorder.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                end_report_name=args.end_report,
                start_report_name=args.start_report,
                event_report_name=args.event_report,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

