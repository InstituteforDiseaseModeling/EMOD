#!/usr/bin/python
# This SFT test the following statements:
# An infected individual can become a chronic carrier after both acute and sub-clinical stages.

import re
import json
import dtk_test.dtk_sft as sft


def application(report_file):
    sft.wait_for_done()
    # print( "Post-processing: " + report_file )

    cdj = json.loads(open("config.json").read())["parameters"]
    start_time = cdj["Start_Time"]
    timestep = start_time
    lines = []
    count_chronic = 0
    count_recovered = 0
    count_chronic_daily = []
    count_recovered_daily = []
    with open(sft.sft_test_filename) as logfile:
        for line in logfile:
            if re.search("Update\(\): Time:", line):
                # calculate time step
                timestep += 1
                count_chronic_daily.append(count_chronic)
                count_recovered_daily.append(count_recovered)
                count_chronic = 0
                count_recovered = 0
            if re.search("just went chronic", line):
                # append time step and all Infection stage transition to list
                line = "TimeStep: "+str(timestep) + " " + line
                lines.append(line)
                count_chronic += 1
            if re.search("just recovered", line):
                # append time step and all Infection stage transition to list
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
                count_recovered += 1

    success = True
    with open(sft.sft_output_filename, "w") as report_file:
        if len(lines) == 0:
            success = False
            report_file.write("Found no data matching test case.\n")
        else:
            for line in lines:
                age = float(sft.get_val(" age ", line))
                sex = "female" if (re.search("sex 1", line) or re.search("sex Female", line)) else "male"
                previous_infection_stage = sft.get_char("from ", line)
                if sum(count_chronic_daily) == 0:
                    success = False
                    report_file.write("Found no Chronic case in data.\n")
                if sum(count_recovered_daily) == 0:
                    success = False
                    report_file.write("Found no Recovered case in data.\n")
                if (not re.search("from subclinical", line)) and (not re.search("from acute", line)) and \
                        (not re.search("from Subclinical", line)) and (not re.search("from Acute", line)) and \
                        (not re.search("from SubClinical", line)):
                    success = False
                    if re.search("just went chronic", line):
                        report_file.write("BAD: individual age {0}, sex {1} went to Chronic state from {2} state, "
                                          "expected Acute state or SubClinical state."
                                          "\n".format(age, sex, previous_infection_stage))
                    else:
                        ind_id = int(sft.get_val("Individual ", line))
                        report_file.write("BAD: individual {0} age {1}, sex {2} went to Susceptible state from {3}"
                                          " state, expected Acute state or SubClinical state."
                                          "\n".format(ind_id, age, sex, previous_infection_stage))
        if success:
            report_file.write(sft.format_success_msg(success))

    # not much to graph, we're checking if individuals who go chronic or recover do so from the correct state
    # no "theoretical" data as such. Currently plotting # of people who recovered and who went chronic daily
    sft.plot_data_sorted(count_chronic_daily, count_recovered_daily, label1="Count Chronic Daily",
                  label2="Count Recovered Daily",
                  title="Daily counts of Chronic and Recovered",
                  xlabel="Time (days)", ylabel="Number of occurrences", category='acu_subc_to_chr_sus_transition')

if __name__ == "__main__":
    # execute only if run as a script
    application("")
