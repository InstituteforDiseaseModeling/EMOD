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
This test is testing the HIV relationship targeting: HasBeenOnArtMoreOrLessThanNumMonths feature with the these options:
    More_Or_Less
    Num_Months

Test data: ReportEventRecorder.csv

Output: scientific_feature_report.txt

"""

class Event:
    HasHIV_GiveART= "HasHIV_GiveART"
    OnARTMoreThan6Months = "OnARTMoreThan6Months"

# hardcoded for now
num_months = 6


def create_report_file(event_df, event_report_name, config_filename, report_name):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")

        output_report_file.write(f"Testing {event_report_name}: \n")
        OnARTMoreThan6Months_df = event_df[event_df[ReportEventRecorder.Column.Event_Name.name] ==
                                                Event.OnARTMoreThan6Months]
        if OnARTMoreThan6Months_df.empty:
            succeed = False
            output_report_file.write(f"BAD: There is 0 {Event.OnARTMoreThan6Months} event in {event_report_name}.\n")

        GiveART_df = event_df[event_df[ReportEventRecorder.Column.Event_Name.name] ==
                                                Event.HasHIV_GiveART]
        #GiveART_df = GiveART_df[GiveART_df.duplicated(subset=ReportEventRecorder.Column.Individual_ID.name, keep='first')]
        GiveART_ids = GiveART_df[ReportEventRecorder.Column.Individual_ID.name].tolist()
        on_art_time_test = True
        for index, row in OnARTMoreThan6Months_df.iterrows():
            event_report_year = row[ReportEventRecorder.Column.Year.name]
            id = row[ReportEventRecorder.Column.Individual_ID.name]
            if id not in GiveART_ids:
                succeed = False
                output_report_file.write(f"\tBAD: individual {id} didn't receive {Event.HasHIV_GiveART} event but "
                                         f"received {Event.OnARTMoreThan6Months} event.\n")
            else:
                ART_start_year = GiveART_df[GiveART_df[ReportEventRecorder.Column.Individual_ID.name] == id][
                    ReportEventRecorder.Column.Year.name].min()
                if  event_report_year - ART_start_year < num_months / 12 :
                    succeed = on_art_time_test = False
                    output_report_file.write(f"\tBAD: individual {id} received {Event.OnARTMoreThan6Months} event at "
                                             f"year {event_report_year}, which was within {num_months} months since "
                                             f"{Event.HasHIV_GiveART} event was distributed at year {ART_start_year}.\n")
        if on_art_time_test:
            output_report_file.write(f"GOOD: Everyone received {Event.OnARTMoreThan6Months} event are on ART for "
                                     f"more than {num_months} months.\n")

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

