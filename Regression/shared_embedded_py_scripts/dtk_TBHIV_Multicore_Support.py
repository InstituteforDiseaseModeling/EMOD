#!/usr/bin/python

import json
import dtk_test.dtk_sft as sft
import os
import pandas as pd
import numpy as np
"""

This is a support library for TBHIV Multicore SFTs

"""


def get_node_counts(demog_filename):
    with open(demog_filename) as infile:
        dgj = json.load(infile)["Nodes"]
    node_list = []
    for node in dgj:
        node_id = node["NodeID"]
        node_list.append(node_id)

    return node_list


def load_emod_parameters(keys, config_filename="config.json", debug=False):
    """reads config file and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_CONFIG_NAME, etc., keys (e.g.)
    """
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {}

    try:
        for key in keys:
            param_obj[key] = cdj[key]

        if debug:
            with open("DEBUG_param_object.json", 'w') as outfile:
                json.dump(param_obj, outfile, indent=4)
    except Exception as ex:
        print("Failed to load param from {0}, got exception: {1}".format(config_filename, ex))

    return param_obj


def load_migration_report(migration_report_filename, output_folder):
    df = None
    try:
        df = pd.read_csv(os.path.join(output_folder, migration_report_filename))
    except Exception as ex:
        print("failed to load {0}, get exception : {1}.\n".format(migration_report_filename, ex))
    return df

def parse_json_report(keys, insetchart_name="InsetChart.json", output_folder="output", debug=False):
    """
    creates report_data_obj structure with keys
    :param insetchart_name: file to parse (InsetChart.json)
    :param output_folder:
    :return: report_data_obj structure, dictionary with KEY_NEW_INFECTION etc., keys (e.g.)
    """
    insetchart_path = os.path.join(output_folder, insetchart_name)
    with open(insetchart_path) as infile:
        icj = json.load(infile)["Channels"]
    report_data_obj = {}
    try:
        for key in keys:
            data = icj[key]["Data"]
            report_data_obj[key] = data
        if debug:
            # this plot is for debugging only
            sft.plot_data(keys[0], dist2=None, label1=keys[0] + " channel",
                              label2="NA",
                              title=keys[0],
                              xlabel="time step", ylabel=keys[0], category=keys[0],
                              show=True, line=False)

            with open("DEBUG_data_InsetChart.json", "w") as outfile:
                json.dump(report_data_obj, outfile, indent=4)
    except Exception as ex:
        print("Failed to parse {0}, got exception: {1}".format(insetchart_name, ex))

    return report_data_obj


def check_test_condition(num_core, node_list,  migration_df, outfile):
    success = True
    node_count = len(node_list)
    outfile.write("  -- multi node count: {} nodes\n".format(node_count))
    if node_count <= 1:
        success = False
        outfile.write("     BAD: the demographics node count is {} expected a multinode test, "
                      "please fix the test.\n".format(node_count))

    outfile.write("  -- migration: \n")
    if migration_df is None:
        success = False
        outfile.write("     BAD: no migration report, please check the test output.\n")
    else:
        outfile.write("     migration: {} times.\n".format(len(migration_df)))
        if len(migration_df) < 1000:
            success = False
            outfile.write("     BAD: expected lots of migration happening. Please check the migraion report.\n")

    num_core = int(num_core)
    outfile.write("  -- multicore: {} cores\n".format(num_core))
    if num_core <= 1:
        success = False
        outfile("     BAD: this test is expected to run in multicore, get {0} = {1}.\n".format(NUM_CORES_KEY, num_core))
    outfile.write("check test condition result is : {}\n".format(success))
    return success


def compare_report_json(column_to_test, channel_to_test, groupby_df, json_obj, outfile):
    """
    Compare the the test column in report with the test channel in insetchat.
    :param column_to_test:
    :param channel_to_test:
    :param groupby_df:
    :param json_obj:
    :param outfile:
    :return:
    """
    incidence_json = json_obj[channel_to_test]
    success = True
    if not incidence_json or not len(incidence_json):
        success = False
        outfile.write("BAD: insetchart has no {} data.\n".format(channel_to_test))
    else:
        i = 0
        years = groupby_df.index.values
        incidence_counts = []
        for t in range(len(incidence_json)):
            if i < len(years):
                year = years[i]
                if t == round(year * sft.DAYS_IN_YEAR): # at the last time step of each time window
                    incidence_count = incidence_json[t] - sum(incidence_counts)# new incidence = total incidence at the last time step - sum of incidence from all previous time windows
                    reporter_sum = int(groupby_df[groupby_df.index == year][column_to_test])
                    incidence_counts.append(incidence_count)
                    if incidence_count != reporter_sum:
                        success = False
                        outfile.write(
                            "BAD: in year {0} the {1} count get from reporter is {2}, while insetchart.json reports"
                            " {3} cases over this report time window.\n".format(
                                year, column_to_test, reporter_sum, incidence_count))
                    i += 1
            else:
                break

        sft.plot_data(incidence_counts, dist2=np.array(groupby_df[column_to_test]), label1="insetchart",
                          label2="reporter", title=str(column_to_test),
                          xlabel="every half year", ylabel=str(column_to_test), category=str(column_to_test)+"_insetchart",
                          show=True, line=False, alpha=0.8, overlap=True)
    return success
