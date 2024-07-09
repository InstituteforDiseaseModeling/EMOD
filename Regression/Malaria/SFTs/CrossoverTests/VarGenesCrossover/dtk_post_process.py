
import dtk_test.dtk_malaria_support as mal_sup
import dtk_test.dtk_sft as dtk_sft
import os

"""
Verifying the var genes (MSP and PFEMP1) travel and are crossed over along with the barcode genes.
Barcode gene crossovers verified elsewhere. We are piggy backing on barcode locations, "attaching" var
genes by having them be right next to them. We are verifying that when barcode genes (values) move, so do the
var genes. 
 
"""

config_name = "Config_Name"
barcode_locations = "Barcode_Genome_Locations"
pfemp1_locations = "PfEMP1_Variants_Genome_Locations"
msp_location = "MSP_Genome_Location"


def create_report_file(output_folder, sql_report, param_obj, report_name, debug):
    success = True
    fails = []
    fails_log_file = "DEBUG_fails_log.txt"
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj[config_name]} : \n")
        # create_nucleotide_sequence_association
        var_genes = sorted(param_obj[pfemp1_locations] + [param_obj[msp_location]])
        barcode_loc = param_obj[barcode_locations]
        associates = dict(zip(var_genes, barcode_loc))
        sql_db = mal_sup.MalariaSqlReport()
        sql_db.Open(os.path.join(output_folder, sql_report))
        all_genomes = list(set(sql_db.cursor.execute("SELECT GenomeID FROM Infections").fetchall()))
        mixed_genomes = 0
        for genome in all_genomes:
            first = False
            second = False
            nucleotide_sequence = sql_db.cursor.execute(f"SELECT NucleotideSequence FROM GenomeSequenceData WHERE GenomeID ={genome}").fetchall()
            genome_locations = sql_db.cursor.execute(f"SELECT GenomeLocation FROM GenomeSequenceData WHERE GenomeID ={genome}").fetchall()
            chromosome = dict(zip(genome_locations, nucleotide_sequence))
            for loc in chromosome:
                value = chromosome[loc]
                if loc in var_genes:
                    if value in range(100, 151):  # we expect the associate barcode location to have 0
                        first = True
                        if chromosome[associates[loc]] != 3:
                            success = False
                            outfile.write(f"BAD: Genome {genome}, We were expecting barcode value of 3 in {associates[loc]} as it is "
                                          f"associated with \n "
                                          f"var_genes location {loc} with value of {value}, but we got {chromosome[associates[loc]]}\n")
                    elif value in range(200, 251):  # we expect the associate barcode location to have 1
                        second = True
                        if chromosome[associates[loc]] != 2:
                            success = False
                            outfile.write(f"BAD: Genome {genome}, We were expecting barcode value of 2 in {associates[loc]} as it is "
                                          f"associated with \n "
                                          f"var_genes location {loc} with value of {value}, but we got {chromosome[associates[loc]]}\n")
                    else:
                        success = False
                        outfile.write("BAD: Got a value in a bad range. Please check settings.\n")
            if first and second:
                mixed_genomes += 1
        outfile.write(f"There were {mixed_genomes} genomes with crossovers\n")
        if success:
            outfile.write("GOOD: All var genes were crossed over with barcode genes.\n")
        outfile.write(dtk_sft.format_success_msg(success))


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json",
                sql_report="SqlReportMalariaGenetics.db",
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
    create_report_file(output_folder, sql_report, param_obj, report_name, debug)


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
