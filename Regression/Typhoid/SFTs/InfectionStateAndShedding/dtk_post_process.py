#!/usr/bin/python
# This SFT test the following statements:
# All and only individuals in infection state are depositing to route contact and environment at the same time step

import re
import json
import dtk_test.dtk_sft as sft


def application(report_file, debug=False):
    sft.wait_for_done()
    # print( "Post-processing: " + report_file )

    # get params from config.json
    cdj = json.loads(open("config.json").read())["parameters"]
    start_time = cdj["Start_Time"]
    simulation_duration = cdj["Simulation_Duration"]
    lines = []
    timestep = start_time

    with open("test.txt") as logfile:
        for line in logfile:
            if re.search("Update\(\): Time:", line):
                # calculate time step
                timestep += 1
            if re.search("depositing", line):
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            if re.search("AcquireNewInfection:", line):
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            if re.search("state_to_report", line):
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)

    if debug:
        with open("filtered_lines.txt", "w") as report_file:
            for line in lines:
                report_file.write(line)

    success = True
    dict_depositing = {}
    dict_infection = {}
    with open(sft.sft_output_filename, "w") as report_file:
        if not lines:
            success = False
            report_file.write("Found no data matching test case.\n")
        else:
            for i in range(0, len(lines)):
                timestep = int(sft.get_val("TimeStep: ", lines[i]))
                if re.search("depositing", lines[i]):
                    ind_id = int(sft.get_val("Individual ", lines[i]))
                    state = sft.get_char("in state ", lines[i])
                    route = sft.get_char("to route ", lines[i])
                    key = "Time  " + str(timestep) + " individual " + str(ind_id) + " route " + str(route)
                    if key in dict_depositing:
                        success = False
                        report_file.write("BAD: At time {0} individual {1} is depositing to route {2} more than once."
                                          "\n".format(timestep, ind_id, route))
                    else:
                        dict_depositing[key] = state
                elif re.search("AcquireNewInfection:", lines[i]):
                    ind_id = sft.get_val("individual=", lines[i])
                    state = "PRE"
                    if not re.search("route=0", lines[i]):
                        # if the infection is caused by contact or environment route which happen after the shedding
                        # the state is use for next step
                        timestep = int(timestep) + 1
                    key = "Time  " + str(timestep) + " individual " + str(ind_id)
                    if timestep != start_time + simulation_duration:
                        # the last time step has no shedding information
                        if key in dict_infection:
                            if state != dict_infection.get(key):
                                success = False
                                report_file.write("BAD: At time {0} individual {1} is reported to be in state {2} "
                                                  "expected {3}\n".format(timestep, ind_id, state, dict_infection.get(key)))
                        else:
                            dict_infection[key] = state
                elif re.search("state_to_report", lines[i]):
                    ind_id = int(sft.get_val("individual ", lines[i]))
                    state = sft.get_char("= ", lines[i])
                    if state == 'SUS':
                        # skip for susceptible state
                        continue
                    # this state is use for next time step
                    timestep = int(timestep) + 1
                    key = "Time  " + str(timestep) + " individual " + str(ind_id)
                    if timestep != start_time + simulation_duration:
                        # the last time step has no shedding information
                        if key in dict_infection:
                            if state != dict_infection.get(key):
                                success = False
                                report_file.write("BAD: At time {0} individual {1} is reported to be in state {2} "
                                                  "expected {3}\n".format(timestep, ind_id, state, dict_infection.get(key)))
                        else:
                            dict_infection[key] = state

        if debug:
            with open("dict_d.txt", "w") as report_file:
                for key, value in dict_depositing.iteritems():
                    report_file.write("{0}: {1}\n".format(key, value))
            with open("dict_i.txt", "w") as report_file:
                for key, value in dict_infection.iteritems():
                    report_file.write("{0}: {1}\n".format(key, value))

            for key in dict_infection:
                key_1 = key + " route contact"
                key_2 = key + " route environmental"
                state = dict_infection.get(key)
                if key_1 in dict_depositing:
                    depositing_state = dict_depositing.get(key_1)
                    if depositing_state != state:
                        success = False
                        report_file.write("BAD: {0} is depositing in state {1}, expected {2}."
                                          "\n".format(key_1, depositing_state, state))
                    dict_depositing.pop(key_1, None)
                else:
                    success = False
                    report_file.write("BAD: {0} is in infection state {1} but it's not depositing in route contact."
                                      "\n".format(key, state))
                if key_2 in dict_depositing:
                    depositing_state = dict_depositing.get(key_2)
                    if depositing_state != state:
                        success = False
                        report_file.write("BAD: {0} is depositing in state {1}, expected {2}."
                                          "\n".format(key_2, depositing_state, state))
                    dict_depositing.pop(key_2, None)
                else:
                    success = False
                    report_file.write("BAD: {0} is in infection state {1} but it's not depositing in route "
                                      "environment.\n".format(key, state))
            if len(dict_depositing) != 0:
                success = False
                report_file.write("BAD: Some individuals are depositing while they are not reported to be in "
                                  "Infection state. Please see \"depositing.txt\" for details\n")
                with open("depositing.txt", "w") as depositing_file:
                    for key, value in dict_depositing.iteritems():
                        depositing_file.write("{0} is depositing as state {1}.\n".format(key, value))

        report_file.write(sft.format_success_msg(success))

        # not sure what to do for graphing of this data.

if __name__ == "__main__":
    # execute only if run as a script
    application("")
