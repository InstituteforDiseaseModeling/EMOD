#!/usr/bin/python
# This SFT test the following statement:
# At each time step, the # of new Acute infections in log (- exit acute at the same time step)matches the data in
# "Number of New Acute Infections" channel in InsetChart.json
# note:"Enable_Vital_Dynamics": 0

import json
import dtk_test.dtk_sft as sft


def application(do_not_use):
    report_file = ""
    debug = True
    if not report_file:
        report_file = "test.txt"
    sft.wait_for_done()

    isj = json.loads(open("output/InsetChart.json").read())["Channels"]
    new_infections_contact = isj["New Infections By Route (CONTACT)"]["Data"]

    time_regex = "Time:"
    regex0 = "for individual 123"
    regex1 = "current_dose_attenuation_contact"

    success = True
    contact_attenuation = []
    filtered_lines = []
    time = 0
    latest_time = 0
    with open(report_file) as logfile:
        for line in logfile:
            if time_regex in line:
                time += 1
            if regex0 in line and regex1 in line:
                if time > latest_time:
                    contact_attenuation.append(float(line.split()[8].rstrip(',')))
                    latest_time = time
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

    with open(sft.sft_output_filename, "w") as report_file:
        if not contact_attenuation:
            # make sure we have found the correct debug data
            success = False
            report_file.write("BAD: Couldn't find current_dose_attenuation_contact for individual 123.\n")
        # #2890
        # if len(contact_attenuation) != len(new_infections_contact):
        if len(contact_attenuation) + 1 != len(new_infections_contact):
            success = False
            err_template = "BAD: Length difference between Contact attenuation: {0} new_infections_contact: {1}\n"
            report_file.write(err_template.format(
                                  len(contact_attenuation),
                                  len(new_infections_contact)))
        else:
            for i in range(len(contact_attenuation)):
                # when attenuation is 0, contagion should be at 0
                if (contact_attenuation[i] == 0 and new_infections_contact[i + 1] != 0) or \
                        (contact_attenuation[i] !=0 and new_infections_contact[i + 1] ==0):
                    success = False
                    report_file.write("BAD: At time {0}: the current dose attenuation contact is {1} from Stdout, "
                                      "while the new infections via contact are {2} from InsetChart.json.\n".format(
                                          i+1, contact_attenuation[i], new_infections_contact[i + 1]))

        report_file.write(sft.format_success_msg(success))

        # mapping all the large contagion #s to 1s for better viewing of attenuation data
        for i in range(0, len(new_infections_contact)):
            if new_infections_contact[i] > 1:
                new_infections_contact[i] = 1

        # plotting more interesting subsection
        sft.plot_data(contact_attenuation, new_infections_contact[1:],
                      label1="Attenuation",
                      label2="New Infections_Contact",
                      title="Linear Dose Contact (when attenuation 0, new infections is 0)", xlabel="Time",
                      ylabel="units",
                      category='vaccine_linear_dose_contact', overlap=False, alpha=0.5)


if __name__ == "__main__":
    # execute only if run as a script
    application("")
