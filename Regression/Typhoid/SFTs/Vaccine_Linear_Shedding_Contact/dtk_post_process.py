#!/usr/bin/python
# This SFT test the following statement:
# At each time step, the # of new Acute infections in log (- exit acute at the same time step)matches the data in
# "Number of New Acute Infections" channel in InsetChart.json
# note:"Enable_Vital_Dynamics": 0

import json
import datetime
import time
import dtk_test.dtk_sft as sft


def application(do_not_use):
    report_file = ""
    debug = False
    if not report_file:
        report_file = "test.txt"
    sft.wait_for_done()
    # pdb.set_trace()
    # print( "Post-processing: " + report_file )

    isj = json.loads(open("output/InsetChart.json").read())["Channels"]
    contact_contagion_population = isj["Contact Contagion Population"]["Data"]

    regex0 = "for individual 12"
    regex1 = "current_shedding_attenuation_contact"

    # success = False
    # start_time = datetime.datetime.now()
    # max_time = datetime.timedelta(seconds=600)
    # elapsed_time = datetime.timedelta(seconds=0)
    # while not (success or elapsed_time > max_time):
    #     contact_attenuation = []
    #     filtered_lines = []
    #     with open(report_file) as logfile:
    #         for line in logfile:
    #             if regex0 in line and regex1 in line:
    #                 contact_attenuation.append(float(line.split()[8].rstrip(',')))
    #                 if debug:
    #                     filtered_lines.append(line)
    #     if len(contact_attenuation) + 1 >= len(contact_contagion_population):
    #         success = True
    #     else:
    #         time.sleep(10)
    #         elapsed_time = datetime.datetime.now() - start_time

    success = True
    contact_attenuation = []
    filtered_lines = []
    with open(report_file) as logfile:
        for line in logfile:
            if regex0 in line and regex1 in line:
                contact_attenuation.append(float(line.split()[8].rstrip(',')))
                if debug:
                    filtered_lines.append(line)

    if debug:
        with open("filtered_lines.txt", "w") as outfile:
            outfile.write("First filtered line split: \n")
            splits = filtered_lines[0].split()
            for i in range(0, len(splits)):
                outfile.write("\t " + str(i) + " " + splits[i] + " \n")
            outfile.write("Filtered Lines: \n")
            for line in filtered_lines:
                outfile.write(line)
    success = True
    with open(sft.sft_output_filename, "w") as report_file:
        report_file.write("{0}, {1}\n".format(len(contact_attenuation), len(contact_contagion_population)))
        if len(contact_attenuation) == 0:
            # make sure we have found the correct debug data
            success = False
            report_file.write("Couldn't find current_shedding_attenuation_contact for individual 123.\n")
        else:
            if len(contact_attenuation) + 1 != len(contact_contagion_population):
                err_template = "WARNING: Length difference between Contact attenuation: {0} Contact_Contagion_Population: {1}\n"
                report_file.write(err_template.format(
                                      len(contact_attenuation),
                                      len(contact_contagion_population)))
            for i in range(0, len(contact_contagion_population)-2):
                # when attenuation is 0, contact contagion should be 0
                if contact_attenuation[i] == 0 and contact_contagion_population[i + 2] != 0:
                    success = False
                    err_template = "BAD: At time {0}: the current shedding attenuation contact is {1} from " +\
                                   "Stdout, while the contagion is {2} from InsetChart.json.\n"
                    report_file.write(err_template.format(i + 1,
                                                          contact_attenuation[i],
                                                          contact_contagion_population[i + 2]))

        report_file.write(sft.format_success_msg(success))

        # mapping all the large contagion #s to 1s for better viewing of attenuation data
        for i in range(0, len(contact_contagion_population)):
            if contact_contagion_population[i] > 1:
                contact_contagion_population[i] = 1

        # showing a subsection
        sft.plot_data(contact_attenuation[0:1200], contact_contagion_population[2:1202],
                      label1="Attenuation",
                      label2="Contagion_Contact",
                      title="Vaccine Linear Shedding Contact\n(when attenuation 0, contagion is 0)", xlabel="Time",
                      ylabel="",
                      category='vaccine_linear_shedding_contact', overlap=False, alpha=0.5)

if __name__ == "__main__":
    # execute only if run as a script
    application("")
