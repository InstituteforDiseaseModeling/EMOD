#!/usr/bin/python

import dtk_test.dtk_sft as sft
import pandas as pd
import json
import os.path as path


"""
This test operates thusly:
1. The immigration rates are defined in the 3x3_Age_Gender_Local.json
    A. Node layout  N
                  1 2 3
                W 4 5 6 E
                  7 8 9
                    S
    B. Age Structure: All agents are in one of three age buckets
      i.   20 - 20.1 years (young)
      ii.  40 - 40.1 years
      iii. 60 - 60.1 years (old)
    C. Migration rate calculation. Add the following:
      i.   Base migration probability to any neighbor node is 0.04
      ii.  Young people add 0.02 to Western migration, -0.01 to Eastern
           Old people add 0.02 to Eastern migration, -0.01 to Western
      iii. Males add 0.03 to Northern migration, -0.01 to Southern
           Females add 0.03 to Southern migration, -0.01 to Northern
2. The other immigration files (*.bin, *.bin.json) were generated with
 the files in /Scripts/MigrationTools
3. Enable ReportNodeDemographics in custom_reports.json
4. Run sim for 400 days
5. After which
    A. Northern nodes should have more men
    B. Southern nodes should have more women
    C. Eastern nodes should have more old people
    D. Western nodes should have more young people
6. The migration binaries ONLY exist in this folder, validating DtkTrunk issue 2166
"""


def application(output_folder="output", csv_name="ReportNodeDemographics.csv", config_filename="config.json",
                report_name=sft.sft_output_filename, debug=False):
    if debug:
        print("output_folder: " + output_folder + "\n")
        print("csv_name: " + csv_name + "\n")
        print("config_name: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()

    with open(config_filename) as infile:
        test_config_name = json.load(infile)["parameters"]["Config_Name"]
    sft.start_report_file(report_name, test_config_name)

    all_data_df = pd.read_csv(path.join(output_folder, csv_name))
    all_data_df.rename(columns={' NodeID': 'NodeID', ' Gender': 'Gender',
                                ' AgeYears': 'AgeYears', ' NumIndividuals': 'NumIndividuals',
                                ' NumInfected': 'NumInfected'}, inplace=True)
    all_data_df.drop(['NumInfected'], axis=1, inplace=True)
    if debug:
        all_data_df.to_csv("DEBUG_all_data_cleaned.csv")

    last_day_df = all_data_df.loc[all_data_df['Time']==399]
    if debug:
        last_day_df.to_csv("DEBUG_day_399_data_cleaned.csv")
    northern_nodes = last_day_df[last_day_df['NodeID'].isin([1,2,3])]
    southern_nodes = last_day_df[last_day_df['NodeID'].isin([7,8,9])]
    eastern_nodes = last_day_df[last_day_df['NodeID'].isin([3,6,9])]
    western_nodes = last_day_df[last_day_df['NodeID'].isin([1,4,7])]

    create_report_file(sft_filename=report_name, n_nodes=northern_nodes,
                       s_nodes=southern_nodes, e_nodes=eastern_nodes,
                       w_nodes=western_nodes)

def create_report_file(n_nodes, s_nodes, e_nodes, w_nodes,
                       debug=False, sft_filename=sft.sft_output_filename):
    with open(sft_filename, "a") as report_file:
        success = True

        old_east_count = e_nodes[e_nodes['AgeYears'] == 62]['NumIndividuals'].sum()
        old_west_count = w_nodes[w_nodes['AgeYears'] == 62]['NumIndividuals'].sum()
        young_east_count = e_nodes[e_nodes['AgeYears'] == 22]['NumIndividuals'].sum()
        young_west_count = w_nodes[w_nodes['AgeYears'] == 22]['NumIndividuals'].sum()
        report_file.write("Old people should go east.\n")
        report_file.write(f"There were {old_east_count} E old people and {old_west_count} W old.\n")
        report_file.write(f"There were {old_east_count} E old peple and {young_east_count} E young.\n")
        if old_east_count <= old_west_count:
            report_file.write(f"BAD: There were {old_east_count} E old people and {old_west_count} W old.\n")
            success = False
        if old_east_count <= young_east_count:
            report_file.write(f"BAD: There were {old_east_count} E old peple and {young_east_count} E young.\n")
            success = False
        report_file.write("Young people should go west.\n")
        report_file.write(f"There were {young_west_count} W young people and {young_east_count} E young.\n")
        report_file.write(f"There were {young_west_count} W young people and {old_west_count} W old.\n")
        if young_west_count <= young_east_count:
            report_file.write(f"BAD: There were {young_west_count} W young people and {young_east_count} E young.\n")
            success = False
        if young_west_count <= old_west_count:
            report_file.write(f"BAD: There were {young_west_count} W young people and {old_west_count} W old.\n")
        north_men_count = n_nodes[n_nodes['Gender'] == 'M']['NumIndividuals'].sum()
        north_women_count = n_nodes[n_nodes['Gender'] == 'F']['NumIndividuals'].sum()
        south_men_count = s_nodes[s_nodes['Gender'] == 'M']['NumIndividuals'].sum()
        south_women_count = s_nodes[s_nodes['Gender'] == 'F']['NumIndividuals'].sum()
        report_file.write("Men should go north.\n")
        report_file.write(f"There were {north_men_count} N Men and {north_women_count} N Women.\n")
        report_file.write(f"There were {north_men_count} N Men and {south_men_count} S Men.\n")
        if north_men_count <= north_women_count:
            report_file.write(f"BAD: There were {north_men_count} N Men and {north_women_count} S Women.\n")
            success = False
        if north_men_count <= south_men_count:
            report_file.write(f"BAD: There were {north_men_count} N Men and {south_men_count} S Men.\n")
            success = False
        report_file.write("Women should go south.\n")
        report_file.write(f"There were {south_women_count} S Women and {south_men_count} S Men.\n")
        report_file.write(f"There were {south_women_count} S Women and {north_women_count} N Women.\n")
        if south_women_count <= south_men_count:
            report_file.write(f"BAD: There were {south_women_count} S Women and {south_men_count} S Men.\n")
            success = False
        if south_women_count <= north_women_count:
            report_file.write(f"BAD: There were {south_women_count} S Women and {north_women_count} N Men.\n")
            success = False
        report_file.write(sft.format_success_msg(success))

if __name__ == "__main__":
    # execute only if run as a script
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-m', '--migrationreport', default="ReportNodeDemographics.csv", help="Migration report to load (ReportNodeDemographics.csv)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output, csv_name=args.migrationreport,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)
