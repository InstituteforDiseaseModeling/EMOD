import json
import dtk_test.dtk_malaria_support as mal_sup
import dtk_test.dtk_sft as dtk_sft
import os
import pandas as pd

"""
In this test we are verifying values for "age_day", "fever_status"

We use the MalariaPatientReport to verify this data.

"""

age_day = "age_day"
individual_id = "IndividualID"
fever_status = "fever_status"
sim_day = "day"

report_columns = [individual_id, age_day, fever_status, sim_day]


def create_report_file(report_under_test, patient_report, genetics_report, output_folder, param_obj, report_name,
                       debug):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj['Config_Name']}: \n")
        with open(os.path.join(output_folder, patient_report)) as custom_reports:
            patient_report_data = json.load(custom_reports)["patient_array"]
        initial_age = {}
        for patient in patient_report_data:
            initial_age[patient["id"]] = patient["initial_age"]
        # get genetics report clinical symptoms data
        genetics_report_df = pd.read_csv(os.path.join(output_folder, genetics_report))
        num_has_clinical_symptoms = genetics_report_df["NumHasClinicalSymptoms"]
        # get report data
        report_df = pd.read_csv(os.path.join(output_folder, report_under_test))
        report_data = {}
        for column in report_columns:
            report_data[column] = report_df[column]
        fpg_fevers = {}
        for row in range(len(report_data[age_day])):
            person = report_data[individual_id][row]
            report_fever = report_data[fever_status][row]
            day = report_data[sim_day][row]
            # accumulating fever data to check later
            if report_fever:
                if day in fpg_fevers:
                    fpg_fevers[day] += 1
                else:
                    fpg_fevers[day] = 1
            report_age = report_data[age_day][row]
            calculated_age = initial_age[person] + day
            if calculated_age != report_age:
                success = False
                outfile.write(f"BAD: At infIndex {row} of {report_under_test}, person {person} age is "
                              f"{report_age}, but we expected {calculated_age} based on "
                              f"{patient_report}. \n")

        # checking fever data
        with open("custom_reports.json") as custom_reports:
            fpg_report_config = json.load(custom_reports)["Reports"][1]
            if fpg_report_config["class"] != "ReportFpgOutputForObservationalModel":
                fpg_report_config = json.load(custom_reports)["Reports"][0]
        fpg_report_start_day = fpg_report_config["Start_Day"]
        fpg_report_sampling_period = fpg_report_config["Sampling_Period"]
        possible_report_days = [fpg_report_start_day + x * fpg_report_sampling_period for x in range(0, 100)]
        for day in range(len(num_has_clinical_symptoms)):
            if day in possible_report_days:
                if num_has_clinical_symptoms[day] > 0 and day not in fpg_fevers:
                    success = False
                    outfile.write(f"BAD: on day {day}, we expected to see {num_has_clinical_symptoms[day]} "
                                  f"fevers, but couldn't find any.\n ")
                elif num_has_clinical_symptoms[day] != fpg_fevers[day]:
                    success = False
                    outfile.write(f"BAD: on day {day}, we expected to see {num_has_clinical_symptoms[day]} "
                                  f"fevers, but counted {fpg_fevers[day]}.\n ")
        if success:
            outfile.write(f"GOOD: {fever_status} and {age_day} are correct for all of {report_under_test}. \n")

        outfile.write(dtk_sft.format_success_msg(success))


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json", genetics_report="ReportNodeDemographicsMalariaGenetics.csv",
                report_name=dtk_sft.sft_output_filename,
                report_under_test="infIndexRecursive-genomes-df.csv",
                patient_report="MalariaPatientReport_test.json",
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("report_under_test: " + report_under_test + "\n")
        print("patient_report: " + patient_report + "\n")
        print("genetics_report: " + genetics_report + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done()
    param_obj = mal_sup.load_genetics_parameters(config_filename, debug)
    create_report_file(report_under_test, patient_report, genetics_report, output_folder, param_obj, report_name, debug)


if __name__ == "__main__":
    application()
