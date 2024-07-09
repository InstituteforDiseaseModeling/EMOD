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
ReportEventRecorder - Add Time column
https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/4564

ReportEventRecorder - Add controls to filter via year
https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/4548
-	Added more than just year controls

This test is testing basic filtering controls in ReportEventRecorder including:
    Start_Year
    End_Year
    Min_Age_Years
    Max_Age_Years
    
It also verifies that the new Time column in ReportEventRecorder matches the Year column.

Report event is set to HappyBirthday so by configuring the initial age distribution, we could predict when and who 
should report the event in ReportEventRecorder.
"""


class ReportEventRecorderTest(SFT):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # self.json_report_name = "InsetChart.json"
        self.params_keys = [ConfigKeys.Report_Event_Recorder,
                            ConfigKeys.Report_Event_Recorder_Events,
                            ConfigKeys.Report_Event_Recorder_Start_Year,
                            ConfigKeys.Report_Event_Recorder_End_Year,
                            ConfigKeys.Report_Event_Recorder_Min_Age_Years,
                            ConfigKeys.Report_Event_Recorder_Max_Age_Years,
                            ConfigKeys.Base_Year,
                            ConfigKeys.Simulation_Duration]

    def load_config(self):
        super(ReportEventRecorderTest, self).load_config(params_keys=self.params_keys)

    # overwrite the test method
    def test(self):
        start_year = self.params[ConfigKeys.Report_Event_Recorder_Start_Year]
        end_year = self.params[ConfigKeys.Report_Event_Recorder_End_Year]
        min_age = self.params[ConfigKeys.Report_Event_Recorder_Min_Age_Years]
        max_age = self.params[ConfigKeys.Report_Event_Recorder_Max_Age_Years]
        base_year = self.params[ConfigKeys.Base_Year]

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
            csv_event_reporter = csv_event_reporter[["Time", "Year", "Individual_ID", "Age", "Event_Name"]].copy()

            self.msg.append("Testing ReportEventRecorder with: time vs. year\n")

            # using .loc to avoid SettingWithCopyWarning
            # csv_event_reporter['time matches year'] = np.where(
            #     (csv_event_reporter['Year'] == round(csv_event_reporter['Time']/365, 2) + base_year), 1, 0)

            csv_event_reporter.loc[:, 'time matches year'] = 0
            match = (csv_event_reporter['Year'] == round(csv_event_reporter['Time'] / 365, 2) + base_year)
            csv_event_reporter.loc[match, 'time matches year'] = 1

            if any(csv_event_reporter['time matches year'] == 0):
                self.success = False
                self.msg.append("\tBAD: Time column and Year column in ReportEventRecorder don't match.\n")
            else:
                self.msg.append("\tGOOD: Time column matches Year column in ReportEventRecorder.\n")

            self.msg.append("Testing ReportEventRecorder with: start year and end year:\n")
            min_year = min(csv_event_reporter['Year'])
            self.msg.append(f"The earliest date we start listening is {min_year}.\n")
            if min_year < start_year:
                self.success = False
                self.msg.append(f"\tBAD: We start listening in ReportEventRecorder before "
                                f"{ConfigKeys.Report_Event_Recorder_Start_Year} = {start_year}.\n")
            elif min_year != start_year:
                self.success = False
                self.msg.append(f"\tBAD: We did not start listening in ReportEventRecorder at "
                                f"{ConfigKeys.Report_Event_Recorder_Start_Year} = {start_year}.\n")
            else:
                self.msg.append(f"\tGOOD: We start listening in ReportEventRecorder at "
                                f"{ConfigKeys.Report_Event_Recorder_Start_Year} = {start_year}.\n")

            max_year = max(csv_event_reporter['Year'])
            self.msg.append(f"The latest date we end listening is {max_year}.\n")
            if max_year > end_year:
                self.success = False
                self.msg.append(f"\tBAD: We stop listening in ReportEventRecorder after "
                                f"{ConfigKeys.Report_Event_Recorder_End_Year} = {end_year}.\n")
            elif max_year != end_year:
                self.success = False
                self.msg.append(f"\tBAD: We did not stop listening in ReportEventRecorder at "
                                f"{ConfigKeys.Report_Event_Recorder_End_Year} = {end_year}.\n")
            else:
                self.msg.append(f"\tGOOD: We stop listening in ReportEventRecorder at "
                                f"{ConfigKeys.Report_Event_Recorder_End_Year} = {end_year}.\n")

            self.msg.append("Testing ReportEventRecorder with: min age and max age:\n")
            min_age_at_birthday = min(csv_event_reporter['Age']) / 365.0
            self.msg.append(f"The youngest age reported is {min_age_at_birthday}.\n")
            if min_age_at_birthday < min_age:
                self.success = False
                self.msg.append(f"\tBAD: We report age = {min_age_at_birthday} in ReportEventRecorder which is younger "
                                f"than {ConfigKeys.Report_Event_Recorder_Min_Age_Years} = {min_age}.\n")
            elif not math.isclose(min_age_at_birthday, min_age, abs_tol=0.1):
                self.success = False
                self.msg.append(f"\tBAD: We report youngest age = {min_age_at_birthday} in ReportEventRecorder which "
                                f"does not match {ConfigKeys.Report_Event_Recorder_Min_Age_Years} = {min_age}.\n")
            else:
                self.msg.append(f"\tGOOD: We report youngest age = {min_age_at_birthday} in ReportEventRecorder "
                                f"which matches config {ConfigKeys.Report_Event_Recorder_Min_Age_Years} = {min_age}.\n")

            max_age_at_birthday = max(csv_event_reporter['Age']) / 365.0
            self.msg.append(f"The oldest age reported is {max_age_at_birthday}.\n")
            if max_age_at_birthday > max_age:
                self.success = False
                self.msg.append(f"\tBAD: We report age = {max_age_at_birthday} in ReportEventRecorder which is older "
                                f"than {ConfigKeys.Report_Event_Recorder_Max_Age_Years} = {max_age}.\n")
            elif not math.isclose(max_age_at_birthday, max_age, abs_tol=0.1):
                self.success = False
                self.msg.append(f"\tBAD: We report oldest age = {max_age_at_birthday} in ReportEventRecorder which "
                                f"does not match {ConfigKeys.Report_Event_Recorder_Max_Age_Years} = {max_age}.\n")
            else:
                self.msg.append(f"\tGOOD: We report oldest age = {max_age_at_birthday} in ReportEventRecorder which "
                                f"matches config {ConfigKeys.Report_Event_Recorder_Max_Age_Years} = {max_age}.\n")
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
