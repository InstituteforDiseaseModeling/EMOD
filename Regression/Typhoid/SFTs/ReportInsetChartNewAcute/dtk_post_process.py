#!/usr/bin/python
# This SFT test the following statement:
# At each time step, the # of new Acute infections in log (- exit acute at the same time step)matches the data in
# "Number of New Acute Infections" channel in InsetChart.json
# note:"Enable_Vital_Dynamics": 0


import re
import json
import os
import dtk_test.dtk_sft as sft


def application(report_file, debug=False):
    # pdb.set_trace()
    # print( "Post-processing: " + report_file )
    sft.wait_for_done()

    cdj = json.loads(open("config.json").read())["parameters"]
    start_time = cdj["Start_Time"]
    isj = json.loads(open( os.path.join("output", "InsetChart.json")).read())["Channels"]
    nonai = isj["Number of New Acute Infections"]["Data"]
    count = 0
    sum_count = 0
    counts = []
    with open("test.txt") as logfile:
        for line in logfile:
            if re.search("Update\(\): Time:", line):
                counts.append(count)
                sum_count += count
                count = 0
            if re.search("Infection stage transition", line) and re.search("->Acute", line):
                count += 1

    success = True
    with open(sft.sft_output_filename, "w") as report_file:
        if sum_count == 0:
            # make sure there is at least one acute infection during the test
            success = False
            report_file.write("Found no Acute Infections in test case.\n")
        else:
            # #2890
            # for i in range(1, len(counts)):
            #     count_log = counts[i]
            #     count_icj = nonai[i-1]
            for i in range(0, len(counts)):
                count_log = counts[i]
                count_icj = nonai[i]
                timestep = start_time + i
                if count_log != count_icj:
                    success = False
                    report_file.write("BAD: At time {0}: new Acute infections is {1} from Stdout log, while it's"
                                      " {2} from InsetChart.json.\n".format(timestep, count_log, count_icj))

        # #2890
        # counts2 = counts[1:len(counts)-1]
        # sft.plot_data(counts2, nonai, label1="New Acute from stdout", label2="New Acute from InsertChart",
        sft.plot_data(counts, nonai, label1="New Acute from stdout", label2="New Acute from InsertChart",
                      title="New Acute Infections (InsetChart Test)", xlabel="Time",
                      ylabel="Number of New Acute Infections",
                      category='report_inset_chart_new_acute',
                             alpha=0.5, overlap=True)

        report_file.write(sft.format_success_msg(success))


if __name__ == "__main__":
    # execute only if run as a script
    application("")
