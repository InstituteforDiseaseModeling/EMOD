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
This test is testing the Generic targeting feature with HasIP.

This simulation has 2 individual properties: Accessibility and QualityOfCare, each individual property has 3 values.

A SimpleVaccine intervention(100% efficacy) is given to all population at the beginning of the simulation, followed by 
an OutbreakIndividual with Ignore_Immunity set to 1 for some individuals. Since the effect of SimpleVaccine is 100%, 
there is no individual transmission. 

Later on, we give out multiple SimpleDiagnostic interventions(Base_Sensitivity and Base_Specificity both set to 1) 
targeting different Accessibility groups or QualityOfCare groups.

Data to test: ReportEventRecorder.csv

main test: SimpleDiagnostic interventions should be distributed to our targeted individual property value(s). 
           Positive_Diagnosis_Event should broadcast to infected individuals within the targeted individual group(s).
           SimpleDiagnostic interventions should be distributed to non-targeted property evenly between value(s).

Other parameters under test are Demographic_Coverage and Is_Equal_To.
            
Output: scientific_feature_report.txt

"""


class Campaign:
    Events = "Events"
    Property_Restrictions = "Property_Restrictions"
    Event_Coordinator_Config = "Event_Coordinator_Config"
    Intervention_Config = "Intervention_Config"
    class_name = "class"
    SimpleDiagnostic = "SimpleDiagnostic"
    Demographic_Coverage = "Demographic_Coverage"
    OutbreakIndividual = "OutbreakIndividual"
    Targeting_Config = "Targeting_Config"
    Is_Equal_To = "Is_Equal_To"
    Positive_Diagnosis_Event = "Positive_Diagnosis_Event"
    IP_Key_Value = "IP_Key_Value"
    HasIP = "HasIP"
    Ignore_Immunity = "Ignore_Immunity"


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
    """
        Read IndividualProperties key:value pairs and InitialPopulation from demographic file.
    Args:
        demo_filename:

    Returns:

    """
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
    """
        Load demographic coverage from OutbreakIndividual intervention.
        Load Targeting_Config(IP_Key_Value and Is_Equal_To), demographic coverage and Positive_Diagnosis_Event from
    SimpleDiagnostic interventions.
    Args:
        campaign_filename:

    Returns:

    """
    with open(campaign_filename, 'r') as campaign_file:
        cf = json.load(campaign_file)
        try:
            events = cf[Campaign.Events]
            res = dict()
            for event in events:
                iv = event[Campaign.Event_Coordinator_Config][Campaign.Intervention_Config]
                if iv[Campaign.class_name] == Campaign.OutbreakIndividual:
                    demographic_coverage = event[Campaign.Event_Coordinator_Config][Campaign.Demographic_Coverage]
                    res[Campaign.OutbreakIndividual] = demographic_coverage
                    if iv[Campaign.Ignore_Immunity] != 1:
                        raise ValueError(f"{Campaign.Ignore_Immunity} for {Campaign.OutbreakIndividual} must set to 1,"
                                         f" please fix the {campaign_filename}.\n")
                elif iv[Campaign.class_name] == Campaign.SimpleDiagnostic:
                    positive_event = iv[Campaign.Positive_Diagnosis_Event]
                    tc = event[Campaign.Event_Coordinator_Config][Campaign.Targeting_Config]
                    class_name = tc[Campaign.class_name]
                    is_equal_to = tc[Campaign.Is_Equal_To]
                    ip_key_value = tc[Campaign.IP_Key_Value]
                    if class_name not in res:
                        res[class_name] = dict()
                    demographic_coverage = event[Campaign.Event_Coordinator_Config][Campaign.Demographic_Coverage]
                    if positive_event in res[class_name]:
                        raise ValueError(f"{positive_event} is used in another {Campaign.SimpleDiagnostic} intervention"
                                         f" , please used a different {Campaign.Positive_Diagnosis_Event}.\n")
                    else:
                        res[class_name][positive_event] = [is_equal_to, demographic_coverage, ip_key_value]
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

        # load campaign file and demo file
        campaign_object = load_campaign(campaign_filename)
        demo_object, node_population = load_demo_file(demo_filename[-1])
        if debug:
            output_report_file.write(f"campaign object is {campaign_object}.\n")
            output_report_file.write(f"node_population is {node_population} and demo_object is {demo_object}.\n")

        if Campaign.HasIP not in campaign_object:
            succeed = False
            output_report_file.write(f"BAD: {Campaign.HasIP} is not in the {campaign_filename}, please fix "
                                     f"the test.\n")
        elif Campaign.OutbreakIndividual not in campaign_object:
            succeed = False
            output_report_file.write(f"BAD: {Campaign.OutbreakIndividual} is not in {campaign_filename}, please fix the"
                                     f" test.\n")
        else:
            # main test
            outbreak_coverage = campaign_object[Campaign.OutbreakIndividual]
            for positive_event, (is_equal_to, target_demographic_coverage, ip_key_value) in \
                    campaign_object[Campaign.HasIP].items():
                ip_group, ip_value = ip_key_value.split(':')
                output_report_file.write(f"Test {Campaign.Positive_Diagnosis_Event} {positive_event} with "
                                         f"{Campaign.Is_Equal_To} = {is_equal_to} and {Campaign.IP_Key_Value} = "
                                         f"{ip_key_value}: \n")
                output_report_file.write(
                    f"  1. check number of {positive_event} events broadcast.\n")
                target_values = [ip_value] if is_equal_to else \
                    [value for value in demo_object[ip_group] if value != ip_value]
                target_population = node_population * sum([demo_object[ip_group][value] for value in target_values])
                output_report_file.write(f"\tThis intervention should target {ip_group} with values: {target_values}, "
                                         f"total number of individuals in these groups is {target_population}.\n")
                # Sensitivity and Specificity of SimpleDiagnostic are all 100%.
                test_positive_percentage = outbreak_coverage * target_demographic_coverage
                test_positive_event_count = target_population * test_positive_percentage
                tested_event_df = event_df[(event_df["Event_Name"] == positive_event)]
                actual_positive_event_count = len(tested_event_df)
                tolerance = sft.cal_tolerance_binomial(target_population, test_positive_percentage, prob=0.01) if \
                   0 < test_positive_percentage < 1 else 0
                message = f"simulation broadcasts {actual_positive_event_count} {positive_event} events, " \
                          f"expected {test_positive_event_count} events with tolerance = {tolerance}.\n"
                if math.fabs(actual_positive_event_count - test_positive_event_count) > \
                        tolerance * test_positive_event_count:
                    succeed = False
                    output_report_file.write("\tBAD: " + message)
                else:
                    output_report_file.write("\tGOOD: " + message)

                output_report_file.write(f"  2. make sure {positive_event} events broadcast to "
                                         f"individuals within {ip_group} group based on the targeting IP criteria.\n")
                actual_target_values = tested_event_df[ip_group].unique()
                message = f"simulation broadcasts {positive_event} events to {Demo.Property} {ip_group} with " \
                    f"{Demo.Values}: {actual_target_values}, expected {target_values}.\n"
                if set(actual_target_values) != set(target_values):
                    succeed = False
                    output_report_file.write("\tBAD: " + message)
                else:
                    output_report_file.write("\tGOOD: " + message)

                output_report_file.write(f"  3. make sure {positive_event} events broadcast evenly to "
                                         f"individuals outside {ip_group} group.\n")
                other_ip_groups = [group for group in demo_object.keys() if group != ip_group]
                for other_ip_group in other_ip_groups:
                    output_report_file.write(f"\tLooking at group {other_ip_group}:\n\t ")
                    event_count = tested_event_df[other_ip_group].value_counts()
                    if set(demo_object[other_ip_group].keys()) != set(event_count.index):
                        succeed = False
                        output_report_file.write(f"\t  BAD: {positive_event} was distributed to {event_count.index}, "
                                                 f"expected {demo_object[other_ip_group].keys()}.\n")
                    actual_proportions = []
                    expected_percentage = []
                    for value, percentage in demo_object[other_ip_group].items():
                        actual_proportions.append(event_count.loc[value])
                        expected_percentage.append(percentage)
                    if not sft.test_multinomial(actual_proportions, expected_percentage,
                                         report_file=output_report_file, prob_flag=True):
                        succeed = False

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




