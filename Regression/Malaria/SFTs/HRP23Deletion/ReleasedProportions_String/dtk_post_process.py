import json

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_vector_genetics_support as genetics_support
import numpy as np

"""
This test verifies that the hrp2 (and absence of it) is correctly detected by the MalariaDiagnostic and correctly
displayed in InsetChart. We use BARCODE_STRING option in the OutbreakMalariaGenetics to give 0.3 of the population
hrp2-positive infections and, in a separate outbreak, 0.7 of the population hrp-negative outbreak. Since this is two 
different outbreaks, they overlap,  we will have people with no infections at all and people with both infections. 
Total infected: 1 - (0.3 * 0.7) = about 0.79, hrp2 deleted fraction of infected people is 0.7/0.79 = 0.89
"""


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done()
    genetics_support.hrp2_support(expected_pfhrp2_prevalence=0.3, expected_hrp_deleted_fraction_of_infections=0.7,
                                  expected_hrp_deleted_fraction_of_infected_people=0.89, expected_infected=0.79,
                                  expected_num_total_infections=1000)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                config_filename=args.config, report_name=args.reportname, debug=args.debug)
