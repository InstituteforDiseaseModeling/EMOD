#!/usr/bin/python
import os
if __name__ == '__main__':
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as dtk_sft
from dtk_test.dtk_OutputFile import ReportEventRecorder

"""
This test is testing the relationship targeting: IsCircumcised feature

IAmNotCircumcised event broadcast to all female individuals.
IAmCircumcised event broadcast to all male individuals.
No male individual get infected since the MaleCircumcision is 100% blocking.

Test data: RelationshipStart.csv, RelationshipEnd.csv and ReportEventRecorder.csv

Output: scientific_feature_report.txt
        
"""

class Event:
    IAmNotCircumcised = "IAmNotCircumcised"
    IAmCircumcised = "IAmCircumcised"
    NewInfectionEvent = "NewInfectionEvent"
    CheckMyGender = "CheckMyGender"


gender_map = {"female": "F",
              "male": "M"}


def create_report_file(event_df, event_report_name, config_filename, report_name):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")

        output_report_file.write(f"Get population size by gender from {event_report_name}\n.")
        CheckMyGender_df = event_df[event_df[ReportEventRecorder.Column.Event_Name.name] ==
                                    Event.CheckMyGender]
        pop_by_gender_count = dict()
        for gender, gender_string in gender_map.items():
            count = len(CheckMyGender_df[CheckMyGender_df[ReportEventRecorder.Column.Gender.name] == gender_string])
            pop_by_gender_count[gender] = count
        output_report_file.write(f"population size are: {pop_by_gender_count}.\n")

        for event, gender in [(Event.IAmNotCircumcised, "female"),
                              (Event.IAmCircumcised, "male"),
                              (Event.NewInfectionEvent, "female")]:
            output_report_file.write(f"Make sure {event} event broadcast to {gender} individuals only.\n")
            df = event_df[event_df[ReportEventRecorder.Column.Event_Name.name] == event]
            expected_event_count = pop_by_gender_count[gender]
            if not df.empty:
                actual_gender = set(df[ReportEventRecorder.Column.Gender.name])
                expected_gender = set([gender_map[gender]])
                if actual_gender != expected_gender:
                    succeed = False
                    output_report_file.write(f"\tBAD: {event} event is distributed to {actual_gender} individuals, expected "
                                             f"{expected_gender}.\n")
                else:
                    output_report_file.write(
                        f"\tGOOD: {event} event is distributed to {actual_gender} individuals only.\n")
                    msg = f"there are {len(df)} {event} events, expected {expected_event_count} events.\n"
                    if len(df) != expected_event_count:
                        succeed = False
                        output_report_file.write(f"\tBAD: {msg}")
                    else:
                        output_report_file.write(f"\tGOOD: {msg}")
            else:
                succeed = False
                output_report_file.write(f"\tBAD: There is no {event} event in {event_report_name}.\n")

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

