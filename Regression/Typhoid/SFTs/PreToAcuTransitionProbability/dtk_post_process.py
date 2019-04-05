#!/usr/bin/python
# This SFT test the following statements:
# All infections begin in prepatent.
# The proportion of individuals who move to acute infections is determined by the config parameter Config:TSF.
# The remainder shall move to subclinical.
# All new acute cases and subclinical cases are transited from prepatent state only.
# This test passes when the number of transmission from prepatent to Acute cases is within binomial
# 95% confidence interval for each test, there is 5% of chance that we will reject the hypothesis while it's true

import re
import json
import math
import dtk_test.dtk_sft as sft
import matplotlib.pyplot as plt
import numpy as np


def get_char_before(key, line):
    regex = "(\w*\d*\w*\d*)" + key
    match = re.search(regex, line)
    if match:
        return match.group(1)
    else:
        raise LookupError


def get_char_after(key, line):
    regex = key + "(\w*\d*\w*\d*)"
    match = re.search(regex, line)
    if match:
        return match.group(1)
    else:
        raise LookupError


def application(report_file):
    sft.wait_for_done()
    # print( "Post-processing: " + report_file )

    cdj = json.loads(open("config.json").read())["parameters"]
    tsf = cdj["Typhoid_Symptomatic_Fraction"]
    start_time = cdj["Start_Time"]
    lines = []
    timestep = start_time
    with open("test.txt") as logfile:
        for line in logfile:
            if "Update(): Time:" in line:
                # alculate time step
                timestep += 1
            elif "Infection stage transition" in line:
                # append time step and all Infection stage transition to list
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)

    success = True
    with open(sft.sft_output_filename, "w") as report_file:
        if not lines:
            success = False
            report_file.write("BAD: Found no data matching test case.\n")
        else:
            subcount = 0
            acutecount = 0
            for line in lines:
                if ((not re.search("->SubClinical:", line) and not re.search("->Acute:", line)) and
                        re.search(", Prepatent->", line)) or \
                                (re.search("->SubClinical:", line) or re.search("->Acute:", line)) \
                        and not re.search(", Prepatent->", line):
                    success = False
                    ind_id = int(sft.get_val("Individual=", line))
                    current_infection_stage = get_char_after("->", line)
                    previous_infection_stage = get_char_before("->", line)
                    report_file.write(
                        "BAD: individuals {0} went to {1} state from {2} state, expected Prepatent->Acute "
                        "or Prepatent->SubClinical.\n".format(ind_id, current_infection_stage,
                                                              previous_infection_stage))
                elif ", Prepatent->Acute:" in line:
                    # count # of cases: from Prepatent to Acute
                    acutecount += 1
                elif ", Prepatent->SubClinical:" in line:
                    # count # of cases: from Prepatent to Subclinical
                    subcount += 1
            actual_tsf = acutecount / float(subcount + acutecount)
            if subcount + acutecount == 0:
                success = False
                report_file.write("Bad:Found no individual exits Prepatent state in log.\n")
            else:
                # for cases that binomial confidence interval will not work
                if tsf < 1e-2 or tsf > 0.99:
                    actual_tsf = acutecount / float(subcount + acutecount)
                    if math.fabs(actual_tsf - tsf) > 5e-2:
                        success = False
                        report_file.write("BAD: Proportion of prepatent cases that become acute vs. subclinical is {0} "
                                          "instead of {1}. Actual Acute case = {2} vs. Actual SubClinical case = {3}."
                                          "\n".format(actual_tsf, tsf, acutecount, subcount))
                # use binomial 95% confidence interval
                elif not sft.test_binomial_95ci(acutecount, subcount + acutecount, tsf, report_file,
                                                'PrePatent to Acute transition'):
                    success = False

        report_file.write("acute_count = {0} and subc_count = {1}.\n".format(acutecount, subcount))
        report_file.write(sft.format_success_msg(success))

        # for spec
        fig = plt.figure()
        plt.bar(np.arange(2),height=[acutecount, subcount], width=0.5, align='center',alpha=0.5)
        plt.title("Typhoid Symptomatic Fraction = {}".format(tsf))
        plt.xticks(np.arange(2), ['Prepatent->Acute','Prepatent->SubClinical'])
        for a,b in zip(np.arange(2),[acutecount, subcount]):
            plt.text(a, b, str(b), horizontalalignment='center', verticalalignment='top')#, bbox=dict(facecolor='red', alpha=0.1))
        plt.ylabel("Occurrence")
        if sft.check_for_plotting():
            plt.show()
        fig.savefig("typhoid_symptomatic_fraction.png")
        # sft.plot_data(actual_tsf, tsf, label1="Actual ({})".format(round(actual_tsf, 3)),
        #              label2="Expected ({})".format(tsf),
        #              title="Typhoid Symptomatic Fraction", xlabel="Occurrence",
        #              ylabel="Fraction",
        #              category='typhoid_symptomatic_fraction_plot')

if __name__ == "__main__":
    # execute only if run as a script
    application("")
