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
This test is testing the relationship targeting: HasRelationship feature with With_Partner_Who and That_Recently 
option set to ENDED. The intervention is used with a NodeLevelHealthTriggeredIV listening for the ExitedRelationship event.

The main test is making sure the MyRelationshipJustEnded event is only distributed to who has relationship that have 
ended within one time-step and the partner in this relationship is in Risk:High group.

Test data: RelationshipEnd.csv and ReportEventRecorder.csv

Output: scientific_feature_report.txt
        
"""
start_day = 100


def create_report_file(end_df, event_df, end_report_name, event_report_name,
                       config_filename, output_report_name, testing_options):
    succeed = True
    with open(output_report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")
        output_report_file.write(f"1. Collecting the existing relationship pairs when intervention happens from "
                                 f"{end_report_name}.\n")
        simulation_duration, base_year = dtk_sft.get_config_parameter(config_filename, parameters=["Simulation_Duration",
                                                                                                   "Base_Year"])
        # Keep only relationship that are recently ended from end_df
        # end_df = end_df[(end_df[Constant.rel_actual_end_time] >= Constant.start_day - 1) &
        #     (end_df[Constant.rel_actual_end_time] <= Constant.start_day)]
        end_df = end_df[end_df[rt_support.Constant.rel_actual_end_time] >= start_day]

        if end_df.empty:
            succeed = False
            output_report_file.write(f"BAD: There are no relationship that have ended at time {start_day - 1}"
                                     f" and {start_day}.\n")
            

        output_report_file.write(f"2. Collecting the intervention information from {event_report_name}.\n")
        result, partner_df, target_df = rt_support.get_highriskpartner_and_target_df(
            rt_support.Event.my_relationship_just_ended,
            event_df, event_report_name, output_report_file)
        if not result:
            succeed = False

        if succeed:
            # Make column names in RelationshipEnd dataframe look like RelationshipStart
            end_df[rt_support.GenderStrings.female_id] = end_df[rt_support.GenderStrings.female_id_end]
            end_df[rt_support.GenderStrings.male_id] = end_df[rt_support.GenderStrings.male_id_end]
            if not rt_support.hasRelationship_test(partner_df, target_df, end_df, testing_options,
                                                   rt_support.Event.my_relationship_just_ended,
                                                   "Risk:High group in a relationship that have ended within one time-step",
                                                   output_report_file):
                succeed = False

        output_report_file.write(dtk_sft.format_success_msg(succeed))

        return succeed


def application( output_folder="output", stdout_filename="test.txt",
                 end_report_name="RelationshipEnd.csv",
                 event_report_name="ReportEventRecorder.csv",
                 config_filename="config.json",
                 report_name=dtk_sft.sft_output_filename,
                 debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("end_report_name: " + end_report_name + "\n")
        print("event_report_name: " + event_report_name + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done(stdout_filename)

    end_df = sti_support.parse_relationship_end_report(report_path=output_folder, report_filename=end_report_name)

    event_df = ReportEventRecorder(os.path.join(output_folder, event_report_name)).df

    testing_options = ["With_Partner_Who", "That_Recently"]
    create_report_file(end_df, event_df, end_report_name, event_report_name,
                       config_filename, report_name, testing_options)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-E', '--end_report', default="RelationshipEnd.csv",
                        help="Relationship end report to parse (RelationshipEnd.csv)")
    parser.add_argument('-e', '--event_report', default="ReportEventRecorder.csv",
                        help="Event Recorder report to parse (ReportEventRecorder.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                end_report_name=args.end_report,
                event_report_name=args.event_report,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

