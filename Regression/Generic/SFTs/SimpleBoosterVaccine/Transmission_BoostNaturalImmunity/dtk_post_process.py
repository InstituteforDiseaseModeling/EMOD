#!/usr/bin/python

import json
import os.path as path
import dtk_sft as sft
import math

KEY_NEW_INFECTIONS_GROUP = [ "New Infections:QualityOfCare:1_Seed_Control",
                             "New Infections:QualityOfCare:2_Seed_Test",
                             "New Infections:QualityOfCare:3_Control",
                             "New Infections:QualityOfCare:4_Test"
                             ]
KEY_STATISTICAL_POPULATION_GROUP = [ "Statistical Population:QualityOfCare:1_Seed_Control",
                                     "Statistical Population:QualityOfCare:2_Seed_Test",
                                     "Statistical Population:QualityOfCare:3_Control",
                                     "Statistical Population:QualityOfCare:4_Test"
                                     ]
Outbreak_Start_Day = 1
Timesteps_Between_Repetitions = 10
Number_Repetitions = 2
prime = 0.2
boost = 0.6
threshold = 0.0

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

def create_report_file(report_data_obj, report_name, debug):
    with open(report_name, "w") as outfile:
        success = True

        timestep = Outbreak_Start_Day
        tb_effect = prime
        baseline = 2 # group 3_Control is the baseline
        test = 3 # group 4_Test is the test group
        # first outbreak
        pre_new_infection_baseline = report_data_obj[KEY_NEW_INFECTIONS_GROUP[baseline]][timestep]
        pre_new_infection_test= report_data_obj[KEY_NEW_INFECTIONS_GROUP[test]][timestep]
        # second outbreak
        timestep +=  Timesteps_Between_Repetitions
        new_infection_baseline = report_data_obj[KEY_NEW_INFECTIONS_GROUP[baseline]][timestep]
        statistical_population_baseline = report_data_obj[KEY_STATISTICAL_POPULATION_GROUP[baseline]][timestep]
        new_infection_test= report_data_obj[KEY_NEW_INFECTIONS_GROUP[test]][timestep]
        statistical_population_test = report_data_obj[KEY_STATISTICAL_POPULATION_GROUP[test]][timestep]
        # because expected_new_infection_test / ((1.0 - tb_effect)* statistical_population_test)= new_infection_baseline / statistical_population_baseline, so
        expected_new_infection_test = (1.0 - tb_effect) * new_infection_baseline * statistical_population_test / statistical_population_baseline
        tolerance = 0.0 if expected_new_infection_test == 0.0 else 2e-2 * statistical_population_test
        if math.fabs(new_infection_test - expected_new_infection_test) > tolerance:
            success = False
            outfile.write("BAD: At time step {0}, {1} reported new infections in Group 4_Test, expected {2}.\n".format(
                timestep, new_infection_test, expected_new_infection_test))

        sft.plot_data_unsorted([pre_new_infection_baseline, new_infection_baseline],[pre_new_infection_test, new_infection_test],
                               label1= "control_group", label2 = "test_group",
                               xlabel= "0: first outbreak, 1: second outbreak",ylabel="new infection",
                               title = "control vs. test",
                               category = 'New_infections',show = True )

        outfile.write(sft.format_success_msg(success))
        if debug:
            print "SUMMARY: Success={0}\n".format(success)
        return success

def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json",campaign_filename="campaign.json",
                 demographics_filename = "../../demographics.json",
                 propertyreport_name="PropertyReport.json",
                 report_name=sft.sft_output_filename,
                 debug=True):
    if debug:
        print "output_folder: " + output_folder
        print "stdout_filename: " + stdout_filename+ "\n"
        print "config_filename: " + config_filename + "\n"
        print "campaign_filename: " + campaign_filename + "\n"
        print "demographics_filename: " + demographics_filename + "\n"
        print "propertyreport_name: " + propertyreport_name + "\n"
        print "report_name: " + report_name + "\n"
        print "debug: " + str(debug) + "\n"

    sft.wait_for_done()
    report_data_obj = parse_json_report(output_folder, propertyreport_name, debug)
    create_report_file(report_data_obj, report_name, debug)

if __name__ == "__main__":
    # execute only if run as a script
    application( "output" )
