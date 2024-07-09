#!/usr/bin/python

import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds


"""
0.7 of vectors feed inside on a three day schedule, so if we kill all the 
feeding vectors using this intervention for three days, this should reduce
the vector population and biting by 0.7 to 0.3 of initial for one of the nodes. 
one node (with one person):
0.3/1.3 = 0.23
other node
1/1.3 = 0.77

"""


def application(output_folder="output",
                config_filename="config.json",
                stdout="test.txt",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("stdout_filename: " + stdout + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    param_obj = veds.load_emod_parameters(config_filename)
    humans, human_bites, bites_by_day = veds.track_bites(stdout, start_time=5, debug=True)
    veds.create_biting_report_file(param_obj=param_obj, humans=humans, bites_by_day=bites_by_day,
                                   report_name=report_name, human_bites=human_bites,
                                   with_intervention_proportion=0.23,
                                   without_intervention_proportion=0.77,
                                   expected_people_with_intervention=1,
                                   expected_people_without_intervention=1,
                                   x_error=1,
                                   debug=debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)
