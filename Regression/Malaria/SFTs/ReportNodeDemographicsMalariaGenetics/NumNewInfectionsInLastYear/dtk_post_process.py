import json
import dtk_test.dtk_malaria_support as mal_sup
import dtk_test.dtk_sft as dtk_sft
import os
import pandas as pd

"""
This test verifies "NumNewInfectionsInLastYear" is correct. 
We have an outbreak of two-infections-into-one-person-a-day, so up until the year, 
there should be +2 each day of infection, while the "New Infections" in the InsetChart will show 1 as only
one person is getting the new infections
After a year, the number of "NumNewInfectionsInLastYear" should always be 2*365
There are no vectors, so there shouldn't be any new infections outside of the outbreak

"""

new_infections_in_last_year = "NumNewInfectionsInLastYear"
chart_new_humans_with_infections = "New Infections"


def create_report_file(report_under_test, inset_chart, output_folder, param_obj, report_name, debug):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj['Config_Name']}: \n")
        report_df = pd.read_csv(os.path.join(output_folder, report_under_test))
        new_infections_data = report_df[new_infections_in_last_year]
        with open(os.path.join(output_folder, inset_chart), "r") as output:
            output_data = json.load(output)["Channels"]
        chart_new_humans_with_infections_data = output_data[chart_new_humans_with_infections]["Data"]
        year = 365
        expected_new_infections = 0
        infections_per_outbreak = 2
        # main check
        for day in range(len(new_infections_data)):
            if day < year:
                expected_new_infections += infections_per_outbreak
            else:
                expected_new_infections = infections_per_outbreak * year
            if new_infections_data[day] != expected_new_infections:
                success = False
                outfile.write(f"BAD: On day {day} (0-index) we expected {expected_new_infections}, but got "
                              f"{new_infections_data[day]}.\n")
        if success:
            outfile.write(f"GOOD: '{new_infections_in_last_year}' 'has correct values throughout the simulation.\n")
        # secondary check to make sure that InsetChart's "New Infections" is recording one "new infection" per day
        # as it's not counting new infection object, but rather how many people got any new infections
        for day in range(len(chart_new_humans_with_infections_data)):
            if chart_new_humans_with_infections_data[day] != 1:
                success = False
                outfile.write(f"BAD: We expect there to be 1 {chart_new_humans_with_infections} in {inset_chart} "
                              f"per day, but on day {day} we got {chart_new_humans_with_infections_data[day]}. \n")
        if success:
            outfile.write(f"GOOD: We expect there to be 1 {chart_new_humans_with_infections} in {inset_chart} "
                          f"per day and there was.\n")
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
