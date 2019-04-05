#!/usr/bin/python
# This SFT test the following statement:
# At each time step, the # of total Chronic infections in "Number of Chronic Carriers" channel in InsetChart.json
# equals the new Chronic infections in log plus the total # of Chronic case from previous time step
# the duration of Chronic state is life long, so the total Chronic infection will not decrease
# (note:"Enable_Vital_Dynamics": 0 )

import re
import json
import os
import dtk_test.dtk_sft as sft


def application( report_file, debug = False ):
    # pdb.set_trace()
    # print( "Post-processing: " + report_file )
    sft.wait_for_done()

    cdj = json.loads(open("config.json").read())["parameters"]
    start_time = cdj["Start_Time"]
    isj = json.loads(open(os.path.join("output", "InsetChart.json")).read())["Channels"]
    nocc = isj["Number of Chronic Carriers"]["Data"]
    count = 0
    sum_count = 0
    counts = []
    sum_counts = []
    with open("test.txt") as logfile:
        for line in logfile:
            if re.search("Update\(\): Time:", line):
                counts.append(count)
                sum_count += count
                sum_counts.append(sum_count)
                count = 0
            if re.search("just went chronic", line):
                count += 1

    success = True
    with open( sft.sft_output_filename, "w") as report_file:
        if not sum_count:
            success = False
            report_file.write("BAD: Found no Chronic Infections in test case.\n")
        else:
            pre_sum_count_icj = 0
            debug_count = []
            # #2890
            # for i in range(1, len(counts)):
            #     timestep = start_time + i
            #     count_log = counts[i]
            #     cur_sum_count_icj = nocc[i - 1]
            for i in range(0, len(counts)):
                timestep = start_time + i
                count_log = counts[i]
                cur_sum_count_icj = nocc[i]
                if cur_sum_count_icj != count_log + pre_sum_count_icj:
                    success = False
                    report_file.write("BAD: Total Chronic infections is {0} from InsetChart.json at time {1}, it's {2} "
                                      "at previous time step, expected {3} more based on Stdout log."
                                      "\n".format(cur_sum_count_icj, timestep, pre_sum_count_icj, count_log))

                debug_count.append(count_log + pre_sum_count_icj)
                pre_sum_count_icj = cur_sum_count_icj

            sft.plot_data_sorted(debug_count, nocc, label1="Total Chronic from stdout",
                          label2="Total Chronic from InsertChart",
                          title="Chronic Infections (InsetChart Test)", xlabel="Time",
                          ylabel="Total Number of Chronic Infections",
                          category='report_inset_chart_total_chronic',
                          alpha=0.5, overlap=True)

            report_file.write(sft.format_success_msg(success))


if __name__ == "__main__":
    # execute only if run as a script
    application("")
