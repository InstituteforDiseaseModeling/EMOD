#!/usr/bin/python

import json
import os.path as path
import dtk_test.dtk_sft as sft
import numpy as np

COINFECTION_MORTALITY_RATE_OFF_ART = "CoInfection_Mortality_Rate_Off_ART"
COINFECTION_MORTALITY_RATE_ON_ART = "CoInfection_Mortality_Rate_On_ART"

"""
This test assumes that the TB and HIV are given out on day 1 and that the tb active state duration as set by various
tb cure, mortality and back to latency rates is really long (with those rates set to really small numbers) and the
coinfection mortality rates are fairly high to the coinfection mortality always happens first. For this test we're
assuming the person is OFF ART

"""


def load_emod_parameters(config_filename="config.json"):
    """reads config file and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with keys
    """
    cdj = None
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {COINFECTION_MORTALITY_RATE_OFF_ART: cdj[COINFECTION_MORTALITY_RATE_OFF_ART],
                 COINFECTION_MORTALITY_RATE_ON_ART: cdj[COINFECTION_MORTALITY_RATE_ON_ART]}
    return param_obj


def parse_stdout_file(curr_timestep=0, stdout_filename="test.txt", debug=False):
    """
    :param curr_timestep:   first timestep from config
    :param stdout_filename: file to parse (test.txt)
    :param debug:           whether or not we write an additional file that's full of the matched lines
    :return:                array of lines of interest
    """
    died_of_coinfection = "died of CoInfection"
    state_active_symptomatic = "infectionstatechange TBActivation "
    filtered_lines = []
    update_time = "Update(): Time:"
    time = 0
    with open(stdout_filename) as logfile:
        for line in logfile:
            if update_time in line:
                time += 1
            elif died_of_coinfection in line:
                new_line = sft.add_time_stamp(time, line)
                filtered_lines.append(new_line)
            elif state_active_symptomatic in line:
                new_line = sft.add_time_stamp(time, line)
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
    coinfection_mortality_rate_off_art = data[2]
    coinfection_mortality_rate_on_art = data[3]

    died_of_coinfection = "died of CoInfection"
    state_active_symptomatic = "infectionstatechange TBActivation "
    time_to_death_data = []
    active_infections_dictionary = {}
    success = True
    with open(report_name, "w") as outfile:
        if not lines:
            outfile.write("BAD: No relevant test data found.\n")
            success = False
        for line in lines:
            if died_of_coinfection in line:
                ind_id = int(sft.get_val("Individual ", line))
                time_stamp = int(sft.get_val("time= ", line))
                if ind_id in active_infections_dictionary.keys():
                    time_to_death_data.append(time_stamp - active_infections_dictionary[ind_id])
                else:
                    success = False
                    outfile.write("BAD: Individual {} died of coinfection without going active, at time {}."
                                  "\n".format(ind_id, time_stamp))
            elif state_active_symptomatic in line:
                ind_id = int(sft.get_val("Individual ", line))
                start_time_stamp = int(sft.get_val("time= ", line))
                if ind_id in active_infections_dictionary.keys():
                    outfile.write("Individual {} went active symptomatic while already being active symptomatic"
                                  "at time {}. \n".format(ind_id, start_time_stamp))
                else:
                    active_infections_dictionary[ind_id] = start_time_stamp
        # expected_data here only used for graphing purposes
        expected_data = map(int, np.random.exponential(1/coinfection_mortality_rate_on_art, len(time_to_death_data)))
        if not sft.test_exponential(time_to_death_data, coinfection_mortality_rate_on_art, outfile, integers=True,
                                        roundup=False, round_nearest=False):
            success = False
        outfile.write("Data points checked = {}.\n".format(len(time_to_death_data)))
        outfile.write("SUMMARY: Success={0}\n".format(success))

        sft.plot_data(sorted(time_to_death_data), sorted(expected_data), label1="Actual", label2="Expected",
                          title="Time from Smear Positive On ART TBHIV to Death", xlabel="Data Points", ylabel="Days",
                          category="tbhiv_mortality_smear_positive_on_art",line = True, overlap=True)


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json",
                insetchart_name="InsetChart.json",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "insetchart_name: " + insetchart_name + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )
    sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename)
    parsed_data = parse_stdout_file()
    inset_days = parse_json_report()
    create_report_file([report_name, parsed_data, param_obj.get(COINFECTION_MORTALITY_RATE_OFF_ART),
                        param_obj.get(COINFECTION_MORTALITY_RATE_ON_ART)])

if __name__ == "__main__":
    # execute only if run as a script
    application("output")
