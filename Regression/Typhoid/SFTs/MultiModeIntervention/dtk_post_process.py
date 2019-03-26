#!/usr/bin/python

import re
import dtk_test.dtk_sft as sft

    
def application(report_file):
    sft.wait_for_done()
    lines = []
    with open("test.txt") as logfile:
        for line in logfile:
            if re.search("Initializing Typhoid immunity object", line) and re.search("age=0.000000", line):
                lines.append(line)

    success = True
    actual_newborn_immunity_data = []
    expected_newborn_immunity_data = []
    with open(sft.sft_output_filename, "w") as report_file:
        if not lines:
            success = False
            report_file.write("BAD: Found no data matching test case.\n")
        else:
            for line in lines:
                immunity = float(sft.get_val(" immunity modifier", line))
                actual_newborn_immunity_data.append(immunity)
                expected_newborn_immunity_data.append(1)
                if immunity != 1.000:
                    success = False
                    report_file.write("BAD: immunity for newborn={0} instead of 1.0\n".format(immunity))

        report_file.write(sft.format_success_msg(success))

        sft.plot_data_sorted(actual_newborn_immunity_data,
                      expected_newborn_immunity_data, label1="Actual",
                      label2="Expected", title="Newborn Immunity Data",
                      xlabel="Individuals",
                      ylabel="Immunity",
                      category='multi_mode_intervention')

if __name__ == "__main__":
    # execute only if run as a script
    application("")
