import json
import dtk_test.dtk_malaria_support as mal_sup
import dtk_test.dtk_sft as dtk_sft
import os
import pandas as pd

"""
In this test we will make sure that the numbers for infections are internally consistent within the report
and cross-reference applicable numbers with "Avg Num Infections", "Infected", "Statistical Population",
and "New Infections" channels from InsetChar.json. We are assuming InsetChart.json has correct data due to 
historical presence.

This test validates internal consistency with some external support for "NumNewInfectionsInLastYear", 
"AvgNumInfections", "NumIndividuals", "NumInfected", "NumInfectionsCleared", "CurrentNumInfections" columns.

Checks:
1. "AvgNumInfections" in report == "Avg Num Infections" in InsetChart
2. "NumInfected"  in report == "Infected" * "Statistical Population" in InsetChart
3. "NumIndividuals" in report == "Statistical Population" in InsetChart
4.  calculating current infections with "NumNewInfectionsInLastYear" - cumulative "NumInfectionsCleared" 
    for the first 365 days
5. "NumInfected" * "AvgNumInfections" == "CurrentNumInfections"
6. Verify that report's "CurrentNumInfections" == insetchart's "Num Total Infections"
    
"""

new_infections_in_last_year = "NumNewInfectionsInLastYear"
avg_num_infections = "AvgNumInfections"
num_individual = "NumIndividuals"
num_infected = "NumInfected"
num_cleared = "NumInfectionsCleared"
num_current = "CurrentNumInfections"
chart_num_infected = "Infected"
chart_avg_num_infections = "Avg Num Infections"
chart_statistical_population = "Statistical Population"
chart_num_total_infections = "Num Total Infections"
inset_chart_channels = [chart_num_total_infections, chart_num_infected, chart_statistical_population,
                        chart_avg_num_infections]
report_columns = [new_infections_in_last_year, avg_num_infections, num_individual, num_infected, num_current,
                  num_cleared]


def create_report_file(report_under_test, inset_chart, output_folder, param_obj, report_name, debug):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj['Config_Name']}: \n")
        report_df = pd.read_csv(os.path.join(output_folder, report_under_test))
        report_data = {}
        chart_data = {}
        for column in report_columns:
            report_data[column] = report_df[column]
        with open(output_folder + "//" + inset_chart, "r") as output:
            output_data = json.load(output)["Channels"]
        for channel in inset_chart_channels:
            chart_data[channel] = output_data[channel]["Data"]
        a_data_length = len(chart_data[chart_statistical_population])
        if a_data_length != len(report_data[num_individual]):
            success = False
            outfile.write(f"BAD: length of data from InsetChart = {a_data_length} is not"
                          f" equal to length of data from {report_under_test} = {len(report_data[num_individual])}."
                          f"\n")
        else:
            # check 1
            little_success = True
            for day in range(a_data_length):
                if round(chart_data[chart_avg_num_infections][day], 5) != report_data[avg_num_infections][day]:
                    little_success = False
                    outfile.write(f"Check 1: BAD: On Day {day} (0-index), {inset_chart}'s {chart_avg_num_infections} "
                                  f"value "
                                  f"{chart_data[chart_avg_num_infections][day]} was not"
                                  f" equal to {report_under_test}'s {avg_num_infections} value "
                                  f"{report_data[avg_num_infections][day]}. \n")
            if little_success:
                outfile.write(f"Check 1: GOOD: {inset_chart}'s {chart_avg_num_infections} values matched {report_under_test}'s "
                              f"{avg_num_infections} values for all the days.\n")
            else:
                success = False
            # check 2 "NumInfected"  in report == "Infected" * "Statistical Population" in InsetChart
            little_success = True

            for day in range(a_data_length):
                chart_infected_on_day = chart_data[chart_num_infected][day] * chart_data[chart_statistical_population][day]
                if round(chart_infected_on_day, 0) != report_data[num_infected][day]:
                    little_success = False
                    outfile.write(f"Check 2: BAD: On Day {day} (0-index), {inset_chart}'s {chart_num_infected} "
                                  f"calculated value "
                                  f"{round(chart_infected_on_day, 0)} was not"
                                  f" equal to {report_under_test}'s {num_infected} value "
                                  f"{report_data[num_infected][day]}. \n")
            if little_success:
                outfile.write(f"Check 2: GOOD: {inset_chart}'s {chart_num_infected} values matched {report_under_test}'s "
                              f"{num_infected} values for all the days.\n")
            else:
                success = False
            # check 3 "NumIndividuals" in report == "Statistical Population" in InsetChart
            little_success = True
            for day in range(a_data_length):
                if chart_data[chart_statistical_population][day] != report_data[num_individual][day]:
                    little_success = False
                    outfile.write(f"Check 3: BAD: On Day {day} (0-index), {inset_chart}'s "
                                  f"{chart_statistical_population} value "
                                  f"{chart_data[chart_statistical_population][day]} was not"
                                  f" equal to {report_under_test}'s {num_individual} value "
                                  f"{report_data[num_individual][day]}. \n")
            if little_success:
                outfile.write(f"Check 3: GOOD: {inset_chart}'s {chart_statistical_population} values matched "
                              f"{report_under_test}'s {num_individual} values for all the days.\n")
            else:
                success = False
            # check 4 calculating current infections with "NumNewInfectionsInLastYear" - cumula"NumInfectionsCleared"
            # for the first 365 days
            cumulative_infections_cleared = 0
            little_success = True
            for day in range(365):
                cumulative_infections_cleared += report_data[num_cleared][day]
                calculated_current_infections = report_data[new_infections_in_last_year][day] - cumulative_infections_cleared
                if calculated_current_infections != report_data[num_current][day]:
                    little_success = False
                    outfile.write(f"Check 4: BAD: On Day {day} (0-index), {inset_chart}'s "
                                  f"calculated_current_infections ('NumNewInfectionsInLastYear' - "
                                  f"'NumInfectionsCleared') value "
                                  f"{calculated_current_infections} was not"
                                  f" equal to {report_under_test}'s {num_current} value "
                                  f"{report_data[num_current][day]}. \n")
            if little_success:
                outfile.write(f"Check 4: GOOD: {inset_chart}'s "
                              f"calculated_current_infections values matched "
                              f"{report_under_test}'s {num_current} values for all the days.\n")
            else:
                success = False
            # check 5 "NumInfected" * "AvgNumInfections" == "CurrentNumInfections"
            little_success = True
            for day in range(a_data_length):
                calculated_current_num_infections = round(report_data[num_infected][day] * report_data[avg_num_infections][day], 0)
                if calculated_current_num_infections != report_data[num_current][day]:
                    little_success = False
                    outfile.write(f"Check 5: BAD: On Day {day} (0-index), 'NumInfected' * 'AvgNumInfections' "
                                  f"{calculated_current_num_infections} was not"
                                  f" equal to {report_under_test}'s {num_current} value "
                                  f"{report_data[num_current][day]}. \n")
            if little_success:
                outfile.write(f"Check 5: GOOD: 'NumInfected' * 'AvgNumInfections' matched {report_under_test}'s "
                              f"{num_current} values for all the days.\n")
            else:
                success = False
            # check 6 Verify that report's "CurrentNumInfections" == insetchart's "Num Total Infections"
            little_success = True
            for day in range(a_data_length):
                if chart_data[chart_num_total_infections][day] != report_data[num_current][day]:
                    little_success = False
                    outfile.write(f"Check 6: BAD: On Day {day} (0-index), {inset_chart}'s {chart_num_total_infections} "
                                  f" {chart_data[chart_num_total_infections][day]} was not"
                                  f" equal to {report_under_test}'s {num_current} value "
                                  f"{report_data[num_current][day]}. \n")
            if little_success:
                outfile.write(f"Check 6: GOOD: {inset_chart}'s {chart_num_total_infections} "
                              f"matched {report_under_test}'s "
                              f"{num_current} values for all the days.\n")
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
