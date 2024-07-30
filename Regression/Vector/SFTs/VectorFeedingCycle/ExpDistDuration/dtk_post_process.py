#!/usr/bin/python

from __future__ import division
import numpy as np
import math
import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import dtk_test.dtk_sft_svetlana as svet

"""
Tests that the mean feeding cycle duration should be equal to the
average of an exponentially distributed rate 
of the feeding cycle durations.
"""


def parse_output_file(stdout, debug=False):
    """
    Parses following lines, example:

    Mean gonotrophic cycle duration = 10.54870 days at 15.77 degrees C.

    The mosquito successfully fed on an animal, id = 18

    Args:
        stdout: file which we are parsing
        debug: when True, lines of interest from file are saved out as a separate file

    Returns: array of feeding durations, value of mean of feeding durations, value of temperature and an
    output dictionary
    """
    filtered_lines = []
    mean_duration = 0
    temperature_k = 0
    feeding_tally = {}
    feeding_durations = []
    time = 0
    output_dict = {time: []}
    counter = 0
    with open(stdout) as infile:
        for line in infile:
            if "Update(): Time" in line:
                filtered_lines.append(line)
                time += 1
                output_dict[time] = []
            if time:   # skipping things that happen before the official beginning of the sim
                if "Mean gonotrophic cycle" in line:
                    mean_duration = float(sft.get_val("gonotrophic cycle duration = ", line))
                    temperature_k = 273.15 + float(sft.get_val(" days at ", line))
                    counter += 1
                elif "The mosquito successfully " in line:
                    filtered_lines.append(line)
                    output_dict[time].append(1)
                    id = int((sft.get_val("id = ", line)).rstrip(".,"))
                    if id not in feeding_tally:
                        feeding_tally[id] = time
                    else:
                        feeding_duration = time - feeding_tally[id]
                        feeding_durations.append(feeding_duration)
                        feeding_tally[id] = time
                elif "The mosquito died attempting" in line:
                    filtered_lines.append(line)
                    output_dict[time].append(1)
                    id = int((sft.get_val("id = ", line)).rstrip(".,"))
                    if id in feeding_tally:
                        del feeding_tally[id]

    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as debug_file:
            debug_file.writelines(filtered_lines)

    return feeding_durations, mean_duration, temperature_k, output_dict


def application(output_folder="output",
                config_filename="config.json",
                stdout="test.txt",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    param_obj = veds.load_emod_parameters(config_filename)
    feeding_durations, mean_duration, temperature_k, output_dict = parse_output_file(stdout, debug)
    create_report_file(param_obj, report_name, feeding_durations, mean_duration, temperature_k,
                       output_dict, debug)


def create_report_file(param_obj, report_name, feeding_durations,
                       mean_duration, temperature_k, output_dict, debug=False):

    avg_feeding_duration = np.mean(feeding_durations)
    theoretical_rate = veds.vector_dur_calc(temperature_k, param_obj)
    with open(report_name, "w") as outfile:
        outfile.write("\n# Test name: " + param_obj[veds.ConfigKeys.CONFIG_NAME] + ", Run number: " +
                      str(param_obj[veds.ConfigKeys.RUN_NUMBER]) +
                      "\n# Test compares between the feeding cycle durations to verify "
                      "feeding is uniformly distributed about the mean,\n# and the mean "
                      "of that distribution is"
                      " equal to 1/the Arrhenius rate at that temperature.\n")
        success = True
        sizes = []
        for val in output_dict.values():
            sizes.append(len(val))
        y = sizes
        ind = sizes.index(0)  # Want to truncate at first occurrence where mosquitoes don't feed
        y = y[0:ind]
        distribution = []
        for i in range(0, len(y)):
            distribution += int(y[i]) * [i+1]
        outfile.write("The average of the exponential distribution of feeding cycle durations: "
                      "{}\n".format(avg_feeding_duration))
        theoretical_feeding_duration = 1 / theoretical_rate
        outfile.write("The actual 1/the rate that the average of the exponential distribution"
                      " should equal: {}\n".format(theoretical_feeding_duration))
        tolerance = 5e-2 * theoretical_feeding_duration
        if abs(theoretical_feeding_duration - avg_feeding_duration) > tolerance:
            success = False
            outfile.write("BAD: The average feeding duration, {}, was not within 5% of the theoretical, {}."
                          "\n".format(avg_feeding_duration, theoretical_feeding_duration))
        else:
            outfile.write("GOOD: The average feeding duration, {}, was within 5% of the theoretical, {}."
                          "\n".format(avg_feeding_duration, theoretical_feeding_duration))

        svet.plot_scatter_dist_w_avg(feeding_durations, theory=[mean_duration] * len(feeding_durations),
                                     title='Feeding Durations of Each Mosquito vs. Theoretical Mean',
                                     category="plot_durations")
        svet.plot_scatter_dist_w_avg(np.mean(feeding_durations), theory=mean_duration,
                                     title='Experimental vs. Theoretical Feeding Cycle Mean', category="plot_mean")
        # Now testing to see whether or not an exponential distribution is followed
        a2 = param_obj[veds.ConfigKeys.VectorSpeciesParams.CYCLE_ARRHENIUS_2]
        lower_a2 = a2 / 2
        higher_a2 = a2 * 2
        a2_vector = np.linspace(lower_a2, higher_a2, 100)
        rate_list = []
        for a2 in a2_vector:
            rate = param_obj[veds.ConfigKeys.VectorSpeciesParams.CYCLE_ARRHENIUS_1] * math.exp(-a2 / temperature_k)
            rate_list.append(rate)
        if not svet.is_exponential_behavior(rate_list, outfile, "feeding cycle"):
            success = False
        svet.plot_scatter_dist_w_avg(rate_list, title='Exponential Distribution modifying Arrhenius param',
                                     xaxis=a2_vector, xlabel="A2 param", category='a2_plot')
        outfile.write(sft.format_success_msg(success))

    if debug:
        print(param_obj)
        print(sft.format_success_msg(success))


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
                stdout=args.stdout,
                report_name=args.reportname, debug=args.debug)
