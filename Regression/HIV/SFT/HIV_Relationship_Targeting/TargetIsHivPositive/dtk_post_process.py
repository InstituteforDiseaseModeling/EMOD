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
This test is testing the HIV relationship targeting: IsHivPositive feature with the following options:
    Is_Equal_To
    And_Has_Ever_Been_Tested
    And_Has_Ever_Tested_Positive
    And_Has_Received_Positive_Results
    
Everyone should receive exactly one event in ['Tested_Positive', 'Tested_Negative', 'NotTested_Positive', 'NotTested_Negative'].
Individual who received HasHIV also received Tested_Positive.
Individual who received NoHIV also received Tested_Negative.
Individual who didn't receive HIVRapidHIVDiagnostic intervention all received a 'NotTested_**' event.
Individuals who received Received_Positive_Results event is a subset of individuals who received Tested_Positive event.

Test data: ReportEventRecorder.csv

Output: scientific_feature_report.txt
        
"""

class Event:
    HasHIV= "HasHIV"
    NoHIV = "NoHIV"
    NotTested_Negative = "NotTested_Negative"
    NotTested_Positive = "NotTested_Positive"
    Tested_Negative = "Tested_Negative"
    Tested_Positive = "Tested_Positive"
    Received_Positive_Results = "Received_Positive_Results"


def create_report_file(event_df, event_report_name, config_filename, report_name):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")

        output_report_file.write(f"Testing {event_report_name}: \n")
        event_list = [Event.Tested_Positive,
                      Event.Tested_Negative,
                      Event.NotTested_Positive,
                      Event.NotTested_Negative]
        diagnostic_event_list = [Event.HasHIV, Event.NoHIV]
        output_report_file.write(f"Everyone should receive exactly one event in {event_list}.\n")
        exact_one = True

        all_ids = set(event_df[ReportEventRecorder.Column.Individual_ID.name])
        min_id = min(all_ids)
        max_id = max(all_ids)

        for ind_id in range(min_id, max_id+1):
            my_df = event_df[event_df[ReportEventRecorder.Column.Individual_ID.name] == ind_id]
            if my_df.empty:
                succeed = exact_one = False
                output_report_file.write(f"\tBAD: individual {ind_id} doesn't have any event.\n")
            else:
                my_test_event = my_df[my_df[ReportEventRecorder.Column.Event_Name.name].isin(event_list)]
                if my_test_event.empty:
                    succeed = exact_one = False
                    output_report_file.write(f"\tBAD: individual {ind_id} doesn't have any event in {event_list}.\n")
                elif len(my_test_event) != 1:
                    succeed = exact_one = False
                    output_report_file.write(f"\tBAD: individual {ind_id} received more than one events in "
                                             f"{event_list}, they are: {my_test_event}.\n")
                else:
                    my_event = my_test_event[ReportEventRecorder.Column.Event_Name.name].tolist()[0]
                    infected = my_test_event[ReportEventRecorder.Column.Infected.name].tolist()[0]
                    msg = f"\tBAD: individual {ind_id} received {my_event} event but isInfected is {infected}.\n"
                    if "Positive" in my_event:
                        if infected != 1:
                            succeed = False
                            output_report_file.write(msg)
                    else:
                        if infected != 0:
                            succeed = False
                            output_report_file.write(msg)
        if exact_one:
            output_report_file.write(f"GOOD: Everyone received exactly one event in {event_list}.\n")

        for event_to_compare_1, event_to_compare_2 in [(Event.HasHIV, Event.Tested_Positive),
                                                       (Event.NoHIV, Event.Tested_Negative)]:
            ids_1 = set(event_df[event_df[ReportEventRecorder.Column.Event_Name.name] == event_to_compare_1][
                            ReportEventRecorder.Column.Individual_ID.name])
            ids_2 = set(event_df[event_df[ReportEventRecorder.Column.Event_Name.name] == event_to_compare_2][
                            ReportEventRecorder.Column.Individual_ID.name])
            if not rt_support.compare_ids_with_2_events(ids_1, ids_2, event_to_compare_1, event_to_compare_2,
                                                        output_report_file):
                succeed = False

        Expected_NotTested_ids = all_ids - set(event_df[event_df[ReportEventRecorder.Column.Event_Name.name].isin(
            diagnostic_event_list)][ReportEventRecorder.Column.Individual_ID.name])
        Actual_NotTested_ids = set(event_df[event_df[ReportEventRecorder.Column.Event_Name.name].str.contains(
            "NotTested")][ReportEventRecorder.Column.Individual_ID.name])

        if Expected_NotTested_ids != Actual_NotTested_ids:
            succeed = False
            output_report_file.write("BAD: Individual who didn't receive HIVRapidHIVDiagnostic intervention should "
                                     "receive a 'NotTested_**' event.\n")
        else:
            output_report_file.write("GOOD: Individual who didn't receive HIVRapidHIVDiagnostic intervention all "
                                     "received a 'NotTested_**' event.\n")

        Received_Positive_Results_ids = set(event_df[event_df[ReportEventRecorder.Column.Event_Name.name] ==
                                                     Event.Received_Positive_Results][
                            ReportEventRecorder.Column.Individual_ID.name])
        Tested_Positive_ids = set(event_df[event_df[ReportEventRecorder.Column.Event_Name.name] ==
                                                     Event.Tested_Positive][
                            ReportEventRecorder.Column.Individual_ID.name])
        if Received_Positive_Results_ids.issubset(Tested_Positive_ids):
            output_report_file.write(f"GOOD: Individuals who received {Event.Received_Positive_Results} event is "
                                     f"a subset of individuals who received {Event.Tested_Positive} event.\n")
        else:
            succeed = False
            output_report_file.write(f"BAD: Individuals who received {Event.Received_Positive_Results} event is not "
                                     f"a subset of individuals who received {Event.Tested_Positive} event.\n")

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

