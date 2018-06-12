#!/usr/bin/python

import json
import os.path as path
import dtk_sft as sft
import math

KEY_TOTAL_TIMESTEPS = "Simulation_Duration"
KEY_NEW_INFECTIONS_GROUP = [ "New Infections:QualityOfCare:1_VaccineOnly",
                             "New Infections:QualityOfCare:2_2Vaccines",
                             "New Infections:QualityOfCare:3_3Vaccines",
                             "New Infections:QualityOfCare:4_5Vaccines"]
KEY_STATISTICAL_POPULATION_GROUP = [ "Statistical Population:QualityOfCare:1_VaccineOnly",
                                     "Statistical Population:QualityOfCare:2_2Vaccines",
                                     "Statistical Population:QualityOfCare:3_3Vaccines",
                                     "Statistical Population:QualityOfCare:4_5Vaccines"]
Outbreak_Start_Day = 10
Timesteps_Between_Repetitions = 30
Number_Repetitions = 1

"""
Christian Selinger needs the ability for the vaccine efficacy to be additive. He is distributing
multiple does of the vaccine - one initial and multiple boosters. He needs the ability to specify
his vaccine efficacys such that the boosters are added to the previous ones.

The current/previous method was to multiply efficacys when a person had multiple vaccines.
"""
def parse_json_report(output_folder="output", propertyreport_name="PropertyReport.json", debug=False):
    """
    creates report_data_obj structure with keys
    :param propertyreport_name: PropertyReport.json file with location (output/PropertyReport.json)
    :returns: report_data_obj structure, dictionary with KEY_NEW_INFECTIONS etc., keys (e.g.)
    """
    propertyreport_path = path.join(output_folder, propertyreport_name)
    with open(propertyreport_path) as infile:
        icj = json.load(infile)["Channels"]

    report_data_obj = {}
    for key in KEY_NEW_INFECTIONS_GROUP:
        new_infections = icj[key]["Data"]
        report_data_obj[key] = new_infections
    for key in KEY_STATISTICAL_POPULATION_GROUP:
        statistical_population = icj[key]["Data"]
        report_data_obj[key] = statistical_population

    if debug:
        with open("data_PropertyReport.json", "w") as outfile:
            json.dump(report_data_obj, outfile, indent=4)

    return report_data_obj

def calc_immunity(debug = False):
    """
    calculate the expected immunity for each group. we are using hardcoded values in Campaign file.
    :param debug:
    :return: immunity
    """
    initial = 0.25
    immunity = []
    immunity.append(initial)   # 1_VaccineOnly
    immunity.append(2 * initial)       # 2_2Vaccines
    immunity.append(3 * initial) # 3_3Vaccines
    immunity.append(1.0)   # 4_5Vaccines
    if debug:
        print( immunity )
    return immunity

def create_report_file(report_data_obj, report_name, debug):
    with open(report_name, "w") as outfile:
        success = True
        immunity = calc_immunity(debug)
        new_infections = []
        expected_new_infections = []
        timestep = Outbreak_Start_Day
        for i in range(len(KEY_NEW_INFECTIONS_GROUP)):
            new_infection = report_data_obj[KEY_NEW_INFECTIONS_GROUP[i]][timestep]
            statistical_population = report_data_obj[KEY_STATISTICAL_POPULATION_GROUP[i]][timestep]
            expected_new_infection = statistical_population * (1.0 - immunity[i])
            tolerance = 0.0 if expected_new_infection == 0.0 else 2e-2 * statistical_population
            if math.fabs(new_infection - expected_new_infection) > tolerance:
                success = False
                outfile.write("BAD: At time step {0}, {1} has {2} reported, expected {3}.\n".format(
                    timestep, KEY_NEW_INFECTIONS_GROUP[i], new_infection, expected_new_infection))
            new_infections.append(new_infection)
            expected_new_infections.append(expected_new_infection)
        outfile.write(sft.format_success_msg(success))
    sft.plot_data(new_infections,expected_new_infections,
                           label1= "Actual", label2 = "Expected",
                           xlabel= "0: VaccineOnly, 1: 2Vaccines, 2: 3Vaccines, 3: 5Vaccines",ylabel="new infection",
                           title = "Actual new infection vs. expected new infection",
                           category = 'New_infections',show = True )
    if debug:
        print( "SUMMARY: Success={0}\n".format(success) )
    return success

def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json",campaign_filename="campaign.json",
                 demographics_filename = "../../demographics.json",
                 propertyreport_name="PropertyReport.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
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
    create_report_file(report_data_obj, report_name, debug)

if __name__ == "__main__":
    # execute only if run as a script
    application( "output" )
