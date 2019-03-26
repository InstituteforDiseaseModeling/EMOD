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
import pdb


def application( report_file, debug = False ):
    # pdb.set_trace()
    # print( "Post-processing: " + report_file )
    sft.wait_for_done()

    cdj = json.loads(open("config.json").read())["parameters"]
    start_time = cdj["Start_Time"]
    isj = json.loads(open(os.path.join("output", "InsetChart.json")).read())["Channels"]
    nocc = isj["Number of Chronic Carriers"]["Data"]
    count = 0
    chronics_from_diagnostic = 0
    sum_count = 0
    counts = []
    counts2 = {}
    sum_counts = []
    with open("test.txt") as logfile:
        for line in logfile:
            if re.search("Update\(\): Time:", line):
                counts.append(count)
                sum_count += count
                sum_counts.append(sum_count)
                count = 0

                if chronics_from_diagnostic > 0: # not a good test if none are found, but we can fix that
                    counts2[ len( sum_counts )-1 ] = chronics_from_diagnostic
                    chronics_from_diagnostic = 0

            if re.search("just went chronic", line):
                count += 1
            elif re.search("positive from God", line):
                chronics_from_diagnostic += 1

    success = True
    with open( sft.sft_output_filename, "w") as report_file:
        if not sum_count:
            success = False
            report_file.write("BAD: Found no Chronic Infections in test case.\n")
        else:
            pre_sum_count_icj = 0
            debug_count = []
            for timestep in counts2.keys():
                if counts2[timestep] != sum_counts[timestep]:
                    print( "BAD!" )
                    success = False
                    pdb.set_trace() 
                    report_file.write("BAD: Total Chronic infections is {0} from InsetChart.json at time {1}, it's {2} "
                                      "at previous time step, expected {3} more based on Stdout log."
                                      "\n".format(cur_sum_count_icj, timestep, pre_sum_count_icj, count_log)) 
               
            non_sparse_diagnostic_array = []
            for tstep in range( len( sum_counts ) ):
                if tstep in counts2.keys():
                    non_sparse_diagnostic_array.append( counts2[ tstep ] )
                else:
                    non_sparse_diagnostic_array.append( 0 )

            sft.plot_data(sum_counts, non_sparse_diagnostic_array, label1="Total Chronic from stdout",
                          label2="Chronic counts from periodic diagnostic",
                          title="Chronic Infections (Diagnostic)", xlabel="Time",
                          ylabel="Total Number of Chronic Carriers",
                          category='report_diagnostic_chronic')

            report_file.write(sft.format_success_msg(success))


if __name__ == "__main__":
    # execute only if run as a script
    application("")
