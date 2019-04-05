#!/usr/bin/python
# This SFT test the following statements:
# infected individuals shed into route contact, the total shedding into route contact per time step is the sum of
# each individual shedding(contact). Individuals get exposed by route contact with cp = cp(contact) from the end of
# last time step + total shedding(contact) from current time step/stats population
# the contact cp decays, Ct2 = 0
# validate InsetChart "Contact Contagion Population" channel against log_valid logging

if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../shared_embedded_py_scripts').resolve().absolute()) )

import json
import math
import dtk_test.dtk_sft as sft
import numpy as np


def application(report_file, debug = False):
    sft.wait_for_done()
    # print( "Post-processing: " + report_file )

    cdj = json.loads(open("config.json").read())["parameters"]
    start_time = cdj["Start_Time"]

    isj = json.loads(open("output/InsetChart.json").read())["Channels"]
    ccp = isj["Contact Contagion Population"]["Data"]

    timestep = start_time
    lines = []
    cum_all = []
    cum = 0
    Statpop = []
    adding_cp_log = []
    adding_cp = 0
    with open("test.txt") as logfile:
        for line in logfile:
            # collect all lines of
            if "Update(): Time:" in line:
                # calculate time step
                timestep += 1
                # lines.append(line)
                # append the accumulated shedding and reset the counter at the end of each time step.
                # data for timestep 1 is stored in cum_all[1]
                cum_all.append(cum)
                cum = 0
                adding_cp_log.append(adding_cp)
                pop = float(sft.get_val("StatPop: ", line))
                Statpop.append(pop)
            elif "[MultiRouteTransmissionGroups] Adding " in line and "route:0" in line:
                # append time step and total shedding line to lists
                # line = "TimeStep: " + str(timestep) + " " + line
                # lines.append(line)
                adding_cp = float(sft.get_val("Adding ", line))
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            elif "Exposing " in line and "route 'contact'" in line:
                # append time step and Exposing line to lists
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
                # get shedding of contact route and add it to accumulated shedding
            elif "depositing" in line and "route contact" in line:
                shedding = float(sft.get_val("depositing ", line))
                cum += shedding
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            # elif "scaled by " in line and "route:0" in line:
            #     # get population
            #     pop = float(sft.get_val("scaled by ", line))
            #     Statpop.append(pop)
    with open("DEBUG_filtered_from_StdOut.txt", "w") as file:
        file.write("".join(lines))

    success = True
    observed_cp = []
    expected_cp_log = []
    with open(sft.sft_output_filename, "w") as report_file:
        if len(lines) == 0:
            success = False
            report_file.write("Found no data matching test case.\n")
        else:
            fContact_dict = {}
            for line in lines:
                if "Exposing" in line:
                    timestep = int(sft.get_val("TimeStep: ", line))
                    if timestep not in fContact_dict:
                        fContact = float(sft.get_val("fContact=", line))
                        expected_cp = cum_all[timestep - start_time] / Statpop[timestep - start_time - 1]
                        fContact_dict[timestep] = [fContact, expected_cp]
                        if math.fabs(fContact-expected_cp) > 1e-2:
                            success = False
                            ind_id = int(sft.get_val("inividual ", line))
                            report_file.write("BAD:  at time {0}, individuals are exposed on route contact with contagion population "
                                              "= {1} StatPop = {2}, expected {3}."
                                              "\n".format(timestep, fContact, Statpop[timestep - start_time - 1], expected_cp))

            for x in range(0, len(cum_all) - 1):
                # comment out these code since we are not reporting the sccumulated sheeding in the logging anymore.
                # # report_file.write("timestep = {0}, StatPop = {1}, cum_all= {2}, adding_cp_log =
                # # {3}.\n".format(x, Statpop[x+1] ,cum_all[x], adding_cp_log[x]))
                # if math.fabs(cum_all[x] - adding_cp_log[x]) > 1e-2:
                #     success = False
                #     report_file.write(
                #         "BAD: At time {0}, the accumulated shedding is {1} from log, expected {2}."
                #         "\n".format(x, adding_cp_log[x], cum_all[x]))

                # the InsetChart is missing the first time step, so we're taking it out from the log. #2890
                # expected_cp = cum_all[x + 1] / Statpop[x]
                expected_cp = cum_all[x] / Statpop[x]

                # y = ccp[x-1] * Statpop[x - 1]
                # observed_cp.append(y)
                expected_cp_log.append(expected_cp)
                if math.fabs(expected_cp - ccp[x]) > 1e-2:
                    success = False
                    report_file.write(
                        "BAD: At time {0}, the accumulated shedding is {1} from InsetChart.json, expected {2}."
                        "\n".format(x, ccp[x], expected_cp))

            report_file.write(sft.format_success_msg(success))

    sft.plot_data(ccp, expected_cp_log, label1="InsetChart Contact Contagion Population",
                  label2="shedding data", title="Contact Contagion Population Data\nInsetChart vs. shedding",
                  xlabel="Time",
                  ylabel="Contagion",
                  category='contact_contagion_InsetChart_vs_shedding_logs',
                  alpha=0.5, overlap=True)

    sft.plot_data(np.array(list(fContact_dict.values()))[:, 0], np.array(list(fContact_dict.values()))[:, 1],
                  label1="Contact Exposure/Contagion",
                  label2="sum of shedding", title="Contact Contagion Population Data\nfContact vs. sheeding",
                  xlabel="Time",
                  ylabel="Contagion",
                  category='contact_contagion_fContact_vs_shedding_logs',
                  alpha=0.5, overlap=True)


if __name__ == "__main__":
    # execute only if run as a script
    application("")
