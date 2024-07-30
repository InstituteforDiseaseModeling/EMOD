#!/usr/bin/python
import os
import matplotlib.pyplot as plt
import numpy as np
import math

if __name__ == '__main__':
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../shared_embedded_py_scripts/').resolve().absolute()) )

from dtk_test.dtk_sft_class import SFT, arg_parser
from dtk_test.dtk_General_Support import ConfigKeys
from dtk_test.dtk_OutputFile import CsvOutput

"""
Testing tickets:
RelationshipStart.csv - Add filtering and column controls
https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/4556

This test is testing the new filtering controls for RelationshipStart.csv including:
    Min_Age_Years
    Max_Age_Years
    Start_Year
    End_Year
    Must_Have_IP_Key_Value
    Must_Have_Intervention
    Node_IDs_Of_Interest
"""


class ReportRelationshipStartTest(SFT):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # self.json_report_name = "InsetChart.json"
        self.params_keys = [ConfigKeys.Report_Event_Recorder,
                            ConfigKeys.Report_Event_Recorder_Events,
                            ConfigKeys.Report_Relationship_Start,
                            ConfigKeys.Report_Relationship_Start_Min_Age_Years,
                            ConfigKeys.Report_Relationship_Start_Max_Age_Years,
                            ConfigKeys.Report_Relationship_Start_Start_Year,
                            ConfigKeys.Report_Relationship_Start_End_Year,
                            ConfigKeys.Report_Relationship_Start_Must_Have_IP_Key_Value,
                            ConfigKeys.Report_Relationship_Start_Must_Have_Intervention,
                            ConfigKeys.Report_Relationship_Start_Node_IDs_Of_Interest,
                            ConfigKeys.Base_Year,
                            ConfigKeys.Simulation_Duration]

    def load_config(self):
        super(ReportRelationshipStartTest, self).load_config(params_keys=self.params_keys)

    # overwrite the test method
    def test(self):
        start_year = self.params[ConfigKeys.Report_Relationship_Start_Start_Year]
        end_year = self.params[ConfigKeys.Report_Relationship_Start_End_Year]
        min_age = self.params[ConfigKeys.Report_Relationship_Start_Min_Age_Years]
        max_age = self.params[ConfigKeys.Report_Relationship_Start_Max_Age_Years]
        ip_control = self.params[ConfigKeys.Report_Relationship_Start_Must_Have_IP_Key_Value]
        must_have_intervention = self.params[ConfigKeys.Report_Relationship_Start_Must_Have_Intervention]
        node_id = self.params[ConfigKeys.Report_Relationship_Start_Node_IDs_Of_Interest]
        base_year = self.params[ConfigKeys.Base_Year]

        if self.params[ConfigKeys.Report_Event_Recorder] != 1 or \
                self.params[ConfigKeys.Report_Relationship_Start] != 1:
            self.success = False
            self.msg.append(f'BAD: please set {ConfigKeys.Report_Event_Recorder} and '
                            f'{ConfigKeys.Report_Relationship_Start} to 1 in '
                            f'{self.config_filename}.\n')
        else:
            self.msg.append("parse report event recorder\n")
            self.parse_report_event_recorder()
            csv_event_reporter = self.csv.df
            # csv_event_reporter = csv_event_reporter[["Individual_ID", "Event_Name", "Risk", "InterventionStatus"]]

            self.msg.append("parse RelationshipStart.csv\n")
            relationship_start_df = CsvOutput(file=os.path.join(self.output_folder, "RelationshipStart.csv")).df

            self.msg.append("Testing RelationshipStart with: start year and end year:\n")
            min_day = min(relationship_start_df['Rel_start_time'])
            self.msg.append(f"The earliest date we start listening is {min_day}.\n")
            if min_day / 365 + base_year < start_year:
                self.success = False
                self.msg.append(f"\tBAD: We start listening in RelationshipStart before "
                                f"{ConfigKeys.Report_Relationship_Start_Start_Year} = {start_year}.\n")
            elif not math.isclose(min_day / 365 + base_year,  start_year, abs_tol=0.02):
                self.success = False
                self.msg.append(f"\tBAD: We did not start listening in RelationshipStart at "
                                f"{ConfigKeys.Report_Relationship_Start_Start_Year} = {start_year}.\n")
            else:
                self.msg.append(f"\tGOOD: We start listening in RelationshipStart at "
                                f"{ConfigKeys.Report_Relationship_Start_Start_Year} = {start_year}.\n")

            max_day = max(relationship_start_df['Rel_start_time'])
            self.msg.append(f"The latest date we end listening is {max_day}.\n")
            if max_day / 365 + base_year > end_year:
                self.success = False
                self.msg.append(f"\tBAD: We stop listening in RelationshipStart after "
                                f"{ConfigKeys.Report_Relationship_Start_End_Year} = {end_year}.\n")
            elif not math.isclose(max_day / 365 + base_year,  end_year, abs_tol=0.02):
                self.success = False
                self.msg.append(f"\tBAD: We did not stop listening in ReportEventRecorder at "
                                f"{ConfigKeys.Report_Relationship_Start_End_Year} = {end_year}.\n")
            else:
                self.msg.append(f"\tGOOD: We stop listening in ReportEventRecorder at "
                                f"{ConfigKeys.Report_Relationship_Start_End_Year} = {end_year}.\n")

            self.msg.append("Testing RelationshipStart with: min age and max age:\n")
            result = True
            for index, row in relationship_start_df.iterrows():
                a_min_age_at_relationship_start = row['A_age']
                b_min_age_at_relationship_start = row['B_age']
                if (not min_age < a_min_age_at_relationship_start < max_age) and \
                    (not min_age < b_min_age_at_relationship_start < max_age):
                    self.success = result = False
                    r_id = row['Rel_ID']
                    self.msg.append(
                        f"\tBAD: We report A_age = {a_min_age_at_relationship_start} and B_age = "
                        f"{b_min_age_at_relationship_start} in RelationshipStart(Rel_ID = {r_id}) which do not match "
                        f"{ConfigKeys.Report_Relationship_Start_Min_Age_Years} = {min_age} and "
                        f"{ConfigKeys.Report_Relationship_Start_Max_Age_Years} = {max_age}.\n")
            if result:
                self.msg.append(
                    f"\tGOOD: A_age and B_age in RelationshipStart match "
                    f"{ConfigKeys.Report_Relationship_Start_Min_Age_Years} = {min_age} and "
                    f"{ConfigKeys.Report_Relationship_Start_Max_Age_Years} = {max_age}.\n")

            self.msg.append("Testing RelationshipStart with: must_have_intervention:\n")
            if any(relationship_start_df["A_IP='Risk'"] != "HIGH"):
                self.success = False
                self.msg.append(
                    f"\tBAD: {ConfigKeys.Report_Relationship_Start_Must_Have_Intervention} is set to "
                    f"{must_have_intervention}, we should only report A_IP='Risk' = 'HIGH' which is the property of the"
                    f" group that got the intervention.\n")
            else:
                self.msg.append(
                    f"\tGOOD: {ConfigKeys.Report_Relationship_Start_Must_Have_Intervention} is set to "
                    f"{must_have_intervention}, we only report A_IP='Risk' = 'HIGH' which is the property of the"
                    f" group that got the intervention.\n")

            self.msg.append("Testing RelationshipStart with: Must_Have_IP_Key_Value:\n")
            result = True
            for index, row in relationship_start_df.iterrows():
                a_intervention_status = row["A_IP='InterventionStatus'"]
                b_intervention_status = row["B_IP='InterventionStatus'"]
                if (not a_intervention_status == "Monitor") and \
                    (not b_intervention_status == "Monitor"):
                    self.success = result = False
                    r_id = row['Rel_ID']
                    self.msg.append(
                        f"\tBAD: We report (A_IP='InterventionStatus') = {a_intervention_status} and "
                        f"(B_IP='InterventionStatus') = {b_intervention_status} in RelationshipStart(Rel_ID = {r_id}) "
                        f"which do not match {ConfigKeys.Report_Relationship_Start_Must_Have_IP_Key_Value} = "
                        f"{ip_control}.\n")
            if result:
                self.msg.append(
                    f"\tGOOD: at least one of (A_IP='InterventionStatus') and (B_IP='InterventionStatus') in "
                    f"RelationshipStart is Monitor, which match "
                    f"{ConfigKeys.Report_Relationship_Start_Must_Have_IP_Key_Value} = "
                    f"{ip_control}.\n")

            self.msg.append("Testing RelationshipStart with: Node_IDs_Of_Interest:\n")
            if any(relationship_start_df["Current_node_ID"] != node_id[0]):
                self.success = False
                self.msg.append(
                    f"\tBAD: {ConfigKeys.Report_Relationship_Start_Node_IDs_Of_Interest} is set to "
                    f"{node_id}, we should only report Current_node_ID in {node_id}.\n")
            else:
                self.msg.append(
                    f"\tGOOD: {ConfigKeys.Report_Relationship_Start_Node_IDs_Of_Interest} is set to "
                    f"{node_id}, we only report Current_node_ID in {node_id}.\n")

            pass


def application(output_folder="output", my_arg=None):
    if not my_arg:
        my_sft = ReportRelationshipStartTest()
    else:
        my_sft = ReportRelationshipStartTest(
            output=my_arg.output, stdout=my_arg.stdout, json_report=my_arg.json_report, event_csv=my_arg.event_csv,
            config=my_arg.config, campaign=my_arg.campaign, report_name=my_arg.report_name, debug=my_arg.debug)
    my_sft.run()


if __name__ == "__main__":
    # execute only if run as a script
    my_arg = arg_parser()
    application(my_arg=my_arg)
