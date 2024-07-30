
import dtk_test.dtk_malaria_support as mal_sup
import dtk_test.dtk_sft as dtk_sft
from collections import Counter
import numpy as np

"""
This test verifies that for the final four chromosomes 

                                     
    CHROMOSOME_LENGTH_1  =  643000;     
    CHROMOSOME_LENGTH_2  =  947000;    
    CHROMOSOME_LENGTH_3  = 1100000;     
    CHROMOSOME_LENGTH_4  = 1200000;     
    CHROMOSOME_LENGTH_5  = 1300000;     
    CHROMOSOME_LENGTH_6  = 1400000;     
    CHROMOSOME_LENGTH_7  = 1400000;     
    CHROMOSOME_LENGTH_8  = 1300000;     
    CHROMOSOME_LENGTH_9  = 1500000;
    CHROMOSOME_LENGTH_10 = 1700000;
    CHROMOSOME_LENGTH_11 = 2000000;
    CHROMOSOME_LENGTH_12 = 2300000;
    CHROMOSOME_LENGTH_13 = 2700000;
    CHROMOSOME_LENGTH_14 = 3300000;
"""

config_name = "Config_Name"
barcode_locations = "Barcode_Genome_Locations"
chromosome_ends = [643000, 1590000, 2690000, 3890000, 5190000, 6590000, 7990000, 9290000,
                   10790000, 12490000, 14490000, 16790000, 19490000, 22790000]


def parse_output_file(output_filename="test.txt", barcode_indices=None, debug=False):
    """
    line example:
    name(BeforeIA)
    chromo(0)
    Child_0 = AAAAAAAAACCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
    Child_1 = CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
    Child_2 = AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    Child_3 = CCCCCCCCCAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA

    we look at each log line (as above) and count how many crossovers there are total

    """

    def get_kids(line, chromosome):
        stop = barcode_indices[chromosome]
        if chromosome == 0:
            start = 0
        else:
            start = barcode_indices[chromosome - 1] + 1
        logs = line.split()
        child_0 = logs[-1].split("=")[1]
        child_1 = logs[-2].split("=")[1]
        child_2 = logs[-3].split("=")[1]
        child_3 = logs[-4].split("=")[1]
        return [child_0[start:stop], child_1[start:stop], child_2[start:stop], child_3[start:stop]]

    def get_kid_diff(beforeIA, afterIA):
        result_diff = [-1, -1, -1, -1]
        for i in range(4):
            for j in range(4):
                if afterIA[i] == beforeIA[j]:
                    result_diff[i] = j
        if -1 in result_diff:
            raise ValueError("We could not map chromatid barcodes, make sure you're swapping the correct locations.\n")
        return result_diff

    chromosomes = {0: [], 1: [], 2: [], 3: [], 4: [], 5: [], 6: [], 7: [], 8: [], 9: [], 10: [], 11: [], 12: [], 13: []}
    before = ""
    check_twice = [0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13]
    with open(output_filename) as logfile:
        for line in logfile:
            for chromosome in check_twice:
                if f"chromo({chromosome})" in line:
                    if "BeforeIA" in line:
                        before = get_kids(line, chromosome)
                    elif "AfterIA" in line:
                        after = get_kids(line, chromosome)
                        diff = get_kid_diff(before, after)
                        if diff:
                            chromosomes[chromosome].append(diff)
                    else:
                        raise ValueError("We shouldn't be here\n")

    return chromosomes


def get_barcode_indices_for_chromosome(barcode_genome_locations):
    """
    returns first and last barcode index for the gives chromosome
    Args:
        barcode_genome_locations: barcode genome locations from the config.json
    Returns:
        returns first and last barcode index for the given chromosome
    """
    chromosome_barcode_indices = []
    location = 0
    for chromosome in chromosome_ends:
        for location in barcode_genome_locations:
            if location > chromosome:
                # found the beginning of next chromosome, so the index before is the end of the previous one
                chromosome_barcode_indices.append((barcode_genome_locations.index(location))-1)
                break
    chromosome_barcode_indices.append(barcode_genome_locations.index(location))
    return chromosome_barcode_indices


def check_each_chromosome(list_of_lists, what_we_looking_at, outfile):
    """
    This function takes all the first/second/third/fourth-place child chromosomes from all the generated genomes
    and verifies that in each index there's about an equal number of original indices in the new one, which is what
    we expect with independent assortment
    ex: Chromosome 1. first child's new indices across all final genomes are [3,2,1,3,2,1,1,2,32,0,2,0,3,1,0,1,0,0,3,2,1,1,2,3,0]
    and we check the counts and what proportions each index appears in.

    Args:
        list_of_lists: indices for a chromosome
        what_we_looking_at: description of the data
        outfile: log file

    Returns:
        success if proportions are reasonable
    """
    success = True
    for child in range(4):
        fails = 0
        child_indices = [indices[child] for indices in list_of_lists]
        counts = Counter(child_indices)
        outfile.write(f"For {what_we_looking_at} child {child}, the counts are: {counts}.\n")
        total = len(child_indices)
        for count in counts:
            proportion = counts[count]/total
            counts[count] = (counts[count], proportion)
            # as in stochastic sims, sometimes one chromatid index gets overrepresented, and that's ok
            if (proportion - 0.25) > 0.1:
                fails += 1
                outfile.write(f"BAD: For {what_we_looking_at} child {child}, for count {counts[count]} proportion is "
                              f"{proportion}. Not close enough to 0.25 as expected.\n")
        if fails > 1:
            success = False
            outfile.write(f"BAD: There are multiple failures for {what_we_looking_at}. Please check.\n")
        else:
            outfile.write(f"GOOD: {what_we_looking_at} child {child} looks good.\n")

    return success


def create_report_file(chromosomes, param_obj, report_name, debug):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj[config_name]}  \n")
        if not chromosomes:
            success = False
            outfile.write("BAD: No logging data for child genes found.\n")
        else:
            # checking that per chromosome, each chromatid seeing uniform distribution of indices
            # verifies that the first chromatid gets replaced with other chromatids uniformly
            for chromosome in chromosomes:
                result = check_each_chromosome(chromosomes[chromosome], f"chromosome {chromosome + 1} chromatid", outfile)
                if not result:
                    success = False
            list_of_full_genomes = []
            # organizing each chromatid's genomes together
            for children in range(len(chromosomes[0])):
                for i in range(4):
                    temp = []
                    for chromosome in chromosomes:
                        temp.append(chromosomes[chromosome][children][i])
                    list_of_full_genomes.append(temp)

            # this goes through every final genome and verifies that the chromosomes within the genome are from
            # different chromatids, so that not every chromatid is mixed up the same way and the final
            # genome is made up of chromosomes from different chromatids
            failures = 0
            for genome in list_of_full_genomes:
                counts = Counter(genome)
                fails = 0
                for index in counts:
                    proportions = counts[index]/14
                    counts[index] = (counts[index], proportions)
                    if (proportions - 0.25) > 0.2:
                        fails += 1
                # since we are only looking at 14 spots (chromosomes) - it could happen that there's more of one
                # chromatid represented than others, and that's ok.
                if fails > 1:
                    failures += 1
                    outfile.write(f"BAD: The counts and proportions are: {counts}\n")
                elif debug:
                    outfile.write(f"GOOD: The counts and proportions are: {counts}\n")

            if failures/len(list_of_full_genomes) > 0.01:
                success = False
                outfile.write(f"BAD: There were {failures} out of {len(list_of_full_genomes)} and that's too many. Please check.\n")
            else:
                outfile.write(f"GOOD: Confirmed independent assortment across full genome for each chromatid.\n"
                              f"There were only {failures} independent failures out of {len(list_of_full_genomes)}.\n ")
            if debug:
                with open("DEBUG_chromatid_data.txt", "w") as debug_file:
                    for r in chromosomes:
                        debug_file.write(f"{r}: {chromosomes[r]}\n")

        outfile.write(dtk_sft.format_success_msg(success))


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done()
    param_obj = mal_sup.load_genetics_parameters(config_filename, debug)
    barcode_indices = get_barcode_indices_for_chromosome(param_obj[barcode_locations])
    chromosomes = parse_output_file(output_filename=stdout_filename, barcode_indices=barcode_indices, debug=debug)
    create_report_file(chromosomes, param_obj, report_name, debug)


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

    application(output_folder=args.output, stdout_filename=args.stdout,
                config_filename=args.config, report_name=args.reportname, debug=args.debug)
