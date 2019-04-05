#!/usr/bin/python

import re
import json
import dtk_test.dtk_sft as sft
from scipy import stats
import math


def application(report_file, debug=False):
    # print( "Post-processing: " + report_file )
    sft.wait_for_done()

    lines = []
    with open("test.txt") as logfile:
        for line in logfile:
            if re.search("Calculated prepatent duration", line):
                # append New Infection to list
                lines.append(line)


    lognormal_mu_h = 1.5487
    lognormal_sigma_h = 0.3442
    lognormal_mu_m = 2.002
    lognormal_sigma_m = 0.7604
    lognormal_mu_l = 2.235
    lognormal_sigma_l = 0.4964
    success = True
    if debug:
        with open("filtered_lines", "w") as report_file:
            for line in lines:
                report_file.write(line)

    with open(sft.sft_output_filename, "w") as report_file: 
        timers_low = []
        timers_high = []
        if not lines:
            success = False
            report_file.write("BAD: Found no data matching test case.\n")
        else:
            for line in lines:
                duration = float(sft.get_val("Calculated prepatent duration = ", line))
                if re.search("doseTracking = Low", line):
                    timers_low.append(duration)
                elif re.search("doseTracking = High", line):
                    timers_high.append(duration) 
                else:
                    category = line.split()[-1]
                    report_file.write("Found dose category: {} that does not fall in High and Low category."
                                      "\n".format(category))

            # it's ok that there is no data in one/two category, see https://github.com/jgauld/DtkTrunk/issues/77
            if not timers_high and not timers_low:
                success = False
                report_file.write("BAD: Found no data matching test case.\n")
            else:
                if not timers_low:
                    report_file.write("Found no Prepatent case in Category doseTracking = Low.\n")
                elif not sft.test_lognorm(timers_low, lognormal_mu_l, lognormal_sigma_l, report_file, "Low"):
                    print( "BAD: ks test failed for 1 case." )
                    success = False
                if not timers_high:
                    report_file.write("Found no Prepatent case in Category doseTracking = High.\n")
                elif not sft.test_lognorm(timers_high, lognormal_mu_h, lognormal_sigma_h, report_file, "high"):
                    print( "BAD: ks test failed for 1 case." )
                    success = False

            if debug:
                # dump duration into json file
                report_file.write("Timers and logs in sorted_duration.json\n")
                myjson = {}
                duration_timers_high = {"Data": sorted(timers_high)}
                duration_timers_low = {"Data": sorted(timers_low)}
                myjson["duration_timers_high"] = duration_timers_high
                myjson["duration_timers_low"] = duration_timers_low
                with open("sorted_duration.json", "w") as outfile:
                    outfile.write(json.dumps(myjson, indent=4))

        report_file.write(sft.format_success_msg(success))

        # plotting
        dist_lognormal_scipy1 = stats.lognorm.rvs(s=lognormal_sigma_h, loc=0,
                                                  scale=math.exp(lognormal_mu_h),
                                                  size=len(timers_high))
        sft.plot_data(timers_high, dist_lognormal_scipy1, label1="Actual", label2="Expected",
                      title="Prepatent Duration for High Dose (Sorted)", xlabel="Occurrences",
                      ylabel="Duration time (in days)",
                      category='prepatent_duration_times_high_dose',
                      alpha=0.5, overlap=True,
                      sort=True)

        dist_lognormal_scipy3 = stats.lognorm.rvs(s=lognormal_sigma_l, loc=0,
                                                  scale=math.exp(lognormal_mu_l),
                                                  size=len(timers_low))
        sft.plot_data(timers_low, dist_lognormal_scipy3, label1="Actual", label2="Expected",
                      title="Prepatent Duration for Low Dose (Sorted)", xlabel="Occurrences",
                      ylabel="Duration time (in days)",
                      category='prepatent_duration_times_low_dose',
                      alpha=0.5, overlap=True,
                      sort=True)


if __name__ == "__main__":
    # execute only if run as a script
    application("")
