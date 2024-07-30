#!/usr/bin/python
import os
import matplotlib.pyplot as plt
import numpy as np
import math

if __name__ == '__main__':
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts/').resolve().absolute()) )

from dtk_test.dtk_sft_class import SFT, arg_parser
from dtk_test.dtk_General_Support import ConfigKeys

"""
Testing tickets:
HIV-Ongoing: Add PropertyChange filter to ReportEventRecorder
https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/4566

This test is testing the new PropertyChange filtering control in ReportEventRecorder. The parameter we are interested in
is Report_Event_Recorder_PropertyChange_IP_Key_Of_Interest. 
    --When is the not None, PropertyChange event should be listened automatically in ReportEventRecorder. 
    --The report should only reports when the individuals change the given IP key that specified in the parameter.
"""


class ReportEventRecorderTest(SFT):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # self.json_report_name = "InsetChart.json"
        self.params_keys = [ConfigKeys.Report_Event_Recorder,
                            ConfigKeys.Report_Event_Recorder_PropertyChange_IP_Key_Of_Interest,
                            ConfigKeys.Simulation_Duration]

    def load_config(self):
        super(ReportEventRecorderTest, self).load_config(params_keys=self.params_keys)

    # overwrite the test method
    def test(self):
        ip_key_of_interest = self.params[ConfigKeys.Report_Event_Recorder_PropertyChange_IP_Key_Of_Interest]

        if self.params[ConfigKeys.Report_Event_Recorder] != 1:
            self.success = False
            self.msg.append(f'BAD: please set {ConfigKeys.Report_Event_Recorder} to 1 in '
                            f'{self.config_filename}.\n')
        elif ip_key_of_interest != 'Risk':
            self.success = False
            self.msg.append(f'BAD: please set {ConfigKeys.Report_Event_Recorder_PropertyChange_IP_Key_Of_Interest} to '
                            f'Risk in {self.config_filename}.\n')
        else:
            # parse report event recorder
            self.parse_report_event_recorder()
            csv_event_reporter = self.csv.df
            csv_event_reporter = csv_event_reporter[["Risk", "QualityOfCare", "InterventionStatus", "Event_Name"]]

            self.msg.append(f"Testing PropertyChange is listened automatically in ReportEventRecorder.\n")
            if not all(csv_event_reporter["Event_Name"] == "PropertyChange"):
                self.success = False
                self.msg.append(f"\tBAD: PropertyChange should be listened automatically in ReportEventRecorder when "
                                f"Report_Event_Recorder_PropertyChange_IP_Key_Of_Interest is turn on.\n")
            else:
                self.msg.append(f"\tGOOD: PropertyChange is listened automatically in ReportEventRecorder when "
                                f"Report_Event_Recorder_PropertyChange_IP_Key_Of_Interest is turn on.\n")

            self.msg.append(f"Testing ReportEventRecorder with "
                            f"{ConfigKeys.Report_Event_Recorder_PropertyChange_IP_Key_Of_Interest} = "
                            f"{ip_key_of_interest}\n")
            if any(csv_event_reporter["InterventionStatus"] != "Monitor"):
                self.success = False
                self.msg.append(f"\tBAD: We report InterventionStatus != Monitor in ReportEventRecorder which means we"
                                f" are not filtering base on Report_Event_Recorder_PropertyChange_IP_Key_Of_Interest."
                                f"Please check campaign.json to see if anything not configured correctly.\n")
            else:
                self.msg.append(f"\tGOOD: We report InterventionStatus = Monitor only in ReportEventRecorder which "
                                f"means we are filtering base on "
                                f"Report_Event_Recorder_PropertyChange_IP_Key_Of_Interest.\n")

            if len(csv_event_reporter) != 17396:  # total population = 17396
                self.success = False
                self.msg.append(
                    f"\tBAD: We should report 17396 events in ReportEventRecorder, but we got {len(csv_event_reporter)}"
                    f".\n")
            else:
                self.msg.append(
                    f"\tGOOD: We report 17396 events in ReportEventRecorder.\n")

            pass


def application(output_folder="output", my_arg=None):
    if not my_arg:
        my_sft = ReportEventRecorderTest()
    else:
        my_sft = ReportEventRecorderTest(
            output=my_arg.output, stdout=my_arg.stdout, json_report=my_arg.json_report, event_csv=my_arg.event_csv,
            config=my_arg.config, campaign=my_arg.campaign, report_name=my_arg.report_name, debug=my_arg.debug)
    my_sft.run()


if __name__ == "__main__":
    # execute only if run as a script
    my_arg = arg_parser()
    application(my_arg=my_arg)
