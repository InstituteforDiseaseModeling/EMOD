#!/usr/bin/python
# This SFT test the following statements:
# infected individuals shed into route environment, the total shedding into route environment per time step is
# the sum of each individual shedding(enviro) individuals get exposed by the environment with cp = cp(enviro)
# from the end of last time step + total shedding(enviro) from current time step/stats population
# the environment decays, Ct2 = Ct1 * math.exp(-param:NCDR)
# validate InsetChart "Environmental Contagion Population" channel against log_valid logging

if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../shared_embedded_py_scripts').resolve().absolute()) )

import re
import json
import math
import dtk_test.dtk_sft as sft
import numpy as np


def application(report_file):
    sft.wait_for_done()
    # print( "Post-processing: " + report_file )
    cdj = json.loads(open("config.json").read())["parameters"]
    ncdr = cdj["Node_Contagion_Decay_Rate"]
    start_time = cdj["Start_Time"]
    simulation_duration = int(cdj["Simulation_Duration"])
    isj = json.loads(open("output/InsetChart.json").read())["Channels"]
    environmental_contagion_population = isj["Environmental Contagion Population"]["Data"]

    timestep = start_time
    lines = []
    cum_all = []
    cum = 0
    Statpop = []
    adding_cp_log = []
    adding_cp = 0
    shedding = 0
    with open("test.txt") as logfile:
        for line in logfile:
            if "Update(): Time:" in line:
                # calculate time step
                timestep += 1
                pop = float(sft.get_val("StatPop: ", line))
                Statpop.append(pop)

                # append the accumulated shedding and reset the counter at the end of each time step.
                # data for timestep 1 is stored in cum_all[1]
                cum_all.append(cum)
                # environmental cp decay
                cum *= 1.0 - ncdr
                adding_cp_log.append(adding_cp)
                # resetting shedding and adding_cp variables
                shedding = 0
                adding_cp = 0
            elif "[MultiRouteTransmissionGroups] Adding " in line and "route:1" in line:
                # append time step and total shedding line to lists
                # line = "TimeStep: " + str(timestep) + " " + line
                # lines.append(line)
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
                adding_cp = float(sft.get_val("Adding ",line))
            elif "Exposing " in line and "route 'environment'" in line:
                # append time step and Exposing line to lists
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            elif  "depositing" in line and "route environment" in line:
                # get shedding of contact route and add it to accumulated shedding
                shedding = float(sft.get_val("depositing ", line))
                cum += shedding
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            # elif "scaled by " in line and "route:1" in line:
            #     # get population
            #     if timestep > 0:
            #         # start collecting population at time 1
            #         pop = float(sft.get_val("scaled by ", line))
            #         Statpop.append(pop)
    with open("DEBUG_filtered_from_StdOut.txt", "w") as file:
        file.write("".join(lines))
    expected_cp_insetchart = []
    cp_stdout_dict = {}
    success = True
    with open(sft.sft_output_filename, "w") as report_file:
        if len(lines) == 0:
            success = False
            report_file.write("Found no data matching test case.\n")
        else:
            if len(cum_all) != simulation_duration or len(Statpop) != simulation_duration :
                report_file.write("WARNING: the test.txt file is incomplete. len(cum_all)={0},  len(Statpop)={1},"
                                  " expected {2}.\n".format(len(cum_all), len(Statpop), simulation_duration))
            for line in lines:
                if re.search("Exposing", line):
                    environment = float(sft.get_val("environment=", line))
                    timestep = int(sft.get_val("TimeStep: ", line))
                    expected_cp = cum_all[timestep-start_time] / Statpop[timestep -start_time - 1]
                    if timestep not in cp_stdout_dict:
                        cp_stdout_dict[timestep] = [environment, expected_cp]
                        if math.fabs(environment-expected_cp) > 1e-2 * expected_cp:
                            success = False
                            ind_id = int(sft.get_val("inividual ", line))
                            report_file.write("BAD: at time {0}, individuals are exposed on route environment with contagion "
                                              "population = {1} StatPop = {2}, expected {3}."
                                              "\n".format(timestep, environment, Statpop[timestep - start_time - 1],
                                                          expected_cp))
            pre_cum = 0
            for timestep in range(len(cum_all) - 1):
                # report_file.write("timestep={0}, StatPop = {1}, cum_all= {2}, adding_cp_log = {3}.
                # \n".format(x,Statpop[x+1],cum_all[x], adding_cp_log[x]))

                # the InsetChart is missing the first time step, so we're taking it out from the log. #2890
                # cur_cum = cum_all[timestep + 1]
                cur_cum = cum_all[timestep]

                # comment out these code since we are no longer logging the accumulated shedding in StdOut file.
                # adding_cp = cur_cum - pre_cum * (1.0 - ncdr)
                # # pass cur_cum to pre_cum for next time step
                # pre_cum = cur_cum
                # if math.fabs(adding_cp - adding_cp_log[x]) > 1e-1:
                #     success = False
                #     report_file.write(
                #         "BAD: At time {0}, the accumulated shedding is {1} from StdOut, expected {2}."
                #         "\n".format(timestep, adding_cp_log[x], adding_cp))

                expected_cp = cur_cum / Statpop[timestep]
                expected_cp_insetchart.append(expected_cp)
                if math.fabs(expected_cp - environmental_contagion_population[timestep]) > 1e-2 * expected_cp:
                    success = False
                    report_file.write(
                        "BAD: At time {0}, the environmental contagion population is {1} from InsetChart.json, "
                        "expected {2}.\n".format(timestep, environmental_contagion_population[timestep], expected_cp))


        report_file.write(sft.format_success_msg(success))

        sft.plot_data(environmental_contagion_population, expected_cp_insetchart,
                      label1="InsetChart",
                      label2="expected data",
                      title="Environmental Contagion Population\n InsetChart vs. expected",
                      xlabel="Day",
                      ylabel="Environmental Contagion Population",
                      category='environmental_contagion_population_InsetChart',
                      alpha=0.5, overlap=True)

        sft.plot_data(np.array(list(cp_stdout_dict.values()))[:, 0], np.array(list(cp_stdout_dict.values()))[:, 1],
                      label1="StdOut",
                      label2="expected data",
                      title="Exposure(Contagion)\n StdOut vs. expected",
                      xlabel="Day",
                      ylabel="Contagion",
                      category='exposure_StdOut',
                      alpha=0.5, overlap=True)

        cp_list_for_plot = []
        for key in sorted(cp_stdout_dict):
            cp_list_for_plot.append([cp_stdout_dict[key][0],environmental_contagion_population[key-1]])
        sft.plot_data(np.array(cp_list_for_plot)[:, 1][1:], np.array(cp_list_for_plot)[:, 0],
                      label1="InsetChart",
                      label2="StdOut",
                      title="Environmental Contagion Population\n InsetChart vs. StdOut",
                      xlabel="Day",
                      ylabel="Contagion",
                      category='environmental_contagion_population',
                      alpha=0.5, overlap=True)
if __name__ == "__main__":
    # execute only if run as a script
    application("")
