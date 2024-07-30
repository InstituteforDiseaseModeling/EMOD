import json

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_vector_genetics_support as genetics_support
import numpy as np

"""
This test verifies that if we keep treating only people with hrp2-detectable infections, hrp2-negative infections
will start to dominate. 
"""


def create_report_file(config_filename, report_name):
    success = True
    with open("config.json", "r") as config_file:
        cf = json.load(config_file)
    test_name = cf["parameters"]["Config_Name"]
    with open("output/InsetChart.json", "r") as output:
        output_data = json.load(output)["Channels"]
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {test_name}  \n")
        day = 130
        day_index = day - 1
        data = "Data"
        hrp_deleted_fraction_of_infections = "HRP Deleted Fraction of All Infections"
        first_day_index = 3
        expected_initial_release_fraction = 0.125
        all_hrp_deleted = 1
        actual_initial_release_fraction = output_data[hrp_deleted_fraction_of_infections][data][first_day_index]
        final_hrp_deleted_fraction = output_data[hrp_deleted_fraction_of_infections][data][day_index]
        if actual_initial_release_fraction != expected_initial_release_fraction:
            success = False
            outfile.write(f"BAD: On Day {first_day_index + 1}, we expect the initial "
                          f"{hrp_deleted_fraction_of_infections} to be {expected_initial_release_fraction}, "
                          f"but it was {actual_initial_release_fraction}\n")
        else:
            outfile.write(f"GOOD: On Day {first_day_index + 1}, we expect the initial "
                          f"{hrp_deleted_fraction_of_infections} to be {expected_initial_release_fraction}, "
                          f"and it was {actual_initial_release_fraction}\n")
        if final_hrp_deleted_fraction < all_hrp_deleted:
            success = False
            outfile.write(f"BAD: On Day {day}, we expect the {hrp_deleted_fraction_of_infections} to be at least "
                          f"{all_hrp_deleted} due to test-and-treat methods, "
                          f"but it was {final_hrp_deleted_fraction}\n")
        else:
            outfile.write(f"GOOD: On Day {day}, we expect the {hrp_deleted_fraction_of_infections} to be at least "
                          f"{all_hrp_deleted} due to test-and-treat methods, "
                          f"and it was {final_hrp_deleted_fraction}\n")

        outfile.write(dtk_sft.format_success_msg(success))


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
    create_report_file(config_filename, report_name)


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
