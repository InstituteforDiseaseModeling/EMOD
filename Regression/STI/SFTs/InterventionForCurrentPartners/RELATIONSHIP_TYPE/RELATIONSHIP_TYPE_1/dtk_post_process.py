#!/usr/bin/python
if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../../shared_embedded_py_scripts').resolve().absolute()) )


import dtk_test.dtk_sft as sft
import dtk_test.dtk_ICP_Support as icp_s

"""
InterventionForCurrentPartners\RELATIONSHIP_TYPE
This test is testing the InterventionForCurrentPartners with Prioritize_Partners_By set to RELATIONSHIP_TYPE.

This test is configured with very high concurrency for all relationships, so most of the female individuals have 
multiple partners and have multiple partners in the same relationship, but not all female individuals have all 
relationship types. It used the PFA Joint_Probabilities to make sure male in certain age range can only have one type 
of relationship:

These are the expected values in demo and campaign files:

relationship_table_male = {sti_s.DemoPfaKeys.transitory: 20,
                           sti_s.DemoPfaKeys.informal: 30,
                           sti_s.DemoPfaKeys.marital: 40,
                           sti_s.DemoPfaKeys.commercial: 50}
priorities = relationship_types = [sti_s.DemoPfaKeys.marital,
                                   sti_s.DemoPfaKeys.informal,
                                   sti_s.DemoPfaKeys.transitory,
                                   sti_s.DemoPfaKeys.commercial]

With Maximum_Partners set to 1 and "Relationship_Types" set to ["MARITAL", "INFORMAL","TRANSITORY","COMMERCIAL"], 
make sure the prioritize is marital > informal > transitory > commercial. When there are multiple partners in the 
same type, make sure it's selecting at random between multiple partners of the same type.

Data for test is loaded from ReportEventRecorder.csv, RelationshipStart.csv and RelationshipEnd.csv. 

It performs the following tests:
1. load campaign and make sure test is setup as design.
2. parse pfa overlay file and make sure test is setup as design.
3. load data from RelationshipEnd.csv and make sure it's empty
4. load data from RelationshipStart.csv and perform the following:
    4.1. collect information for relationship_map with key = female_id and 
        value = {relationship type in priorities: list of male partners' id}. 
    4.2. if a relationship is not in the priorities, we ignore it.
5. load data from ReportEventRecorder.csv and perform the following:
    5.1. test if # of expected event = # of female individuals who have relationships in the priorities * max_partner.
        5.1.1. if max_partner is an integer, tolerance is 0.
        5.1.2. if max_partner is a float value, calculate the binomial tolerance with binomial prob = decimal part of 
            max_partner.
    5.2. for each female id in relationship_map, iterate through her relationship map with the order define in 
        priorities, until we collect the enough male partners' id based on max_partner.
        5.2.1. if the # of male partners we have collect in current relationship type is less than or equal to 
            max_partner, we put them into ids_to_broadcast.
        5.2.2. if the # of male partners we have collect in current relationship type is larger than max_partner, 
            we put them into ids_to_broadcast_potential.
    5.3. calculate expected # of event from ids_to_broadcast and upper bound of # of event from ids_to_broadcast_potential.
    5.4. test if male individual received at least n = (expected count) and at most m = (upper bound) events.
    5.4 test if male
6. plotting:
    6.1. plot histogram for age of male ids who received the event.
    6.2. plot actual and expected event count for each male individual with upper limit.

Output:
    scientific_feature_report.txt
    Age.png
    count.png

"""


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
    age_bin = 10
    icp_s.create_report_file_relationship_types(output_folder, report_event_recorder, relationship_start_report,
                                                relationship_end_report, config_filename, output_report_name, age_bin,
                                                debug)


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

