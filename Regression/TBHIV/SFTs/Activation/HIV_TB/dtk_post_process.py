#!/usr/bin/python

import json
import os.path as path
import dtk_sft
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
    new_infection_state = "SetNewInfectionState"
    filtered_lines = []
    update_time = "Update(): Time:"
    incubation_timer = "InitializeLatentInfection"
    time = 0
    with open(stdout_filename) as logfile:
        for line in logfile:
            if update_time in line:
                time += 1
            elif new_infection_state in line:
                new_line = dtk_sft.add_time_stamp(time, line)
                filtered_lines.append(new_line)
            elif incubation_timer in line:
                new_line = dtk_sft.add_time_stamp(time, line)
                filtered_lines.append(new_line)

    if debug:
        with open("filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    return filtered_lines


def create_report_file(data, debug=False):
    report_name = data[0]
    lines = data[1]
    tb_cd4_activation_vector = data[2]  # this test assumes the vector is constant

    latency_data = {}
    duration_data = {}
    success = True
    with open(report_name, "w") as outfile:
        if not lines:
            outfile.write("BAD: No relevant test data found.\n")
            success = False
        for line in lines:
            if "InitializeLatentInfection" in line:
                ind_id = int(dtk_sft.get_val("Individual ", line))
                start_time_stamp = int(dtk_sft.get_val("time= ", line))
                if ind_id in latency_data.keys():
                    outfile.write("Individual {} incubation timer reset at time {}. Please check. "
                                  "\n".format(ind_id, start_time_stamp))
                latency_data[ind_id] = start_time_stamp
            elif "TBActivationPresymptomatic" in line:
                ind_id = int(dtk_sft.get_val("Individual ", line))
                end_time_stamp = int(dtk_sft.get_val("time= ", line))
                if ind_id not in latency_data.keys():
                    outfile.write("Individual {} went presymptomatic without incubation timer update at time {}. "
                                  "Please check. \n".format(ind_id, end_time_stamp))
                else:
                    duration = end_time_stamp - latency_data.get(ind_id)
                    duration_data[ind_id] = duration

        if debug:
            with open("DEBUG_duration_data.json","w") as debug_outfile:
                json.dump(duration_data, debug_outfile, indent=4)
        durations = list(duration_data.values())

        # expected_data here only used for graphing purposes
        expected_data = [int(x+1) for x in np.random.exponential(1/tb_cd4_activation_vector[0], len(duration_data))]
        success = dtk_sft.test_exponential(durations, tb_cd4_activation_vector[0], outfile,
                                           integers=True, roundup=True, round_nearest=False)
        outfile.write("Data points checked = {}.\n".format(len(duration_data)))
        outfile.write("SUMMARY: Success={0}\n".format(success))


        dtk_sft.plot_data(sorted(durations), sorted(expected_data), label1="Actual", label2="Expected",
                          title="Latency Duration HIV then TB (Sorted)", xlabel="Data Points", ylabel="Days",
                          category="tb_activation_and_cd4_hiv_first", line = True, overlap=True)


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json",
                insetchart_name="InsetChart.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "insetchart_name: " + insetchart_name + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )
    dtk_sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename)
    parsed_data = parse_stdout_file(debug=True)
    create_report_file([report_name, parsed_data, param_obj.get(TB_CD4_ACTIVATION_VECTOR)], debug)


if __name__ == "__main__":
    # execute only if run as a script
    application("output", debug=True)
