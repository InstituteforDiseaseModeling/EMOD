#!/usr/bin/python

import json
import os.path as path
import dtk_sft
import numpy as np
from scipy import stats
import numpy as np

TB_CD4_ACTIVATION_VECTOR = "TB_CD4_Activation_Vector"
TB_CD4_STRATA_ACTIVATION = "CD4_Strata_Activation"

"""
The activation time gets modified based on the TB_CD4_Activation_Vector, using CD4_Strata_Activation.
Also, if the cd4 count of the person exceeds the strata's max, currently there is an issue where the multiplier gets
really big (https://github.com/bradwgnr/DtkTrunk/issues/27) and my test catches that.

"""


def load_emod_parameters(config_filename="config.json"):
    """reads config file and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with keys
    """
    cdj = None
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {TB_CD4_ACTIVATION_VECTOR: cdj[TB_CD4_ACTIVATION_VECTOR],
                 TB_CD4_STRATA_ACTIVATION: cdj[TB_CD4_STRATA_ACTIVATION]}
    return param_obj


def parse_stdout_file(curr_timestep=0, stdout_filename="test.txt", debug=False):
    """

    :param curr_timestep:   first timestep from config
    :param stdout_filename: file to parse (test.txt)
    :param debug:           whether or not we write an additional file that's full of the matched lines
    :return:                array of lines of interest
    """
    filtered_lines = []
    update_time = "Update(): Time:"
    incubation_timer_update = "LifeCourseLatencyTimerUpdate"
    art_notify_event = "notifyOnEvent"
    time = 0
    with open(stdout_filename) as logfile:
        for line in logfile:
            if update_time in line:
                time += 1
            elif incubation_timer_update in line:
                new_line = dtk_sft.add_time_stamp(time, line)
                filtered_lines.append(new_line)
            elif art_notify_event in line:
                new_line = dtk_sft.add_time_stamp(time, line)
                filtered_lines.append(new_line)

    if debug:
        with open("filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    return filtered_lines


def parse_json_report(start_time=0, output_folder="output", insetchart_name="InsetChart.json", debug=False):
    """creates inset_days structure

    :param debug: Whether or not we're doing this in debug mode, writes out the data we got if True
    :param output_folder: folder in which json report resides
    :param start_time: start time of the json report
    :param insetchart_name: InsetChart.json file with location (output/InsetChart.json)

    :returns: inset_days structure
    """
    # This is not used in this test

    insetchart_path = path.join(output_folder, insetchart_name)
    with open(insetchart_path) as infile:
        icj = json.load(infile)["Channels"]

    prevalence = icj["Infected"]["Data"]
    end_time = start_time + len(prevalence)
    inset_days = {}
    for x in range(start_time, end_time):
        inset_days[x] = x

    if debug:
        with open("inset_days.json", "w") as outfile:
            json.dump(inset_days, outfile, indent=4)

    return inset_days


def create_report_file(data):
    report_name = data[0]
    lines = data[1]
    tb_cd4_activation_vector = data[2]  # this test assumes the vector is constant

    # StartedArt distribution is currently based on 0.01, while StoppedArt is on mostly 0.0000001, so we expect
    # much longer latency durations for the StoppedArt data, using big arbitrary # that is noticeably bigger than
    # what StartedArt distribution would extremely likely give us
    big_magic_number = 2000
    stopped_art_latency_data = []
    started_art_latency_data = []
    art_events_dict = {}
    success = True
    with open(report_name, "w") as outfile:
        if not lines:
            outfile.write("BAD: No relevant test data found.\n")
            success = False
        for line in lines:
            if "has event" in line:
                ind_id = int(dtk_sft.get_val("Individual ", line))
                art_status = line.split(" ")[9].strip(".")  # get_val only gets digits
                art_events_dict[ind_id] = art_status
            if "LifeCourseLatencyTimerUpdate" in line:
                ind_id = int(dtk_sft.get_val("Individual ", line))
                new_incubation_timer = float(dtk_sft.get_val("timer ", line))
                if ind_id in art_events_dict.keys():
                    if art_events_dict.get(ind_id) == "StartedART":
                        started_art_latency_data.append(new_incubation_timer)
                    else:
                        stopped_art_latency_data.append(new_incubation_timer)
                    art_events_dict.pop(ind_id)
                else:
                    success = False
                    outfile.write("BAD: No art-related event found in the logs for this timer update for Individual {},"
                                  " at time {}.\n".format(ind_id, int(dtk_sft.get_val("time= ", line))))
        # we want the stopped art latency data to NOT match the started art latency data
        # and we expect the stopped art latency data to be long period times as made my our cd4_Activation_vector
        if dtk_sft.test_exponential(stopped_art_latency_data, tb_cd4_activation_vector[2], integers=False,
                                    roundup=False, round_nearest=False):
            outfile.write("BAD: The StoppedArt latency data distribution matches the StartedArt latency data"
                          " distribution, but shouldn't.\n")
            success = False
        expected_stopped_art_data = np.random.exponential(1/tb_cd4_activation_vector[0], len(stopped_art_latency_data))
        small_duration_count = 0
        for duration in stopped_art_latency_data:
            if duration < big_magic_number:
                small_duration_count += 1
        proportion_small = small_duration_count/float(len(stopped_art_latency_data))
        if proportion_small > 0.01:
            outfile.write("BAD: More than 0.5% of our durations are suspiciously small, it is {}. "
                          "Please Investigate.\n".format(proportion_small))
            success = False
        outfile.write("Data points checked = {}.\n".format(len(stopped_art_latency_data)))
        outfile.write("SUMMARY: Success={0}\n".format(success))

        dtk_sft.plot_data(sorted(stopped_art_latency_data), sorted(expected_stopped_art_data), label1="Actual",
                          label2="Expected",
                          title="StoppedART Latency data should have a similar shape/scale of duration but will not "
                                "match",
                          xlabel="Data Points", ylabel="Days",
                          category="tb_activation_and_cd4_hiv_first_on_art_off_art", line = True, overlap=True)


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json",
                insetchart_name="InsetChart.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename + "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "insetchart_name: " + insetchart_name + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )
    dtk_sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename)
    parsed_data = parse_stdout_file()
    inset_days = parse_json_report()
    create_report_file([report_name, parsed_data, param_obj.get(TB_CD4_ACTIVATION_VECTOR)])

if __name__ == "__main__":
    # execute only if run as a script
    application("output")
