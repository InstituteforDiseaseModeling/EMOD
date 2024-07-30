#!/usr/bin/python

import math
import pandas as pd
import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds

"""
Spec:
    The development into larvae is reduced when the habitat is nearing its carrying capacity (CC) by a 
    factor CC/larval count.
    Relevant parameters:
    Egg_Hatch_Density_Dependence: DENSITY_DEPENDENCE
    Egg_Saturation_At_Oviposition: SATURATION_AT_OVIPOSITION
    
    Natural mortality. Eggs experience daily egg mortality, which is independent of climatic factors
    Relevant parameters:
    Egg_Survival_Rate: 0.99
    
    Test assume "Enable_Drought_Egg_Hatch_Delay": 1
"""


# calculation for SATURATION_AT_OVIPOSITION  
def crowding_correction_calc(initial_egg_population, total_larva_count, larvalhabitat,
                             saturation, drought_egg_hatch_delay):
    m_egg_crowding_correction = 1.0

    if saturation == "SATURATION_AT_OVIPOSITION":
        if larvalhabitat > total_larva_count:
            if initial_egg_population > (larvalhabitat - total_larva_count):
                m_egg_crowding_correction = max(drought_egg_hatch_delay,
                                                float(larvalhabitat - total_larva_count)
                                                / float(initial_egg_population))
            else:
                m_egg_crowding_correction = 1.0
        else:
            m_egg_crowding_correction = drought_egg_hatch_delay
        return m_egg_crowding_correction
    elif saturation == "SIGMOIDAL_SATURATION":
        if larvalhabitat > 0:
            m_egg_crowding_correction = math.exp(-total_larva_count/larvalhabitat)
        return m_egg_crowding_correction
    else:
        raise ValueError("Either NO_SATURATION, or an invalid value for saturation test.")


# calculation for density_dependence
def density_dependence_calc(larva_count, larval_cap):
    local_larval_growth_mod = 1.0
    if larva_count > larval_cap:
        local_larval_growth_mod = larval_cap / larva_count
    return local_larval_growth_mod


def parse_output_file(stdout, debug=False):
    """
    This function parses several log lines, examples:

    [VectorHabitat] m_egg_crowding_correction calculated as 0.330000 based on enum value SATURATION_AT_OVIPOSITION,
    larval_habitat = 882.000000, total_larval_count = 5039.000000, m_new_egg_count = 0

    [VectorHabitat]GetLocalLarvalGrowthModifier returning 0.118996 based on current larval count 7412,
    capacity 882.000000.

    [VectorPopulation] Updating egg population due to egg mortality: old_pop = 3400, new_pop = 3365

    Args:
        stdout: the log file from which we parse the information
        debug: flag, if True, we write out lines of interest to a separate file

    Returns: dataframes and arrays of data for the test

    """
    saturation_data = []
    density_dependence_data = []
    egg_pop_data = []
    with open(stdout) as logfile:
        egg_crowding = "m_egg_crowding_correction calculated as "
        larval_growth_mod = "GetLocalLarvalGrowthModifier returning "
        egg_pop = "Updating egg population due to egg mortality: "
        for line in logfile:
            if egg_crowding in line:  # saturation_at_oviposition
                m_egg_crowding_correction = float(sft.get_val(egg_crowding, line))
                initial_egg_population = float(sft.get_val("m_new_egg_count = ", line))
                total_larva_count = float(sft.get_val("total_larval_count = ", line))
                saturation = line.split()[12].strip(',()')  # some of these cannot use get_val because strings
                larval_cap = float(sft.get_val("larval_habitat = ", line))
                data = (initial_egg_population, m_egg_crowding_correction, total_larva_count,
                        larval_cap, saturation)
                saturation_data.append(data)
            if larval_growth_mod in line:  # density_dependence
                larva_count = float(line.split()[12].strip(',()'))  # some can't use it because of complicated splits
                local_larval_growth_mod = float(sft.get_val(larval_growth_mod, line))
                larval_cap = float(line.split()[14].strip(',.()'))
                data = (larva_count, larval_cap, local_larval_growth_mod)
                density_dependence_data.append(data)
            if egg_pop in line:
                old_pop = float(sft.get_val(egg_pop + "old_pop = ", line))
                new_pop = float(sft.get_val("new_pop = ", line))
                data = (old_pop, new_pop)
                egg_pop_data.append(data)
        sat_df = pd.DataFrame(saturation_data)
        sat_df.columns = ['init_egg_pop', 'crowd_corr', 'tot_larva', 'larval_cap', 'sat']

        den_df = pd.DataFrame(density_dependence_data)
        den_df.columns = ['larv_ct', 'larv_cap', 'larv_growth_mod']

        pop_df = pd.DataFrame(egg_pop_data)
        pop_df.columns = ['old_pop', 'new_pop']
        if debug:
            with open("DEBUG_filtered_lines.txt", 'w') as debug_file:
                pd.set_option('display.max_columns', None)
                pd.set_option('display.max_rows', None)
                pd.set_option('display.max_colwidth', -1)
                debug_file.write(f"Saturation data:\n{sat_df}\n")
                debug_file.write(f"Density data:\n{den_df}\n")
                debug_file.write(f"Population data:\n{pop_df}")

        return saturation_data, sat_df, density_dependence_data, den_df, egg_pop_data, pop_df


def create_report_file(param_obj, saturation_data, sat_df, density_dependence_data, den_df, egg_pop_data, pop_df,
                       report_name, debug=False):

    egg_survival_rate = param_obj[veds.ConfigKeys.VectorSpeciesParams.EGG_SURVIVAL_RATE]
    drought_egg_hatch_delay = param_obj[veds.ConfigKeys.DROUGHT_EGG_HATCH_DELAY]
    with open(report_name, "w") as outfile:
        outfile.write(f"# Test name: {param_obj[veds.ConfigKeys.CONFIG_NAME]}, Run number "
                      f"{param_obj[veds.ConfigKeys.RUN_NUMBER]}\n# Tests if the egg mortality rate in the simulation "
                      f"matches the correct rate when crowding is involved.\n")
        if len(saturation_data) == 0 or len(egg_pop_data) == 0 or len(density_dependence_data) == 0:
            outfile.write(sft.sft_no_test_data)
            raise ValueError("There is no data for one or more of the tests!")
        success0 = True
        saturation_data_theory = []
        saturation_data_actual = []
        for i in range(1, len(saturation_data)):
            theory = crowding_correction_calc(sat_df.at[i, 'init_egg_pop'], sat_df.at[i, 'tot_larva'],
                                              sat_df.at[i, 'larval_cap'], sat_df.at[i, 'sat'], drought_egg_hatch_delay)
            actual = sat_df.at[i, 'crowd_corr']
            saturation_data_theory.append(theory)
            saturation_data_actual.append(actual)
            tolerance = 5e-2 * theory
            if abs(theory-actual) > tolerance:
                success0 = False
                outfile.write(f" BAD: Crowding correction not within 5% of expected. initial_egg_population="
                              f"{sat_df.at[i, 'init_egg_pop']}: "
                              f"theory={theory},"
                              f"actual={actual}, larva = {sat_df.at[i, 'tot_larva']}, larval cap = "
                              f"{sat_df.at[i, 'larval_cap']}\n")
        if success0:
            outfile.write("GOOD: Saturation data was within 5% of theory.\n")
        outfile.write("Crowding Correction Calculation SUMMARY: Success={0}\n".format(success0))
        success1 = True
        density_dependence_theory = []
        density_dependence_actual = []
        for i in range(0, len(density_dependence_data)):
            theory = density_dependence_calc(den_df.at[i, 'larv_ct'], den_df.at[i, 'larv_cap'])
            density_dependence_theory.append(theory)
            actual = den_df.at[i, 'larv_growth_mod']
            density_dependence_actual.append(actual)
            tolerance = 5e-2 * theory
            if abs(theory-actual) > tolerance:
                success1 = False
                outfile.write(f"BAD: Density dependence not within 5% of theory.  Expected {theory}, got {actual}\n")
        if success1:
            outfile.write("GOOD: Density dependence was within 5% of theory.\n")
        outfile.write("Density Dependence Calculation SUMMARY: Success={0}\n".format(success1))
        success2 = True

        pop_theory = []
        pop_actual = []
        for i in range(0, len(egg_pop_data)):
            theory_new_pop = int(pop_df.at[i, 'old_pop'] * egg_survival_rate)
            actual_new_pop = pop_df.at[i, 'new_pop']
            pop_theory.append(theory_new_pop)
            pop_actual.append(actual_new_pop)
            tolerance = 5e-2 * theory_new_pop
            if abs(theory_new_pop - actual_new_pop) > tolerance:
                success2 = False
                outfile.write(f"BAD: Egg natural mortality not within 5% of theory.  Expected {theory_new_pop}, "
                              f"got {actual_new_pop}.\n")
        if success2:
            outfile.write("GOOD: Egg natural mortality was within 5% of theory.\n")
        outfile.write("Egg Natural Mortality Calculation SUMMARY: Success={0}\n".format(success2))
        overall_success = True
        if not (success0 and success1 and success2):
            overall_success = False
        sft.plot_data(saturation_data_theory, saturation_data_actual, label1='theoretical saturation',
                          label2='actual density saturation', title='Theoretical vs. Actual Saturation',
                          ylabel='saturation', category='saturation')
        sft.plot_data(pop_theory, pop_actual, label1='theoretical new population',
                          label2='actual new population', title='Theoretical vs. Actual New Population',
                          ylabel='population', category='population')
        sft.plot_data(density_dependence_theory, density_dependence_actual, label1='theoretical density dependence',
                          label2='actual density dependence', title='Theoretical vs. Actual Density Dependence',
                          ylabel='density dependence', category='density_dependence')
        outfile.write(sft.format_success_msg(overall_success))


def application(output_folder, config_filename="config.json", stdout=sft.sft_test_filename,
                report_name=sft.sft_output_filename, debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("stdout_filename: " + stdout + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    param_obj = veds.load_emod_parameters(config_filename)
    saturation_data, sat_df, density_dependence_data, den_df, egg_pop_data, pop_df = parse_output_file(stdout, debug)
    create_report_file(param_obj, saturation_data, sat_df, density_dependence_data, den_df, egg_pop_data, pop_df,
                       report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output,
                config_filename=args.config,
                stdout=args.stdout,
                report_name=args.reportname, debug=args.debug)
