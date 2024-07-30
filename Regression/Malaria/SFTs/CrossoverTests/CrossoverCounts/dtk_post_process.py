import json
import dtk_test.dtk_malaria_support as mal_sup
import dtk_test.dtk_sft as dtk_sft
import numpy as np

"""
Verifying that crossover counts make sense.
I am using the first chromosome, which is 643000 base pairs long. 
With Gamma K, Theta being (2000, 100) average expected jump from the obligate is about 200,000
so an average number of crossovers (not counting the obligate) we should see is close to 3 (4)
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
    children = mal_sup.parse_output_file_for_children(stdout_filename, debug)
    param_obj = mal_sup.load_genetics_parameters(config_filename, debug)
    mal_sup.create_crossover_report_file(children, param_obj, report_name, debug)


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
