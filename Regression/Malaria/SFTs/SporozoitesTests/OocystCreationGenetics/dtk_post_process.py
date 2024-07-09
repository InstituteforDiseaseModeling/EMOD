#!/usr/bin/python

import dtk_test.dtk_sft as sft
import pandas as pd
import json
import re
import matplotlib.pyplot as plt
import numpy as np

"""
Spec: 
We may want to use a threshold in the future to account for the non-linear relationship between sporozoite 
load and infection probability as in (Aleshnick, 2020), the practical implication being that a small subset 
of highly infected mosquitoes may contribute disproportionately to malaria transmission in the field. This is 
related to the action employed by the current parameter Base_Sporozoite_Survival_Fraction, without the ability 
to do thresholding as suggested by literature. 
Set up: We give out four outbreaks to everyone, so people's parasite genetics are in proportions 1:3 with
TAAAAAAAAAAAAAAAAAAAAAAA:AAAAAAAAAAAAAAAAAAAAAAAT respectively
then, at  day 31 when all people are infectious, we let the vectors bite them (before that they are protected by perfect
bednets) with the proportions above and outcrossing, using punnett square, I have determinned that the resulting sprozoites
after recombination should have parts of total sporozoite population as follows:
TAAAAAAAAAAAAAAAAAAAAAAA = ???? 0.15625
ATAAAAAAAAAAAAAAAAAAAAAA = ???? 0.65625
AAAAAAAAAAAAAAAAAAAAAAAA = ???? 0.109375
TTAAAAAAAAAAAAAAAAAAAAAA = ???? 0.09375
"""

config_name = "Config_Name"
sporozoite = "sporozoite"
mom = "mom"
dad = "dad"
aa = "AAAAAAAAAAAAAAAAAAAAAAAA"
at = "ATAAAAAAAAAAAAAAAAAAAAAA"
ta = "TAAAAAAAAAAAAAAAAAAAAAAA"
tt = "TTAAAAAAAAAAAAAAAAAAAAAA"
number = "number"



def load_emod_parameters(config_filename="config.json", debug=False):
    """
    Reads the config filename and takes out relevant parameters.
    Args:
        config_filename: config filename
        debug: when true, the parameters get written out as a json.
    Returns:
    """

    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {config_name: cdj[config_name]}

    if debug:
        with open("DEBUG_emod_params.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj


def get_genes(key, line):
    regex = key + "[\w*\d*\-\w*\d*]"
    match = re.search(regex, line)
    if match != None:
        return match.group(1)
    else:
        raise LookupError


def parse_output_file(stdout, debug=False):
    """
    Parses following lines, example:

    Args:
        stdout: the logging file to parse
        debug: flag, if true writes out the lines that were used to collect the data
    Returns: data frame
    """
    time = 0
    sporozoite_genetics_dict = {"time": [], number: [], sporozoite: [], mom: [], dad: []}
    bite_genomes = {0: 0, 1: 0, 2: 0, 3: 0}
    with open(stdout) as logfile:
        for line in logfile:
            if "Time: " in line:
                time = int(float(sft.get_val("Time: ", line)))
            elif "new sporozoite genome " in line:
                # if time == 31:  # we only care about the first time biting
                gene_data = line.split()
                sporozoite_genetics_dict['time'].append(time)
                sporozoite_genetics_dict[number].append(int(gene_data[-15]))
                sporozoite_genetics_dict[sporozoite].append(gene_data[-11])
                sporozoite_genetics_dict[mom].append(gene_data[-6])
                sporozoite_genetics_dict[dad].append(gene_data[-2])
            elif "sporozoites in bite" in line:
                if time == 50:
                    bite_data = line.split()

                    population = int(bite_data[-1])
                    bite_genome = bite_data[-3]
                    if bite_genome == at:
                        bite_genomes[2] += population
                    elif bite_genome == ta:
                        bite_genomes[1] += population
                    elif bite_genome == aa:
                        bite_genomes[0] += population
                    elif bite_genome == tt:
                        bite_genomes[3] += population

    sporozoite_genetics_df = pd.DataFrame.from_dict(sporozoite_genetics_dict)

    if debug:
        sporozoite_genetics_df.to_csv("sporozoite_genetics_df_DEBUG.csv")
        bite_json = json.dumps(bite_genomes)
        f = open("bite_genomes_dict_DEBUG.json", "w")
        f.write(bite_json)
        f.close()

    return sporozoite_genetics_df, bite_genomes


def proportion_check(num, total, expected_proportion, thing, outfile, allowable_error=0.03):
    error = abs(num / total - expected_proportion)
    if error > allowable_error:
        outfile.write(f"BAD: The actual {thing} gene proportion of {num / total} is not close enough to the expected\n"
                      f"proportion of {expected_proportion} with a difference of {num / total - expected_proportion} \n"
                      f"which is bigger than our allowable error of {allowable_error}. Please check. \n")
        return False
    else:
        outfile.write(
            f"GOOD: The actual {thing} gene proportion of {num / total} is totally close enough to the expected\n"
            f"proportion of {expected_proportion} with an difference of {num / total - expected_proportion}.\n")
        return True


def create_report_file(param_obj, report_name, sporozoite_genetics_df, bite_genomes, debug=False):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Test name: {str(param_obj[config_name])} \n")
        ta_proportion = 0.15625
        at_proportion = 0.65625
        aa_proportion = 0.09375
        tt_proportion = 0.09375
        if not len(sporozoite_genetics_df):
            success = False
            outfile.write(sft.sft_no_test_data)
            outfile.write("No data in sporozoite_genetics_df!\n")
        else:
            aa_counts = int(sporozoite_genetics_df[sporozoite_genetics_df[sporozoite] == aa][number].sum())
            ta_counts = int(sporozoite_genetics_df[sporozoite_genetics_df[sporozoite] == ta][number].sum())
            at_counts = int(sporozoite_genetics_df[sporozoite_genetics_df[sporozoite] == at][number].sum())
            tt_counts = int(sporozoite_genetics_df[sporozoite_genetics_df[sporozoite] == tt][number].sum())
            total = aa_counts + ta_counts + at_counts + tt_counts
            if not proportion_check(ta_counts, total, ta_proportion, "TA", outfile):
                success = False
            if not proportion_check(at_counts, total, at_proportion, "AT", outfile):
                success = False
            if not proportion_check(tt_counts, total, aa_proportion, "TT", outfile):
                success = False
            if not proportion_check(aa_counts, total, tt_proportion, "AA", outfile):
                success = False
            ta_mom_dad = int(sporozoite_genetics_df[sporozoite_genetics_df[mom] == ta][number].sum()) + \
                         int(sporozoite_genetics_df[sporozoite_genetics_df[dad] == ta][number].sum())
            at_mom_dad = int(sporozoite_genetics_df[sporozoite_genetics_df[mom] == at][number].sum()) + \
                         int(sporozoite_genetics_df[sporozoite_genetics_df[dad] == at][number].sum())
            total_mom_dad = ta_mom_dad + at_mom_dad
            ta_mom_dad_proportion = 0.25
            at_mom_dad_proportion = 0.75
            if not proportion_check(ta_mom_dad, total_mom_dad, ta_mom_dad_proportion, "TA in parents",
                                    outfile):
                success = False
            if not proportion_check(at_mom_dad, total_mom_dad, at_mom_dad_proportion, "AT in parents",
                                    outfile):
                success = False
            outfile.write(f"Total number of sporozoites checked: {total}.\n")

            if not success:
                sporozoites_df = pd.DataFrame.from_dict({
                    "TA_exp": [ta_proportion],
                    "AT_exp": [at_proportion],
                    "TT_exp": [tt_proportion],
                    "AA_exp": [aa_proportion],
                    "TA": [ta_counts / total],
                    "AT": [at_counts / total],
                    "TT": [tt_counts / total],
                    "AA": [aa_counts / total],
                    "TA_parent_exp": [ta_mom_dad_proportion],
                    "AT_parent_exp": [at_mom_dad_proportion],
                    "TA_parent": [ta_mom_dad / total_mom_dad],
                    "AT_parent": [at_mom_dad / total_mom_dad]})
                sporozoites_df.to_csv("sporozoites_df.csv")

        # checking what the sporozoites look like in bite:
        outfile.write(f"\nNow checking the genetic marker distribution for sporozoites distributed in bites. \n"
                      f"It should be the same as genetic marker distribution in sporozoites.\n")
        total_bite_sporozoites_distributed = 0
        for gene in bite_genomes:
            total_bite_sporozoites_distributed += bite_genomes[gene]
        if not proportion_check(bite_genomes[1], total_bite_sporozoites_distributed, ta_proportion,
                                "AT in bites",
                                outfile):  # making acceptable error bigger because of the drift
            success = False
        if not proportion_check(bite_genomes[2], total_bite_sporozoites_distributed, at_proportion,
                                "AT in bites",
                                outfile):  # making acceptable error bigger because of the drift
            success = False
        if not proportion_check(bite_genomes[3], total_bite_sporozoites_distributed, tt_proportion,
                                "TT in bites ",
                                outfile):
            success = False
        if not proportion_check(bite_genomes[0], total_bite_sporozoites_distributed, aa_proportion,
                                "AA in bites", outfile):
            success = False
        outfile.write(
            f"Total number of sporozoites distributed through bites: {total_bite_sporozoites_distributed}. \n")

        labels = ["TA", "AT", "TT", "AA"]
        sporozoite_proportions = [ta_counts / total, at_counts / total, tt_counts / total, aa_counts / total]
        expected = [ta_proportion, at_proportion, tt_proportion, aa_proportion]
        bite_sporozoite_proportions = [bite_genomes[1] / total_bite_sporozoites_distributed,
                                       bite_genomes[2] / total_bite_sporozoites_distributed,
                                       bite_genomes[3] / total_bite_sporozoites_distributed,
                                       bite_genomes[0] / total_bite_sporozoites_distributed]

        genetic_marker_proportions = {"expected": dict(zip(labels, expected)),
                                      "sporozoite_proportions": dict(zip(labels, sporozoite_proportions)),
                                      "bite_sporozoite_proportions": dict(zip(labels, bite_sporozoite_proportions))}
        with open('genetic_marker_proportions.json', 'w') as fp:
            json.dump(genetic_marker_proportions, fp)

        # Set position of bar on X axis
        bar_width = 0.25
        r1 = np.arange(len(expected))
        r2 = [x + bar_width for x in r1]
        r3 = [x + bar_width for x in r2]
        # Make the plot

        plt.bar(r1, expected, color='#E80C82', width=bar_width, edgecolor='white', label="Expected Marker Proportions")
        plt.bar(r2, sporozoite_proportions, color='#3960E0', width=bar_width, edgecolor='white',
                label='Sporozoites after Recombination Marker Proportions')
        plt.bar(r3, bite_sporozoite_proportions, color='#0A5235', width=bar_width, edgecolor='white',
                label='Sporozoites in Bite Proportions')
        # Add xticks on the middle of the group bars

        plt.xlabel('group', fontweight='bold')

        plt.xticks([r + bar_width for r in range(len(expected))], labels)

        # Create legend & Show graphic

        plt.legend()
        plt.savefig("results.png")
        # plt.show()

        if not success:
            bite_sporozoite_df = pd.DataFrame.from_dict({
                "ta_exp": [ta_proportion],
                "at_exp": [at_proportion],
                "tt_exp": [tt_proportion],
                "aa_exp": [aa_proportion],
                "TA": [bite_genomes[1] / total_bite_sporozoites_distributed],
                "AT": [bite_genomes[2] / total_bite_sporozoites_distributed],
                "TT": [bite_genomes[3] / total_bite_sporozoites_distributed],
                "AA": [bite_genomes[0] / total_bite_sporozoites_distributed]})
            bite_sporozoite_df.to_csv("bite_sporozoite_df.csv")

        outfile.write(sft.format_success_msg(success))


def application(output_folder="output",
                config_filename="config.json",
                stdout="test.txt",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("stdout_filename: " + stdout + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename)
    sporozoite_genetics_df, bite_genomes = parse_output_file(stdout, debug)
    create_report_file(param_obj, report_name, sporozoite_genetics_df, bite_genomes, debug)


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
                report_name=args.reportname, debug=args.debug)
