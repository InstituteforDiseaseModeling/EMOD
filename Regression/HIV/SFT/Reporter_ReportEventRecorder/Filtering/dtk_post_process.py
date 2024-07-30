#!/usr/bin/python
import os
import matplotlib
from sys import platform
if platform == "linux" or platform == "linux2":
    print('Linux OS. Using non-interactive Agg backend')
    matplotlib.use('Agg')

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
from dtk_test.dtk_StdOut import SearchType

"""
Testing tickets:
ReportEventRecorder - Add controls to filter via year
https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/4548
-	Added more than just year controls

This test is testing more filter controls(other than Age and Year) in ReportEventRecorder including:
    Must_Have_Intervention
        -- tested with MaleCircumcision intervention
    Node_IDs_Of_Interest
    
Report event is set to HappyBirthday so by configuring the initial age distribution, we could predict when and who 
should report the event in ReportEventRecorder.
"""


class ReportEventRecorderTest(SFT):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # self.json_report_name = "InsetChart.json"
        self.params_keys = [ConfigKeys.Report_Event_Recorder,
                            ConfigKeys.Report_Event_Recorder_Events,
                            ConfigKeys.Report_Event_Recorder_Must_Have_Intervention,
                            ConfigKeys.Report_Event_Recorder_Node_IDs_Of_Interest,
                            ConfigKeys.Simulation_Duration]

    def load_config(self):
        super(ReportEventRecorderTest, self).load_config(params_keys=self.params_keys)

    # overwrite the test method
    def test(self):
        must_have_intervention = self.params[ConfigKeys.Report_Event_Recorder_Must_Have_Intervention]
        node_ids_of_interest = self.params[ConfigKeys.Report_Event_Recorder_Node_IDs_Of_Interest]

        if self.params[ConfigKeys.Report_Event_Recorder] != 1:
            self.success = False
            self.msg.append(f'BAD: please set {ConfigKeys.Report_Event_Recorder} to 1 in '
                            f'{self.config_filename}.\n')
        elif self.params[ConfigKeys.Report_Event_Recorder_Events] != ['HappyBirthday']:
            self.success = False
            self.msg.append(f'BAD: please set {ConfigKeys.Report_Event_Recorder_Events} to HappyBirthday in '
                            f'{self.config_filename}.\n')
        else:
            # get initial age
            self.parse_stdout(filter_string_list=["Created human with age="],
                              load_df_param=[["age"],
                                           ["age="],
                                           [SearchType.VAL]],
                              first_only=False)

            age_df = self.stdout.df
            age_df['age'].hist(bins=30)
            plt.savefig("age_hist.png")
            age = age_df['age'].sort_values()  # for debugging

            # parse report event recorder
            self.parse_report_event_recorder()
            csv_event_reporter = self.csv.df
            csv_event_reporter = csv_event_reporter[["Gender", "Node_ID", "Age", "Event_Name"]]

            self.msg.append("Testing ReportEventRecorder with: Report_Event_Recorder_Must_Have_Intervention\n")
            if any(csv_event_reporter['Gender'] != 'M'):
                self.success = False
                self.msg.append(f"\tBAD: Gender column in ReportEventRecorder must be 'M', since we have "
                                f"{ConfigKeys.Report_Event_Recorder_Must_Have_Intervention} = {must_have_intervention}"
                                f".\n")
            else:
                self.msg.append(f"\tGOOD: Gender column in ReportEventRecorder is 'M', since we have "
                                f"{ConfigKeys.Report_Event_Recorder_Must_Have_Intervention} = {must_have_intervention}"
                                f".\n")

            self.msg.append("Testing ReportEventRecorder with: Node_IDs_Of_Interest:\n")
            node_ids_not_interest = csv_event_reporter[~csv_event_reporter["Node_ID"].isin(node_ids_of_interest)]
            if not node_ids_not_interest.empty:
                self.success = False
                self.msg.append(
                    f"\tBAD: {ConfigKeys.Report_Event_Recorder_Node_IDs_Of_Interest} is set to "
                    f"{node_ids_of_interest}, we should only report Node_ID in {node_ids_of_interest}.\n"
                    f"We got\n {node_ids_not_interest}.\n")
            else:
                self.msg.append(
                    f"\tGOOD: {ConfigKeys.Report_Event_Recorder_Node_IDs_Of_Interest} is set to "
                    f"{node_ids_of_interest}, we only report Node_ID in {node_ids_of_interest}.\n")

            self.parse_json_report()
            insetchart_df = self.json_report.df
            n_circumcised_male = insetchart_df["Number of Circumcised Males"].loc[1]
            expected_total_reporting = n_circumcised_male / 2
            if not math.isclose(expected_total_reporting, len(csv_event_reporter), rel_tol=0.05):
                self.success = False
                self.msg.append(
                    f"\tBAD: we should report about {expected_total_reporting} events in ReportEventRecorder, but we "
                    f"got {len(csv_event_reporter)}.\n")
            else:
                self.msg.append(
                    f"\tGOOD: we should report about {expected_total_reporting} events in ReportEventRecorder and we "
                    f"got {len(csv_event_reporter)}.\n")
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
