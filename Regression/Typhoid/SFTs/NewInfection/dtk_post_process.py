#!/usr/bin/python
# This SFT test the following statements:
# All new infections from route contact are in category high
# All new infections from outbreak are in category low
# At each time step, the # of new infections from route contact in log matches the data in
# "New Infections By Route (CONTACT)" channel in InsetChart.json
# At each time step, the # of new infections from route environment in log matches the data in
# "New Infections By Route (ENVIRONMENT)" channel in InsetChart.json

if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../shared_embedded_py_scripts').resolve().absolute()) )

import re
import json
import dtk_test.dtk_sft as sft


def application(report_file):
    sft.wait_for_done()

    cdj = json.loads(open("config.json").read())["parameters"]
    tsf = cdj["Typhoid_Symptomatic_Fraction"]
    start_time = cdj["Start_Time"]

    with open('output/InsetChart.json', 'r') as handle:
        icj = json.load(handle)['Channels']
    nibrc = icj["New Infections By Route (CONTACT)"]["Data"]
    nibre = icj["New Infections By Route (ENVIRONMENT)"]["Data"]
    ni = icj["New Infections"]["Data"]

    timestep = start_time
    count_new_infection = 0
    count_outbreak = 0
    count_contact = 0
    count_enviro = 0
    new_infection = []
    new_outbreak = []
    new_contact = []
    new_enviro = []
    lines = []

    with open("test.txt") as logfile:
        for num, line in enumerate(logfile):
            if "Update(): Time:" in line:
                # calculate time step and collect counters
                # clorton timestep += 1
                timestep = start_time + int(float(sft.get_val('Time: ', line)))
                new_infection.append(count_new_infection)
                new_outbreak.append(count_outbreak)
                new_contact.append(count_contact)
                new_enviro.append(count_enviro)
                # reset all counters at each time step
                count_new_infection = 0
                count_outbreak = 0
                count_contact = 0
                count_enviro = 0
            elif "Calculated prepatent duration" in line:
                count_new_infection += 1
                line = "line: " + str(num) + " TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            elif ("AcquireNewInfection:" in line) and (("route=32" in line) or ("route=0" in line)):
                count_outbreak += 1
                line = "line: " + str(num) + " TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            elif ("AcquireNewInfection:" in line) and ("route=1" in line):
                count_contact += 1
                line = "line: " + str(num) + " TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            elif ("AcquireNewInfection:" in line) and ("route=2" in line):
                count_enviro += 1
                line = "line: " + str(num) + " TimeStep: " + str(timestep) + " " + line
                lines.append(line)

    success = True
    error_log = []
    with open(sft.sft_output_filename, "w") as report_file:
        if not new_infection or not lines:
            success = False
            report_file.write("BAD: Found no data matching test case.\n")
        elif not new_contact or not new_enviro:
            success = False
            report_file.write("BAD: No contact or no environmental infections found")
        else:
            for x in range(0, len(new_infection)):
                # the InsetChart is missing the first time step, so we're taking it out from the log. #2890
                # new_infection_log = new_infection[x + 1]
                # new_outbreak_log = new_outbreak[x + 1]
                # new_contact_log = new_contact[x + 1]
                # new_enviro_log = new_enviro[x + 1]
                new_infection_log = new_infection[x]
                new_outbreak_log = new_outbreak[x]
                new_contact_log = new_contact[x]
                new_enviro_log = new_enviro[x]
                total_log = new_outbreak_log + new_enviro_log + new_contact_log
                new_infection_contact_output = nibrc[x]
                new_infection_environment_output = nibre[x]
                new_infection_output = ni[x]
                if new_infection_log != total_log:
                    success = False
                    report_file.write("BAD: At time {0}: new prepatent case = {1}, expected {2}(new infection by route "
                                      "(contact) = {3}, new infection by route (environment) = {4}, new infection by "
                                      "Outbreak = {5}).\n".format(x + start_time, new_infection_log, total_log,
                                                                  new_contact_log, new_enviro_log, new_outbreak_log))
                if new_contact_log != new_infection_contact_output:
                    success = False
                    report_file.write("BAD: At time {0}: new infection by contact route is {1} from Stdout while it's "
                                      "{2} from InsetChart.\n".format(x + start_time, new_contact_log,
                                                                        new_infection_contact_output))
                if new_enviro_log != new_infection_environment_output:
                    success = False
                    report_file.write("BAD: At time {0}: new infection by environment route is {1} from Stdout while"
                                      " it's {2} from InsetChart.\n".format(x + start_time, new_enviro_log,
                                                                              new_infection_environment_output))
                if new_infection_output != new_infection_log:
                    success = False
                    report_file.write("BAD: At time {0}: total new infection is {1} from Stdout while"
                                      " it's {2} from InsetChart.\n".format(x + start_time, new_infection_log,
                                                                              new_infection_output))

        for i in range(0, len(lines)):
            line = lines[i]
            if "AcquireNewInfection:" in line and ("route=32" in line or "route=2" in line or "route=0" in line):
                next_line = lines[i+1]
                if not re.search("doseTracking = Low", next_line):
                    error_log.append(line)
                    error_log.append(next_line)
            elif "AcquireNewInfection:" in line and "route=1" in line:
                next_line = lines[i+1]
                if not re.search("doseTracking = High", next_line):
                    error_log.append(line)
                    error_log.append(next_line)

        if error_log:
            success = False
            report_file.write("BAD: Some infected individuals didn't fall in the right doseTracking categories."
                              " Expected: route 32/0 or 2 - doseTracking = Low and route 1 - doseTracking = High. "
                              "Please see the details in \"category_error_log.txt\".\n")
            with open("category_error_log.txt", "w") as log:
                for line in error_log:
                    log.write(line)
        report_file.write(sft.format_success_msg(success))

        # the InsetChart is missing the first time step, so we're taking it out from the log. #2890
        sft.plot_data(new_contact, nibrc, label1="StdOut ",
                      label2="InsetChart", title="New Infections by Contact",
                      # xlabel="Time(skip day 0 issue 2890)",
                      xlabel="Time",
                      ylabel="Number of New Infections",
                      category='new_infections_by_contact',
                             alpha=0.5, overlap=True, sort=False)
        sft.plot_data(new_enviro, nibre, label1="StdOut ",
                      label2="InsetChart", title="New Infections by Environment",
                      #xlabel="Time(skip day 0 issue 2890)",
                      xlabel="Time",
                      ylabel="Number of New Infections",
                      category='new_infections_by_environment',
                             alpha=0.5, overlap=True, sort=False)
        sft.plot_data(new_infection, ni, label1="StdOut ",
                      label2="InsetChart", title="Total New Infections\nStdOut vs. InsetChart",
                      #xlabel="Time(skip day 0 issue 2890)",
                      xlabel="Time",
                      ylabel="Number of New Infections",
                      category='new_infections_stdout_vs_insetchart',
                      alpha=0.5, overlap=True, sort=False)

        total_new_infection_log = [sum(x) for x in zip(*[new_outbreak, new_contact, new_enviro])]
        sft.plot_data(new_infection, total_new_infection_log, label1="AcquireNewInfection",
                      label2="Contact + Enviro + Outbreak", title="New Infections from StdOut",
                      xlabel="Time",
                      ylabel="Number of New Infections",
                      category='new_infections_stdout',
                      alpha=0.5, overlap=True, sort=False)


if __name__ == "__main__":
    application("")
