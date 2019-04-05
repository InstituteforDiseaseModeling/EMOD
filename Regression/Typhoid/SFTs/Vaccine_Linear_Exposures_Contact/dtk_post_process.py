#!/usr/bin/python
# This SFT verifies that when the TyphoidWASH intervention is implemented (exposures attenuation is set to 0), no
# new infections are happening. The infectivity and contagion is turned up so there should be at least some infections
# if the intervention wasn't working.

import dtk_test.dtk_sft as sft
from dtk_test.dtk_typhoid_linear_exposures_support import VlesContact


def application(output_folder="output", stdout_filename="test.txt", initial_timestep=0,
                insetchart_name="InsetChart.json", config_filename="config.json",
                report_name=sft.sft_output_filename, debug=False):
    if debug:
        print("output_folder: " + output_folder + "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("initial_timestep: " + str(initial_timestep) + "\n")
        print("insetchart_name: " + insetchart_name + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    vles = VlesContact()
    inset_chart_obj = vles.parse_inset_chart(output_folder=output_folder, insetchart_name=insetchart_name, debug=debug)
    attenuation = vles.parse_stdout_file(initial_timestep=initial_timestep,
                                         stdout_filename=stdout_filename, debug=debug)
    vles.create_report_file(attenuation=attenuation, inset_chart_obj=inset_chart_obj,
                            report_name=report_name, debug=debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--config', default="config.json", help="config.json param file")
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-i', '--insetchart', default="InsetChart.json", help="InsetChart.json data file")
    parser.add_argument('-t', '--time', default=0, help="initial timestep for filterting test.txt data(test.txt)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout, config_filename=args.config,
                initial_timestep=args.time, report_name=args.reportname, debug=True)
