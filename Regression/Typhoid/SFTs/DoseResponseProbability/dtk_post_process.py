#!/usr/bin/python

import json
import datetime
import time
import math
import dtk_test.dtk_sft as sft


KEY_START_TIME = "Start_Time"
KEY_SIMULATION_DURATION = "Simulation_Duration"
KEY_TYPHOID_ACUTE_INFECTIOUSNESS = "Typhoid_Acute_Infectiousness"


def load_emod_parameters(config_filename="config.json"):
    """reads config file and populates params_obj
    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_START_TIME, KEY_SIMULATION_DURATION, KEY_TYPHOID_ACUTE_INFECTIOUSNESS
    """
    cdj = None
    param_obj = {}
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
        param_obj[KEY_START_TIME] = int(cdj[KEY_START_TIME])
        param_obj[KEY_SIMULATION_DURATION] = int(cdj[KEY_SIMULATION_DURATION])
        param_obj[KEY_TYPHOID_ACUTE_INFECTIOUSNESS] = float(cdj[KEY_TYPHOID_ACUTE_INFECTIOUSNESS])
    return param_obj


def parse_stdout_file(param_data, start_time=1, stdout_filename="test.txt", filtered_filename="filtered_lines.txt", debug=False):
    """ Goes through the file and collects lines relevant to contact and environemntal dose exposures
    :param start_time:   first time step from config
    :param stdout_filename: file to parse (test.txt)
    :param filtered_filename:   file to parse to (only happens in debug)
    :param debug: whether or not data gets saved to to the external file
    :return:     lines for contact, lines for environmental
    """

    sim_duration_config = param_data[KEY_SIMULATION_DURATION]
    start_time_config = param_data[KEY_START_TIME]

    UPDATE_TIME = "Update(): Time:"
    EXPOSING = "Exposing "
    ROUTE_ENV = "route 'environment'"
    ROUTE_CONT = "route 'contact'"

    success = False
    start_time = datetime.datetime.now()
    max_time = datetime.timedelta(seconds=600)
    elapsed_time = datetime.timedelta(seconds=0)
    while not (success or elapsed_time > max_time):
        timestep = start_time_config
        lines_e = []
        lines_c = []
        filtered_lines = []
        with open(stdout_filename) as logfile:
            for line in logfile:
                if UPDATE_TIME in line:
                    if debug:
                        filtered_lines.append(line)
                    # calculate time step
                    timestep += 1
                elif (EXPOSING and ROUTE_ENV) in line:
                    # collect dose_response probabilities and dose for route environment
                    if debug:
                        filtered_lines.append(line)
                    tmpline = "TimeStep: " + str(timestep) + " " + line
                    lines_e.append(tmpline)
                elif (EXPOSING and ROUTE_CONT) in line:
                    # collect dose_response probabilities and dose for route contact
                    if debug:
                        filtered_lines.append(line)
                    tmpline = "TimeStep: " + str(timestep) + " " + line
                    lines_c.append(tmpline)
        if sim_duration_config - start_time_config >= timestep:
            success = True
        else:
            time.sleep(10)
            elapsed_time = datetime.datetime.now() - start_time

    if debug:
        with open(filtered_filename,"w") as outfile:
            outfile.write("First lines_e split: \n")
            splits = lines_e[0].split()
            for i in range(0, len(splits)):
                outfile.write("\t " + str(i) + " " + splits[i] + " \n")
            outfile.write("First lines_c split: \n")
            splits = lines_c[0].split()
            for i in range(0, len(splits)):
                outfile.write("\t " + str(i) + " " + splits[i] + " \n")
            outfile.write("Filtered Lines: \n")
            for line in filtered_lines:
                outfile.write(line)

    return [lines_c, lines_e]


def parse_json_report(start_time=1, output_folder="output", insetchart_name="InsetChart.json", debug=False):

    inset_days = []
    return inset_days


def create_report_file(data, inset_days, report_name=sft.sft_output_filename):

    lines_c = data[0]
    lines_e = data[1]
    typhoid_acute_infectiousness = data[2]
    ALPHA = 0.175
    N50 = 1.11e6
    dose_response_data_c = []
    dose_response_data_e = []
    dose_response_data_theoretic_c = []
    dose_response_data_theoretic_e = []
    success = True
    with open(report_name, "w") as report_file:
        if len(lines_c) == 0:
            success = False
            report_file.write("Found no individual exposed from route contact.\n" )
        else:
            for line in lines_c:
                dose_response_c = float(sft.get_val("infects=", line))
                dose_response_data_c.append(dose_response_c)
                ind_id_c = int(sft.get_val("individual ", line))
                timestep_c = int(sft.get_val("TimeStep: ", line))
                fContact = float(sft.get_val("fContact=", line))
                dose_response_theoretic_c = float(fContact / typhoid_acute_infectiousness)
                dose_response_data_theoretic_c.append(dose_response_theoretic_c)
                if math.fabs(dose_response_theoretic_c - dose_response_c) > 5e-2:
                    success = False
                    report_file.write(
                        "BAD: Dose-response probability for individual {0} at time {1}, route contact is {2}, "
                        "expected {3}.\n".format(ind_id_c, timestep_c, dose_response_c, dose_response_theoretic_c))
        if len(lines_e) == 0:
            success = False
            report_file.write("Found no individual exposed from route environment.\n")
        else:
            for line in lines_e:
                dose_response_e = float(sft.get_val("infects=", line))
                dose_response_data_e.append(dose_response_e)
                ind_id = int(sft.get_val("individual ", line))
                timestep = int(sft.get_val("TimeStep: ", line))
                exposure = float(sft.get_val("exposure=", line))
                dose_response_theoretic_e = 1.0 - math.pow(1.0 + exposure *
                                                           (math.pow(2.0, 1.0 / ALPHA) - 1.0) / N50, -1.0 * ALPHA)
                dose_response_data_theoretic_e.append(dose_response_theoretic_e)
                if math.fabs(dose_response_theoretic_e - dose_response_e) > 5e-2:
                    success = False
                    report_file.write(
                        "BAD: Dose-response probability for individual {0} at time {1}, route environment is {2}, "
                        "expected {3}.\n".format(ind_id, timestep, dose_response_e, dose_response_theoretic_e))

        report_file.write("Sample size of dose response is {0} for route contact and {1} for route environment."
                          "\n".format(len(lines_c), len(lines_e)))
        report_file.write(sft.format_success_msg(success))

        return [dose_response_data_c, dose_response_data_theoretic_c,
                dose_response_data_e, dose_response_data_theoretic_e]


def application(output_folder="output", stdout_filename="test.txt",
                        config_filename="config.json",
                        insetchart_name="InsetChart.json",
                        report_name=sft.sft_output_filename,
                        debug=False):

    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename + "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "insetchart_name: " + insetchart_name + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )
    sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename)
    parsed_data = parse_stdout_file(param_obj, stdout_filename, debug=debug)
    inset_days = parse_json_report()
    parsed_data.extend([param_obj[KEY_TYPHOID_ACUTE_INFECTIOUSNESS]])
    data_to_graph = create_report_file(parsed_data, inset_days,
                                       report_name=sft.sft_output_filename)

    sft.plot_data_sorted(data_to_graph[0], data_to_graph[1], label1="Actual", label2="Expected",
                  title="Contact Dose Response Probability", xlabel="Occurrences",
                  ylabel="Dose Response",
                  category='contact_dose_response_probability_plot',
                         alpha=0.5, overlap=True)
    sft.plot_data_sorted(data_to_graph[2], data_to_graph[3], label1="Actual", label2="Expected",
                  title="Environmental Dose Response Probability", xlabel="Occurrences",
                  ylabel="Dose Response",
                  category='environmental_dose_response_probability_plot',
                         alpha=0.5, overlap=True)


if __name__ == "__main__":
    # execute only if run as a script
    application("output")
