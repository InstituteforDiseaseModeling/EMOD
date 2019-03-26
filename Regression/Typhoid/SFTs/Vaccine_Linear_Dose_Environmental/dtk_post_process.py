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
    debug = True
    if not report_file:
        report_file = "test.txt"
    sft.wait_for_done()
    # pdb.set_trace()
    # print( "Post-processing: " + report_file )

    isj = json.loads(open("output/InsetChart.json").read())["Channels"]
    chan_title = "New Infections By Route (ENVIRONMENT)"
    env_new_infections = isj[chan_title]["Data"]

    regex0 = "Risk:Target_Demographic"
    regex1 = "Set current_dose_attenuation_environment"

    success = False
    start_time = datetime.datetime.now()
    max_time = datetime.timedelta(seconds=600)
    elapsed_time = datetime.timedelta(seconds=0)
    while not (success or elapsed_time > max_time):
        env_attenuation = []
        filtered_lines = []
        with open(report_file) as logfile:
            for line in logfile:
                if regex0 in line and regex1 in line:
                    env_attenuation.append(float(line.split()[8].rstrip(',')))
                    if debug:
                        filtered_lines.append(line)
        if len(env_attenuation) + 1 >= len(env_new_infections):
            success = True
        else:
            time.sleep(10)
            elapsed_time = datetime.datetime.now() - start_time

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
        if not env_attenuation:
            # make sure we have found the correct debug data
            success = False
            report_file.write("BAD: Couldn't find current_dose_attenuation_environment.\n")
        if len(env_attenuation) + 1 != len(env_new_infections):
            success = False
            err_template = "BAD: Length difference between Env attenuation: {0} env_new_infections: {1}\n"
            report_file.write(err_template.format(
                                  len(env_attenuation),
                                  len(env_new_infections)))
        else:
            for i in range(0, len(env_new_infections)-1):
                # when attenuation is 0, new infections should be at 0.
                if env_attenuation[i] == 0 and env_new_infections[i + 1] != 0:
                    success = False
                    err_template = "BAD: At time {0}: the {1} is {2} from Stdout, " + \
                                   "while the new infections by route is {3} from InsetChart.json.\n"
                    report_file.write(err_template.format(
                        i + 1,
                        regex1,
                        env_attenuation[i],
                        env_new_infections[i + 1]))

        report_file.write(sft.format_success_msg(success))

        # mapping all the large contagion #s to 1s for better viewing of attenuation data
        for i in range(0, len(env_new_infections)):
            if env_new_infections[i] > 1:
                env_new_infections[i] = 1

        # plotting more interesting subsection
        sft.plot_data_sorted(env_attenuation[0:1200], env_new_infections[1:1201],
                      label1="Attenuation",
                      label2="New Infections_Env",
                      title="Vaccine Linear Dose Environmental\n(when attenuation is 0, new infections should be 0)",
                      xlabel="Time",
                      ylabel="",
                      category='vaccine_linear_dose_environmental', overlap=False, alpha=0.5)


if __name__ == "__main__":
    # execute only if run as a script
    application("")
