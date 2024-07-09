import json
import dtk_test.dtk_malaria_support as mal_sup
import dtk_test.dtk_sft as dtk_sft
import os
import pandas as pd

"""
 This test verifies that "CurrentUniqueGenomes", "CurrentUniqueBarcodes", "TotalUniqueGenomes", "TotalUniqueBarcodes",
 "UniqueGenomesLast_1_Years", and "UniqueBarcodesLast_1_Years" are correct. 
 
 Setup: Every day we infect one person with two infections. The infections have the same barcode that is unique 
 to that day. This means that the unique barcodes are increasing +1 and unique genomes are increasing +2 per day. 
 
 Checks:
1. TotalUniqueGenomes are +2 every day for the entire simulation and == NumTotalRoots
2. TotalUniqueBarcodes * 2 = TotalUniqueGenomes since we release 2 of the same Barcode per day
3. TotalUniqueBarcodes are +1 every day for the entire simulation and == accumulated "New Infections" from InsetChart
4. and 5 UniqueBarcodes/GenomesLast_1_Years increase until day 365 and then continue with Barcodes == 365, Genomes == 2 * 365
6. CurrentUniqueGenomes == CurrentNumInfections == "Num Total Infections" from insetchart
7. CurrentUniqueGenomes[day] == TotalUniqueGenomes[day] - accumulated "NumInfectionsCleared" up to [day]
8. CurrentUniqueGenomes will always be more than or equal the the CurrentUniqueBarcodes

"""
current_unique_genomes = "CurrentUniqueGenomes"
current_unique_barcodes = "CurrentUniqueBarcodes"
total_unique_genomes = "TotalUniqueGenomes"
total_unique_barcodes = "TotalUniqueBarcodes"
unique_genomes_one_year = "UniqueGenomesLast_1_Years"
unique_barcodes_one_year = "UniqueBarcodesLast_1_Years"
total_num_roots = "NumTotalRoots"
current_infections = "CurrentNumInfections"
infections_cleared = "NumInfectionsCleared"
chart_new_humans_with_infections = "New Infections"
chart_num_total_infections = "Num Total Infections"
report_columns = [current_unique_barcodes, current_unique_genomes, total_unique_genomes, total_unique_barcodes,
                  unique_barcodes_one_year, unique_genomes_one_year, total_num_roots,
                  current_infections, infections_cleared]


def create_report_file(report_under_test, inset_chart, output_folder, param_obj, report_name, debug):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj['Config_Name']}: \n")
        report_df = pd.read_csv(os.path.join(output_folder, report_under_test))
        report_data = {}
        chart_data = {}
        for column in report_columns:
            report_data[column] = report_df[column]
        with open(os.path.join(output_folder, inset_chart), "r") as output:
            output_data = json.load(output)["Channels"]
        chart_data[chart_new_humans_with_infections] = output_data[chart_new_humans_with_infections]["Data"]
        chart_data[chart_num_total_infections] = output_data[chart_num_total_infections]["Data"]
        a_data_length = len(chart_data[chart_num_total_infections])
        genomes_per_day = 2
        year = 365
        if a_data_length != len(report_data[unique_genomes_one_year]):
            success = False
            outfile.write(f"BAD: length of data from InsetChart = {a_data_length} is not"
                          f" equal to length of data from {report_under_test} = {len(report_data[unique_genomes_one_year])}."
                          f"\n")
        else:
            # 1. TotalUniqueGenomes are +2 every day for the entire simulation and == NumTotalRoots
            little_success = True
            expected_total_unique_genomes = 0
            for day in range(a_data_length):
                expected_total_unique_genomes += genomes_per_day
                if expected_total_unique_genomes != report_data[total_unique_genomes][day]:
                    little_success = False
                    outfile.write(f"Check 1: BAD: On Day {day} (0-index), expected_total_unique_genomes value was "
                                  f"{expected_total_unique_genomes} but was not"
                                  f" equal to {report_under_test}'s {total_unique_genomes} value "
                                  f"{report_data[total_unique_genomes][day]}. \n")
                if report_data[total_unique_genomes][day] != report_data[total_num_roots][day]:
                    outfile.write(f"Check 1: BAD: On Day {day} (0-index),{report_under_test}'s {total_num_roots} value "
                                  f"{report_data[total_num_roots][day]} was not"
                                  f" equal to {report_under_test}'s {total_unique_genomes} value "
                                  f"{report_data[total_unique_genomes][day]}. \n")
            if little_success:
                outfile.write(f"Check 1: GOOD: {report_under_test}'s "
                              f"{total_num_roots} values matched {report_under_test}'s "
                              f"{total_unique_genomes} values and expected_total_unique_genomes values "
                              f"for all the days.\n")
            else:
                success = False
            #  2. TotalUniqueBarcodes * 2 = TotalUniqueGenomes since we release 2 of the same Barcode per day
            little_success = True
            for day in range(a_data_length):
                if report_data[total_unique_barcodes][day] * 2 != report_data[total_unique_genomes][day]:
                    little_success = False
                    outfile.write(f"Check 2: BAD: On Day {day} (0-index), {report_under_test}'s "
                                  f"{total_unique_barcodes} * 2 value of "
                                  f"{report_data[total_unique_barcodes][day] * 2} was not"
                                  f" equal to {report_under_test}'s {total_unique_genomes} value "
                                  f"{report_data[total_unique_genomes][day]}. \n")

            if little_success:
                outfile.write(f"Check 2: GOOD: {report_under_test}'s "
                              f"{total_unique_barcodes} * 2 values matched {report_under_test}'s "
                              f"{total_unique_genomes} values for all the days.\n")
            else:
                success = False
            little_success = True
            # 3. TotalUniqueBarcodes are +1 every day for the entire simulation and == accumulated
            # "New Infections" from InsetChart
            expected_total_unique_barcodes = 0
            for day in range(a_data_length):
                expected_total_unique_barcodes += chart_data[chart_new_humans_with_infections][day]
                if expected_total_unique_barcodes != report_data[total_unique_barcodes][day]:
                    little_success = False
                    outfile.write(f"Check 3: BAD: On Day {day} (0-index), expected_total_unique_barcodes calculated"
                                  f"from {inset_chart}'s {chart_new_humans_with_infections} with value of "
                                  f"{expected_total_unique_barcodes} was not"
                                  f" equal to {report_under_test}'s {total_unique_barcodes} value "
                                  f"{report_data[total_unique_barcodes][day]}. \n")

            if little_success:
                outfile.write(f"Check 3: GOOD: expected_total_unique_barcodes calculated"
                              f"from {inset_chart}'s {chart_new_humans_with_infections} values matched {report_under_test}'s "
                              f"{total_unique_genomes} values for all the days.\n")
            else:
                success = False
            # 4. and 5 UniqueBarcodes/GenomesLast_1_Years increase until day 365 and then continue with
            # Barcodes == 365, Genomes == 2 * 365
            expected_unique_genomes = 0
            expected_unique_barcodes = 0
            little_success1 = True
            little_success = True
            for day in range(a_data_length):
                if day < year:
                    expected_unique_genomes += genomes_per_day
                    expected_unique_barcodes += 1
                else:
                    expected_unique_genomes = genomes_per_day * year
                    expected_unique_barcodes = 1 * year
                if report_data[unique_genomes_one_year][day] != expected_unique_genomes:
                    little_success1 = False
                    outfile.write(f"Check 4: BAD: On day {day} (0-index) we expected {unique_genomes_one_year} to be "
                                  f"{expected_unique_genomes}, but got "
                                  f"{report_data[unique_genomes_one_year][day]}.\n")
                if report_data[unique_barcodes_one_year][day] != expected_unique_barcodes:
                    little_success = False
                    outfile.write(f"Check 5: BAD: On day {day} (0-index) we expected {unique_barcodes_one_year} to be "
                                  f"{expected_unique_barcodes}, but got "
                                  f"{report_data[unique_barcodes_one_year][day]}.\n")
            if little_success1:
                outfile.write(f"Check 4: GOOD: '{unique_genomes_one_year}' "
                              f"has correct values throughout the simulation.\n")
            else:
                success = False
            if little_success:
                outfile.write(f"Check 5: GOOD: '{unique_barcodes_one_year}' "
                              f"has correct values throughout the simulation.\n")
            else:
                success = False
        # 6. CurrentUniqueGenomes == CurrentNumInfections == "Num Total Infections" from insetchart
        little_success = True
        for day in range(a_data_length):
            if report_data[current_unique_genomes][day] == report_data[current_infections][day] == \
                    chart_data[chart_num_total_infections][day]:
                pass
            else:
                little_success = False
                outfile.write(f"Check 6: BAD: On day {day} (0-index) we expected {report_under_test}'s "
                              f"{current_unique_genomes} and "
                              f"{current_infections}, and {inset_chart}'s {chart_num_total_infections} to be equal, "
                              f"but the values are {report_data[current_unique_genomes][day]}, "
                              f"{report_data[current_infections][day]}, "
                              f"and {chart_data[chart_num_total_infections][day]}.\n")

        if little_success:
            outfile.write(f"Check 6: GOOD:  {report_under_test}'s {current_unique_genomes} and "
                          f"{current_infections}, and {inset_chart}'s {chart_num_total_infections} are equal for all "
                          f"timesteps as expected. \n")
        else:
            success = False
        #  7. CurrentUniqueGenomes[day] == TotalUniqueGenomes[day] - accumulated "NumInfectionsCleared" up to [day]
        little_success = True
        infections_cleared_to_date = 0
        for day in range(a_data_length):
            infections_cleared_to_date += report_data[infections_cleared][day]
            calculated_current_unique_genomes = report_data[total_unique_genomes][day] - infections_cleared_to_date
            if report_data[current_unique_genomes][day] == calculated_current_unique_genomes:
                pass
            else:
                little_success = False
                outfile.write(f"Check 7: BAD: On day {day} (0-index) we expected {report_under_test}'s "
                              f"{current_unique_genomes} and to be equal to calculated_current_unique_genomes "
                              f"{calculated_current_unique_genomes}, "
                              f"but the value is {report_data[current_unique_genomes][day]}.\n")

        if little_success:
            outfile.write(f"Check 7: GOOD:  {report_under_test}'s {current_unique_genomes} and "
                          f"calculated_current_unique_genomes are equal for all "
                          f"timesteps as expected. \n")
        else:
            success = False

        # 8. This rule-of-thumb test to make sure that CurrentUniqueBarcodes makes sense by always being at
        # least equal or more to CurrentUniqueGenomes
        little_success = True
        for day in range(a_data_length):
            if report_data[current_unique_genomes][day] >= report_data[current_unique_barcodes][day] :
                pass
            else:
                little_success = False
                outfile.write(f"Check 8: BAD: On day {day} (0-index) we expected {report_under_test}'s "
                              f"{current_unique_barcodes} with value {report_data[current_unique_genomes][day]} "
                              f"to be greater or equal than  {current_unique_genomes}, "
                              f"but the value is {report_data[current_unique_genomes][day]}.\n")

        if little_success:
            outfile.write(f"Check 8: GOOD:  {report_under_test}'s {current_unique_genomes} is always great or equal "
                          f" to {current_unique_barcodes} for all the days. \n")
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
    create_report_file(report_under_test, inset_chart, output_folder, param_obj, report_name, debug)


if __name__ == "__main__":
    application()
