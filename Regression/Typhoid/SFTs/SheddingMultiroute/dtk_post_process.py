#!/usr/bin/python

import re
import json
import dtk_test.dtk_sft as sft


def application(report_file):
    sft.wait_for_done()
    # print( "Post-processing: " + report_file )
    cdj = json.loads(open("config.json").read())["parameters"]
    start_time = cdj["Start_Time"]
    timestep = start_time
    lines_contact = []
    lines_environment = []
    contact_shedding_count = 0
    contact_shedding_count_per_day = []
    env_shedding_count = 0
    env_shedding_count_per_day = []
    with open("test.txt") as logfile:
        for line in logfile:
            # collect all lines of
            if re.search("Update\(\): Time:", line):
                # calculate time step
                timestep += 1
                contact_shedding_count_per_day.append(contact_shedding_count)
                env_shedding_count_per_day.append(env_shedding_count)
                contact_shedding_count = env_shedding_count = 0 # reset counter
            elif re.search("depositing", line):
                # append time step and depositing line to lists
                ind_id = sft.get_val("Individual ", line)
                line = "TimeStep: " + str(timestep) + " ind_id: " + str(ind_id) + " " + line
                if re.search("contact", line):
                    lines_contact.append(line)
                    contact_shedding_count += 1
                elif re.search("environment", line):
                    lines_environment.append(line)
                    env_shedding_count += 1
    success = True
    with open(sft.sft_output_filename, "w") as report_file:
        if contact_shedding_count_per_day != env_shedding_count_per_day:
            success = False
            report_file.write("BAD: the # od individual shedding to contact and environment pool doesn't matches")
        sft.plot_data(contact_shedding_count_per_day, env_shedding_count_per_day,
                      label1="Contact",
                      label2="Environmental",
                      title="Multi-route Shedding(count of individual shedding per day)", xlabel="Time",
                      ylabel="# of individuals shedding into pool",
                      category='shedding_multiroute_count',
                      overlap=True, alpha=0.5)
        if not lines_contact or not lines_environment:
            success = False
            report_file.write("BAD: Found no data matching test case.\n")
        else:
            contact_shedding_per_day = []
            env_shedding_per_day = []
            for line in lines_contact:
                infectiousness2 = None
                ind_id = int(sft.get_val("ind_id: ", line))
                timestep = sft.get_val("TimeStep: ", line)
                infectiousness1 = float(sft.get_val("depositing ", line))
                for l in lines_environment:
                    if "ind_id: " + str(ind_id) + " " in l and \
                            "TimeStep: " + str(timestep) + " " in l:
                        infectiousness2 = float(sft.get_val("depositing ", l))
                        if infectiousness1 != infectiousness2:
                            success = False
                            report_file.write("BAD: Individual {0} is depositing {1} to route {2} and {3} to route {4} "
                                              "at time {5}.\n".format(ind_id, infectiousness1, "contact",
                                                                      infectiousness2, "environment", timestep))
                        # for plotting
                        if ind_id == 1:
                            contact_shedding_per_day.append(round(infectiousness1))
                            env_shedding_per_day.append(round(infectiousness2))
                        lines_environment.remove(l)
                        break
                if infectiousness2 is None:
                    success = False
                    report_file.write("BAD: Individual {0} is not depositing to route environment at time {1}."
                                      "\n".format(ind_id, timestep))

            sft.plot_data(contact_shedding_per_day, env_shedding_per_day,label1="Contact",
                      label2="Environmental",
                      title="Multi-route Shedding(Infectiousness of individual 1)", xlabel="Time",
                      ylabel="Infectiousness",
                      category='shedding_multiroute',
                      overlap=True, alpha=0.5)

            if lines_environment:
                success = False
                report_file.write("BAD: {0} Individuals are not depositing to route contact while they are depositing "
                                  "to route contact.\n".format(len(lines_environment), timestep))
                for l in lines_environment:
                    report_file.write(l)

        report_file.write(sft.format_success_msg(success))


if __name__ == "__main__":
    # execute only if run as a script
    application("")
