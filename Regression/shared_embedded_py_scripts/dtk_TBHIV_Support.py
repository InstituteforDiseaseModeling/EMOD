#!/usr/bin/python

import json
import os.path as path
import dtk_test.dtk_sft as sft
import math

# region outbreak support
matches = [  "New HIV infection for individual", "Initializing a latent infection"]

class CampaignKeys:
    Start_Day = "Start_Day"
    Infection_Type = "Infection_Type"
    Coverage = "Demographic_Coverage"
    Intervention_Config = "Intervention_Config"
    Event_Coordinator_Config = "Event_Coordinator_Config"
    Key_HIV = "HIV"
    Key_TB = "TB"
    Key_TB_Drug_Name = "TB_Drug_Name"

class ConfigKeys:
    KEY_DrugParams = "TBHIV_Drug_Params"
    KEY_SimulationDuration = "Simulation_Duration"
    KEY_StartTime = "Start_Time"
    class DrugParams:
        KEY_HivCureRate = "TB_Drug_Cure_Rate_HIV"
        KEY_TbDrugMortalityRateHIV = "TB_Drug_Mortality_Rate_HIV"
        KEY_PrimaryDecayConstant = "TB_Drug_Primary_Decay_Time_Constant"
        KEY_TbDrugInactivationRateHIV = "TB_Drug_Inactivation_Rate_HIV"
        KEY_TbDrugResistanceRate = "TB_Drug_Resistance_Rate_HIV"
        KEY_TbDrugPrimaryDecayConstant = "TB_Drug_Primary_Decay_Time_Constant"
        KEY_TbDrugRelapseRateHIV = "TB_Drug_Relapse_Rate_HIV"

class InsetChart:
    KEY_data = "Data"
    KEY_channels = "Channels"
    class Channels:
        KEY_ActiveTbPrevalence = "Active TB Prevalence"
        KEY_LatentTbPrevalence = "Latent TB Prevalence"
        KEY_HIV_Prevalence = "HIV prevalence"
        KEY_Newly_Cleared_TB_Infections = "Newly Cleared TB Infections"
        KEY_StatisticalPopulation = "Statistical Population"
        KEY_MdrTbPrevalence = "MDR TB Prevalence"
        KEY_Infected = "Infected"
        KEY_DiseaseDeaths = "Disease Deaths"
        KEY_ActiveTBTreatment= "Number Active TB on Treatment"

class ParamKeys:
    KEY_DrugStartTime = "Drug_Start_Time"
    KEY_DrugRegimenLength = "Drug_Regimen_Length"
    KEY_EventName = "Event_Name"
    KEY_TbDrugResistanceRate = "TB_Drug_Resistance_Rate_HIV"
    KEY_RelapseRelativeTime = "Relapse_Relative_Time"
    KEY_ResistanceRelativeTime = "Resistance_Relative_Time"
"""
Requirements description goes here. The OutbreakIndividualTBorHIV 'intervention' shall be able to distribute outbreaks
to existing individuals according to the usual demographic targeting capabilities of the EventCoordinator used. It can
either distribute TB infections or HIV infections.

Test: The purpose of this test is to confirm the outbreak results in the expected number of TB infections. Note that
the focus isn't on the expected number, but the TB infection. We will check the TB prevalance one timestep after the 
outbreak. Decided to use logging instead of InsetChart.json. Should check that TB infections do NOT lead to HIV 
infections and vice-versa.
"""
def load_emod_parameters(config_filename = "config.json", campaign_filename="campaign.json"):
    """reads campaign file and config.json and populates params_obj

    :param campaign_filename: name of campaign file (campaign.json)
    :returns params_obj:     dictionary with KEY_START_TIMESTEP, etc., keys (e.g.)
    """
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    with open(campaign_filename) as infile:
        cf = json.load(infile)["Events"]
    params_obj = {}
    start_day_hiv = start_day_tb = coverage_hiv = coverage_tb = 0
    for event in cf:
        infection_type = event[CampaignKeys.Event_Coordinator_Config]\
            [CampaignKeys.Intervention_Config][CampaignKeys.Infection_Type]
        if infection_type == CampaignKeys.Key_HIV:
            start_day_hiv = event[CampaignKeys.Start_Day]
            coverage_hiv = event[CampaignKeys.Event_Coordinator_Config][CampaignKeys.Coverage]
        elif infection_type == CampaignKeys.Key_TB:
            start_day_tb = event[CampaignKeys.Start_Day]
            coverage_tb = event[CampaignKeys.Event_Coordinator_Config][CampaignKeys.Coverage]

    params_obj[CampaignKeys.Start_Day] = [start_day_hiv, start_day_tb]
    params_obj[CampaignKeys.Infection_Type] = [CampaignKeys.Key_HIV, CampaignKeys.Key_TB]
    params_obj[CampaignKeys.Coverage] = [coverage_hiv, coverage_tb]
    params_obj[ConfigKeys.KEY_StartTime] = cdj[ConfigKeys.KEY_StartTime]
    return params_obj

def parse_stdout_file(stdout_filename="test.txt", debug=False):
    """count HIV and TB infections

    :param stdout_filename: file to parse (test.txt)
    :return:                HIV and TB infections counts
    """

    filtered_lines = []
    count = [0]*len(matches)
    with open(stdout_filename) as logfile:
        for line in logfile:
            for i in range(len(matches)):
                match = matches[i]
                if match in line:
                    filtered_lines.append(line)
                    count[i] += 1
    if debug:
        with open("filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    return count

def parse_json_report(start_time, output_folder="output", insetchart_name="InsetChart.json", debug=False):
    """creates inset_days structure with timestep key and Statistical_Population value

    :param insetchart_name: InsetChart.json file with location (output/InsetChart.json)
    :returns: inset_days structure
    """
    insetchart_path = path.join(output_folder, insetchart_name)
    with open(insetchart_path) as infile:
        icj = json.load(infile)[InsetChart.KEY_channels]

    #prevalence = icj["Infected"]["Data"]
    pop        = icj[InsetChart.Channels.KEY_StatisticalPopulation][InsetChart.KEY_data]
    end_time = start_time + len(pop)
    inset_days = {}
    for x in range(start_time, end_time):
        inset_days[x] = pop[x]

    if debug:
        with open("inset_days.json", "w") as outfile:
            json.dump(inset_days, outfile, indent=4)

    return inset_days


def create_report_file( param_obj,
                        actual_infections,
                        inset_days,
                        report_name):
    success = True
    error_tolerance = 2.5e-2
    with open(report_name, "w") as outfile:
        # Test HIV infections
        test_tstep_hiv = param_obj[CampaignKeys.Start_Day][0] + 1
        actual_hiv_infections = actual_infections[0]
        expected_hiv_infections_fraction = param_obj[CampaignKeys.Coverage][0]
        actual_hiv_infection_fraction = 1.0 * actual_hiv_infections / inset_days[test_tstep_hiv]
        outfile.write("Total HIV infections fraction expected: {0} \n".format(expected_hiv_infections_fraction))
        outfile.write("Total HIV infections: {0} \n".format(actual_hiv_infections))
        outfile.write("Total HIV infection fraction: {0} \n".format(actual_hiv_infection_fraction))
        total_error_hiv = math.fabs(expected_hiv_infections_fraction - actual_hiv_infection_fraction)

        if total_error_hiv > error_tolerance:
            success = False
            outfile.write("BAD: Total error is {0} for HIV, error tolerance is {1}\n".format(total_error_hiv, error_tolerance))

        # Test TB infections
        test_tstep_tb = param_obj[CampaignKeys.Start_Day][1] + 1
        actual_tb_infections = actual_infections[1]
        expected_tb_infections_fraction = param_obj[CampaignKeys.Coverage][1]
        actual_tb_infection_fraction = 1.0 * actual_tb_infections / inset_days[test_tstep_tb]
        outfile.write("Total TB infections fraction expected: {0} \n".format(expected_tb_infections_fraction))
        outfile.write("Total TB infections: {0} \n".format(actual_tb_infections))
        outfile.write("Total TB infection fraction: {0} \n".format(actual_tb_infection_fraction))
        total_error_tb = math.fabs(expected_tb_infections_fraction - actual_tb_infection_fraction)
        success = True

        if total_error_tb > error_tolerance:
            success = False
            outfile.write("BAD: Total error is {0} for TB, error tolerance is {1}\n".format(total_error_tb, error_tolerance))

        outfile.write(sft.format_success_msg(success))
# endregion

# region TBDrug Support
def get_test_event( campaign_events, target_key=ParamKeys.KEY_EventName,
                    target_value="InterventionUnderTest"):
    test_events = []
    for event in campaign_events:
        if target_key in event.keys() and target_value in event[target_key]:
            test_events.append(event)
    if len(test_events) != 1:
        raise ValueError("There should only be one test_event, we found {0}".format(test_events))
    test_event = test_events[0]
    return test_event

def get_drug_type( drug_event):
    iv_config = drug_event[CampaignKeys.Event_Coordinator_Config][CampaignKeys.Intervention_Config]
    return iv_config[CampaignKeys.Key_TB_Drug_Name]
# endregion
