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
This test is testing the HIV relationship targeting: IsOnART feature with the Is_Equal_To option

Test data: ReportEventRecorder.csv

Output: scientific_feature_report.txt
        
"""

class Event:
    HasHIV_GiveART= "HasHIV_GiveART"
    IAMNotOnART = "IAMNotOnART"
    IAMOnART = "IAMOnART"


pop_size = 173 # Hard-coded for quick testing for now.


def create_report_file(event_df, event_report_name, config_filename, report_name):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")

        output_report_file.write(f"Testing {event_report_name}: \n")
        ART_event_list = [Event.IAMNotOnART,
                          Event.IAMOnART]
        output_report_file.write(f"Everyone should receive exactly one event in {ART_event_list}.\n")
        exact_one = True
        ART_event_df = event_df[event_df[ReportEventRecorder.Column.Event_Name.name].isin(ART_event_list)]
        if len(ART_event_df) != pop_size:
            succeed = exact_one = False
            output_report_file.write(f"BAD: There are {len(ART_event_df)} {ART_event_df} events, expected {pop_size}"
                                     f" events.\n")
        got_multiple = ART_event_df[ART_event_df.duplicated(subset=ReportEventRecorder.Column.Individual_ID.name,
                                                            keep='first')]
        if not got_multiple.empty:
            succeed = exact_one = False
            output_report_file.write(f"BAD: These individuals receives more than one event in {ART_event_list}.\n")
        if exact_one:
            output_report_file.write(f"GOOD: Everyone received exactly one event in {ART_event_list}.\n")

        GiveART_ids = set(event_df[event_df[ReportEventRecorder.Column.Event_Name.name] == Event.HasHIV_GiveART][
                              ReportEventRecorder.Column.Individual_ID.name])
        OnArt_ids = set(event_df[event_df[ReportEventRecorder.Column.Event_Name.name] == Event.IAMOnART][
                        ReportEventRecorder.Column.Individual_ID.name])

        if not rt_support.compare_ids_with_2_events(GiveART_ids, OnArt_ids, Event.HasHIV_GiveART, Event.IAMOnART,
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

