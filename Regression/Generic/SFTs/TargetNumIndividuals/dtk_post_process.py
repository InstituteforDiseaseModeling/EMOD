#!/usr/bin/python

import json
import os.path as path
import dtk_test.dtk_sft as sft
import pandas as pd

"""
This test verified  "Individual_Selection_Type": "TARGET_NUM_INDIVIDUALS" parameter works. There are several
scenarios set up in the campaign file, this post-process checks that the correct number of folks are infected
with those scenarios. It also verifies that folks are being about equally infected in all the nodes. 
"""


def check_report(df=None, day=None, expected_infected=None, gender_restrictions=None, age_restrictions=None,
                 outfile=None, check_distribution_per_node=False):
    """
        Gets number of infections from the report according to the restrictions passed in and compares to the
        expected number of infections
    Args:
        df: DataFrame with the report data
        day: Day or days at which we are looking (list or int)
        expected_infected: Expected infections (from campaign)
        gender_restrictions: if there are gender restrictions in the campaign "M"/"F", default is None
        age_restrictions: if there are are restrictions in the campaign
        outfile: name of the scientific_feature_report.txt for logging purposes
        check_distribution_per_node: tests that infected people are evenly distributed between notes
    Returns:
        True/False whether the expected and actual infections are equal.

    """
    if type(day) is not list:
        day = [day]
    df = df[(df["Time"].isin(day))]
    if gender_restrictions:
        df = df[(df["Gender"] == gender_restrictions)]
    if age_restrictions:
        df = df[(df["AgeYears"].isin(age_restrictions))]
    if check_distribution_per_node:
        success = True
        gr_df = df.groupby(["NodeID"]).sum()
        total_inf = gr_df["NumInfected"].sum()
        close_enough = 0.05 * total_inf  # let's say within 5% is good enough
        expected = total_inf / len(gr_df.index)
        for x, infected in enumerate(gr_df["NumInfected"]):
            if abs(infected - expected) < close_enough:
                outfile.write(
                    f"GOOD: Expected number of infected for NodeID {x} is about {expected} and it was {infected}, "
                    f"which is close enough.\n")
            else:
                success = False
                outfile.write(
                    f"BAD: Expected number of infected for NodeID {x} is about {expected} and it was {infected}, "
                    f"which is not close enough.\n")
        return success
    else:
        actual_infected = df["NumInfected"].sum()
        if actual_infected == expected_infected:
            outfile.write(f"GOOD: Expected number of infections for day {day} and actual infections "
                          f"equals {expected_infected}.\n")
            return True
        else:
            outfile.write(f"BAD: Expected number of infections for day {day} is {expected_infected}, "
                          f"but found {actual_infected}.\n")
            return False


def get_reportnodedemograhics(report_path, debug=False):
    """
        Converts the ReportNodeDemographics.csv into a dataframe, cleans up leading spaces from column names,
    sets "Time" as one of the columns instead of the index.
    Args:
        report_path: path to the report

    Returns:
        DataFrame with the data from the report
    """
    data = pd.read_csv(report_path)
    rename_dict = {}
    for column in data.columns:
        new_column = column.strip()
        rename_dict[column] = new_column
    data = data.rename(columns=rename_dict)
    if debug:
        data.to_csv("DEBUG_data.csv")
    return data


def create_report_file(output_folder, reportnodedemographics, config_filename, report_name, debug):
    with open(report_name, "w") as outfile:
        with open(config_filename) as config:
            config_name = json.load(config)["parameters"]["Config_Name"]
        outfile.write(f"Starting test for {config_name}: \n")
        success = True
        report_path = path.join(output_folder, reportnodedemographics)
        data = get_reportnodedemograhics(report_path, debug)
        if not check_report(df=data, day=2, expected_infected=125, outfile=outfile):
            success = False
        # check that there's about the same number in each node:
        if not check_report(df=data, day=5, expected_infected=12, gender_restrictions="M", outfile=outfile):
            success = False
        if not check_report(df=data, day=8, expected_infected=19, gender_restrictions="F", outfile=outfile):
            success = False
        if not check_report(df=data, day=11, expected_infected=50, gender_restrictions="M", age_restrictions=[50],
                            outfile=outfile):
            success = False
        if not check_report(df=data, day=14, expected_infected=74, age_restrictions=[50], outfile=outfile):
            success = False

        days = []
        for x in range(20):
            days.append(x * 3 + 17)
        if not check_report(df=data, day=days, outfile=outfile, check_distribution_per_node=True):
            success = False

        outfile.write(sft.format_success_msg(success))
    return success


def application(output_folder="output", config_filename="config.json",
                reportnodedemographics="ReportNodeDemographics.csv",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("reportnodedemographics: " + reportnodedemographics + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    # sft.wait_for_done()
    create_report_file(output_folder, reportnodedemographics, config_filename, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    application("output")
