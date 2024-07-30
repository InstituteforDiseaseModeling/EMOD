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
from dtk_test.dtk_OutputFile import CsvOutput

"""
Testing tickets:
HIV_Ongoing: RelationshipConsummated.csv - Add more filtering & Transmission Info
https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/4492

Testing the filtering control feature in RelationshipConsummated.csv including:
        Start_Year
        End_Year
        Max_Age
        Min_Age
        Must_Have_IP_Key_Value
        Must_Have_Intervention
        Node_IDs_Of_Interest
"""


class ReportRelationshipStartTest(SFT):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # self.json_report_name = "InsetChart.json"
        self.params_keys = [ConfigKeys.Report_Event_Recorder,
                            ConfigKeys.Report_Coital_Acts,
                            ConfigKeys.Report_Coital_Acts_Start_Year,
                            ConfigKeys.Report_Coital_Acts_End_Year,
                            ConfigKeys.Report_Coital_Acts_Min_Age_Years,
                            ConfigKeys.Report_Coital_Acts_Max_Age_Years,
                            ConfigKeys.Report_Coital_Acts_Must_Have_IP_Key_Value,
                            ConfigKeys.Report_Coital_Acts_Must_Have_Intervention,
                            ConfigKeys.Report_Coital_Acts_Node_IDs_Of_Interest,
                            ConfigKeys.Base_Year,
                            ConfigKeys.Simulation_Duration]

    def load_config(self):
        super(ReportRelationshipStartTest, self).load_config(params_keys=self.params_keys)

    # overwrite the test method
    def test(self):
        start_year = self.params[ConfigKeys.Report_Coital_Acts_Start_Year]
        end_year = self.params[ConfigKeys.Report_Coital_Acts_End_Year]
        min_age = self.params[ConfigKeys.Report_Coital_Acts_Min_Age_Years]
        max_age = self.params[ConfigKeys.Report_Coital_Acts_Max_Age_Years]
        ip_control = self.params[ConfigKeys.Report_Coital_Acts_Must_Have_IP_Key_Value]
        must_have_intervention = self.params[ConfigKeys.Report_Coital_Acts_Must_Have_Intervention]
        node_ids = self.params[ConfigKeys.Report_Coital_Acts_Node_IDs_Of_Interest]
        base_year = self.params[ConfigKeys.Base_Year]

        if self.params[ConfigKeys.Report_Event_Recorder] != 1 or \
                self.params[ConfigKeys.Report_Coital_Acts] != 1:
            self.success = False
            self.msg.append(f'BAD: please set {ConfigKeys.Report_Event_Recorder} and '
                            f'{ConfigKeys.Report_Coital_Acts} to 1 in '
                            f'{self.config_filename}.\n')
        else:
            self.msg.append("parse report event recorder\n")
            self.parse_report_event_recorder()
            csv_event_reporter = self.csv.df
            # csv_event_reporter = csv_event_reporter[["Individual_ID", "Event_Name", "Risk", "InterventionStatus"]]

            self.msg.append("parse RelationshipConsummated.csv\n")
            relationship_consummated_df = CsvOutput(file=os.path.join(self.output_folder, "RelationshipConsummated.csv")).df

            self.msg.append("Testing RelationshipConsummated with: start year and end year:\n")
            min_day = min(relationship_consummated_df['Time'])
            self.msg.append(f"The earliest date we start listening is {min_day}.\n")
            if min_day / 365 + base_year < start_year:
                self.success = False
                self.msg.append(f"\tBAD: We start listening in RelationshipConsummated at {min_day / 365 + base_year} "
                                f"which is before {ConfigKeys.Report_Coital_Acts_Start_Year} = {start_year}.\n")
            elif not math.isclose(min_day / 365 + base_year,  start_year, abs_tol=0.02):
                self.success = False
                self.msg.append(f"\tBAD: We did not start listening in RelationshipConsummated at "
                                f"{ConfigKeys.Report_Coital_Acts_Start_Year} = {start_year}, the earlier time in report"
                                f"is {min_day / 365 + base_year} .\n")
            else:
                self.msg.append(f"\tGOOD: We start listening in RelationshipConsummated at "
                                f"{ConfigKeys.Report_Coital_Acts_Start_Year} = {start_year}.\n")

            max_day = max(relationship_consummated_df['Time'])
            self.msg.append(f"The latest date we end listening is {max_day}.\n")
            if max_day / 365 + base_year > end_year:
                self.success = False
                self.msg.append(f"\tBAD: We stop listening in RelationshipConsummated at {max_day / 365 + base_year} "
                                f"which is after {ConfigKeys.Report_Coital_Acts_End_Year} = {end_year}.\n")
            elif not math.isclose(max_day / 365 + base_year,  end_year, abs_tol=0.02):
                self.success = False
                self.msg.append(f"\tBAD: We did not stop listening in RelationshipConsummated at "
                                f"{ConfigKeys.Report_Coital_Acts_End_Year} = {end_year}, the latest time in report "
                                f"is {max_day / 365 + base_year}.\n")
            else:
                self.msg.append(f"\tGOOD: We stop listening in RelationshipConsummated at "
                                f"{ConfigKeys.Report_Coital_Acts_End_Year} = {end_year}.\n")

            self.msg.append("Testing RelationshipConsummated with: min age and max age:\n")
            result = True
            for index, row in relationship_consummated_df.iterrows():
                a_min_age_at_relationship = row['A_Age']
                b_min_age_at_relationship = row['B_Age']
                if (not min_age < a_min_age_at_relationship < max_age) and \
                    (not min_age < b_min_age_at_relationship < max_age):
                    self.success = result = False
                    r_id = row['Rel_ID']
                    self.msg.append(
                        f"\tBAD: We report A_Age = {a_min_age_at_relationship} and B_Age = "
                        f"{b_min_age_at_relationship} in RelationshipConsummated(Rel_ID = {r_id}) which do not match "
                        f"{ConfigKeys.Report_Coital_Acts_Min_Age_Years} = {min_age} and "
                        f"{ConfigKeys.Report_Coital_Acts_Max_Age_Years} = {max_age}.\n")
            if result:
                self.msg.append(
                    f"\tGOOD: A_Age and B_Age in RelationshipConsummated match "
                    f"{ConfigKeys.Report_Coital_Acts_Min_Age_Years} = {min_age} and "
                    f"{ConfigKeys.Report_Coital_Acts_Max_Age_Years} = {max_age}.\n")

            self.msg.append("Testing RelationshipConsummated with: must_have_intervention:\n")
            if any(relationship_consummated_df["A_IP='Risk'"] != "HIGH"):
                self.success = False
                self.msg.append(
                    f"\tBAD: {ConfigKeys.Report_Coital_Acts_Must_Have_Intervention} is set to "
                    f"{must_have_intervention}, we should only report A_IP='Risk' = 'HIGH' which is the property of the"
                    f" group that got the intervention.\n")
            else:
                self.msg.append(
                    f"\tGOOD: {ConfigKeys.Report_Coital_Acts_Must_Have_Intervention} is set to "
                    f"{must_have_intervention}, we only report A_IP='Risk' = 'HIGH' which is the property of the"
                    f" group that got the intervention.\n")

            self.msg.append("Testing RelationshipConsummated with: Must_Have_IP_Key_Value:\n")
            result = True
            for index, row in relationship_consummated_df.iterrows():
                a_intervention_status = row["A_IP='InterventionStatus'"]
                b_intervention_status = row["B_IP='InterventionStatus'"]
                if (not a_intervention_status == "Monitor") and \
                    (not b_intervention_status == "Monitor"):
                    self.success = result = False
                    r_id = row['Rel_ID']
                    self.msg.append(
                        f"\tBAD: We report (A_IP='InterventionStatus') = {a_intervention_status} and "
                        f"(B_IP='InterventionStatus') = {b_intervention_status} in RelationshipConsummated(Rel_ID = {r_id}) "
                        f"which do not match {ConfigKeys.Report_Coital_Acts_Must_Have_IP_Key_Value} = "
                        f"{ip_control}.\n")
            if result:
                self.msg.append(
                    f"\tGOOD: at least one of (A_IP='InterventionStatus') and (B_IP='InterventionStatus') in "
                    f"RelationshipConsummated is Monitor, which match "
                    f"{ConfigKeys.Report_Coital_Acts_Must_Have_IP_Key_Value} = "
                    f"{ip_control}.\n")

            self.msg.append("Testing RelationshipConsummated with: Node_IDs_Of_Interest:\n")
            if any(relationship_consummated_df["Node_ID"] != node_ids[0]):
                reported_node_ids = relationship_consummated_df["Node_ID"].unique()
                self.success = False
                self.msg.append(
                    f"\tBAD: {ConfigKeys.Report_Coital_Acts_Node_IDs_Of_Interest} is set to "
                    f"{node_ids}, we should only report Node_ID in {node_ids}, but we get node ids = "
                    f"{reported_node_ids} in the report.\n")
            else:
                self.msg.append(
                    f"\tGOOD: {ConfigKeys.Report_Coital_Acts_Node_IDs_Of_Interest} is set to "
                    f"{node_ids}, we only report Node_ID in {node_ids}.\n")

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
