#!/usr/bin/python

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_vector_genetics_support as dvgs


def application(output_folder="output", stdout_filename="test.txt", config_filename="config.json",
                report_name=dtk_sft.sft_output_filename, debug=False):
    dtk_sft.wait_for_done()
    dvgs.application(output_folder=output_folder, stdout_filename=stdout_filename, config_filename=config_filename,
                     report_name=report_name, debug=debug)


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

    application(output_folder=args.output, stdout_filename=args.stdout, config_filename=args.config,
                     report_name=args.reportname, debug=args.debug)
