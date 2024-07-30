import json
import dtk_test.dtk_sft as dtk_sft
import numpy as np
import os
import sqlite3

config_name = "Config_Name"
gamma_k = "Crossover_Gamma_K"
gamma_theta = "Crossover_Gamma_Theta"
barcode_locations = "Barcode_Genome_Locations"
pfemp1_locations = "PfEMP1_Variants_Genome_Locations"
parasite_genetics = "Parasite_Genetics"
msp_location = "MSP_Genome_Location"
fever_prevalence = "Fever Prevalence"
blood_gametocytes = "Blood Smear Gametocyte Prevalence"
data = "Data"


class MalariaSqlReport:
    """
    A class for reading MalariaSqlReport
    """

    def __init__(self):
        self.fn = ""
        self.conn = None
        self.cursor = None

    def Open(self, filename):
        """
        Open the database specified by filename and prepare for queries
        """
        self.fn = filename

        self.conn = sqlite3.connect(self.fn)
        self.conn.row_factory = lambda cursor, row: row[0]  # makes the output lists of values instead of tuples
        self.cursor = self.conn.cursor()

    def Close(self):
        self.conn.close()



def calc_expected_crossovers(chromosome_length=643000, gamma_k=2, gamma_theta=0.38, num_rep=20000):
    """
    Author: Kurt Frey
    Gives you an estimate of average number of chrossovers to expect, depending on the
    length of chromosome, and gamma K and Theta.

    Args:
        chromosome_length: length of the chromosome (default is the first chromosome)
        gamma_k: K parameter used for size of jumps
        gamma_theta: Theta parameter used for size of jumps
        num_rep: how many samples (higher samples = better estimate)

    Returns:

    """

    # Params
    len_tot = chromosome_length
    num_rep = num_rep
    gam_k = gamma_k
    gam_thet = gamma_theta
    pf_bp_per_cM_per_gen = 1500000  # to convert kilo base-pairs per centimorgan

    # Math
    side1 = np.random.uniform(low=0.0, high=len_tot, size=(num_rep, 1))
    side2 = len_tot - side1

    jumps1 = pf_bp_per_cM_per_gen * (np.random.gamma(gam_k, scale=gam_thet, size=(num_rep, 20)))
    jtot1 = np.cumsum(jumps1, axis=1)
    jmade1 = np.argmax(jtot1 > side1, axis=1)

    jumps2 = pf_bp_per_cM_per_gen * np.random.gamma(gam_k, scale=gam_thet, size=(num_rep, 20))
    jtot2 = np.cumsum(jumps2, axis=1)
    jmade2 = np.argmax(jtot2 > side2, axis=1)

    jumps = jmade1 + jmade2 + 1

    return np.mean(jumps)


def load_genetics_parameters(config_filename="config.json", debug=False):
    """
    Reads the config filename and takes out relevant parameters.
    Args:
        config_filename: config filename
        debug: when true, the parameters get written out as a json.

    Returns:

    """
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {config_name: cdj[config_name],
                 gamma_k: cdj[parasite_genetics][gamma_k],
                 gamma_theta: cdj[parasite_genetics][gamma_theta],
                 barcode_locations: cdj[parasite_genetics][barcode_locations],
                 pfemp1_locations: cdj[parasite_genetics][pfemp1_locations],
                 msp_location: cdj[parasite_genetics][msp_location]
                 }

    if debug:
        with open("DEBUG_emod_params.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj


def parse_output_file_for_children(output_filename="test.txt", debug=False):
    """
        Looks for line "Gamma with k = 3.000000 and theta = 1.000000 generated 1.095271" and
        created a dictionary with (k, theta) tuples as key and alll the found generated values in a list as value

    Args:
        output_filename: the logging file
        debug: if True saves off a file with just the lines of interest and the list of lists of children barcodes

    Returns:
       list of lists of Barcode sequence of children.
    """
    filtered_lines = []
    children = []
    with open(output_filename) as logfile:
        for line in logfile:
            if "obligate" in line:
                filtered_lines.append(line)
                obligate = line.split()[-1]
            elif "FinalRecombination" in line:
                filtered_lines.append(line)
                logs = line.split()
                child_0 = logs[-1].split("=")[1]
                child_1 = logs[-2].split("=")[1]
                child_2 = logs[-3].split("=")[1]
                child_3 = logs[-4].split("=")[1]
                children.append([obligate, child_0, child_1, child_2, child_3])

    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)
        with open("DEBUG_children.txt", "w") as outfile:
            for siblings in children:
                outfile.write("\n")
                for child in siblings:
                    outfile.write(f"{child}\n")

    return children


def create_crossover_report_file(children, param_obj, report_name, debug):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(
            f"Running test: {param_obj[config_name]} with {gamma_k}: {param_obj[gamma_k]} , {gamma_theta} : {param_obj[gamma_theta]}  \n")
        if not children:
            success = False
            outfile.write("BAD: No logging data for child genes found.\n")
        else:
            expected_cross = calc_expected_crossovers(gamma_k=param_obj[gamma_k], gamma_theta=param_obj[gamma_theta],
                                                      num_rep=20000)
            switches_list = []
            for num in range(len(children)):
                switches = 0
                for child in range(1, len(children[num])):
                    kid = list(children[num][child])
                    compare_gene = kid[0]
                    for gene in range(len(kid)):
                        new_gene = kid[gene]
                        if new_gene != compare_gene:
                            switches += 1
                            compare_gene = new_gene
                # dividing by 2 because there are two pairs we're looking at switched between
                switches_list.append(switches / 2)

            allowable_error = 0.05
            actual_crossovers = sum(switches_list) / len(switches_list)
            if abs(actual_crossovers - expected_cross) > allowable_error:
                success = False
                outfile.write(f"BAD: We were expecting average number of crossovers to be {expected_cross}, \n"
                              f"but got {actual_crossovers} after looking at {len(children)} results. \n")
            else:
                outfile.write(f"GOOD: We were expecting average number of crossovers to be {expected_cross}, \n"
                              f"and got {actual_crossovers} after looking at {len(children)} results. \n")

            if debug or not success:
                with open("DEBUG_switches.txt", "w") as debug_file:
                    switch_dict = {}
                    for switch in switches_list:
                        if switch in switch_dict:
                            switch_dict[switch] += 1
                        else:
                            switch_dict[switch] = 1
                    debug_file.write(
                        f"{gamma_k}: {param_obj[gamma_k]} , {gamma_theta} : {param_obj[gamma_theta]} expected average number of crossovers: {expected_cross} \n")
                    debug_file.write(f"average = {sum(switches_list) / len(switches_list)} \n")
                    debug_file.write("\n")
                    for switch in switch_dict:
                        debug_file.write(f"{switch} : {switch_dict[switch]}\n")
                    debug_file.write("\n")
                    for switches in switches_list:
                        debug_file.write(f"{switches}\n")

        outfile.write(dtk_sft.format_success_msg(success))


def create_neighborhood_report_file( insetchart_data, compare, insetchart_data_reference,  param_obj, report_name, debug):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj[config_name]} : \n")
        # after time step 400, all loaded inset chart data in recent sim is less than the reference
        data_types = [fever_prevalence, blood_gametocytes]
        min_time_step = 500
        for data in data_types:
            for i in range(min_time_step, len(insetchart_data[data])):
                if compare == "lower":
                    if insetchart_data_reference[data][i] < insetchart_data[data][i]:
                        success = False
                        outfile.write(f"BAD: On time step {i+1} {data} is {insetchart_data[data][i]} in our simulation and "
                                      f"{insetchart_data_reference[data][i]} in the reference. \n We expected our simulation"
                                      f" data to be {compare} than our reference.\n")
                elif compare == "higher":
                    if insetchart_data_reference[data][i] > insetchart_data[data][i]:
                        success = False
                        outfile.write(f"BAD: On time step {i+1} {data} is {insetchart_data[data][i]} in our simulation and "
                                      f"{insetchart_data_reference[data][i]} in the reference. \n We expected our simulation"
                                      f" data to be {compare} than our reference.\n")

        if success:
            outfile.write("GOOD: Reference and simulation data compare as expected.\n")
        outfile.write(dtk_sft.format_success_msg(success))


def load_neighborhood_insetchart(output_folder=None, insetchart_filename="InsetChart.json", debug=False):
    """
    Reads the config filename and takes out relevant parameters.
    Args:
        config_filename: config filename
        debug: when true, the parameters get written out as a json.

    Returns:

    """
    insetchart_path = os.path.join(output_folder, insetchart_filename) if output_folder else insetchart_filename
    with open(insetchart_path) as infile:
        icj = json.load(infile)["Channels"]
    param_obj = {fever_prevalence: icj[fever_prevalence][data],
                 blood_gametocytes: icj[blood_gametocytes][data]
                 }

    if debug:
        with open("DEBUG_emod_params.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj
