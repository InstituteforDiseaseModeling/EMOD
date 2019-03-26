#!/usr/bin/python

import re
import json
import math
import dtk_test.dtk_sft as sft
import matplotlib.pylab as plt


def application(report_file):
    sft.wait_for_done()
    regex = "immunity calculated as"
    lines = []
    with open("test.txt") as logfile:
        for line in logfile:
            if re.search(regex, line):
                lines.append(line)

    cdj = json.loads(open("config.json").read())["parameters"]
    protection_per_infection = cdj["Typhoid_Protection_Per_Infection"]

    immunity_observed_data = []
    immunity_predicted_data = []
    immunity_based_on_infection_count = {}
    success = True
    with open(sft.sft_output_filename, "w") as report_file:
        if not lines:
            success = False
            report_file.write("BAD: Found no data matching test case.\n")
        else:
            for line in lines:
                immunity = float(line.split()[8])
                immunity_observed_data.append(immunity)
                infection_count = int((sft.get_val("_infection_count=", line)).rstrip('.'))
                immunity_predicted = float(format(math.pow(1 - protection_per_infection, infection_count), '0.6f'))
                immunity_predicted_data.append(immunity_predicted)
                if abs(immunity - immunity_predicted) > 1e-5:
                    success = False
                    report_file.write("BAD: Immunity is {} instead of predicted {}\n".format(immunity,
                                                                                             immunity_predicted))
                if infection_count not in immunity_based_on_infection_count:
                    immunity_based_on_infection_count[infection_count] = immunity_predicted


        report_file.write(sft.format_success_msg(success))

        sft.plot_data(immunity_observed_data, immunity_predicted_data, label1="Actual",
                             label2="Expected", title="Immunity based on Infection Count, TPPI={0}"
                                                      "".format(protection_per_infection),
                             xlabel="Occurrence",
                             ylabel="Immunity Level",
                             category='immunity_based_on_infection_data',
                             alpha=0.5, overlap=True, sort=True)
        # plot for Typhod spec:
        lists = sorted(immunity_based_on_infection_count.items())  # sorted by key, return a list of tuples
        x, y = zip(*lists)  # unpack a list of pairs into two tuples
        fig = plt.figure()
        plt.plot(x, y, "b*",  markersize=6)
        plt.title("Immunity based on Infection Count, TPPI={0}".format(protection_per_infection))
        plt.xlabel("Infection Count")
        plt.ylabel("Immunity")
        if sft.check_for_plotting():
            plt.show()
        fig.savefig('immunity_based_on_infection_count.png')


if __name__ == "__main__":
    # execute only if run as a script
    application("")
