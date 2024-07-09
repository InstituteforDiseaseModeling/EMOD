import json

import dtk_test.dtk_vector_genetics_support as genetics_support
import dtk_test.dtk_sft as dtk_sft


"""
This test distributes hrp2-present infections to all 1k people and 300 hrp2-negative infections
to 300 of the all 1k people. We then make sure that MalariaDiagnostic can detect all the people's infections, 
because they at least have one hrp2-present infection. We also verify that the data in the InsetChart is correct. 
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
    genetics_support.hrp2_support(expected_pfhrp2_prevalence=1, expected_hrp_deleted_fraction_of_infections=0.23,
                                  expected_hrp_deleted_fraction_of_infected_people=0.3, expected_infected=1,
                                  expected_num_total_infections=1300)


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
