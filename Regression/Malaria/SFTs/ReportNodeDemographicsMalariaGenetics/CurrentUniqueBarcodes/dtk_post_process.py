import os
import dtk_test.dtk_malaria_support as mal_sup
import dtk_test.dtk_sft as dtk_sft
import pandas as pd
import math

"""
 This test verifies that  "CurrentUniqueBarcodes" and "FractionOfRootsPresentInLastYear" are correct. 
 
 We have an outbreak that gives out a unique barcodes every day once a day. 
 That means that Roots are same as Genomes are same as Barcodes same as Infections

Check:
1. CurrentUniqueBarcodes == CurrentUniqueGenomes == CurrentNumInfections
2. FractionOfRootsPresentInLastYear == new roots for the past year/totalnumroots
"""
current_unique_genomes = "CurrentUniqueGenomes"
current_unique_barcodes = "CurrentUniqueBarcodes"
current_infections = "CurrentNumInfections"
infections_cleared = "NumInfectionsCleared"
fraction_of_roots_in_last_year = "FractionOfRootsPresentInLastYear"
total_roots = "NumTotalRoots"
report_columns = [current_unique_barcodes, current_unique_genomes,
                  current_infections, fraction_of_roots_in_last_year, infections_cleared,
                  total_roots]


def create_report_file(report_under_test, output_folder, param_obj, report_name):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj['Config_Name']}: \n")
        report_df = pd.read_csv(os.path.join(output_folder, report_under_test))
        report_data = {}
        for column in report_columns:
            report_data[column] = report_df[column]
        a_data_length = len(report_data[current_infections])
        year = 365
        # 1. CurrentUniqueBarcodes == CurrentUniqueGenomes == CurrentNumInfections
        little_success = True
        for day in range(a_data_length):
            if report_data[current_unique_barcodes][day] == report_data[current_unique_genomes][day] == \
                    report_data[current_infections][day]:
                pass
            else:
                little_success = False
                outfile.write(f"Check 1: BAD: On Day {day} (0-index), we expected {report_under_test}'s "
                              f"{current_unique_barcodes}, {current_unique_genomes}, and {current_infections} "
                              f"to have the same value, but the values are {report_data[current_unique_barcodes][day]}, "
                              f"{report_data[current_unique_genomes][day]}, and {report_data[current_infections][day]} "
                              f"respectively. \n")
        if little_success:
            outfile.write(f"Check 1: GOOD: {report_under_test}'s "
                          f"{current_unique_barcodes}, {current_unique_genomes}, and {current_infections} "
                          f"had same values for all the days.\n")
        else:
            success = False
        # 2. FractionOfRootsPresentInLastYear == new roots for the past year/totalnumroots
        little_success = True
        for day in range(a_data_length):
            if day >= year:
                expected_fraction_of_roots_in_last_year = year / report_data[total_roots][day]
            else:
                expected_fraction_of_roots_in_last_year = 1
            from_report = report_data[fraction_of_roots_in_last_year][day]
            if math.isclose(expected_fraction_of_roots_in_last_year, from_report, abs_tol=0.000002):
                pass
            else:
                little_success = False
                outfile.write(f"Check 2: BAD: On Day {day} (0-index), we expected {report_under_test}'s "
                              f"{fraction_of_roots_in_last_year} to be {expected_fraction_of_roots_in_last_year}, but "
                              f"the value is {from_report}. \n")
        if little_success:
            outfile.write(f"Check 2: GOOD: {report_under_test}'s "
                          f"{fraction_of_roots_in_last_year} have correct values for all the days.\n")
        else:
            success = False

        outfile.write(dtk_sft.format_success_msg(success))


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json", inset_chart="InsetChart.json",
                report_name=dtk_sft.sft_output_filename,
                report_under_test="ReportNodeDemographicsMalariaGenetics.csv",
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("report_under_test: " + report_under_test + "\n")
        print("inset_chart: " + inset_chart + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done()
    param_obj = mal_sup.load_genetics_parameters(config_filename, debug)
    create_report_file(report_under_test, output_folder, param_obj, report_name)


if __name__ == "__main__":
    application()
