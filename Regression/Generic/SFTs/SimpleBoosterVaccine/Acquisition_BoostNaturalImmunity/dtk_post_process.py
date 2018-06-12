#!/usr/bin/python

import json
import os.path as path
import dtk_sft as sft
import math

KEY_TOTAL_TIMESTEPS = "Simulation_Duration"
KEY_IMMUNITY_ACQUISITION_FACTOR = "Post_Infection_Acquisition_Multiplier"
KEY_NEW_INFECTIONS_GROUP = [ "New Infections:QualityOfCare:1_Control",
                             "New Infections:QualityOfCare:2_Test"]
KEY_STATISTICAL_POPULATION_GROUP = [ "Statistical Population:QualityOfCare:1_Control",
                                     "Statistical Population:QualityOfCare:2_Test"]
Outbreak_Start_Day = 2
Timesteps_Between_Repetitions = 5
Number_Repetitions = 2

"""
SimpleBoosterVaccine is derived from SimpleVaccine, and preserves many of the same parameters.
The behavior is much like SimpleVaccine, except that upon distribution and successful take,
the vaccine's effect is determined by the recipient's immune state. If the recipient's immunity
modifier in the corresponding channel (Acquisition, Transmission, Mortality) is above a user-specified
threshold (Boost_Threshold), then the vaccine's initial effect will be equal to the parameter
Prime_Effect. If the recipient's immune modifier is below this threshold, then the vaccine's
initial effect will be equal to the parameter Boost_Effect. After distribution, the effect evolves
according to the Waning_Config, just like a Simple Vaccine. In essence, this intervention provides
a SimpleVaccine with one effect to all naive (below- threshold) individuals, and another effect to
all primed (above-threshold) individuals; this behavior is intended to mimic, to some degree,
biological priming and boosting.

All parameters necessary to define a SimpleVaccine are also necessary for a SimpleBoosterVaccine
(though the Initial_Effect of the Waning_Config will be overwritten by one of Prime_Effect or
Boost_Effect upon distribution).
"""
def load_emod_parameters(config_filename="config.json"):
    """
    reads config file and populates params_obj
    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_TOTAL_TIMESTEPS, etc., keys (e.g.)
    """
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {}
    param_obj[KEY_TOTAL_TIMESTEPS] = cdj[KEY_TOTAL_TIMESTEPS]
    param_obj[KEY_IMMUNITY_ACQUISITION_FACTOR] = cdj[KEY_IMMUNITY_ACQUISITION_FACTOR]
    return param_obj

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

def calc_expected_new_infection_portion(param_obj, debug = False):
    """
    calculate the expected new infection(percentage) for each group. we are using hardcoded values in Campaign file.
    :param debug:
    :return: immunity
    """
    prime = 0.2
    boost = 0.6
    # threshold_1 = 0.0
    new_infection_portions = []
    natural_immunuty = 1.0 - param_obj[KEY_IMMUNITY_ACQUISITION_FACTOR]
    new_infection_portions.append(1.0 - natural_immunuty)                       # 1_Control
    new_infection_portions.append((1.0 - natural_immunuty)* (1.0 - boost))       # 2_Test
    if debug:
        print( new_infection_portions )
    return new_infection_portions

def create_report_file(param_obj, report_data_obj, report_name, debug):
    with open(report_name, "w") as outfile:
        success = True
        new_infection_portions = calc_expected_new_infection_portion(param_obj, debug)
        new_infections = []
        expected_new_infections = []
        # skip the first outbreak, which gives the natual immunity
        timestep = Outbreak_Start_Day + Timesteps_Between_Repetitions
        for i in range(len(KEY_NEW_INFECTIONS_GROUP)):
            new_infection = report_data_obj[KEY_NEW_INFECTIONS_GROUP[i]][timestep]
            statistical_population = report_data_obj[KEY_STATISTICAL_POPULATION_GROUP[i]][timestep]
            expected_new_infection = statistical_population * (new_infection_portions[i])
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
                           xlabel= "0: Control group, 1: Test group",ylabel="new infection",
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
    param_obj = load_emod_parameters(config_filename)
    report_data_obj = parse_json_report(output_folder, propertyreport_name, debug)
    create_report_file(param_obj, report_data_obj, report_name, debug)

if __name__ == "__main__":
    # execute only if run as a script
    application( "output" )
