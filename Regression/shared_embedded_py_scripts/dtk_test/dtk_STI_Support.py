import os
import pandas as pd
import dtk_test.dtk_sft as dtk_sft
import json


class DemoPfaKeys():
    defaults = "Defaults"
    society = "Society"
    transitory = "TRANSITORY"
    informal = "INFORMAL"
    marital = "MARITAL"
    commercial = "COMMERCIAL"
    # PFA
    pair_formation_parameters = "Pair_Formation_Parameters"
    joint_probabilities = "Joint_Probabilities"
    first_agebin_male = "Age_of_First_Bin_Edge_Male"
    first_agebin_female = "Age_of_First_Bin_Edge_Female"
    num_agebin_male = "Number_Age_Bins_Male"
    num_agebin_female = "Number_Age_Bins_Female"
    year_bt_bin_male = "Years_Between_Bin_Edges_Male"
    year_bt_bin_female = "Years_Between_Bin_Edges_Female"
    formation_rate_constant = "Formation_Rate_Constant"
    # relationship
    relationship_parameters = "Relationship_Parameters"
    heterogeneity = "Duration_Weibull_Heterogeneity"
    scale = "Duration_Weibull_Scale"
    # Concurrency_Parameters
    concurrency_parameters = "Concurrency_Parameters"
    max_rel_female = "Max_Simultaneous_Relationships_Female"
    max_rel_male = "Max_Simultaneous_Relationships_Male"
    p_extra_rel_female = "Prob_Extra_Relationship_Female"
    p_extra_rel_male = "Prob_Extra_Relationship_Male"


class ReportHeader():
    rel_type = "Rel_type"
    rel_ID = "Rel_ID"
    a_age = "A_age"
    a_agebin = "A_agebin"
    b_age = "B_age"
    b_agebin = "B_agebin"
    percent = "Percent"
    counts = "counts"
    actual_duration = "Actual_duration"
    scheduled_duration = "Scheduled_duration"
    scheduled_duration_year = "Scheduled_duration_year"
    rel_actual_end_time = "Rel_actual_end_time"
    rel_start_time = "Rel_start_time"
    rel_scheduled_end_time = "Rel_scheduled_end_time"
    termination_reason = "Termination_Reason"
    brokeup = "BROKEUP"
    a_ID = "A_ID"
    b_ID = "B_ID"
    male_ID = "male_ID"
    female_ID = "female_ID"
    a_is_infected = "A_is_infected"
    b_is_infected = "B_is_infected"
    type = "Type"


relationship_table = {0: DemoPfaKeys.transitory, 1: DemoPfaKeys.informal, 2: DemoPfaKeys.marital, 3: DemoPfaKeys.commercial}
inv_relationship_table = {v: k for k, v in relationship_table.items()}


def parse_relationship_start_report(report_path="output", report_filename="RelationshipStart.csv"):
    """
        parse start relationship report and return the information about relationship type and durations as a data frame
    Args:
        report_path: file path of RelationshipStart.csv
        report_filename: file name of RelationshipStart.csv

    Returns: report_df

    """
    report_file = os.path.join(report_path, report_filename)
    report_df = pd.read_csv(report_file)
    report_df[ReportHeader.rel_type] = report_df.filter(like=ReportHeader.rel_type)
    report_df[ReportHeader.scheduled_duration] = report_df[ReportHeader.rel_scheduled_end_time] - \
                                                 report_df[ReportHeader.rel_start_time]
    report_df[ReportHeader.scheduled_duration_year] = report_df[ReportHeader.scheduled_duration] / dtk_sft.DAYS_IN_YEAR
    return report_df


def parse_relationship_end_report(report_path="output", report_filename="RelationshipEnd.csv"):
    """
        parse end relationship report and return the information about relationship type and durations as a data frame
    Args:
        report_path: file path of RelationshipEnd.csv
        report_filename: file name of RelationshipEnd.csv

    Returns: report_df

    """
    report_file = os.path.join(report_path, report_filename)
    report_df = pd.read_csv(report_file)
    report_df[ReportHeader.rel_type] = report_df.filter(like=ReportHeader.rel_type)
    report_df[ReportHeader.actual_duration] = report_df[ReportHeader.rel_actual_end_time] - \
                                              report_df[ReportHeader.rel_start_time]
    report_df[ReportHeader.scheduled_duration] = report_df[ReportHeader.rel_scheduled_end_time] - \
                                                 report_df[ReportHeader.rel_start_time]
    report_df[ReportHeader.scheduled_duration_year] = report_df[ReportHeader.scheduled_duration] / dtk_sft.DAYS_IN_YEAR
    # report_df = report_df[report_df[ReportHeader.termination_reason] == ReportHeader.brokeup]

    return report_df


def parse_demo_pfa_file(demo_file_name="pfa_joint_probabilities.json"):
    """
    Parse demo file and get Joint_Probabilities for transitory, informal and
    marital relationships.
    :param demo_file_name:
    :return: demo_object: {relationship_name: dictionary of Joint_Probabilities}
    """
    with open(demo_file_name, 'r') as demo_file:
        society = json.load(demo_file)[DemoPfaKeys.defaults][DemoPfaKeys.society]
        demo_object = {}
        for rel_name in inv_relationship_table:
            pfa_params = society[rel_name][DemoPfaKeys.pair_formation_parameters]
            param = {DemoPfaKeys.joint_probabilities:
                         pfa_params[DemoPfaKeys.joint_probabilities],
                     DemoPfaKeys.first_agebin_male:
                         pfa_params[DemoPfaKeys.first_agebin_male],
                     DemoPfaKeys.first_agebin_female:
                         pfa_params[DemoPfaKeys.first_agebin_female],
                     DemoPfaKeys.num_agebin_male:
                         pfa_params[DemoPfaKeys.num_agebin_male],
                     DemoPfaKeys.num_agebin_female:
                         pfa_params[DemoPfaKeys.num_agebin_female],
                     DemoPfaKeys.year_bt_bin_male:
                         pfa_params[DemoPfaKeys.year_bt_bin_male],
                     DemoPfaKeys.year_bt_bin_female:
                         pfa_params[DemoPfaKeys.year_bt_bin_female]
                     }
            demo_object[rel_name] = param

    return demo_object


def parse_demo_max_relationship(demo_file_name="pfa_joint_probabilities.json"):
    """
    Parse demo file and get Concurrency_Parameters for transitory, informal and
    marital relationships.
    :param demo_file_name:
    :return: demo_object: {relationship_name: dictionary of Concurrency_Parameters}
    """
    with open(demo_file_name, 'r') as demo_file:
        society = json.load(demo_file)[DemoPfaKeys.defaults][DemoPfaKeys.society]
        demo_object = {}
        for rel_name in inv_relationship_table:
            cc_params = society[rel_name][DemoPfaKeys.concurrency_parameters]["NONE"]
            param = {DemoPfaKeys.max_rel_female:
                         cc_params[DemoPfaKeys.max_rel_female],
                     DemoPfaKeys.max_rel_male:
                         cc_params[DemoPfaKeys.max_rel_male],
                     DemoPfaKeys.p_extra_rel_female:
                         cc_params[DemoPfaKeys.p_extra_rel_female],
                     DemoPfaKeys.p_extra_rel_male:
                         cc_params[DemoPfaKeys.p_extra_rel_male]
                     }
            demo_object[rel_name] = param

    return demo_object

