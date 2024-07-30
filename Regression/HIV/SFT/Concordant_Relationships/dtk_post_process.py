#!/usr/bin/python
import os
if __name__ == '__main__':
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_STI_Support as sti_support
import json
from scipy import stats
import matplotlib
from sys import platform
if platform == "linux" or platform == "linux2":
    print('Linux OS. Using non-interactive Agg backend')
    matplotlib.use('Agg')
import matplotlib.pyplot as plt
from scipy.stats import gaussian_kde
import numpy as np
import math
import pandas as pd

"""
This test is added based on ticket https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/2585

In this test, we are focus on verifying the Num_Concordant_MARITAL and  HasHIV columns work fine in 
ReportHIVByAgeAndGender.csv. Test data are loaded from output files: ReportEventRecorder.csv, RelationshipStart.csv, 
RelationshipEnd.csv and ReportHIVByAgeAndGender.csv.

It performs the following tasks:
    1. Load 4 reports into 4 dataframes.
    2. Iterate through each reporting period, filter dataframes by reporting time and relationship type = Marital. 
       At each reporting period, does the following:
        2.1. Append new relationships from RelationshipStart.csv in a list stored in relationships dictionary based on 
             gender and hiv status. Also store the relationships in relationship_map(key is male id and value is list of 
             female ids that in any relationship with this male id.)
        2.2. Get new infection reported ReportEventRecorder.csv, store id in infections list. if individual is in any 
             relationship, update relationships dictionary(remove it from relationships[gender]['N'] and add it to 
             relationships[gender]['Y']).
        2.3. Remove relationships found in RelationshipEnd.csv from relationships and relationship_map.
        2.4. Iterate through each rows in ReportHIVByAgeAndGender.csv for current reporting period. Expected 4 rows for 
             4 categories: Males don't have HIV; Males have HIV; Females don't have HIV and Females have HIV, does the 
             following:
                2.4.1  Compare Num_MARITAL column: 
                       Value in Num_MARITAL column should equal to length of the ids list in relationships dictionary 
                       based on gender and HIV status.
                2.4.2  Compare Num_Concordant_MARITAL column:
                       Iterate through list of ids in relationships dictionary based on gender and HIV status, get its 
                       partner's ids from relationship_map and count expected Num_Concordant_MARITAL value. It should 
                       equal to value in Num_Concordant_MARITAL column.
    3. Plot actual values from ReportHIVByAgeAndGender.csv and expected values for each categories along reporting 
       period for both Num_MARITAL and Num_Concordant_MARITAL columns.

Output: scientific_feature_report.txt
        num_marital.png
        num_concordant_marital.png
        
ps: the Simulation_Timestep needs to be large enough so that the change in time can be seen in the two decimals of 
precision in year. Please see https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/4525 for more information.
        
"""


class EventReportHeader:
    year = "Year"
    event_name = "Event_Name"
    individual_id = "Individual_ID"
    gender = "Gender"
    hashiv = "HasHIV"


class HivReportHeader:
    year = "Year"
    gender = " Gender"
    hashiv = " HasHIV"
    num_marital = "Num_MARITAL"
    num_concordant_marital = "Num_Concordant_MARITAL"


class Constant:
    report_hiv_period = "Report_HIV_Period"
    newinfectionevent = "NewInfectionEvent"
    exitedrelationship = "ExitedRelationship"
    enteredrelationship = "EnteredRelationship"
    nondiseasedeaths = "NonDiseaseDeaths"
    diseasedeaths = "DiseaseDeaths"
    base_year = "Base_Year"
    counts = "counts"


def list_index(values: list, value):
    start = -1
    indexes = []
    while True:
        try:
            index = values.index(value, start + 1)
        except ValueError:
            break
        else:
            indexes.append(index)
            start = index
    return indexes


def truncate(n, decimals=0):
    multiplier = 10 ** decimals
    return int(n * multiplier) / multiplier


def round_up(n, decimals=0):
    multiplier = 10 ** decimals
    return math.ceil(n * multiplier) / multiplier


def create_report_file(event_df, hiv_df, start_df, end_df,
                       hiv_report_name, config_filename, report_name):
    succeed = True
    start_df["Year"] = round(start_df[sti_support.ReportHeader.rel_start_time] / dtk_sft.DAYS_IN_YEAR, 2)
    end_df["Year"] = round(end_df[sti_support.ReportHeader.rel_actual_end_time] / dtk_sft.DAYS_IN_YEAR, 2)
    with open(report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")
        report_period = dtk_sft.get_config_parameter(config_filename, Constant.report_hiv_period)[0] / 2
        report_period_year = report_period / dtk_sft.DAYS_IN_YEAR
        base_year = dtk_sft.get_config_parameter(config_filename, Constant.base_year)[0]
        report_step = 1
        simulation_duration_year = dtk_sft.get_simulation_duration(config_filename) / dtk_sft.DAYS_IN_YEAR
        relationships = {'F': {'Y': [],
                               'N': []},
                         'M': {'Y': [],
                               'N': []}}
        relationship_map = {}
        infections = []
        relationship_total = [[] for _ in range(4)]
        relationship_concordant = [[] for _ in range(4)]
        expected_relationship_total = [[] for _ in range(4)]
        expected_relationship_concordant = [[] for _ in range(4)]

        while (report_step + 1) * report_period_year < simulation_duration_year:
            # parse data from 4 csv reports:
            # step 1: collect data from RelationshipStart.csv
            # filter by the reporting period in days and relationship type(2 = MARITAL).
            pre_report_time_in_year = round((report_step - 1) * report_period_year, 2)
            report_time_in_year = round(report_step * report_period_year, 2)
            if report_step == 1:  # include t = 0
                start_df_filtered = start_df[
                    (start_df["Year"] <= report_time_in_year) &
                    (start_df[sti_support.ReportHeader.rel_type] == 2)]  # only look at Marital relationship
            else:
                start_df_filtered = start_df[
                    (pre_report_time_in_year < start_df["Year"]) &
                    (start_df["Year"] <= report_time_in_year) &
                    (start_df[sti_support.ReportHeader.rel_type] == 2)]  # only look at Marital relationship
            for index, row in start_df_filtered.iterrows():
                male_id = row[sti_support.ReportHeader.a_ID]
                male_hasHIV = "N" if row[sti_support.ReportHeader.a_is_infected] == 0 else "Y"
                female_id = row[sti_support.ReportHeader.b_ID]
                female_hasHIV = "N" if row[sti_support.ReportHeader.b_is_infected] == 0 else "Y"
                # add both ids into relationship tables
                relationships['M'][male_hasHIV].append(male_id)
                relationships['F'][female_hasHIV].append(female_id)
                if male_id in relationship_map:
                    relationship_map[male_id].append(female_id)
                else:
                    relationship_map[male_id] = [female_id]

            # step 2: collect data from report event recorder:
            # filter by the reporting period in year, precision is 2
            pre_report_time = round((report_step - 1) * report_period_year + base_year, 2)
            report_time = round(report_step * report_period_year + base_year, 2)
            if report_step == 1:  # include t = 0
                event_df_filtered = event_df[event_df[EventReportHeader.year] <= report_time]
            else:
                event_df_filtered = event_df[(pre_report_time < event_df[EventReportHeader.year]) &
                                             (event_df[EventReportHeader.year] <= report_time)]

            for index, row in event_df_filtered.iterrows():
                event = row[EventReportHeader.event_name]
                id = row[EventReportHeader.individual_id]
                # using RelationshipStart.csv and RelationshipEnd.csv to update the relationships dictionary instead.
                # if event == Constant.enteredrelationship:
                #     relationships[row[EventReportHeader.gender]][row[EventReportHeader.hashiv]].append(id)
                # elif event == Constant.exitedrelationship:
                #     relationships[row[EventReportHeader.gender]][row[EventReportHeader.hashiv]].remove(id)
                # elif event == Constant.newinfectionevent:
                if event == Constant.newinfectionevent:
                    infections.append(id)
                    if id in relationships[row[EventReportHeader.gender]]['N']:
                        relationships[row[EventReportHeader.gender]]['N'].remove(id)
                        relationships[row[EventReportHeader.gender]]['Y'].append(id)
                # comment out the following code.
                # individual that dies and has infection previously will stay in the infections list, don't worry about them.
                # elif event == Constant.diseasedeaths or event == Constant.nondiseasedeaths:
                #     death.append(id)
                #     # if id in relationships[row[EventReportHeader.gender]][row[EventReportHeader.hashiv]]:
                #     #     relationships[row[EventReportHeader.gender]][row[EventReportHeader.hashiv]].remove(id)
                #     if id in infections:
                #         infections.remove(id)

            # step 3: collect data from RelationshipEnd.csv
            # filter by the reporting period in days and relationship type(2 = MARITAL).
            if report_step == 1:  # include t = 0
                end_df_filtered = end_df[
                    (end_df["Year"] <= report_time_in_year) &
                    (end_df[sti_support.ReportHeader.rel_type] == 2)]
            else:
                end_df_filtered = end_df[
                    (pre_report_time_in_year < end_df["Year"]) &
                    (end_df["Year"] <= report_time_in_year) &
                    (end_df[sti_support.ReportHeader.rel_type] == 2)]
            for index, row in end_df_filtered.iterrows():
                male_id = row[sti_support.ReportHeader.male_ID]
                female_id = row[sti_support.ReportHeader.female_ID]
                # get infection status reported in report event recorder
                male_hasHIV = "Y" if male_id in infections else "N"
                female_hasHIV = "Y" if female_id in infections else "N"
                # remove both from table
                relationships['M'][male_hasHIV].remove(male_id)
                relationships['F'][female_hasHIV].remove(female_id)

                # remove female id from relationship map
                relationship_map[male_id].remove(female_id)
                # remove male id if he's not partner with any female
                if not len(relationship_map[male_id]):
                    relationship_map.pop(male_id)

            # step 4: collect data from HIV report and compare with data from the other 3 reports
            # filter by the reporting period in year, precision is 2
            hiv_df_filtered = hiv_df[hiv_df[HivReportHeader.year] == report_time]
            if len(hiv_df_filtered) != 4:
                succeed = False
                output_report_file.write(f"BAD: {hiv_report_name} has {len(hiv_df_filtered)} rows for "
                                         f"{HivReportHeader.year} = {report_time}, expected 4 rows.\n")
            else:
                # step 5: compare data from four csv files for each subcategory(gender, hiv status)
                for i in [0, 1]: # gender: M, F
                    for j in [0, 1]: # hasHiv: N, Y
                        # hiv report data filtered by gender and hiv status
                        hiv_df_filtered_gender_hiv = hiv_df_filtered[(hiv_df_filtered[HivReportHeader.gender] == i) &
                                                                     (hiv_df_filtered[HivReportHeader.hashiv] == j)]
                        gender = 'F' if i == 1 else 'M'
                        has_hiv = 'N' if j == 0 else 'Y'
                        index = i + j * 2  # for saving results into each gender and hiv status category

                        # compare num_marital:
                        relationship_total[index].append(hiv_df_filtered_gender_hiv[HivReportHeader.num_marital].iloc[0]
                                                         if not hiv_df_filtered_gender_hiv.empty
                                                         else 0)
                        expected_relationship_total[index].append(len(relationships[gender][has_hiv]))
                        if relationship_total[index][-1] != expected_relationship_total[index][-1]:
                            succeed = False
                            output_report_file.write(f"BAD: at time: {report_time}, {hiv_report_name} reports "
                                                     f"{relationship_total[index][-1]} for "
                                                     f"{HivReportHeader.num_marital} "
                                                     f"relationships for {HivReportHeader.gender}: {i} and "
                                                     f"{HivReportHeader.hashiv}: {j}, while expected "
                                                     f"{expected_relationship_total[index][-1]}.\n")

                        # compare num_concordant_marital:
                        relationship_concordant[index].append(
                            hiv_df_filtered_gender_hiv[HivReportHeader.num_concordant_marital].iloc[0]
                            if not hiv_df_filtered_gender_hiv.empty
                            else 0)
                        expected_concordant = 0
                        for id in relationships[gender][has_hiv]:
                            if gender == 'M':
                                partner_ids = relationship_map[id]
                            else:
                                partner_ids = []
                                for key in relationship_map.keys():
                                    if id in relationship_map[key]:
                                        partner_ids.append(key)
                            for partner_id in partner_ids:
                                expected_concordant += 1 if has_hiv == 'Y' and partner_id in infections else 0
                                expected_concordant += 1 if has_hiv == 'N' and partner_id not in infections else 0

                        if relationship_concordant[index][-1] != expected_concordant:
                            succeed = False
                            output_report_file.write(f"BAD: at time: {report_time}, {hiv_report_name} reports "
                                                     f"{relationship_concordant[index][-1]} for "
                                                     f"{HivReportHeader.num_concordant_marital} "
                                                     f"relationships for {HivReportHeader.gender}: {i} and "
                                                     f"{HivReportHeader.hashiv}: {j}, while expected "
                                                     f"{expected_concordant}.\n")
                        expected_relationship_concordant[index].append(expected_concordant)

            # move to next reporting period
            report_step += 1
        labels = ['Male_HivNeg', 'Male_HIVPos', 'Female_HIVNeg', 'Male_HIVPos']
        xtick = [base_year + round(report_period_year * i, 2) for i in range(1, report_step)]
        plot_all(relationship_concordant, expected_relationship_concordant,
                 labels=labels, xtick=xtick,
                 title='num_concordant_marital',
                 xlabel='reporting period', ylabel='num_concordant_marital',
                 category='num_concordant_marital')
        plot_all(relationship_total, expected_relationship_total,
                 labels=labels, xtick=xtick,
                 title='num_marital',
                 xlabel='reporting period', ylabel='num_marital',
                 category='num_marital')
        output_report_file.write(dtk_sft.format_success_msg(succeed))

        return succeed


def plot_all(l1, l2, labels, xtick, title=None, xlabel=None, ylabel=None, category='plot_data', show=True):
    if not dtk_sft.check_for_plotting():
        show = False

    fig = plt.figure()
    ax = fig.add_axes([0.12, 0.20, 0.76, 0.74])
    if title:
        ax.set_title(title)
    if xlabel:
        ax.set_xlabel(xlabel)
    if ylabel:
        ax.set_ylabel(ylabel)

    for i in range(len(l1)):
        ax.plot(l1[i], lw=1, label=labels[i])
        plt.plot(l2[i], '--', lw=1, label=labels[i] + 'expected')

    width = 0.35
    x_ind = np.arange(len(xtick))
    ax.set_xticks(x_ind + width / 2)
    ax.set_xticklabels(xtick, rotation=45)

    ax.legend(loc=0)

    fig.savefig(str(category) + '.png')
    if show:
        plt.show()
    plt.close(fig)
    return None


def application(output_folder="output", stdout_filename="test.txt",
                event_report_name="ReportEventRecorder.csv",
                hiv_report_name="ReportHIVByAgeAndGender.csv",
                start_report_name="RelationshipStart.csv",
                end_report_name="RelationshipEnd.csv",
                config_filename="config.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("event_report_name: " + event_report_name + "\n")
        print("start_report_name: " + start_report_name + "\n")
        print("end_report_name: " + end_report_name + "\n")
        print("hiv_report_name: " + hiv_report_name + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done(stdout_filename)
    event_df = pd.read_csv(os.path.join(output_folder, event_report_name))
    hiv_df = pd.read_csv(os.path.join(output_folder, hiv_report_name))
    start_df = sti_support.parse_relationship_start_report(report_path=output_folder, report_filename=start_report_name)
    end_df = sti_support.parse_relationship_end_report(report_path=output_folder, report_filename=end_report_name)
    create_report_file(event_df, hiv_df, start_df, end_df,
                       hiv_report_name, config_filename, report_name)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-e', '--event_report', default="ReportEventRecorder.csv",
                        help="Report Event Recorder to parse (ReportEventRecorder.csv)")
    parser.add_argument('-H', '--HIV_report', default="ReportHIVByAgeAndGender.csv",
                        help="HIV report to parse (ReportHIVByAgeAndGender.csv)")
    parser.add_argument('-S', '--start_report', default="RelationshipStart.csv",
                        help="Relationship start report to parse (RelationshipStart.csv)")
    parser.add_argument('-E', '--end_report', default="RelationshipEnd.csv",
                        help="Relationship end report to parse (RelationshipEnd.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                event_report_name=args.event_report,
                hiv_report_name=args.HIV_report,
                start_report_name=args.start_report,
                end_report_name=args.end_report,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

