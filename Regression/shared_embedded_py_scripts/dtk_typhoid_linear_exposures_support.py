#!/usr/bin/python
# This support the following Typoid SFT tests:
# Vaccine_Linear_Exposures_Contact
# Vaccine_Linear_Exposures_Environmental

import re
import json
import math
import pdb
import os
import dtk_test.dtk_sft as sft
import pandas as pd
import dtk_test.dtk_General_Support as General_Support


class LinearExposuresSupport(object):
    def __init__(self, route):
        self.route = route
        self.chart_channels = ["New Infections By Route ({})".format(route.upper())]
        self.attenuation_text = "Set current_exposures_attenuation_{} to ".format(route)

    def parse_inset_chart(self, output_folder, insetchart_name, debug):
        return General_Support.parse_inset_chart(output_folder=output_folder, insetchart_name=insetchart_name,
                                                 insetkey_list=self.chart_channels, debug=debug)

    def parse_stdout_file(self, initial_timestep=0, stdout_filename="test.txt", debug=False):
        """
        :param initial_timestep:   first timestep from config
        :param stdout_filename: file to parse (test.txt)
        :param debug:           whether or not we write an additional file that's full of the matched lines
        :return:                array of attenuation for each day
        """
        attenuation = []
        filtered_lines = []
        time_step = initial_timestep
        latest_time = initial_timestep
        with open(stdout_filename) as logfile:
            for line in logfile:
                if "Time: " in line:
                    time_step += 1
                if self.attenuation_text in line:
                    if time_step > latest_time:
                        attenuation.append(float(sft.get_val(self.attenuation_text, line)))
                        latest_time = time_step
                        if debug:
                            filtered_lines.append(line)
        if debug:
            with open("filtered_lines.txt", "w") as outfile:
                for line in filtered_lines:
                    outfile.write(line)
        return attenuation

    def create_report_file(self, attenuation, inset_chart_obj, report_name, debug):
        # contagion trails attenuation by one day. Attenuation changes to 0, next day contagion should be 0
        success = True
        config_name = General_Support.load_config_parameters(keys=[General_Support.ConfigKeys.Config_Name])[General_Support.ConfigKeys.Config_Name]
        with open(report_name, "w") as report_file:
            report_file.write("Results for {}:\n".format(config_name))
            if not inset_chart_obj:
                success = False
                report_file.write("BAD: Not finding any data from inset chart.\n")
            elif not attenuation:
                # make sure we have found the correct debug data
                success = False
                report_file.write("BAD: Couldn't find current_exposures_attenuation_{} ."
                                  "\n".format(self.route))
            else:
                new_infections = inset_chart_obj[self.chart_channels[0]]
                if len(attenuation) + 1 != len(new_infections):
                    success = False
                    err_template = "BAD: Length difference between {2} attenuation: {0} new infections {2}: {1}\n"
                    report_file.write(err_template.format(
                        len(attenuation),
                        len(new_infections),
                        self.route))
                else:
                    for i in range(len(attenuation)):
                        # when the exposures attenuation is set to 0, there should be no new infections (the next day)
                        if attenuation[i] == 0 and new_infections[i + 1] != 0:
                            success = False
                            report_file.write("BAD: At time {0}: the {1} is {2} from Stdout, "
                                              "while the contagion is {3} from InsetChart.json.\n".
                                              format(i + 1, self.attenuation_text, attenuation[i], new_infections[i + 1]))
                    # mapping all the large contagion #s to 1s for better viewing of attenuation data
                    for i in range(0, len(new_infections)):
                        if new_infections[i] > 1:
                            new_infections[i] = 1

                    # showing relevant subset
                    sft.plot_data(attenuation, new_infections[1:],
                                      label1="Attenuation",
                                      label2="New Infections {}".format(self.route),
                                      title="Vaccine Linear Exposures {}\n"
                                            "(when attenuation is 0, New Infections should be "
                                            "0)".format(self.route),
                                      xlabel="Time",
                                      ylabel="",
                                      category='vaccine_linear_exposures_{}'.format(self.route),
                                      overlap=False, alpha=0.5)

            if success:
                report_file.write("GOOD: When Attenuation of {} exposures is set to 0, there are no new infections."
                                  "\n".format(self.route))
            report_file.write(sft.format_success_msg(success))


class VlesContact(LinearExposuresSupport):
    def __init__(self):
        LinearExposuresSupport.__init__(self, 'contact')


class VlesEnv(LinearExposuresSupport):
    def __init__(self):
        LinearExposuresSupport.__init__(self, 'environment')
