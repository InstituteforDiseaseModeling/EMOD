import json
import dtk_test.dtk_sft as dtk_sft
import math

"""
This test verifies that the obligate location is selected with a uniform distribution within the chromosome
We are just checking the first chromosome. 

"""

config_name = "Config_Name"
male = "male"
female = "female"

def load_emod_parameters(config_filename="config.json", debug=False):
    """
    Reads the config filename and takes out relevant parameters.
    Args:
        config_filename: config filename
        debug: when true, the parameters get written out as a json.

    Returns:

    """
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {config_name: cdj[config_name]}

    if debug:
        with open("DEBUG_emod_params.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj


def parse_output_file(output_filename="test.txt", debug=False):
    """
        Looks for line "Gamma with k = 3.000000 and theta = 1.000000 generated 1.095271" and
        created a dictionary with (k, theta) tuples as key and alll the found generated values in a list as value

    Args:
        output_filename: the logging file
        debug: if True saves off a file with just the lines of interest and saves the final dataframe

    Returns:
        dictionary with (k, theta) as keys and list of generated gamma values as value
    """
    filtered_lines = []
    female_chromatids = []
    male_chromatids = []
    with open(output_filename) as logfile:
        for line in logfile:
            if "Female chromatid" in line:
                filtered_lines.append(line)
                female_chromatid = int(dtk_sft.get_val("Female chromatid ", line))
                male_chromatid = int(dtk_sft.get_val("male chromatid ", line))
                female_chromatids.append(female_chromatid)
                male_chromatids.append(male_chromatid)

    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    return {female: female_chromatids,male: male_chromatids}


def create_report_file(chromatids, param_obj, report_name, debug):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj[config_name]} : \n")
        if not chromatids[male] or not chromatids[female]:
            success = False
            outfile.write("No logging data for chromatids found.\n")
        else:
            successes = []
            expected_average = 0.5
            error_tolerance = 0.02
            for gender in [male, female]:
                actual_average = sum(chromatids[gender])/len(chromatids[gender])
                if abs(actual_average - expected_average) < error_tolerance:
                    outfile.write(f"GOOD: The distribution of {gender} chromatid choices is uniform.\n")
                else:
                    success = False
                    file = f"{gender}_chromatid_choices.txt"
                    outfile.write(f"BAD: The distribution of {gender} chromatid choices is not uniform, we expected "
                                  f"an average of 0.5, but got {actual_average}. Check {file} .\n")
                    with open(file, 'w') as f:
                        for choice in chromatids[gender]:
                            f.write(f"{choice}\n")

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
    chromatids = parse_output_file(stdout_filename, debug)
    param_obj = load_emod_parameters(config_filename, debug)
    create_report_file(chromatids, param_obj, report_name, debug)


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
