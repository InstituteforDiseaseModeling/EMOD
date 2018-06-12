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
KEY_DISEASE_DEATHS_GROUP = ["Disease Deaths:QualityOfCare:1_Seed_Control",
                            "Disease Deaths:QualityOfCare:2_Seed_Test",
                            "Disease Deaths:QualityOfCare:3_Control",
                            "Disease Deaths:QualityOfCare:4_Test"]

Outbreak_Start_Day = 5
Number_Repetitions = 2
Timesteps_Between_Repetitions = 10

Prime_Acquire=0.1
Prime_Transmit=0.2
Prime_Mortality=0.3
Boost_Acquire= 0.7
Boost_Transmit=0.5
Boost_Mortality=1.0
threshold = 0.0
Outbreak_Demographic_Coverage = 0.5

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
    for key in KEY_DISEASE_DEATHS_GROUP:
        disease_death = icj[key]["Data"]
        report_data_obj[key] = disease_death
    if debug:
        with open("data_PropertyReport.json", "w") as outfile:
            json.dump(report_data_obj, outfile, indent=4, sort_keys= True)

    return report_data_obj

def create_report_file(report_data_obj, report_name, debug):
    with open(report_name, "w") as outfile:
        success = True
        timestep = Outbreak_Start_Day
        effect_a = Prime_Acquire
        effect_t = Prime_Transmit
        effect_m = Prime_Mortality
        new_infections = []
        statistical_populations = []
        new_disease_deaths = []
        for x in range(Number_Repetitions):
            num_group = len(KEY_NEW_INFECTIONS_GROUP)
            for i in range(num_group):
                new_infection = report_data_obj[KEY_NEW_INFECTIONS_GROUP[i]][timestep]
                statistical_population = report_data_obj[KEY_STATISTICAL_POPULATION_GROUP[i]][timestep]
                pre_disease_death = report_data_obj[KEY_DISEASE_DEATHS_GROUP[i]][int(timestep + i/2 - 1)] # disease death in the last 2 groups happen 1 day later than the first 2 groups.
                disease_death = report_data_obj[KEY_DISEASE_DEATHS_GROUP[i]][int(timestep + i/2)]
                new_disease_death = disease_death - pre_disease_death
                new_infections.append(new_infection)
                statistical_populations.append(statistical_population)
                new_disease_deaths.append(new_disease_death)
            # test acquisition blocking
            new_infection_seed_test = new_infections[1 + x * num_group]
            statistical_population_seed_test = statistical_populations[1 + x * num_group]
            expected_new_infection_seed_test = statistical_population_seed_test * (1.0 - effect_a) * Outbreak_Demographic_Coverage
            tolerance_1 = 0.0 if expected_new_infection_seed_test == 0.0 else 2e-2 * statistical_population_seed_test
            if math.fabs(new_infection_seed_test - expected_new_infection_seed_test) > tolerance_1:
                success = False
                outfile.write("BAD: At time step {0}, {1} reported new infections in Group 2_Seed_Test, expected {2}.\n".format(
                    timestep, new_infection_seed_test, expected_new_infection_seed_test))
            # test transmission blocking
            new_infection_seed_control = new_infections[0 + x * num_group]
            new_infection_control = new_infections[2 + x * num_group]
            new_infection_test = new_infections[3+ x * num_group]
            expected_new_infection_test = (1.0 - effect_t) * new_infection_control * new_infection_seed_test/float(new_infection_seed_control)
            statistical_population_test = statistical_populations[3]
            tolerance_2 = 0.0 if expected_new_infection_test == 0.0 else 2e-2 * statistical_population_test
            if math.fabs(new_infection_test - expected_new_infection_test) > tolerance_2:
                success = False
                outfile.write("BAD: At time step {0}, {1} reported new infections in Group 4_Test, expected {2}.\n".format(
                    timestep, new_infection_test, expected_new_infection_test))
            #test mortality blocking
            disease_death_seed_test = new_disease_deaths[1 + x * num_group]
            expected_disease_death_seed_test = new_infection_seed_test * (1.0 - effect_m)
            tolerance_3 = 0.0 if expected_disease_death_seed_test == 0.0 else 2e-2 * new_infection_seed_test
            if math.fabs(disease_death_seed_test - expected_disease_death_seed_test) > tolerance_3:
                success = False
                outfile.write("BAD: At time step {0}, {1} reported disease deaths in Group 2_Seed_Test, expected {2}.\n".format(
                    timestep, disease_death_seed_test, expected_disease_death_seed_test))
            timestep += Timesteps_Between_Repetitions
            effect_a = effect_a + (1.0 - effect_a) * Boost_Acquire
            effect_t = effect_t + (1.0 - effect_t) * Boost_Transmit
            effect_m = effect_m + (1.0 - effect_m) * Boost_Mortality
        outfile.write(sft.format_success_msg(success))
    sft.plot_data(new_infections,new_disease_deaths,
                           label1= "new_infections", label2 = "disease_death",
                           xlabel= "0&4:Seed_Control, 1&5:Seed_Test, 2&6:Control, 4&7:Test",
                           title = "new_infections vs. new_disease_death",
                           category = 'New_infections_vs_new_disease_death',show = True )
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
