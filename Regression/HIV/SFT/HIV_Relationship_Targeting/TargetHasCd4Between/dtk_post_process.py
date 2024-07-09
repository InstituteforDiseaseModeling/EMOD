#!/usr/bin/python
import os
if __name__ == '__main__':
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as dtk_sft
from dtk_test.dtk_OutputFile import ReportEventRecorder
import dtk_test.dtk_RelationshipTargeting_Support as rt_support

"""
This test is testing the HIV relationship targeting: HasCd4BetweenMinAndMax feature

Test data: ReportEventRecorder.csv

Output: scientific_feature_report.txt
        
"""

cd4_event_list = ["0-220", "220-530", "530-600"]
HasHIV_event= "HasHIV"
NoHIV_event = "NoHIV"


def create_report_file(event_df, event_report_name, config_filename, report_name):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")

        output_report_file.write(f"Testing {event_report_name}: \n")

        for cd4_event in cd4_event_list:
            min_cd4, max_cd4 = cd4_event.split("-")
            min_cd4, max_cd4 = float(min_cd4), float(max_cd4)
            event_df_under_test = event_df[event_df[ReportEventRecorder.Column.Event_Name.name]==cd4_event]
            for index, row in event_df_under_test.iterrows():
                id = row[ReportEventRecorder.Column.Individual_ID.name]
                cd4 = row[ReportEventRecorder.Column.CD4.name]
                # From Dan B:
                # "StandardEventCoordinator (SEC) is checking the person's status at the beginning of the time step
                # before the people have been updated. SEC then gives a BroadcastEvent intervention to the person but
                # this doesn't get updated until we are updating the individual. If one uses NLHTIV, they would see
                # the check and the report match."
                if cd4 + 5 < min_cd4 or cd4 > max_cd4: # cd4 count is raising, assume it will not raise more than 5 per time step.
                    succeed = False
                    output_report_file.write(f"\tBAD: individual {id} received {cd4_event} event but CD4 is {cd4}.\n")
        if succeed:
            output_report_file.write(f"GOOD: everyone received {cd4_event_list} events correctly based on their cd4 "
                                     f"count.\n")

        target_by_cd4_ids = set(event_df[event_df[ReportEventRecorder.Column.Event_Name.name].isin(cd4_event_list)][
            ReportEventRecorder.Column.Individual_ID.name
        ])
        hasHIV_ids = set(event_df[event_df[ReportEventRecorder.Column.Event_Name.name] == HasHIV_event][
            ReportEventRecorder.Column.Individual_ID.name
        ])
        if not len(target_by_cd4_ids):
            succeed = False
            output_report_file.write(f"BAD: there is no {cd4_event_list} event in {event_report_name}.\n")

        if not rt_support.compare_ids_with_2_events(target_by_cd4_ids, hasHIV_ids, cd4_event_list, HasHIV_event,
                                                    output_report_file):
            succeed = False

        output_report_file.write(dtk_sft.format_success_msg(succeed))

        return succeed


def application( output_folder="output", stdout_filename="test.txt",
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

    create_report_file(event_df, event_report_name, config_filename, report_name)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-e', '--event_report', default="ReportEventRecorder.csv",
                        help="Event Recorder report to parse (ReportEventRecorder.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                event_report_name=args.event_report,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

