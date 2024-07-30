#!/usr/bin/python

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_vector_life_expectancy_support as vles

"""
Adult_Life_Expectancy:
Verifying that vectors die at the correct rate when it comes to their age and air temperature. 
These are "natural" mortality deaths, not associated with feeding. 
Deaths from feeding (and, also, egg laying) turned off by:
"Anthropophily": 0, 
"Days_Between_Feeds": 10, 
"Egg_Batch_Size": 50, 
"Infectious_Human_Feed_Mortality_Factor": 0, # just in case
"Human_Feeding_Mortality": 0, 

Note: Please note that the test is less stable with Life_Expectancy > 20 as vectors are more likely to reach
age of 60 days and then be pooled together into the same cohort, which makes it hard to track their deaths and
how long they live. Cohorts that get dismissed or added to by being pooled together are "cleaned out" and only cohorts
that go from life do total death as a total cohort are used in the verification.

This assumes there one vector species. 
Sweep suggestions:
Run_Number, Base_Air_Temperature, Male_Life_Expectancy, Adult_Life_Expectancy, Days_Between_Feeds
"""

KEY_CONFIG_NAME = "Config_Name"
ADULT_LIFE_EXPECTANCY = "Adult_Life_Expectancy"
MALE_LIFE_EXPECTANCY = "Male_Life_Expectancy"
AIR_TEMPERATURE = "Base_Air_Temperature"
SIM_DURATION = "Simulation_Duration"
MICROSPORIDIA_MALE_MORTALITY_MODIFIER = "Male_Mortality_Modifier"
MICROSPORIDIA_FEMALE_MORTALITY_MODIFIER = "Female_Mortality_Modifier"


def create_report_file(param_obj, deaths_df, report_name, debug=False):
    with open(report_name, "w") as outfile:
        config_name = param_obj[KEY_CONFIG_NAME]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        microsporidia_male_mortality_modifier = param_obj[MICROSPORIDIA_MALE_MORTALITY_MODIFIER]
        microsporidia_female_mortality_modifier = param_obj[MICROSPORIDIA_FEMALE_MORTALITY_MODIFIER]
        life_expectancy_male = param_obj[MALE_LIFE_EXPECTANCY]
        life_expectancy_female = param_obj[ADULT_LIFE_EXPECTANCY]
        temperature = param_obj[AIR_TEMPERATURE]
        if deaths_df.empty:
            success = False
            outfile.write("No data found in vector_deaths_df, please re-run with debug=True to see the csv file.\n")
        else:
            mortality_checks_male = []
            mortality_checks_female = []
            mortality_checks_male_microsporidia = []
            mortality_checks_female_microsporidia = []
            cohort_ids = deaths_df.cohort_id.unique()
            for current_id in cohort_ids:
                cohort_deaths = deaths_df[deaths_df.cohort_id == current_id]
                if cohort_deaths.sex.nunique() != 1:
                    success = False
                    outfile.write(f"BAD: cohort_id = {current_id} has multiple values in the \"sex\" column.\n")
                cohort_sex = cohort_deaths["sex"].iloc[0]
                if cohort_deaths["age"].min() > 1:
                    if debug:
                        outfile.write(f"Ignoring {cohort_sex} cohort_id={current_id}, because its age doesn't "
                                      f"start at one.\n")
                    deaths_df = deaths_df[deaths_df.cohort_id != current_id]
                # filtering out cohorts where not all vectors have died
                elif not vles.check_all_vectors_died(cohort_deaths, cohort_sex, outfile, debug):
                    deaths_df = deaths_df[deaths_df.cohort_id != current_id]
                else:
                    life_expectancy = life_expectancy_female if cohort_sex == "female" else life_expectancy_male
                    mortality_modifier = 1
                    if cohort_deaths["microsporidia"].iloc[0] == 1:
                        mortality_modifier = microsporidia_female_mortality_modifier if cohort_sex == "female" else microsporidia_male_mortality_modifier
                    test_successes, test_failures, actual_average_lifespan = vles.daily_mortality_check_average_lifespan_calculation(
                        cohort_deaths,
                        life_expectancy,
                        temperature,
                        outfile,
                        aging=True,
                        mortality_modifier=mortality_modifier)
                    if cohort_sex == "female":
                        if cohort_deaths["microsporidia"].iloc[0] == 1:
                            mortality_checks_female_microsporidia.append([test_successes, test_failures])
                        else:
                            mortality_checks_female.append([test_successes, test_failures])
                    else:
                        if cohort_deaths["microsporidia"].iloc[0] == 1:
                            mortality_checks_male_microsporidia.append([test_successes, test_failures])
                        else:
                            mortality_checks_male.append([test_successes, test_failures])

            success_rate = 0.90
            if not vles.check_results(mortality_checks_male, "male ", success_rate, outfile):
                success = False
            if not vles.check_results(mortality_checks_female, "female", success_rate, outfile):
                success = False
            if not vles.check_results(mortality_checks_male_microsporidia, "male with microsporidia ", success_rate, outfile):
                success = False
            if not vles.check_results(mortality_checks_female_microsporidia, "female with microsporidia", success_rate, outfile):
                success = False

            sexes = ["female", "male"]
            microsporidia = [0, 1]
            population_by_sex = {}
            for has_microsporidia in microsporidia:
                for sex in sexes:
                    life_expectancy = life_expectancy_female if sex == "female" else life_expectancy_male
                    mortality_modifier = 1
                    if has_microsporidia == 1:
                        mortality_modifier = microsporidia_female_mortality_modifier if sex == "female" else microsporidia_male_mortality_modifier
                    sub_df = deaths_df[deaths_df.sex == sex]
                    sub_df = sub_df[deaths_df.microsporidia == has_microsporidia].groupby(["age"], as_index=False).sum()
                    if sub_df.empty:
                        outfile.write(f"DATAFRAME EMPTY! sex {sex} microsporidia {has_microsporidia}\n")
                    population_by_sex[sex] = sub_df["population_before_deaths"].tolist()
                    initial_population = population_by_sex[sex][0]
                    dtk_sft.plot_data(population_by_sex[sex], dist2=None, label1=f"{sex} Vectors of that Age",
                                      label2=None, title=f"{sex} Vector Population Age Distribution",
                                      xlabel="Age (in days)", ylabel="Number of vectors",
                                      category=f'{sex}_vector_age_distribution_{"microsporidia" if has_microsporidia else "no_microsporidia"}')
                    sub_df.to_csv("sub_df.csv")
                    test_successes, test_failures, actual_average_lifespan = \
                        vles.daily_mortality_check_average_lifespan_calculation(sub_df,
                                                                                life_expectancy,
                                                                                temperature,
                                                                                outfile,
                                                                                aging=True,
                                                                                mortality_modifier=mortality_modifier)

                    total_deaths = sub_df["dead_without_feed"].sum()
                    theoretical_average_lifespan = vles.brute_force_theoretical_lifespan(initial_population,
                                                                                         life_expectancy,
                                                                                         temperature, aging=True,
                                                                                         mortality_modifier=mortality_modifier)
                    error = 0.6  # lets expect the actual average lifespan to be within 0.5 of a day from theoretical
                    if abs(actual_average_lifespan - theoretical_average_lifespan) > error:
                        success = False
                        outfile.write(f"BAD: Expected average life span for {sex} vectors with aging and "
                                      f"{'' if has_microsporidia else 'no'} microsporidia to be about "
                                      f"{theoretical_average_lifespan}, based on life expectancy of {life_expectancy} days,"
                                      f" as they die faster from aging when older, and due to dry heat mortality, "
                                      f"got {actual_average_lifespan} after looking at {total_deaths} deaths. This is "
                                      f" not within {error} of a day.\n")
                    else:
                        outfile.write(f"GOOD: Expected average life span for {sex} vectors with aging and"
                                      f"{'' if has_microsporidia else 'no'} microsporidia to be about "
                                      f"{theoretical_average_lifespan}, based on life expectancy of {life_expectancy} "
                                      f"days, as they die faster from aging when older, and due to dry heat mortality "
                                      f"and got {actual_average_lifespan} after looking at {total_deaths} deaths. This is "
                                      f"within {error} of a day.\n")
                    if debug or not success:
                        sub_df.to_csv(f"{sex}_sub_df.csv")

                dtk_sft.plot_data(population_by_sex["female"], dist2=population_by_sex["male"],
                                  label1=f"Female Vectors(life expectancy = {life_expectancy_female})",
                                  label2=f"fMale Vectors(life expectancy = {life_expectancy_male})",
                                  title="Both Sexes Vector Population Age Distribution",
                                  xlabel='Age (in days)', ylabel="Number of vectors",
                                  category=f"both_sexes_vector_age_distribution_{'microsporidia' if has_microsporidia else 'no_microsporidia'}")

        outfile.write(dtk_sft.format_success_msg(success))


def application(output_folder="output", stdout_filename="test.txt", config_filename="config.json",
                report_name=dtk_sft.sft_output_filename, debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done()
    param_obj = vles.load_emod_parameters(config_filename, debug)
    # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    # !!! I can't get the test to pass with out parse_output_file writing
    # !!! to the debug file.
    # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    deaths_df = vles.parse_output_file(stdout_filename, True)
    create_report_file(param_obj, deaths_df, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout, config_filename=args.config,
                report_name=args.reportname, debug=args.debug)
