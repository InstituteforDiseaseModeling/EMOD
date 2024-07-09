import json

import dtk_test.dtk_vector_genetics_support as genetics_support
import dtk_test.dtk_sft as dtk_sft

"""
This test verifies that the hrp2 (and absence of it) is correctly detected by the MalariaDiagnostic and correctly
displayed in InsetChart. We use ALLELE_FREQUENCIES option in the OutbreakMalariaGenetics to give everyone and
hrp-negative infection and then give 0.7 of the population hrp-present infection and 0.3 of the population hrp-negative
infection. This means that everyone has two infections, but only about 70% of the population have hrp-detectable
infections. 
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
    genetics_support.hrp2_support(expected_pfhrp2_prevalence=0.7, expected_hrp_deleted_fraction_of_infections=0.3,
                                  expected_hrp_deleted_fraction_of_infected_people=0.3, expected_infected=1,
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
