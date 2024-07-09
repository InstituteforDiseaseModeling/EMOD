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
This test is testing the relationship targeting: HasMoreOrLessThanNumPartners feature with Num_Partners and 
More_Or_Less options.

Test data: RelationshipStart.csv, RelationshipEnd.csv and ReportEventRecorder.csv

Output: scientific_feature_report.txt
        
"""

start_day = 40
population_size = 1739
id_start_from = 2


class Event:
    at_least_8 = "IHaveMoreThanOrEqualTo8Partners"
    less_than_8 = "IHaveLessThan8Partners"
    less_than_3 = "IHaveLessThan3Partners"
    no_partner = "IHaveNoPartner"


def create_report_file(end_df, start_df, event_df, end_report_name, start_report_name, event_report_name,
                       config_filename, output_report_name):
    succeed = True
    with open(output_report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")
        output_report_file.write(f"1. Collecting the existing relationship pairs when intervention happens from "
                                 f"{start_report_name} and {end_report_name}.\n")
        # Remove end day at or after intervention start day from end_df
        end_df = end_df[end_df[rt_support.Constant.rel_actual_end_time] < start_day]
        # Remove start day at or after intervention start day from start_df
        start_df = start_df[start_df[rt_support.Constant.rel_start_time] < start_day]
        # Remove remaining relationships in end_df from start_df
        start_df = start_df[~start_df[rt_support.Constant.rel_id].isin(end_df[rt_support.Constant.rel_id].tolist())]

        output_report_file.write(f"2. Collecting the intervention information from {event_report_name}.\n")
        event_df_at_least_8 = event_df[event_df[ReportEventRecorder.Column.Event_Name.name] == Event.at_least_8]
        event_df_less_than_8 = event_df[event_df[ReportEventRecorder.Column.Event_Name.name] == Event.less_than_8]
        event_df_less_than_3 = event_df[event_df[ReportEventRecorder.Column.Event_Name.name] == Event.less_than_3]
        event_df_no_partner = event_df[event_df[ReportEventRecorder.Column.Event_Name.name] == Event.no_partner]
        individual_at_least_8 = set(event_df_at_least_8[ReportEventRecorder.Column.Individual_ID.name])
        individual_less_than_8 = set(event_df_less_than_8[ReportEventRecorder.Column.Individual_ID.name])
        individual_less_than_3 = set(event_df_less_than_3[ReportEventRecorder.Column.Individual_ID.name])
        individual_no_partner = set(event_df_no_partner[ReportEventRecorder.Column.Individual_ID.name])
        # Basic Sanity checks
        output_report_file.write(f"Runs some basic sanity checks on the event data:\n")

        if not individual_at_least_8 or \
            not individual_less_than_8 or \
            not individual_less_than_3 or \
            not individual_no_partner:
            output_report_file.write(f"\tWARNING: one or more events didn't broadcast at all, please check the test.\n")

        # If n1 < n2, then set(<n1) should be a subset of set(<n2)
        if individual_no_partner.issubset(individual_less_than_3):
            output_report_file.write(f"\tGOOD: Individuals receive {Event.no_partner} is a subset of who receive "
                                     f"{Event.less_than_3}.\n")
        else:
            succeed = False
            output_report_file.write(f"\tBAD: Individuals receive {Event.no_partner} should be a subset of who receive "
                                     f"{Event.less_than_3}.\n")

        if individual_less_than_3.issubset(individual_less_than_8):
            output_report_file.write(f"\tGOOD: Individuals receive {Event.less_than_3} is a subset of who receive "
                                     f"{Event.less_than_8}.\n")
        else:
            succeed = False
            output_report_file.write(f"\tBAD: Individuals receive {Event.less_than_3} should be a subset of who receive "
                                     f"{Event.less_than_8}.\n")

        # set(>=n) and set(<n) should not have common element
        if len(individual_at_least_8 & individual_less_than_8):
            succeed = False
            output_report_file.write(f"\tBAD: Individual should not receive both {Event.less_than_8} and "
                                     f"{Event.at_least_8} at the same time. They are: "
                                     f"{individual_at_least_8 & individual_less_than_8}.\n")
        else:
            output_report_file.write(f"\tGOOD: Individual don't receive {Event.less_than_8} and "
                                     f"{Event.at_least_8} events at the same time.\n")

        # set(>=n) + set(<n) should contain everyone
        if len(individual_at_least_8 | individual_less_than_8) != population_size:
            succeed = False
            output_report_file.write(f"\tBAD: everyone should receive either {Event.at_least_8} or "
                                     f"{Event.less_than_8} event.\n")
        else:
            output_report_file.write(f"\tGOOD: everyone receive either a {Event.at_least_8} or a "
                                     f"{Event.less_than_8} event.\n")

        # Collect a dictionary for N: list of ids who have N relationships/partners, N is from 0 to maximum number of
        # relationships
        relationship_reverse_counter = rt_support.get_relationship_reverse_counter(start_df,
                                                                                   count_who_has_no_partner=True,
                                                                                   id_start_from=id_start_from,
                                                                                   population_size=population_size)

        # Compare expected target ids with actual target ids for each event.
        for event, actual_target_ids, n, operation in \
            [(Event.at_least_8, individual_at_least_8, 7, rt_support.MyCompareOperation.MoreThan),
             (Event.less_than_8, individual_less_than_8, 8, rt_support.MyCompareOperation.LessThan),
             (Event.less_than_3, individual_less_than_3, 3, rt_support.MyCompareOperation.LessThan),
             (Event.no_partner, individual_no_partner, 1, rt_support.MyCompareOperation.LessThan)]:
            output_report_file.write(f"Testing {event} event:\n")
            expected_target_ids = rt_support.get_ids_based_on_num_relationship(relationship_reverse_counter,
                                                                               n,
                                                                               operation)
            if not rt_support.compare_ids(expected_target_ids, actual_target_ids, event, output_report_file):
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

