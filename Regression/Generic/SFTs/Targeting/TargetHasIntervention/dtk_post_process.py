#!/usr/bin/python
import os
if __name__ == '__main__':
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )
import json
import math

import dtk_test.dtk_sft as sft
from dtk_test.dtk_OutputFile import ReportEventRecorder
from dtk_test.dtk_StdOut import StdOut, SearchType


"""
This test is testing the Generic targeting feature with HasIntervention.

This simulation has 4 nodes, each node could have one or more persistent interventions: SimpleVaccine and 
DelayedIntervention at the beginning of the simulation. Later, we give out multiple BroadcastEvent interventions to all
nodes targeting the previous persistent interventions based on intervention name.

For DelayedIntervention, if the delay_period is longer than the time period between DelayedIntervention start day and 
BroadcastEvent start day, we use the intervention name from DelayedIntervention. On the other hand, we use the 
intervention name from the Actual_IndividualIntervention_Configs.

Other parameters under test are Demographic_Coverage and Is_Equal_To.

Data to test: StdOut.txt and ReportEventRecorder.csv

main test: BroadcastEvent interventions should be distributed to our targeted individuals.
            
Output: scientific_feature_report.txt

"""


class Campaign:
    Events = "Events"
    Property_Restrictions = "Property_Restrictions"
    Event_Coordinator_Config = "Event_Coordinator_Config"
    Intervention_Config = "Intervention_Config"
    class_name = "class"
    SimpleVaccine = "SimpleVaccine"
    Demographic_Coverage = "Demographic_Coverage"
    DelayedIntervention = "DelayedIntervention"
    Nodeset_Config = "Nodeset_Config"
    Node_List = "Node_List"
    Targeting_Config = "Targeting_Config"
    HasIntervention = "HasIntervention"
    Is_Equal_To = "Is_Equal_To"
    Intervention_Name = "Intervention_Name"
    Actual_IndividualIntervention_Configs = "Actual_IndividualIntervention_Configs"
    BroadcastEvent = "BroadcastEvent"
    Broadcast_Event = "Broadcast_Event"
    Start_Day = "Start_Day"
    Delay_Period = "Delay_Period_Constant"


class Demo:
    Defaults = "Defaults"
    IndividualProperties = "IndividualProperties"
    Property = "Property"
    Values = "Values"
    Initial_Distribution = "Initial_Distribution"
    Nodes = "Nodes"
    NodeID = "NodeID"
    NodeAttributes = "NodeAttributes"
    InitialPopulation = "InitialPopulation"


def load_demo_file(demo_filename):
    """
        Read InitialPopulation and NodeIDs from demographic file.
    Args:
        demo_filename:

    Returns: node_population, node_list

    """
    with open(demo_filename, 'r') as demo_file:
        demo = json.load(demo_file)
        try:
            nodes = demo[Demo.Nodes]
            node_list = []
            node_population = None
            for node in nodes:
                node_id = node[Demo.NodeID]
                node_list.append(node_id)
                cur_node_population = node[Demo.NodeAttributes][Demo.InitialPopulation]
                if node_population:
                    if cur_node_population != node_population:
                        raise ValueError(f"Every node should have the same {Demo.InitialPopulation}, please fix the "
                                         f"{demo_filename}.\n")
                else:
                    node_population = cur_node_population

            return node_population, node_list

        except KeyError as ke:
            raise KeyError(f"{ke} is not found in {demo_filename}.\n")

def load_campaign(campaign_filename):
    """
        Load intervention name, target node ids and demographic coverage from SimpleVaccine and DelayedIntervention
    interventions.
        Load Targeting_Config(target intervention name and Is_Equal_To), start day, demographic coverage and
    Broadcast_Event from BroadcastEvent interventions.
    Args:
        campaign_filename:

    Returns:

    """
    def append_coverage_to_result(res, node_ids, intervention_name, demographic_coverage):
        if intervention_name not in res:
            res[intervention_name] = dict()

        for node_id in node_ids:
            if node_id not in res[intervention_name]:
                res[intervention_name][node_id] = [demographic_coverage]
            else:
                res[intervention_name][node_id].append(demographic_coverage)

    with open(campaign_filename, 'r') as campaign_file:
        cf = json.load(campaign_file)
        try:
            events = cf[Campaign.Events]
            res = dict()
            for event in events:
                iv = event[Campaign.Event_Coordinator_Config][Campaign.Intervention_Config]
                if iv[Campaign.class_name] == Campaign.SimpleVaccine:
                    demographic_coverage = event[Campaign.Event_Coordinator_Config][Campaign.Demographic_Coverage]
                    intervention_name = iv[Campaign.Intervention_Name]
                    node_ids = event[Campaign.Nodeset_Config][Campaign.Node_List]
                    append_coverage_to_result(res, node_ids, intervention_name, demographic_coverage)

                elif iv[Campaign.class_name] == Campaign.DelayedIntervention:
                    demographic_coverage = event[Campaign.Event_Coordinator_Config][Campaign.Demographic_Coverage]
                    intervention_name = iv[Campaign.Intervention_Name]
                    node_ids = event[Campaign.Nodeset_Config][Campaign.Node_List]
                    delay_period = iv[Campaign.Delay_Period]
                    if delay_period > 10: # Hardcoded value to distinguish long and short delay_period.
                        # DelayedIntervention with short delay_period will be expired when the BroadcastEvent happens
                        # so the intervention name should be the one defined in Actual_IndividualIntervention_Configs.
                        append_coverage_to_result(res, node_ids, intervention_name, demographic_coverage)
                    else:
                        intervention_name_actual_intervention = iv[Campaign.Actual_IndividualIntervention_Configs][0][
                            Campaign.Intervention_Name]
                        append_coverage_to_result(res, node_ids, intervention_name_actual_intervention, demographic_coverage)

                elif iv[Campaign.class_name] == Campaign.BroadcastEvent:
                    broadcast_event = iv[Campaign.Broadcast_Event]
                    start_day = event[Campaign.Start_Day]
                    tc = event[Campaign.Event_Coordinator_Config][Campaign.Targeting_Config]
                    class_name = tc[Campaign.class_name]
                    is_equal_to = tc[Campaign.Is_Equal_To]
                    target_intervention = tc[Campaign.Intervention_Name]
                    if class_name not in res:
                        res[class_name] = dict()
                    demographic_coverage = event[Campaign.Event_Coordinator_Config][Campaign.Demographic_Coverage]
                    res[class_name][target_intervention] = [start_day, is_equal_to, demographic_coverage, broadcast_event]
            return res

        except KeyError as ke:
            raise KeyError(f"{ke} is not found in {campaign_filename}.\n")


def calculate_vaccine_coverage_by_node(node_coverages):
    """
        Calculate vaccine coverage for every node and save the result into a dictionary.
    Args:
        node_coverages:

    Returns:

    """
    result = dict()
    for node_id, vaccine_coverage_list in node_coverages.items():
        result[node_id] = calculate_vaccine_coverage(vaccine_coverage_list)
    return  result


def calculate_vaccine_coverage(coverage_list):
    """
        Calculate actual vaccine coverage from a list of multiple vaccine coverages.
        For example, a group of individuals could receive multiple vaccines and the coverages are [0.8, 0.4, 0.6], so the
    effective vaccine coverage is:
        print(calculate_vaccine_coverage([0.8, 0.4, 0.6]))
        # Output is 0.952
    Args:
        coverage_list:

    Returns:

    """
    if not len(coverage_list):
        return 0
    return coverage_list[0] + (1.0 - coverage_list[0]) * calculate_vaccine_coverage(coverage_list[1:])


def create_report_file(event_df, stdout_df, config_filename, report_name, debug):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        # load parameters to test from config
        config_name, campaign_filename, demo_filename = sft.get_config_parameter(
            config_filename, ["Config_Name", "Campaign_Filename", "Demographics_Filenames"])
        output_report_file.write(f"Config_Name = {config_name}\n")

        # load campaign file and demo file
        campaign_object = load_campaign(campaign_filename)
        node_population, node_list = load_demo_file(demo_filename[-1])
        if debug:
            output_report_file.write(f"campaign object is {campaign_object}.\n")
            output_report_file.write(f"node_population is {node_population} and node_list is {node_list}.\n")

        # collect the parameters under test from demo and campaign files and check some test conditions.
        test_parameter = dict()
        if Campaign.HasIntervention not in campaign_object:
            succeed = False
            output_report_file.write(f"BAD: {Campaign.HasIntervention} is not in the {campaign_filename}, please fix "
                                     f"the test.\n")
        else:
            for target_intervention, values in campaign_object[Campaign.HasIntervention].items():
                if target_intervention not in campaign_object:
                    succeed = False
                    output_report_file.write(f"BAD: {target_intervention} is not a {Campaign.Intervention_Name} in"
                                             f" {campaign_filename}, please fix the test.\n")
                else:
                    broadcast_event = values[-1]
                    if broadcast_event != target_intervention:
                        output_report_file.write(f"WARNING: The {Campaign.Broadcast_Event} of "
                                                 f"{Campaign.BroadcastEvent} is {broadcast_event}, expected "
                                                 f"{target_intervention}, please fix the {campaign_filename}.\n")
                    test_parameter[target_intervention] = values[:]
                    node_dict = dict()

                    vaccine_demographic_coverage_by_node = calculate_vaccine_coverage_by_node(
                        campaign_object[target_intervention])
                    for node_id, vaccine_demographic_coverage in vaccine_demographic_coverage_by_node.items():
                        if vaccine_demographic_coverage == 1 or vaccine_demographic_coverage == 0 :
                            output_report_file.write(f"WARNING: The total{Campaign.Demographic_Coverage} of "
                                                     f"{target_intervention} is "
                                                     f"{vaccine_demographic_coverage}, expect value between 0 and 1.\n")
                        node_dict[node_id] = vaccine_demographic_coverage
                    test_parameter[target_intervention].append(node_dict)

        if succeed:
            # main test
            for target_intervention, (start_day, is_equal_to, target_demographic_coverage, broadcast_event, node_dict) in \
                test_parameter.items():
                output_report_file.write(f"Test target_intervention {target_intervention}: \n")
                for node_id in node_list:
                    output_report_file.write(f"  Test Node {node_id}: \n")
                    output_report_file.write(f"    1. check number of {Campaign.BroadcastEvent} interventions are "
                                             f"distributed.\n")
                    vaccine_demographic_coverage = node_dict[node_id] if node_id in node_dict else 0
                    target_percentage = vaccine_demographic_coverage * target_demographic_coverage if \
                        is_equal_to else \
                        (1.0 - vaccine_demographic_coverage) * target_demographic_coverage
                    target_population = node_population * target_percentage
                    actual_target_population = stdout_df[(stdout_df["Node"] == node_id) &
                                                         (stdout_df["Time"] == start_day)][Campaign.BroadcastEvent].iloc[0]
                    tolerance = 0 if (target_population in [0, 1] or target_percentage in [0, 1]) else \
                        sft.cal_tolerance_binomial(target_population, target_percentage, prob=0.04)
                    message = f"simulation gave out {actual_target_population} {Campaign.BroadcastEvent} " \
                              f"interventions with {Campaign.Is_Equal_To} = {is_equal_to} at Node " \
                              f"{node_id}, expected {target_population} intervention with tolerance = {tolerance}.\n"
                    if math.fabs(target_population - actual_target_population) > tolerance * target_population:
                        succeed = False
                        output_report_file.write("\tBAD: " + message)
                    else:
                        output_report_file.write("\tGOOD: " + message)

                    output_report_file.write(f"    2. make sure {Campaign.BroadcastEvent} interventions are "
                                             f"distributed to individuals based on the targeting criteria.\n")
                    actual_test_positive_count = len(event_df[(event_df["Node_ID"] == node_id) &
                                                              (event_df["Event_Name"] == target_intervention)])
                    expected_test_positive_count = actual_target_population

                    msg = f"{actual_test_positive_count} {target_intervention} events are raised in Node {node_id}, " \
                        f"while expected {expected_test_positive_count} events."
                    if actual_test_positive_count != expected_test_positive_count:
                        succeed = False
                        output_report_file.write(f"\tBAD: {msg}\n")
                    else:
                        output_report_file.write(f"\tGOOD: {msg}\n")

        output_report_file.write(sft.format_success_msg(succeed))

        return succeed


def application(output_folder="output", stdout_filename="test.txt",
                event_report_name="ReportEventRecorder.csv",
                config_filename="config.json",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("event_report_name: " + event_report_name + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done(stdout_filename)

    event_df = ReportEventRecorder(os.path.join(output_folder, event_report_name)).df

    filter_string_list = [Campaign.BroadcastEvent, "Update(): Time: "]
    load_df_param = [[Campaign.BroadcastEvent, "Node"], ["gave out ", "node "], [SearchType.VAL, SearchType.VAL]]
    stdout_df = StdOut(stdout_filename, filter_string_list, load_df_param).df

    create_report_file(event_df, stdout_df, config_filename, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-e', '--event_report', default="ReportEventRecorder.csv",
                        help="Report Event Recorder to parse (ReportEventRecorder.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                event_report_name=args.event_report,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

