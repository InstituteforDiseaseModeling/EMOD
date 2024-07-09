#!/usr/bin/python
if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append(str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )


import json
import os.path as path
import dtk_test.dtk_sft as sft
import dtk_test.dtk_ICP_Support as icp_s
import dtk_test.dtk_STI_Support as sti_s
import matplotlib.pyplot as plt
import pandas as pd
import math
import numpy as np
from collections import Counter

"""
InterventionForCurrentPartners\CHOSEN_AT_RANDOM
This test is testing the InterventionForCurrentPartners with Prioritize_Partners_By set to CHOSEN_AT_RANDOM.

This test is configured with concurrent relationships. It has 7 ICP interventions in campaign.json. Each one is has the
same configuration except:
1. Prioritize_Partners_By is set to all 7 types of priorities including CHOSEN_AT_RANDOM.
2. Broadcast_Event is set to 7 different event names.

Data for test is loaded from ReportEventRecorder.csv, it performs the following tests:
1. Check the number of expected Broadcast_Event for CHOSEN_AT_RANDOM.
2. Check the average age of male individuals who received the CHOSEN_AT_RANDOM event is between the average age of
    YOUNGER_AGE and OLDER_AGE.
3. Check the male ids who received the CHOSEN_AT_RANDOM event should be different with all other types of priority.
4. Plot histogram for age for each priories, they should be different.

Output:
    scientific_feature_report.txt
    CHOSEN_AT_RANDOM.png
    LONGER_TIME_IN_RELATIONSHIP.png
    NO_PRIORITIZATION.png
    OLDER_AGE.png
    RELATIONSHIP_TYPE.png
    SHORTER_TIME_IN_RELATIONSHIP.png
    YOUNGER_AGE.png

"""


def compare_with_relationship_distribution(random_df, start_df, output_report_file):
    """
    CHOSEN_AT_RANDOM events are given to each of the relationship types, and the distribution across relationship
    types is close to the distribution of the relationship types.
    :param random_df:
    :param start_df:
    :param output_report_file:
    :return:
    """
    succeed = True
    # get the distribution for relationship type from start_df
    start_df_c = df_get_count_and_percentage(start_df, sti_s.ReportHeader.rel_type)

    # add relationship type column to random_df based on male age.
    random_df[icp_s.Constant.relationship_types] = random_df[icp_s.Constant.age].apply(
        lambda x: find_relationship_from_age(x))
    # groupby relationship type and get the count/percentage for random_df
    random_df_c = df_get_count_and_percentage(random_df, icp_s.Constant.relationship_types)

    # compare the two data frames:
    counts_to_compare = {"relationship distribution": [],
                         "CHOSEN_AT_RANDOM distribution": [],
                         "CHOSEN_AT_RANDOM distribution percentage": [],
                         "legend": []}
    for rel_type in start_df_c[sti_s.ReportHeader.rel_type]:
        relationship_string = sti_s.relationship_table[rel_type]
        if relationship_string not in list(random_df_c[icp_s.Constant.relationship_types]):
            succeed = False
            output_report_file.write(f"BAD: can't find {relationship_string} relationship for CHOSEN_AT_RANDOM event.\n")
        else:
            counts_to_compare["relationship distribution"].append(
                start_df_c[start_df_c[sti_s.ReportHeader.rel_type] == rel_type]
                [sti_s.ReportHeader.percent].iloc[0])
            counts_to_compare["CHOSEN_AT_RANDOM distribution"].append(
                random_df_c[random_df_c[icp_s.Constant.relationship_types] == relationship_string]
                [sti_s.ReportHeader.counts].iloc[0])
            counts_to_compare["CHOSEN_AT_RANDOM distribution percentage"].append(
                random_df_c[random_df_c[icp_s.Constant.relationship_types] == relationship_string]
                [sti_s.ReportHeader.percent].iloc[0])
            counts_to_compare["legend"].append(relationship_string)
    result = sft.test_multinomial(counts_to_compare["CHOSEN_AT_RANDOM distribution"],
                                  counts_to_compare["relationship distribution"],
                                  report_file=output_report_file, prob_flag=True)
    if not result:
        succeed = False

    # plot compared percentages
    sft.plot_bar_graph(data=[counts_to_compare["CHOSEN_AT_RANDOM distribution percentage"],
                             counts_to_compare["relationship distribution"]],
                       xticklabels=counts_to_compare["legend"],
                       x_label="Relationship Types",
                       y_label="Percentage",
                       legends=["CHOSEN_AT_RANDOM distribution(actual)", "relationship distribution(expected)"],
                       plot_name="actual_vs_expected_distribution",
                       show=True,
                       num_decimal=2)
    return succeed


def find_relationship_from_age(age):
    # return relationship type based on male age defined in demo file.
    relationship_table_male = {sti_s.DemoPfaKeys.transitory: 20,
                               sti_s.DemoPfaKeys.informal: 30,
                               sti_s.DemoPfaKeys.marital: 40,
                               sti_s.DemoPfaKeys.commercial: 50}
    age /= sft.DAYS_IN_YEAR
    if age < relationship_table_male[sti_s.DemoPfaKeys.transitory]:
        return -1
    elif relationship_table_male[sti_s.DemoPfaKeys.transitory] <= age < \
            relationship_table_male[sti_s.DemoPfaKeys.informal]:
        return sti_s.DemoPfaKeys.transitory
    elif relationship_table_male[sti_s.DemoPfaKeys.informal] <= age < \
            relationship_table_male[sti_s.DemoPfaKeys.marital]:
        return sti_s.DemoPfaKeys.informal
    elif relationship_table_male[sti_s.DemoPfaKeys.marital] <= age < \
            relationship_table_male[sti_s.DemoPfaKeys.commercial]:
        return sti_s.DemoPfaKeys.marital
    elif relationship_table_male[sti_s.DemoPfaKeys.commercial] <= age:
        return sti_s.DemoPfaKeys.commercial


def df_get_count_and_percentage(df, groupby_column):
    # groupby and get the count
    df_c = df.groupby(groupby_column). \
        size().reset_index(name=sti_s.ReportHeader.counts)
    # calculate the percentage.
    df_c[sti_s.ReportHeader.percent] = df_c[sti_s.ReportHeader.counts] / df_c[
        sti_s.ReportHeader.counts].sum()
    return df_c


def create_report_file(output_folder, report_event_recorder, relationship_start_report, config_filename,
                       output_report_name):
    succeed = True
    with open(output_report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {sft.get_config_name(config_filename)}\n")
        # load data from RelationshipStart.csv
        start_df = sti_s.parse_relationship_start_report(report_path=output_folder,
                                                         report_filename=relationship_start_report)
        # plot male age histogram groupby relationship type
        start_df.groupby(sti_s.ReportHeader.rel_type)[sti_s.ReportHeader.a_age].plot.hist(legend=True)
        plt.title(f"Age vs. Rel Types(from {relationship_start_report})")
        plt.xlabel("Age")
        plt.legend(sti_s.relationship_table.values())
        plt.savefig("age_vs_reltype-histogram.png")
        if sft.check_for_plotting():
            plt.show()
        plt.close()

        # read report event recorder into dataframe
        report_df = pd.read_csv(path.join(output_folder, report_event_recorder))

        # a dictionary for test data
        data_to_test = {icp_s.Events.CHOSEN_AT_RANDOM: {},
                        icp_s.Events.OLDER_AGE: {},
                        icp_s.Events.YOUNGER_AGE: {},
                        icp_s.Events.LONGER_TIME_IN_RELATIONSHIP: {},
                        icp_s.Events.SHORTER_TIME_IN_RELATIONSHIP: {},
                        icp_s.Events.NO_PRIORITIZATION: {},
                        icp_s.Events.RELATIONSHIP_TYPE: {}
                        }
        for event_name in data_to_test.keys():
            # filter by event name for different campaigns
            data_to_test[event_name]["df"] = report_df[report_df[icp_s.Constant.event_name] == event_name]
            # count event from filtered dataframe
            data_to_test[event_name]["event_count"] = len(data_to_test[event_name]["df"])
            output_report_file.write(f"{data_to_test[event_name]['event_count']} {event_name} events are broadcast.\n")
            # calculate average age for each event
            data_to_test[event_name]["avg_age"] = data_to_test[event_name]["df"]['Age'].mean()/sft.DAYS_IN_YEAR
            output_report_file.write(f"average age for {event_name} event is {data_to_test[event_name]['avg_age']}.\n")
            # collect male ids for each event
            data_to_test[event_name]["male_ids"] = sorted(data_to_test[event_name]["df"]['Individual_ID'].tolist())


        count_expected_num_event = Counter([data_to_test[event_name]["event_count"]
                                            for event_name in data_to_test.keys()
                                            if event_name not in [icp_s.Events.NO_PRIORITIZATION,
                                                                  icp_s.Events.CHOSEN_AT_RANDOM]])
        if len(count_expected_num_event) > 1:
            output_report_file.write(f"WARNING: the number of event for {icp_s.Events.OLDER_AGE}, "
                                     f"{icp_s.Events.YOUNGER_AGE}, "
                                     f"{icp_s.Events.LONGER_TIME_IN_RELATIONSHIP}, "
                                     f"{icp_s.Events.SHORTER_TIME_IN_RELATIONSHIP} and "
                                     f"{icp_s.Events.RELATIONSHIP_TYPE} "
                                     f"should be the same but they are not.\n")
        # test number of event for CHOSEN_AT_RANDOM
        expected_num_event = count_expected_num_event.most_common(1)[0][0]

        msg = f"{data_to_test[icp_s.Events.CHOSEN_AT_RANDOM]['event_count']} " \
              f"{icp_s.Events.CHOSEN_AT_RANDOM} events are broadcast, " \
              f"while {expected_num_event} events are expected.\n"
        if expected_num_event != data_to_test[icp_s.Events.CHOSEN_AT_RANDOM]['event_count']:
            succeed = False
            output_report_file.write("BAD: " + msg)
        else:
            output_report_file.write("GOOD: " + msg)

        # test average age for CHOSEN_AT_RANDOM
        avg_age_random = data_to_test[icp_s.Events.CHOSEN_AT_RANDOM]['avg_age']
        avg_age_younger = data_to_test[icp_s.Events.YOUNGER_AGE]['avg_age']
        avg_age_older = data_to_test[icp_s.Events.OLDER_AGE]['avg_age']
        msg = f"average age for {icp_s.Events.CHOSEN_AT_RANDOM} event is" \
              f" {avg_age_random}, expected between( " \
              f"{avg_age_younger}, " \
              f"{avg_age_older}) which are the" \
              f" average ages for {icp_s.Events.YOUNGER_AGE} and {icp_s.Events.OLDER_AGE} respectively.\n"
        if avg_age_younger < avg_age_random < avg_age_older:
            output_report_file.write("GOOD: " + msg)
        else:
            succeed = False
            output_report_file.write("BAD: " + msg)

        # test male ids for CHOSEN_AT_RANDOM are different compared to all other types
        male_ids = data_to_test[icp_s.Events.CHOSEN_AT_RANDOM]["male_ids"]
        for event_name in data_to_test.keys():
            if event_name == icp_s.Events.CHOSEN_AT_RANDOM: # skip CHOSEN_AT_RANDOM
                continue
            male_ids_to_compare = data_to_test[event_name]["male_ids"]
            if male_ids == male_ids_to_compare:
                succeed = False
                output_report_file.write(f"BAD: male ids for {icp_s.Events.CHOSEN_AT_RANDOM} are the same as "
                                         f"{event_name}, they should be different.\n")
            else:
                output_report_file.write(f"GOOD male ids for {icp_s.Events.CHOSEN_AT_RANDOM} are not the same as "
                                         f"{event_name}.\n")

        output_report_file.write("One more thing to check: CHOSEN_AT_RANDOM events are given to each of the "
                                 "relationship types, and the distribution across relationship types is close to "
                                 "the distribution of the relationship types.\n")
        # get the distribution for relationship type from event_df filtered by CHOSEN_AT_RANDOM event
        random_df = data_to_test[icp_s.Events.CHOSEN_AT_RANDOM]["df"]
        result = compare_with_relationship_distribution(random_df, start_df, output_report_file)
        if not result:
            succeed = False

        output_report_file.write(sft.format_success_msg(succeed))

        # create hist plot for age for all events
        for plot_name in data_to_test.keys():
            df = data_to_test[plot_name]['df']
            df['Age'] = df['Age'] / sft.DAYS_IN_YEAR
            df.plot(y='Age', kind='hist')
            plt.title(plot_name)
            plt.savefig(f"{plot_name}.png")
            if sft.check_for_plotting():
                plt.show()
            plt.close()

    return succeed


def application(output_folder="output", stdout_filename="test.txt",
                relationship_start_report="RelationshipStart.csv",
                report_event_recorder="ReportEventRecorder.csv",
                config_filename="config.json",
                output_report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("relationship_start_report: " + relationship_start_report + "\n")
        print("report_event_recorder: " + report_event_recorder + "\n")
        print("config_filename: " + config_filename + "\n")
        print("output_report_name: " + output_report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done(stdout_filename)
    create_report_file(output_folder, report_event_recorder, relationship_start_report, config_filename,
                       output_report_name)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-S', '--start_report', default="RelationshipStart.csv",
                        help="Relationship start report to parse (RelationshipStart.csv)")
    parser.add_argument('-e', '--event_report', default="ReportEventRecorder.csv",
                        help="Report Event Recorder to parse (ReportEventRecorder.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                relationship_start_report=args.start_report,
                report_event_recorder=args.event_report,
                config_filename=args.config,
                output_report_name=args.reportname, debug=args.debug)

