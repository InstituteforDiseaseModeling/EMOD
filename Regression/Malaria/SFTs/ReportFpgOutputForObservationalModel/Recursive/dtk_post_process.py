import dtk_test.dtk_malaria_support as mal_sup
import dtk_test.dtk_sft as dtk_sft
import numpy as np
import pandas as pd
import ast
import os

"""
In this test we are verifying values for "genome_ids", "infection_ids", "recursive_nid", "recursive_count"
and roots.npy and variants.npy mappings using "SqlReportMalariaGenetics.db"

This data verification is assisted by the fact that our barcodes and genomes and roots are unique to each other, 
since the two root outbreaks are CCCC and GGGG.
We also expect the barcode arrays from variants and roots to match, because
C is encoded as 1 for the variants and the CCCC outbreak is distributed first, making it root 1
G is encoded as 2 for the variatnts and the GGGG outbreak is distributed second, making it root 2
so the roots and the variants digits are mapped 1:1 in this particular case

for CCGG would look like 1122 in variants and also as 1122 in roots. 

"""

genome_ids = "genome_ids"
infection_ids = "infection_ids"
recursive_nid = "recursive_nid"
recursive_count = "recursive_count"

report_columns = [recursive_nid, infection_ids, genome_ids]


def create_report_file(report_under_test, sql_report, rootsnpy, variantsnpy, output_folder, param_obj, report_name,
                       debug):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj['Config_Name']}: \n")
        report_df = pd.read_csv(os.path.join(output_folder, report_under_test))
        report_data = {}
        for column in report_columns:
            report_data[column] = report_df[column].apply(lambda x: ast.literal_eval(x))
        report_data[recursive_count] = report_df[recursive_count]
        roots = np.load(os.path.join(output_folder, rootsnpy))
        variants = np.load(os.path.join(output_folder, variantsnpy))
        for row in range(len(report_data[infection_ids])):
            if len(report_data[genome_ids][row]) == len(report_data[recursive_nid][row]) == \
                    report_data[recursive_count][row]:
                pass
            else:
                success = False
                outfile.write(f"BAD: at infIndex {row}, number of {genome_ids}, number of {recursive_nid}, and "
                              f"{recursive_count} should be the same, but they are: {len(report_data[genome_ids][row])}, "
                              f"{len(report_data[recursive_nid][row])}, and {report_data[recursive_count][row]} "
                              f"respectively.\n")
            # build a map of recursive_nid to genome_ids to verify that it is correct vai the SQL report
            recursive_id_genome_id_map = {}
            for place in range(len(report_data[recursive_nid][row])):
                rec_id = report_data[recursive_nid][row][place]
                genome_id = report_data[genome_ids][row][place]
                if rec_id not in recursive_id_genome_id_map:
                    recursive_id_genome_id_map[rec_id] = genome_id
                elif recursive_id_genome_id_map[rec_id] != genome_id:
                    success = False
                    outfile.write(f"BAD: {recursive_nid} to {genome_ids} mapping is inconsistent. For {recursive_nid} "
                                  f"{rec_id} we already have {recursive_id_genome_id_map[rec_id]} assigned, but "
                                  f"at infIndex {row} the mapping implies {genome_id}.\n")
            barcode_letters = {0: "A", 1: "C", 2: "G", 4: "T"}
            sql_db = mal_sup.MalariaSqlReport()
            sql_db.Open(os.path.join(output_folder, sql_report))
            for rec_id, genome_id in recursive_id_genome_id_map.items():
                # roots and variants should be the same (see comments on top)
                outfile.write(f"{variants[rec_id]}   {roots[rec_id]}\n")
                if list(variants[rec_id]) != list(roots[rec_id]):
                    outfile.write(f"BAD: We expect variants and roots to be the same sequence, but they are "
                                  f"{variants[rec_id]} and {roots[rec_id]} respectively.\n")
                # creating expected barcode and comparing to barcode from the sql database
                variant_barcode = ''
                for digit in variants[rec_id]:
                    variant_barcode = variant_barcode + barcode_letters[digit]
                # comes back from database as a list of one barcode as a string
                expected_barcode = sql_db.cursor.execute(
                    f"SELECT Barcode FROM ParasiteGenomes WHERE GenomeID ={genome_id}").fetchall()[0]
                if variant_barcode != expected_barcode:
                    success = False
                    outfile.write(f"BAD: Based on {recursive_nid} of {rec_id} mapped to {genome_ids} of {genome_id}, "
                                  f"we expected the barcode to be "
                                  f"{variant_barcode}, but from the {sql_report} - barcode matching the genome is "
                                  f"{expected_barcode}.\n")
                else:
                    outfile.write(f"GOOD: {recursive_nid} of {rec_id} mapped to {genome_ids} of {genome_id}, "
                                  f"has expected barcode {variant_barcode} which matches the data from {sql_report}.\n")
        if debug:
            outfile.write(f"roots.npy: \n {roots}\n\nvariants.npy:\n{variants}\n\n")
            outfile.write(f"recursive_id_genome_id_map:\n {str(recursive_id_genome_id_map)}\n\n")

        if success:
            outfile.write(f"GOOD: Number of {genome_ids}, number of {recursive_nid}, and "
                          f"{recursive_count} matched for the entire simulation. \n")

        outfile.write(dtk_sft.format_success_msg(success))


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json", rootsnpy="roots.npy",
                variantsnpy="variants.npy",
                report_name=dtk_sft.sft_output_filename,
                report_under_test="infIndexRecursive-genomes-df.csv",
                sql_report="SqlReportMalariaGenetics.db",
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("report_under_test: " + report_under_test + "\n")
        print("sql_report: " + sql_report + "\n")
        print("variantsnpy: " + variantsnpy + "\n")
        print("rootsnpy: " + rootsnpy + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done()
    param_obj = mal_sup.load_genetics_parameters(config_filename, debug)
    create_report_file(report_under_test, sql_report, rootsnpy, variantsnpy, output_folder, param_obj, report_name,
                       debug)


if __name__ == "__main__":
    application()
