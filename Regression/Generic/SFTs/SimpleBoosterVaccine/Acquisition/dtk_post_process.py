#!/usr/bin/python

import json
import os.path as path
import dtk_test.dtk_sft as sft
import math

KEY_TOTAL_TIMESTEPS = "Simulation_Duration"
KEY_NEW_INFECTIONS_GROUP = [ "New Infections:QualityOfCare:1_VaccineOnly",
                             "New Infections:QualityOfCare:2_BoosterOnly",
                             "New Infections:QualityOfCare:3_VaccinePlusBooster",
                             "New Infections:QualityOfCare:4_BoosterPlusVaccine",
                             "New Infections:QualityOfCare:5_TwoBoosters"]
KEY_STATISTICAL_POPULATION_GROUP = [ "Statistical Population:QualityOfCare:1_VaccineOnly",
                                     "Statistical Population:QualityOfCare:2_BoosterOnly",
                                     "Statistical Population:QualityOfCare:3_VaccinePlusBooster",
                                     "Statistical Population:QualityOfCare:4_BoosterPlusVaccine",
                                     "Statistical Population:QualityOfCare:5_TwoBoosters"]
Outbreak_Start_Day = 10
Timesteps_Between_Repetitions = 30
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
    calculate the expected immunity for each group. we are using values in Campaign file.
    :param debug:
    :return: immunity
    """
    prime = 0.2
    boost = 0.45
    # before 1st outbreak, the booster threshold is set to 0.0, after 1st outbreak, it's 0.6
    threshold_1 = 0.0
    threshold_2 = 0.6
    initial = 0.55
    immunity = []
    immunity.append([initial, initial])   # 1_VaccineOnly
    immunity.append([prime, prime])       # 2_BoosterOnly
    immunity.append([initial + (1.0 - initial)* boost, initial + (1.0 - initial)* prime]) # 3_VaccinePlusBooster
    immunity.append([prime + (1.0 - prime) * initial, prime + (1.0 - prime) * initial])   # 4_BoosterPlusVaccine
    immunity.append([prime + (1.0 - prime) * boost, prime + (1.0 - prime) * prime])       # 5_TwoBoosters
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
        if not report_data_obj:
            success = False
            outfile.write("BAD: There is no data in the PropertyReport report")
        else:
            for i in range(Number_Repetitions):
                for j in range(len(KEY_NEW_INFECTIONS_GROUP)):
                    new_infection = report_data_obj[KEY_NEW_INFECTIONS_GROUP[j]][timestep]
                    statistical_population = report_data_obj[KEY_STATISTICAL_POPULATION_GROUP[j]][timestep]
                    expected_new_infection = statistical_population * (1.0 - immunity[j][i])
                    tolerance = 0.0 if expected_new_infection == 0.0 else 2e-2 * statistical_population
                    result = f"At time step {timestep}, {KEY_NEW_INFECTIONS_GROUP[j]}" \
                             f" new infections: {new_infection}, expected infections: {expected_new_infection}" \
                             f"  with tolerance {tolerance}.\n"
                    if math.fabs(new_infection - expected_new_infection) > tolerance:
                        success = False
                        outfile.write(f"BAD:  {result}")
                    else:
                        outfile.write(f"GOOD: {result}")
                    new_infections.append(new_infection)
                    expected_new_infections.append(expected_new_infection)
                timestep += Timesteps_Between_Repetitions
        outfile.write(sft.format_success_msg(success))
    sft.plot_data(new_infections,expected_new_infections,
                           label1= "Actual", label2 = "Expected",
                           xlabel= "group: 0-4 outbreak 1, 5-9 outbreak 2",ylabel="new infection",
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
