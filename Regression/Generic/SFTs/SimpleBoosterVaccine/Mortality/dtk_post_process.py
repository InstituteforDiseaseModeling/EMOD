#!/usr/bin/python

import json
import os.path as path
import dtk_sft as sft
import math
import dtk_SimpleBoosterVaccine_Support as sbvs

KEY_TOTAL_TIMESTEPS = "Simulation_Duration"
KEY_NEW_INFECTIONS = "New Infections"
KEY_STATISTICAL_POPULATION = "Statistical Population"
KEY_DISEASE_DEATHS = "Disease Deaths"
Interventions = [   "1st: vaccine only",
                    "2nd: booster_l only",
                    "3rd: vaccine + booster_l",
                    "4th: 2 booster_l",
                    "5th: vaccine + 2 booster_l",
                    "6th: vaccine + 2 booster_h",
                    "7th: 2 booster_h",
                    "8th: vaccine + booster_h",
                    "9th: booster_l + 2 booster_h",
                    "10th: booster_h + booster_l",
                    "11th: 2 booster_h + booster_l"]
Outbreak_Start_Day = 5
Timesteps_Between_Repetitions = 20
Number_Repetitions = 11

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

def parse_json_report(output_folder="output", propertyreport_name="InsetChart.json", debug=False):
    """
    creates report_data_obj structure with keys
    :param propertyreport_name: PropertyReport.json file with location (output/PropertyReport.json)
    :returns: report_data_obj structure, dictionary with KEY_NEW_INFECTIONS etc., keys (e.g.)
    """
    propertyreport_path = path.join(output_folder, propertyreport_name)
    with open(propertyreport_path) as infile:
        icj = json.load(infile)["Channels"]

    report_data_obj = {}
    new_infections = icj[KEY_NEW_INFECTIONS]["Data"]
    report_data_obj[KEY_NEW_INFECTIONS] = new_infections
    disease_deaths = icj[KEY_DISEASE_DEATHS]["Data"]
    report_data_obj[KEY_DISEASE_DEATHS] = disease_deaths
    statistical_population = icj[KEY_STATISTICAL_POPULATION]["Data"]
    report_data_obj[KEY_STATISTICAL_POPULATION] = statistical_population

    if debug:
        with open("data_InsetChart.json", "w") as outfile:
            json.dump(report_data_obj, outfile, indent=4)

    return report_data_obj

def calc_effect(debug = False):
    """
    calculate the expected MortalityBlocking effect before outbreak.
    :param debug:
    :return: immunity
    """
    effects = []
    # # "1st: vaccine only"
    effects.append(sbvs.apply_vaccines([sbvs.Vaccine.VACCINE]))
    # # "2nd: booster_l only"
    effects.append(sbvs.apply_vaccines([sbvs.Vaccine.BOOSTER_LOW]))
    # # 3rd: vaccine + booster_l
    effects.append(sbvs.apply_vaccines([sbvs.Vaccine.VACCINE, sbvs.Vaccine.BOOSTER_LOW]))
    # # 4th: 2 booster_l
    effects.append(sbvs.apply_vaccines([sbvs.Vaccine.BOOSTER_LOW, sbvs.Vaccine.BOOSTER_LOW]))
    # # 5th: vaccine + 2 booster_l
    effects.append(sbvs.apply_vaccines([sbvs.Vaccine.VACCINE, sbvs.Vaccine.BOOSTER_LOW, sbvs.Vaccine.BOOSTER_LOW]))
    # # 6th: vaccine + 2 booster_h
    effects.append(sbvs.apply_vaccines([sbvs.Vaccine.VACCINE, sbvs.Vaccine.BOOSTER_HIGH, sbvs.Vaccine.BOOSTER_HIGH]))
    # # 7th: 2 booster_h
    effects.append(sbvs.apply_vaccines([sbvs.Vaccine.BOOSTER_HIGH, sbvs.Vaccine.BOOSTER_HIGH]))
    # # 8th: vaccine + booster_h
    effects.append(sbvs.apply_vaccines([sbvs.Vaccine.VACCINE, sbvs.Vaccine.BOOSTER_HIGH]))
    # # 9th: booster_l + 2 booster_h
    effects.append(sbvs.apply_vaccines([sbvs.Vaccine.BOOSTER_LOW, sbvs.Vaccine.BOOSTER_HIGH, sbvs.Vaccine.BOOSTER_HIGH]))
    # # 10th: booster_h + booster_l
    effects.append(sbvs.apply_vaccines([sbvs.Vaccine.BOOSTER_HIGH, sbvs.Vaccine.BOOSTER_LOW]))
    # # 11th: 2 booster_h + booster_l
    effects.append(sbvs.apply_vaccines([sbvs.Vaccine.BOOSTER_HIGH, sbvs.Vaccine.BOOSTER_HIGH, sbvs.Vaccine.BOOSTER_LOW]))

    if debug:
        print( effects )
    return effects

def create_report_file(report_data_obj, report_name, debug):
    with open(report_name, "w") as outfile:
        success = True
        timestep = Outbreak_Start_Day
        effects = calc_effect(debug)
        new_infections = []
        new_disease_deaths = []
        expected_new_disease_deaths = []
        actual_effects = []
        pre_disease_death = 0

        for i in range(len(Interventions)):
            new_infection = report_data_obj[KEY_NEW_INFECTIONS][timestep]
            disease_death = report_data_obj[KEY_DISEASE_DEATHS][timestep]
            new_disease_death = disease_death - pre_disease_death
            effect = effects[i]
            expected_new_disease_death = (1.0 - effect) * new_infection
            tolerance = 0.0 if expected_new_disease_death == 0.0 else 3e-2 * new_infection
            actual_effect = 1.0 - new_disease_death/float(new_infection) if new_infection != 0 else 0.0
            if math.fabs(new_disease_death - expected_new_disease_death) > tolerance:
                success = False
                outfile.write("BAD: At time step {0}, outbreak {1}, {2} reported new disease death, expected {3}.\n".format(
                    timestep, Interventions[i], new_disease_death, expected_new_disease_death))
                outfile.write("actual MortalityBlocking effect is {0}, expected {1}.\n".format(actual_effect, effect))
            new_disease_deaths.append(new_disease_death)
            expected_new_disease_deaths.append(expected_new_disease_death)
            actual_effects.append(actual_effect)
            new_infections.append(new_infection)
            timestep += Timesteps_Between_Repetitions
            pre_disease_death = disease_death
        sft.plot_data(new_disease_deaths,expected_new_disease_deaths,
                               label1= "Actual", label2 = "Expected",
                               xlabel= "outbreak",ylabel="disease death",
                               title = "Actual disease death vs. expected disease death",
                               category = 'Disease_death',show = True )
        sft.plot_data(new_disease_deaths,new_infections,
                               label1= "death", label2 = "infection",
                               xlabel= "outbreak",ylabel="population",
                               title = "Actual disease death vs. new infections",
                               category = 'disease_death_vs_new_infections',show = True )
        if debug:
            with open("New_disease_death.txt", "w") as file:
                for i in range(len(new_disease_deaths)):
                    file.write("{0}, {1}.\n".format(new_disease_deaths[i],expected_new_disease_deaths[i]))
            with open("Effects.txt", "w") as file:
                for i in range(len(actual_effects)):
                    file.write("{0}, {1}.\n".format(actual_effects[i],effects[i]))

        outfile.write(sft.format_success_msg(success))
        if debug:
            print( "SUMMARY: Success={0}\n".format(success) )
        return success

def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json",campaign_filename="campaign.json",
                 demographics_filename = "../../demographics.json",
                 propertyreport_name="InsetChart.json",
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
