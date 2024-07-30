#!/usr/bin/python

import json
import os.path as path
import dtk_test.dtk_sft as sft


# This object lets you reference keys as fields, e.g. cdj.Start_Time rather than cdj["Start_Time"]
class Jason(object):
    def __init__(self, dictionary={}):
        for key, value in dictionary.items():
            self.__setattr__(key, value)
        return

    def __getitem__(self, item):
        return self.__dict__[item]

KEY_START_TIME_STEP = "curr_timestep"

OUTBREAK_EVENT_NAME = "OutbreakIndividual"
MOSQUITO_TO_HUMAN_TRANSMISSION = "Mosquito->Human infection transmission"
UPDATE_TIME = "Update(): Time:"
matches = [OUTBREAK_EVENT_NAME,
           MOSQUITO_TO_HUMAN_TRANSMISSION,
           UPDATE_TIME]


def has_match(target):
    for match in matches:
        if match in target:
            return True
    return False


def load_emod_parameters(config_filename="config.json"):
    """reads config file and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_START_TIMESEP
    """
    with open(config_filename) as infile:
        cdj = json.load(infile, object_hook=Jason).parameters

    param_obj = {KEY_START_TIME_STEP: cdj.Start_Time}

    return param_obj


def parse_stdout_file(start_time, stdout_filename="test.txt", debug=False):
    """reads a StdOut.txt file and produces an inset_days object

    :param start_time:      first timestep
    :param stdout_filename: Filename to load (test.txt)
    :param debug:           writes filtered_lines.txt to disk (False)
    :return:                stdout_days[int timestep]["calculated" or "logged"]
    """

    filtered_lines = []
    with open(stdout_filename) as logfile:
        for line in logfile:
            if has_match(line):
                filtered_lines.append(line)

    if debug:
        with open ("filtered_lines.txt","w") as outfile:
            outfile.writelines(filtered_lines)

    infections_today = 0
    curr_timestep = start_time
    stdout_days = {}

    for line in filtered_lines:
        if OUTBREAK_EVENT_NAME in line:
            campaign_infections = int(line.split()[7])  # 8 is "OutbreakIndividual"
            infections_today += campaign_infections
        elif MOSQUITO_TO_HUMAN_TRANSMISSION in line:
            existing_infections = int(line.split()[-1].rstrip('.'))
            if existing_infections < 1:
                infections_today += 1
        elif UPDATE_TIME in line:
            stdout_days[curr_timestep] = Jason({"calculated":infections_today, "logged":int(line.split()[12])})
            infections_today = 0
            curr_timestep += 1

    return stdout_days


def parse_json_report(start_time, output_folder="output",
                      insetchart_name="output/InsetChart.json", debug=False):
    """

    :param start_time:
    :param insetchart_name:
    :param debug:
    :return:
    """
    insetchart_path = path.join(output_folder, insetchart_name)

    with open (insetchart_path) as infile:
        icj = json.load(infile, object_hook=Jason).Channels

    infected_fraction = icj.Infected.Data
    new_infections = icj["New Infections"].Data

    inset_days = {}
    end_time = start_time + len(infected_fraction)
    for x in range(start_time, end_time):
        day = Jason({
            "Infected_Fraction": infected_fraction[x],
            "New_Infections": new_infections[x] * 100
        })
        inset_days[x] = day

    if debug:
        with open("inset_days.json", "w") as outfile:
            json.dump(inset_days, outfile, indent=4)

    return inset_days


def create_report_file(inset_days, stdout_days, debug):
    """
    :param inset_days:
    :param stdout_days:
    :return:
    """

    success = (len(inset_days) == len(stdout_days))

    with open(sft.sft_output_filename, "w") as report_file:

        for day in inset_days.keys():

            inset_new_infections = inset_days[day].New_Infections
            inset_inf_fraction = inset_days[day].Infected_Fraction
            stdout_new_infections = stdout_days[day].calculated
            stdout_logged_total_infected = stdout_days[day].logged

            # New infections inset = log
            if abs(inset_new_infections/100 - stdout_new_infections) > 0:
                success = False
                report_file.write(f"BAD: Timestep: {day} - insetchart's new infections is {inset_new_infections/100}, "
                                  f"and StdOut calculated new infections is {stdout_new_infections} \n")

            # Infected fraction = infected / 1000 with pop of 1000
            stdout_infected_fraction = stdout_logged_total_infected / 1000.0
            if abs(inset_inf_fraction - stdout_infected_fraction) > 0.01:
                success = False
                err_format = "BAD: Timestep: {0} insetchart Infected fraction: {1} StdOut infected percentage: {2}\n"
                report_file.write(err_format.format(str(day), inset_inf_fraction, stdout_infected_fraction))
            elif debug:
                report_file.write(f"GOOD: Timestep {day}, insetchart's infected fraction is {inset_inf_fraction},"
                                  f" and stdout infected percentage is {stdout_infected_fraction} \n")

        if success:
            report_file.write(sft.format_success_msg(success))

    return


def application(output_folder="output", insetchart_name="InsetChart.json",
                config_filename="config.json", stdout_filename="test.txt",
                report_name=sft.sft_output_filename,
                debug=False):
    sft.wait_for_done()
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "insetchart_name: " + insetchart_name + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )

    param_obj = load_emod_parameters(config_filename)

    start_time = param_obj[KEY_START_TIME_STEP]

    stdout_days = parse_stdout_file(start_time, stdout_filename, debug)
    insetchart_days = parse_json_report(start_time, output_folder, insetchart_name, debug)
    create_report_file(insetchart_days, stdout_days, debug)

    return


if __name__ == "__main__":
    # execute only if run as a script
    application()
