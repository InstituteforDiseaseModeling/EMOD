import json
import dtk_test.dtk_sft as dtk_sft
import math

"""
This test looks at the values that we get out of our gamma distribution and runs a ks comparison test
with a gamma distribution from numpy. Our gamma distribution code is copied from numpy.
We use gamma distribution with calculating oocysts and sporozoites, so the relevant parameters are:
"Num_Oocyst_From_Bite_Fail"
"Num_Sporozoites_In_Bite_Fail"
"Probability_Oocyst_From_Bite_Fails"
"Probability_Sporozoite_In_Bite_Fails"
Where the probability gets converted to theta of the gamma distribution by: (1-p)/p
"""

config_name = "Config_Name"
parasite_genetics = "Parasite_Genetics"
num_oocyst_from_bite = "Num_Oocyst_From_Bite_Fail"
num_sporozoites_in_bite = "Num_Sporozoites_In_Bite_Fail"
prob_oocyst_from_bite = "Probability_Oocyst_From_Bite_Fails"
prob_sporozoites_in_bite = "Probability_Sporozoite_In_Bite_Fails"


def load_emod_parameters(config_filename="config.json", debug=False):
    """
    Reads the config filename and takes out relevant parameters.
    Args:
        config_filename: config filename
        debug: when true, the parameters get written out as a json.

    Returns:

    """

    """reads config file and populates params_obj
    :param config_filename: name of config file (config.json)
    :param debug: write out parsed data on true
    :returns param_obj:     dictionary with KEY_CONFIG_NAME, etc., keys (e.g.)
    """
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {config_name: cdj[config_name],
                 num_oocyst_from_bite: cdj[parasite_genetics][num_oocyst_from_bite],
                 num_sporozoites_in_bite: cdj[parasite_genetics][num_sporozoites_in_bite],
                 prob_sporozoites_in_bite: cdj[parasite_genetics][prob_sporozoites_in_bite],
                 prob_oocyst_from_bite: cdj[parasite_genetics][prob_oocyst_from_bite]}

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
    generated_gamma = {}
    with open(output_filename) as logfile:
        for line in logfile:
            if "Gamma with" in line:
                filtered_lines.append(line)
                k_shape = float(dtk_sft.get_val("k = ", line))
                theta = float(dtk_sft.get_val("theta = ", line))
                generated = float(dtk_sft.get_val("generated ", line))
                if (k_shape, theta) in generated_gamma:
                    generated_gamma[(k_shape, theta)].append(generated)
                else:
                    generated_gamma[(k_shape, theta)] = [generated]

    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    return generated_gamma


def create_report_file(generated_gamma, param_obj, report_name, debug):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj[config_name]} : \n")
        if not generated_gamma:
            success = False
            outfile.write("No logging data for gamma distribution found.\n")
        else:
            #do sporozoites
            k_sporozoites = param_obj[num_sporozoites_in_bite]
            theta_sporozoites = (1 - param_obj[prob_sporozoites_in_bite]) / param_obj[prob_sporozoites_in_bite]
            if (k_sporozoites, theta_sporozoites) not in generated_gamma:
                success = False
                outfile.write(f"We don't have a matching gamma distribution key in the log file. Cannot find (k, theta)"
                              f" = ({k_sporozoites}, {theta_sporozoites}).\n ")
            else:
                spor_gamma = generated_gamma[(k_sporozoites, theta_sporozoites)]
                result = dtk_sft.test_gamma(spor_gamma, k_sporozoites, theta_sporozoites, outfile)
                if not result:
                    success = False
                    outfile.write(f"BAD: Generated gamma distribution for (k, theta) = ({k_sporozoites}, "
                                  f"{theta_sporozoites}) did not pass the ks test.\n")
                else:
                    outfile.write(f"GOOD: Generated gamma distribution for (k, theta) = ({k_sporozoites}, "
                                  f"{theta_sporozoites}) passed the ks test.\n")

            #do oocysts
            k_oocysts = param_obj[num_oocyst_from_bite]
            theta_oocysts = (1 - param_obj[prob_oocyst_from_bite]) / param_obj[prob_oocyst_from_bite]
            if (k_sporozoites, theta_sporozoites) not in generated_gamma:
                success = False
                outfile.write(f"We don't have a matching gamma distribution key in the log file. Cannot find "
                              f"(k, theta) = ({k_oocysts}, {theta_oocysts}).\n ")
            else:
                oocysts_gamma = generated_gamma[(k_oocysts, theta_oocysts)]
                result = dtk_sft.test_gamma(oocysts_gamma, k_oocysts, theta_oocysts, outfile)
                if not result:
                    success = False
                    outfile.write(f"BAD: Generated gamma distribution for (k, theta) = ({k_oocysts}, "
                                  f"{theta_oocysts}) did not pass the ks test.\n")
                else:
                    outfile.write(f"GOOD: Generated gamma distribution for (k, theta) = ({k_oocysts}, "
                                  f"{theta_oocysts}) passed the ks test.\n")

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
    generated_gamma = parse_output_file(stdout_filename, debug)
    param_obj = load_emod_parameters(config_filename, debug)
    create_report_file(generated_gamma, param_obj, report_name, debug)


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
