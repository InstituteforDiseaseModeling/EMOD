#!/usr/bin/python

import json
import os.path as path
import dtk_test.dtk_sft as sft
import math

KEY_NEW_INFECTIONS = "New Infections"
KEY_STATISTICAL_POPULATION = "Statistical Population"
KEY_INITIAL_EFFECT = "Initial_Effect"
KEY_DEMOGRAPHIC_COVERAGE = "Demographic_Coverage"
KEY_DEMOGRAPHIC_COVERAGE_V = "Demographic_Coverage_Vaccine"
KEY_DEMOGRAPHIC_COVERAGE_O = "Demographic_Coverage_Outbreak"

Outbreak_Start_Day = 5

"""
The OutbreakIndividual intervention is used to seed infected individuals in a population
without changing population via having individuals in the population acquire new infections
at random. The behavior of OutbreakIndividual is such that it does not check a chosen
individual's immune state before infecting them. ImmunityRespectingOutbreakIndividual was
developed to enable a behavior mode in which individual immune state is considered.

Specifically, individuals are chosen at random for potential infection according to the
Demographic_Coverage of the event. Once an individual is chosen, their probability of acquiring
a new infection is equal to the produce of their acquisition immune modifier and any acquisition-reducing
intervention effects - (susceptibility->getModAcquire()*interventions->GetInterventionReducedAcquire()).
In this way, selected individuals who are fully susceptible will definitely acquire an outbreak,
individuals who are fully protected will definitely not, and those in between may or may not acquire infection.

This behavior is useful for testing acquisition-reducing features, by providing a direct
means of challenging individuals with infection. It is useful in simulations in which it is
important that immune individuals do not become reinfected, ever. It is also useful in simulations
in which immune individuals are heavily downsampled, as the remaining immune individuals, who may
represent tens or hundreds of people, may have an outsized effect on disease progression if they
become randomly infected through a standard OutbreakIndividual.
"""
def parse_json_report(output_folder="output", propertyreport_name="InsetChart.json", debug=False):
    """
    creates report_data_obj structure with keys
    :param propertyreport_name: InsetChart.json file with location (output/InsetChart.json)
    :returns: report_data_obj structure, dictionary with KEY_NEW_INFECTIONS etc., keys (e.g.)
    """
    propertyreport_path = path.join(output_folder, propertyreport_name)
    with open(propertyreport_path) as infile:
        icj = json.load(infile)["Channels"]

    report_data_obj = {}
    keys = [KEY_NEW_INFECTIONS, KEY_STATISTICAL_POPULATION]
    for key in keys:
        new_infections = icj[key]["Data"]
        report_data_obj[key] = new_infections

    if debug:
        with open("data_InsetChart.json", "w") as outfile:
            json.dump(report_data_obj, outfile, indent=4)

    return report_data_obj

def load_campaign_file(campaign_filename="campaign.json", debug = False):
    """reads campaign file and populates campaign_obj

    :param campaign_filename: campaign.json file
    :returns: campaign_obj structure, dictionary with KEY_INITIAL_EFFECT, etc., keys (e.g.)
    """
    with open(campaign_filename) as infile:
        cf = json.load(infile)
    campaign_obj = {}
    initial_effect = cf["Events"][0]["Event_Coordinator_Config"]["Intervention_Config"]["Waning_Config"][KEY_INITIAL_EFFECT]
    campaign_obj[KEY_INITIAL_EFFECT] = initial_effect
    demographic_coverage_v = cf["Events"][0]["Event_Coordinator_Config"][KEY_DEMOGRAPHIC_COVERAGE]
    campaign_obj[KEY_DEMOGRAPHIC_COVERAGE_V] = demographic_coverage_v
    demographic_coverage_o = cf["Events"][1]["Event_Coordinator_Config"][KEY_DEMOGRAPHIC_COVERAGE]
    campaign_obj[KEY_DEMOGRAPHIC_COVERAGE_O] = demographic_coverage_o
    if debug:
        print( campaign_obj )

    return campaign_obj

def create_report_file(campaign_obj, report_data_obj, report_name, debug):
    with open(report_name, "w") as outfile:
        success = True
        timestep = Outbreak_Start_Day
        new_infection = report_data_obj[KEY_NEW_INFECTIONS][timestep]
        statistical_population = report_data_obj[KEY_STATISTICAL_POPULATION][timestep]
        initial_effect = campaign_obj[KEY_INITIAL_EFFECT]
        demographic_coverage_v = campaign_obj[KEY_DEMOGRAPHIC_COVERAGE_V]
        demographic_coverage_o = campaign_obj[KEY_DEMOGRAPHIC_COVERAGE_O]
        immunity = initial_effect * demographic_coverage_v
        expected_new_infection = statistical_population * (1.0 - immunity) * demographic_coverage_o
        tolerance = 0.0 if expected_new_infection == 0.0 else 2e-2 * statistical_population
        if math.fabs(new_infection - expected_new_infection) > tolerance:
            success = False
            outfile.write("BAD: At time step {0}, new infections are {1} as reported, expected {2}.\n".format(
                timestep, new_infection, expected_new_infection))
        outfile.write(sft.format_success_msg(success))
    sft.plot_data(new_infection,expected_new_infection,
                           label1= "Actual", label2 = "Expected",
                           ylabel="new infection", xlabel="red: actual data, blue: expected data",
                           title = "Actual new infection vs. expected new infection",
                           category = 'New_infection',show = True )
    if debug:
        print( "SUMMARY: Success={0}\n".format(success) )
    return success

def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json",campaign_filename="campaign.json",
                 demographics_filename = "../../demographics.json",
                 propertyreport_name="InsetChart.json",
                 report_name=sft.sft_output_filename,
                 debug=True):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "campaign_filename: " + campaign_filename + "\n" )
        print( "demographics_filename: " + demographics_filename + "\n" )
        print( "propertyreport_name: " + propertyreport_name + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )

    sft.wait_for_done()

    report_data_obj = parse_json_report(output_folder, propertyreport_name, debug)
    campaign_obj = load_campaign_file(campaign_filename, debug)
    create_report_file(campaign_obj, report_data_obj, report_name, debug)

if __name__ == "__main__":
    # execute only if run as a script
    application( "output" )
