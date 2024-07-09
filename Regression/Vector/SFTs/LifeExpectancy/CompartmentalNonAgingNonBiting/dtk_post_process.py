#!/usr/bin/python


import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_vector_life_expectancy_support as vles

"""
Adult_Life_Expectancy:
Verifying that vectors die at the correct rate when it comes their death rate based on life expectancy and 
air temperature. Aging is turned off. 
These are "natural" mortality deaths and deaths associated with feeding. 
"Days_Between_Feeds": 10, 
"Egg_Batch_Size": 0, 
"Infectious_Human_Feed_Mortality_Factor": 0
"Human_Feeding_Mortality": 0.013, 

This assumes there one vector species. 
Sweep suggestions:
Run_Number, Base_Air_Temperature, Male_Life_Expectancy, Adult_Life_Expectancy, Days_Between_Feeds
"""

CONFIG_NAME = "Config_Name"
ADULT_LIFE_EXPECTANCY = "Adult_Life_Expectancy"
MALE_LIFE_EXPECTANCY = "Male_Life_Expectancy"
AIR_TEMPERATURE = "Base_Air_Temperature"


def create_report_file(param_obj, deaths_df, report_name, debug=False):
    with open(report_name, "w") as outfile:
        config_name = param_obj[CONFIG_NAME]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        life_expectancy_male = param_obj[MALE_LIFE_EXPECTANCY]
        life_expectancy_female = param_obj[ADULT_LIFE_EXPECTANCY]
        temperature = param_obj[AIR_TEMPERATURE]
        if deaths_df.empty:
            success = False
            outfile.write("No data found in vector_deaths_df, please re-run with debug=True to see the csv file.\n")
        else:
            sexes = ["female", "male"]
            population_by_sex = {}
            for sex in sexes:
                life_expectancy = life_expectancy_female if sex == "female" else life_expectancy_male
                sub_df = deaths_df[deaths_df.sex == sex]
                sub_df = sub_df.groupby(["timestep"], as_index=False).sum()
                sub_df["age"] = sub_df["timestep"] + 1
                # vectors that die on the same day as they are created
                # are considered to have lived that day
                population_by_sex[sex] = sub_df["population_before_deaths"].tolist()
                initial_population = population_by_sex[sex][0]
                dtk_sft.plot_data(population_by_sex[sex], dist2=None, label1=f"{sex} Vectors of that Age",
                                  label2=None, title=f"{sex} Vector Population Age Distribution",
                                  xlabel="Age (in days)", ylabel="Number of vectors",
                                  category=f'{sex}_vector_age_distribution')
                if not vles.check_all_vectors_died(sub_df, sex, outfile, debug):
                    success = False
                test_successes, test_failures, actual_average_lifespan = vles.daily_mortality_check_average_lifespan_calculation(sub_df,
                                                                                                           life_expectancy,
                                                                                                           temperature,
                                                                                                           outfile,
                                                                                                           aging=False)
                success_rate = 0.92
                mortality_check = [[test_successes, test_failures]]
                if not vles.check_results(mortality_check, sex, success_rate, outfile):
                    success = False

                theoretical_average_lifespan = vles.brute_force_theoretical_lifespan(initial_population,
                                                                                     life_expectancy,
                                                                                     temperature, aging=False)
                total_deaths = sub_df["dead_without_feed"].sum()
                error = 0.5  # want them to be within half a day
                if abs(actual_average_lifespan - theoretical_average_lifespan) > error:
                    success = False
                    outfile.write(f"BAD: Expected average life span for {sex} vectors to be about "
                                  f"{theoretical_average_lifespan} due to {life_expectancy} life expectancy, and "
                                  f"dry heat mortality, but got {actual_average_lifespan} after "
                                  f"looking at {total_deaths} deaths. This is not within {error} of a day. \n"
                                  "\n")
                else:
                    outfile.write(f"GOOD: Expected average life span for {sex} vectors to be about "
                                  f"{theoretical_average_lifespan} due to {life_expectancy} day life expectancy, and "
                                  f"dry heat mortality, and got {actual_average_lifespan} after "
                                  f"looking at {total_deaths} deaths. This is within {error} of a day. \n"
                                  "\n")
                if debug or not success:
                    sub_df.to_csv(f"{sex}_sub_df.csv")

            dtk_sft.plot_data(population_by_sex["female"], dist2=population_by_sex["male"],
                              label1="Female Vectors(life expectancy = {})".format(life_expectancy_female),
                              label2="Male Vectors(life expectancy = {})".format(life_expectancy_male),
                              title="Both Sexes Vector Population Age Distribution",
                              xlabel='Age (in days)', ylabel="Number of vectors",
                              category="both_sexes_vector_age_distribution")

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
    deaths_df = vles.parse_output_file(stdout_filename, debug)
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
