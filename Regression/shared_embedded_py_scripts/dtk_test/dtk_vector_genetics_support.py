#!/usr/bin/python

import json
import dtk_test.dtk_sft as dtk_sft
import pandas as pd
import dtk_test.dtk_serialization_support as d_ss
import os, glob

"""
Verifying VectorGeneticsReport file. This test is written for one species of mosquito (TestVector).
It compares serialized vector files with the Report data in three places (or as many as you want, controlled by the
"Serialization_Time_Steps" parameter.
. 
Sweep suggestions:
Run_Number, Different Genetics, Base_Air_Temperature, Male_Life_Expectancy, Adult_Life_Expectancy,
"""

KEY_CONFIG_NAME = "Config_Name"
VECTOR_SPECIES_PARAMS = "Vector_Species_Params"
ADULT_LIFE_EXPECTANCY = "Adult_Life_Expectancy"
MALE_LIFE_EXPECTANCY = "Male_Life_Expectancy"
AIR_TEMPERATURE = "Base_Air_Temperature"
GENES = "Genes"
SPECIES_NAME = "Name"
STATE_ADULT = "STATE_ADULT"
STATE_IMMATURE = "STATE_IMMATURE"
STATE_INFECTED = "STATE_INFECTED"
STATE_INFECTIOUS = "STATE_INFECTIOUS"
SERIALIZATION_TIMESTEPS = "Serialization_Time_Steps"
queue_names = {"EggQueues": "STATE_EGG", "AdultQueues": "STATE_ADULT", "MaleQueues": "STATE_MALE",
               "InfectedQueues": "STATE_INFECTED", "InfectiousQueues": "STATE_INFECTIOUS",
               "ImmatureQueues": "STATE_IMMATURE"}
state_dict = {0: "STATE_INFECTIOUS", 1: "STATE_INFECTED", 2: "STATE_ADULT", 3: "STATE_MALE",
              4: "STATE_IMMATURE", 5: "STATE_LARVA", 6: "STATE_EGG"}
MALES = "males"
FEMALES = "females"


def load_emod_parameters(config_filename="config.json", debug=False):
    """reads config file and populates params_obj
    :param config_filename: name of config file (config.json)
    :param debug: write out parsed data on true
    :returns param_obj:     dictionary with KEY_CONFIG_NAME, etc., keys (e.g.)
    """
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {}
    species = cdj[VECTOR_SPECIES_PARAMS][0][SPECIES_NAME]
    param_obj[ADULT_LIFE_EXPECTANCY] = cdj[VECTOR_SPECIES_PARAMS][0][ADULT_LIFE_EXPECTANCY]
    param_obj[MALE_LIFE_EXPECTANCY] = cdj[VECTOR_SPECIES_PARAMS][0][MALE_LIFE_EXPECTANCY]
    param_obj[KEY_CONFIG_NAME] = cdj[KEY_CONFIG_NAME]
    param_obj[AIR_TEMPERATURE] = cdj[AIR_TEMPERATURE]
    try:
        param_obj[GENES] = cdj[VECTOR_SPECIES_PARAMS][0][GENES]
    except ValueError:
        ValueError(f"No genetic information found under {species}\n")
    param_obj[SERIALIZATION_TIMESTEPS] = cdj[SERIALIZATION_TIMESTEPS]

    if debug:
        with open("DEBUG_param_object.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj


def hrp2_support(expected_pfhrp2_prevalence, expected_hrp_deleted_fraction_of_infections,
                 expected_hrp_deleted_fraction_of_infected_people, expected_infected,
                 expected_num_total_infections):
    success = True
    with open("config.json", "r") as config_file:
        cf = json.load(config_file)
    test_name = cf["parameters"]["Config_Name"]
    with open("output/InsetChart.json", "r") as output:
        output_data = json.load(output)["Channels"]
    with open("output/ReportEventCounter.json", "r") as report_event_counter:
        events = json.load(report_event_counter)["Channels"]
    with open(dtk_sft.sft_output_filename, "w") as outfile:
        outfile.write(f"Running test: {test_name}  \n")
        day = 19
        day_index = 19 - 1  # 0-based index
        pfhrp2_prevalence_label = "PfHRP2 Prevalence"
        hrp_deleted_fraction_of_infections_label = "HRP Deleted Fraction of All Infections"
        hrp_deleted_fraction_of_infected_people_label = "HRP Deleted Fraction of Infected People"
        infected_label = "Infected"
        num_total_infections_label = "Num Total Infections"
        data_label = "Data"
        total_population = 1000
        expected = {
            pfhrp2_prevalence_label: expected_pfhrp2_prevalence,
            hrp_deleted_fraction_of_infections_label: expected_hrp_deleted_fraction_of_infections,
            hrp_deleted_fraction_of_infected_people_label: expected_hrp_deleted_fraction_of_infected_people,
            infected_label: expected_infected,
            num_total_infections_label: expected_num_total_infections
        }
        actual = {
            pfhrp2_prevalence_label: round(output_data[pfhrp2_prevalence_label][data_label][day_index], 2),
            hrp_deleted_fraction_of_infections_label: round(output_data[hrp_deleted_fraction_of_infections_label][data_label][
                                                          day_index], 2),
            hrp_deleted_fraction_of_infected_people_label: round(output_data[hrp_deleted_fraction_of_infected_people_label][data_label][
                                                               day_index], 2),
            infected_label: round(output_data[infected_label][data_label][day_index], 2),
            num_total_infections_label: output_data[num_total_infections_label][data_label][day_index]
        }
        # as some of the values are stochastic proportions, we don't expect everything to be exact
        # so I'm giving things 5% wiggle room or to be within 0.02 whichever is bigger for the smaller numbers
        for key in expected.keys():
            if abs(expected[key] - actual[key]) > max((0.05 * expected[key]), 0.03):
                success = False
                outfile.write(
                    f"BAD: At step {day}, for {key} we expected value of {expected[key]}, but got "
                    f"{actual[key]} \n")
            else:
                outfile.write(
                    f"GOOD: At step {day}, for {key}, we expected value of {expected[key]} and got "
                    f"{actual[key]} \n")
        # checking MalariaDiagnostic diagnostics
        parasites_found = events["BLOOD_SMEAR_PARASITES_FOUND"][data_label][day_index]
        hrp2_found = events["HRP2_FOUND"][data_label][day_index]
        if abs(parasites_found - total_population * expected_infected) >= 0.5 * (total_population * expected_infected):
            success = False
            outfile.write(f"BAD: At step {day} MalariaDiagnostic checking for BLOOD_SMEAR_PARASITES should have found "
                          f"{total_population * expected_infected} infections, but we found {parasites_found} .\n")
        if abs(hrp2_found - round(actual[pfhrp2_prevalence_label] * total_population)) > 1:
            success = False
            outfile.write(
                f"BAD: At step {day} MalariaDiagnostic checking for PF_HRP2 should have found "
                f"{round(actual[pfhrp2_prevalence_label] * total_population)}  infections, but we found {hrp2_found} .\n")
        outfile.write(dtk_sft.format_success_msg(success))


def create_gamete_dict(output_filename="test.txt", debug=False):
    """
    Creates a gamete translation from strings, used in VectorGeneticsReport, to bits, used in Serialization files.
    :param output_filename: logging output file usually either test.txt or StdOut.txt
    :param debug: writes out the final dictionary on True
    :return: returns dictionary of string to numbers.

    """
    vector_gamete_dict = {}
    with open(output_filename) as logfile:
        for line in logfile:
            if "[VectorGene] Vector gametes" in line:
                gamete_def = line.split()[-1].split("=")
                if gamete_def[0] not in vector_gamete_dict:
                    vector_gamete_dict[gamete_def[0]] = gamete_def[1]

    if debug:
        with open("DEBUG_gamete_dictionary.json", "w") as gd:
            json.dump(vector_gamete_dict, gd)

    return vector_gamete_dict


def process_serialized_queue(m_vectorpopulations: list = None, queue_list: dict = None,
                             outfile=None):
    """
        Processes a vector queue from serialized file and adds it to the given list
    Args:
        m_vectorpopulations: list to which we're appending data
        queue_list: list of queues of vector population stage
        outfile: log file to put errors into

    Returns:
        list passed with with data from the queues added
    """
    temp_dict = {}
    state = None
    if type(queue_list) is dict:
        queue_list = queue_list["collection"]
    for queue in queue_list:
        species = queue["species_index"]
        genes_mom = queue["genome_self"]["m_GameteMom"]
        genes_dad = queue["genome_self"]["m_GameteDad"]
        population = queue["population"]
        state_code = int(queue["state"])
        state = state_dict[state_code] if state_code in state_dict else outfile.write(f"{state_code} not found in look "
                                                                                      f"up dictionary of known states.\n")
        if (species, state, genes_mom, genes_dad) in temp_dict:
            temp_dict[(species, state, genes_mom, genes_dad)] = population + temp_dict[
                (species, state, genes_mom, genes_dad)]
        else:
            temp_dict[(species, state, genes_mom, genes_dad)] = population

    for key in temp_dict.keys():
        to_list = list(key)
        to_list.append(temp_dict[key])
        m_vectorpopulations.append([to_list[0], to_list[1], to_list[2], to_list[3], to_list[4]])
    return m_vectorpopulations


def create_report_file(param_obj, gamete_dict, report_name, debug):
    with open(report_name, "w") as outfile:
        rvg_filename = "ReportVectorGenetics_TestVector_GENOME.csv"
        rvg_df = pd.read_csv("output/" + rvg_filename)
        rvg_df[['gamete_mom', 'gamete_dad']] = rvg_df.Genome.str.split(":", expand=True)
        config_name = param_obj[KEY_CONFIG_NAME]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        if not gamete_dict:
            success = False
            outfile.write("Gamete Dictionary is empty. Please verify that VectorGene logging is working.\n")
        else:
            for serialization_timestep in param_obj[SERIALIZATION_TIMESTEPS]:
                this_success = True
                serialization_timestep_name = str(serialization_timestep).zfill(5)
                outfile.write(f"Test for {serialization_timestep} timestep:\n")
                dtk_file = d_ss.DtkFile(f"output/state-{serialization_timestep_name}.dtk")
                dtk_json = f"node_{serialization_timestep_name}"
                dtk_file.write_node_to_disk(node_filename=dtk_json, node_index=0)
                dtk_json = dtk_json + "-new.json"
                with open(dtk_json, "r") as rvg_to_be:
                    rvg_json = json.load(rvg_to_be)["m_vectorpopulations"]
                m_vectorpopulations = []
                for queue in queue_names:
                    if rvg_json[0][queue]:
                        m_vectorpopulations = process_serialized_queue(m_vectorpopulations, rvg_json[0][queue],
                                                                       outfile)
                vector_pop_df = pd.DataFrame(m_vectorpopulations)
                vector_pop_df.columns = ["species", "state", "gamete_mom", "gamete_dad", "population"]
                rvg_df_time = rvg_df[
                    rvg_df.Time == serialization_timestep - 1]  # reporter vs serialization are off by 1
                if debug:
                    vector_pop_df.to_csv(f"DEBUG_vector_pop_df_{serialization_timestep_name.csv}")
                    rvg_df_time.to_csv(f"DEBUG_rvg_df_time_{serialization_timestep_name.csv}")
                rvg_dict = rvg_df_time.to_dict(orient='records')
                for record in rvg_dict:
                    genome = record["Genome"]
                    gametes = genome.split(":")
                    mom_gamete = int(gamete_dict[gametes[0]]) if gametes[
                                                                     0] in gamete_dict else f"BAD: {gametes[0]} not in the " \
                                                                                            f"gamete_dictionary, you can see the dictionary by running in debug.\n"
                    dad_gamete = int(gamete_dict[gametes[1]]) if gametes[
                                                                     1] in gamete_dict else f"BAD: {gametes[1]} not in the " \
                                                                                            f"gamete_dictionary, you can see the dictionary by running in debug.\n"
                    serialized_subset = vector_pop_df[
                        (vector_pop_df.gamete_mom == mom_gamete) & (vector_pop_df.gamete_dad == dad_gamete)]
                    if debug:
                        serialized_subset.to_csv("DEBUG_subset.csv")
                    for state in queue_names.values():
                        if state in serialized_subset["state"].tolist():
                            serialized_pop = serialized_subset[serialized_subset.state == state]["population"].iloc[0]
                        else:
                            serialized_pop = 0
                        report_pop = record[state]
                        if serialized_pop != report_pop:
                            this_success = False
                            success = False
                            outfile.write(
                                f"BAD: For timestep {serialization_timestep}, for genome {genome}, for state {state} "
                                f"the serialized population {serialized_pop} does not match report population "
                                f"{report_pop}\n")
                        elif serialized_pop != 0:
                            if debug:
                                outfile.write(
                                    f"GOOD: For timestep {serialization_timestep}, for genome {genome}, for state {state} "
                                    f"the serialized population {serialized_pop} matches report population "
                                    f"{report_pop}\n")
                if this_success:
                    outfile.write(
                        f"GOOD: All data for timestep {serialization_timestep} matches between serialized file and "
                        f"the VectorGeneticReport.\n")

            if success and not debug:  # cleaning up the decoded serialized files and serialized files, they are BIG
                for filename in glob.glob("node_*.json"):
                    os.remove(filename)
                for filename in glob.glob("output/*.dtk"):
                    os.remove(filename)

        outfile.write(dtk_sft.format_success_msg(success))


def basics_report_file(param_obj, output_folder, report_name, debug):
    with open(report_name, "w") as outfile:
        success = True
        rvg_filename = "ReportVectorGenetics_TestVector_GENOME.csv"
        rvg_df = pd.read_csv(output_folder + "/" + rvg_filename)
        config_name = param_obj["Config_Name"]
        outfile.write(f"Running test for {config_name}\n")
        rvg_debug_name = "DEBUG_ReportVectorGenetics.csv"
        # verifying that all female states add up to VectorPopulation
        test_1 = "test_1"
        rvg_df["Vector_Sum"] = rvg_df[[STATE_ADULT, STATE_INFECTIOUS, STATE_INFECTED]].sum(axis=1)
        rvg_df[test_1] = rvg_df.VectorPopulation == rvg_df.Vector_Sum
        if not rvg_df.test_1.all():
            success = False
            outfile.write(f"BAD: For some values of VectorPopulation does not equal to vector adults + infected + "
                          f"infectious , "
                          f"but should.\nPlease see {rvg_debug_name} for details.\n")
            rvg_df[~rvg_df[test_1]].to_csv(rvg_debug_name)
        else:
            outfile.write("GOOD: Value of VectorPopulation equals to vector adults + infected + infectious.\n")

        # checking that infectious is after infected for each genome
        genomes = rvg_df.Genome.unique()
        infectious_vectors_appear = 0
        for genome in genomes:
            if "Y" not in genome:
                single_genome_df = rvg_df[rvg_df.Genome == genome]
                firsts = single_genome_df.ne(0).idxmax()
                adult_time = single_genome_df.loc[firsts[STATE_ADULT], "Time"]
                infected_time = single_genome_df.loc[firsts[STATE_INFECTED], "Time"]
                infectious_time = single_genome_df.loc[firsts[STATE_INFECTIOUS], "Time"]
                if infectious_time > infectious_vectors_appear:
                    infectious_vectors_appear = infectious_time
                if not (adult_time < infected_time < infectious_time):
                    success = False
                    outfile.write(f"BAD: For genome {genome}, {STATE_ADULT} is on {adult_time}, {STATE_INFECTED} is on "
                                  f"{infected_time}, {STATE_INFECTIOUS} is on {infectious_time}. Should be "
                                  f"{adult_time} < {infected_time} < {infectious_time}."
                                  "\n")
                else:
                    if debug:
                        outfile.write(
                            f"GOOD: For genome {genome}, {STATE_ADULT} is on {adult_time}, {STATE_INFECTED} is on "
                            f"{infected_time}, {STATE_INFECTIOUS} is on {infectious_time}.\n")

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
    param_obj = load_emod_parameters(config_filename, debug)
    gamete_dict = create_gamete_dict(stdout_filename, debug)
    create_report_file(param_obj, gamete_dict, report_name, debug)
