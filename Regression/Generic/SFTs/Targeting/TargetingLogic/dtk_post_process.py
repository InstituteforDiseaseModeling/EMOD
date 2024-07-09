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


"""
This test is testing the Generic targeting feature with TargetingLogic.

In this simulation a BroadcastEvent is distributed with a two-dimensional array of targeting logic. The test makes sure
the logic in the inner array are added(AND logic) together and the logic in the outer array are OR logic.

main test: Broadcast_Event should distributed to our targeted individuals based on targeting logic.

Other parameters under test are Demographic_Coverage and Is_Equal_To.
            
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
    Targeting_Config = "Targeting_Config"
    Is_Equal_To = "Is_Equal_To"
    Positive_Diagnosis_Event = "Positive_Diagnosis_Event"
    IP_Key_Value = "IP_Key_Value"
    HasIP = "HasIP"
    Ignore_Immunity = "Ignore_Immunity"
    BroadcastEvent = "BroadcastEvent"
    Broadcast_Event = "Broadcast_Event"
    Tested_Event = "Tested_Event"
    Logic = "Logic"
    HasIntervention = "HasIntervention"
    Intervention_Name = "Intervention_Name"
    OutbreakIndividual = "OutbreakIndividual"
    TargetingLogic = "TargetingLogic"


class Demo:
    Defaults = "Defaults"
    IndividualProperties = "IndividualProperties"
    Property = "Property"
    Values = "Values"
    Initial_Distribution = "Initial_Distribution"
    Nodes = "Nodes"
    NodeAttributes = "NodeAttributes"
    InitialPopulation = "InitialPopulation"


def load_demo_file(demo_filename):
    with open(demo_filename, 'r') as demo_file:
        demo = json.load(demo_file)
        try:
            ips = demo[Demo.Defaults][Demo.IndividualProperties]
            ip_key_values = dict()
            for ip in ips:
                key = ip[Demo.Property]
                values = ip[Demo.Values]
                initial_distribution = ip[Demo.Initial_Distribution]
                ip_key_values[key] = dict()
                for i in range(len(values)):
                    ip_key_values[key][values[i]] = initial_distribution[i]
            node_population = demo[Demo.Nodes][0][Demo.NodeAttributes][Demo.InitialPopulation]
            return ip_key_values, node_population

        except KeyError as ke:
            raise KeyError(f"{ke} is not found in {demo_filename}.\n")


def load_campaign(campaign_filename):
    with open(campaign_filename, 'r') as campaign_file:
        cf = json.load(campaign_file)
        try:
            events = cf[Campaign.Events]
            res = dict()
            for event in events:
                iv = event[Campaign.Event_Coordinator_Config][Campaign.Intervention_Config]
                if iv[Campaign.class_name] == Campaign.SimpleVaccine:
                    vaccine_demographic_coverage = event[Campaign.Event_Coordinator_Config][Campaign.Demographic_Coverage]
                    intervention_name = iv[Campaign.Intervention_Name]
                    res[intervention_name] = vaccine_demographic_coverage
                    if vaccine_demographic_coverage in [0, 1]:
                        raise ValueError(f"{vaccine_demographic_coverage} for {Campaign.SimpleVaccine} should not be "
                                         f"0 or 1, please fix the {campaign_filename}.\n")
                elif iv[Campaign.class_name] == Campaign.OutbreakIndividual:
                    demographic_coverage = event[Campaign.Event_Coordinator_Config][Campaign.Demographic_Coverage]
                    res[Campaign.OutbreakIndividual] = demographic_coverage
                    if iv[Campaign.Ignore_Immunity] != 0:
                        raise ValueError(f"{Campaign.Ignore_Immunity} for {Campaign.OutbreakIndividual} must set to 0,"
                                         f" please fix the {campaign_filename}.\n")
                    if demographic_coverage != 1:
                        raise ValueError(f"{demographic_coverage} for {Campaign.OutbreakIndividual} must set to 1,"
                                         f" please fix the {campaign_filename}.\n")
                elif iv[Campaign.class_name] == Campaign.BroadcastEvent:
                    res[Campaign.BroadcastEvent] = []
                    tested_event = iv[Campaign.Broadcast_Event]
                    if tested_event != Campaign.Tested_Event:
                        raise ValueError(f"{Campaign.Broadcast_Event} must be {Campaign.Tested_Event}, please "
                                         f"fix the {campaign_filename}.\n")
                    tc = event[Campaign.Event_Coordinator_Config][Campaign.Targeting_Config]
                    logic_list = tc[Campaign.Logic]
                    demographic_coverage = event[Campaign.Event_Coordinator_Config][Campaign.Demographic_Coverage]

                    for inner_logic_list in logic_list:
                        res[Campaign.BroadcastEvent].append([])
                        for logic in inner_logic_list:
                            class_name = logic[Campaign.class_name]
                            is_equal_to = logic[Campaign.Is_Equal_To]
                            if class_name == Campaign.HasIntervention:
                                intervention_name = logic[Campaign.Intervention_Name]
                                res[Campaign.BroadcastEvent][-1].append(
                                    [class_name, is_equal_to, demographic_coverage, intervention_name])
                            elif class_name == Campaign.HasIP:
                                ip_key_value = logic[Campaign.IP_Key_Value]
                                res[Campaign.BroadcastEvent][-1].append(
                                    [class_name, is_equal_to, demographic_coverage, ip_key_value])

            return res

        except KeyError as ke:
            raise KeyError(f"{ke} is not found in {campaign_filename}.\n")


def create_report_file(event_df, config_filename, report_name, debug):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        # load parameters to test from config
        config_name, campaign_filename, demo_filename = sft.get_config_parameter(
            config_filename, ["Config_Name", "Campaign_Filename", "Demographics_Filenames"])
        output_report_file.write(f"Config_Name = {config_name}\n")

        # Load campaign file and demo file
        campaign_object = load_campaign(campaign_filename)
        demo_object, node_population = load_demo_file(demo_filename[-1])
        if debug:
            output_report_file.write(f"campaign object is {campaign_object}.\n")
            output_report_file.write(f"node_population is {node_population} and demo_object is {demo_object}.\n")

        # Check some test conditions
        if Campaign.BroadcastEvent not in campaign_object:
            succeed = False
            output_report_file.write(f"BAD: {Campaign.BroadcastEvent} is not in the {campaign_filename}, please fix "
                                     f"the test.\n")
        elif Campaign.OutbreakIndividual not in campaign_object:
            succeed = False
            output_report_file.write(f"BAD: {Campaign.OutbreakIndividual} is not in {campaign_filename}, please fix the"
                                     f" test.\n")
        else:
            # Main test
            output_report_file.write(f"Test {Campaign.Broadcast_Event} {Campaign.Tested_Event} with each OR Login: \n")
            target_demographic_coverages = []
            for logic_list in campaign_object[Campaign.BroadcastEvent]: # Outer logic
                output_report_file.write(f"  This OR logic has {len(logic_list)} inner AND logic: \n")
                output_report_file.write(
                    f"  1. check number of {Campaign.Tested_Event} events broadcast to a specific group with "
                    f"specific intervention status.\n")
                target_demographic_coverage = 1 # default value
                demographic_coverage = intervention_name = ip_group = target_values = None
                tested_event_df = event_df
                for inner_logic_list in logic_list: # Inner logic
                    logic_class, is_equal_to, demographic_coverage, logic_value = inner_logic_list
                    if logic_class == Campaign.HasIntervention:
                        intervention_name = logic_value
                        if intervention_name not in campaign_object:
                            # Targeted intervention name must be defined in campaign.
                            succeed = False
                            output_report_file.write(f"\tBAD: {Campaign.Intervention_Name} {logic_class} is not in "
                                                     f"{campaign_filename}, please fix the test.\n")
                        else:
                            # SimpleVaccine coverage
                            intervention_coverage = campaign_object[intervention_name]
                            inner_demographic_coverage = intervention_coverage if is_equal_to else \
                                (1.0 - intervention_coverage)
                            output_report_file.write(f"\t{logic_class} with {Campaign.Is_Equal_To} = {is_equal_to}, "
                                                     f"{Campaign.Intervention_Name} = {intervention_name}, we calculated the"
                                                     f" target demographic coverage is {inner_demographic_coverage}\n")
                            # AND logic, multiple by each inner coverage
                            target_demographic_coverage *= inner_demographic_coverage
                            # Ignore_Immunity for Outbreak is set to 0, so the infected status depends on whether
                            # individual is vaccinated already
                            expected_infected = 0 if is_equal_to else 1
                            # Filter test data by inner targeting logic
                            tested_event_df = tested_event_df[tested_event_df["Infected"] == expected_infected]
                    elif logic_class == Campaign.HasIP:
                        ip_key_value = logic_value
                        ip_group, ip_value = ip_key_value.split(':')
                        # Collect targeted IP values based on is_equal_to
                        target_values = [ip_value] if is_equal_to else \
                            [value for value in demo_object[ip_group] if value != ip_value]
                        # Inner coverage is sum of Initial_Distribution for IP values in the targeted values.
                        inner_demographic_coverage = sum([demo_object[ip_group][value] for value in target_values])
                        output_report_file.write(f"\t{logic_class} with {Campaign.Is_Equal_To} = {is_equal_to}, "
                                                 f"{Campaign.IP_Key_Value} = {ip_key_value}, we calculated the"
                                                 f" target demographic coverage is {inner_demographic_coverage}\n")
                        # AND logic, multiple by each inner coverage
                        target_demographic_coverage *= inner_demographic_coverage
                        # Filter test data by inner targeting logic
                        tested_event_df = tested_event_df[tested_event_df[ip_group].isin(target_values)]
                    else:
                        # Generic sim only support two types of targeting logic.
                        succeed =False
                        output_report_file.write(f"\t{Campaign.TargetingLogic} must be either {Campaign.HasIP} or "
                                                 f"{Campaign.HasIntervention}, not {logic_class}.\n")
                # If Demographic_Coverage of BroadcastEvent is defined
                if demographic_coverage:
                    # Actual targeted coverage needs to be multiplied by BroadcastEvent coverage.
                    target_demographic_coverage *= demographic_coverage
                output_report_file.write(f"    target demographic coverage for this OR logic is "
                                         f"{target_demographic_coverage} which is a product of demographic coverages "
                                         f"from each inner AND logic.\n")
                # Collect a list of demographic coverage for each Outer logic(OR logic)
                target_demographic_coverages.append(target_demographic_coverage)
                expected_event_count = node_population * target_demographic_coverage
                actual_event_count = len(tested_event_df)
                tolerance = sft.cal_tolerance_binomial(expected_event_count, sum(target_demographic_coverages),
                                                       prob=0.05) if \
                    0 < sum(target_demographic_coverages) < 1 else 0
                message = f"simulation broadcasts {actual_event_count} {Campaign.Tested_Event} events to " \
                          f"{Campaign.HasIntervention} = {intervention_name} and " \
                          f"{Campaign.HasIP} targeted {ip_group}:{target_values}, " \
                          f"expected {expected_event_count} events with tolerance = {tolerance}.\n"
                if math.fabs(expected_event_count - actual_event_count) > tolerance * expected_event_count:
                    succeed = False
                    output_report_file.write("    BAD: " + message)
                else:
                    output_report_file.write("    GOOD: " + message)

            output_report_file.write(
                f"  2. check total number of {Campaign.Tested_Event} events broadcast.\n")
            # Total target coverage is sum of each Outer logic(OR logic)
            target_population = node_population * sum(target_demographic_coverages)
            # Filter the original test data with event under test.
            total_tested_event_df = event_df[(event_df["Event_Name"] == Campaign.Tested_Event)]
            total_actual_event_count = len(total_tested_event_df)
            tolerance = sft.cal_tolerance_binomial(target_population, sum(target_demographic_coverages), prob=0.05) if \
                0 < sum(target_demographic_coverages) < 1 else 0
            message = f"simulation broadcasts {total_actual_event_count} {Campaign.Tested_Event} events, " \
                f"expected {target_population} events with tolerance = {tolerance}.\n"
            if math.fabs(target_population - total_actual_event_count) > tolerance * target_population:
                succeed = False
                output_report_file.write("    BAD: " + message)
            else:
                output_report_file.write("    GOOD: " + message)

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

    create_report_file(event_df, config_filename, report_name, debug)


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

