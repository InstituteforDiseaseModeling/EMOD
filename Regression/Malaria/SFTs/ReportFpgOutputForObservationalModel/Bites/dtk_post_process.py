import json
import dtk_test.dtk_malaria_support as mal_sup
import dtk_test.dtk_sft as dtk_sft
import os
import ast
import pandas as pd

"""
In this test we are verifying values for "individual_id", "bite_ids", "infection_ids", "day", "month", "year", "infIndex"

We verify bite and infections ids using logging, by keeping track of new infections and bites and cleared infections. 
Date data we verify with direct calculations based on simulation day

"""

individual_id = "IndividualID"
bite_ids = "bite_ids"
infection_ids = "infection_ids"
sim_day = "day"
month = "month"
year = "year"
inf_index = "infIndex"
report_columns = [individual_id, bite_ids, infection_ids, sim_day, month, year, inf_index]


def parse_stdout_file(stdout_filename="test.txt", debug=False):
    """
        Parsing log fife
    Args:
        stdout_filename: Name of file
        debug: if flag is set to true, the log lines of interest and final data are written out as a text and csv files

    Returns: dataframe of daily feed counts and daily vector infection counts

    """
    with open(stdout_filename, "r") as log_file:
        filtered_lines = []

        infection_bite_ids_today = {2: [], 3: [], 4: [], 5: [], 6: []}
        infections_bites_ids_per_day = {2: [], 3: [], 4: [], 5: [], 6: []}
        infections_cleared_today = []
        infections_cleared_per_day = []
        for line in log_file:
            if "Update(): Time:" in line:
                filtered_lines.append(line)
                for person_id in infections_bites_ids_per_day:
                    infections_bites_ids_per_day[person_id].append(infection_bite_ids_today[person_id])
                infections_cleared_per_day.append(infections_cleared_today)
                for person_id in infection_bite_ids_today:
                    infection_bite_ids_today[person_id] = []
                infections_cleared_today = []
            elif "got bite" in line:
                filtered_lines.append(line)
                person_id = int(dtk_sft.get_val(" Person ", line))
                bite_id = int(dtk_sft.get_val("got bite ", line))
                infection_id = int(dtk_sft.get_val("creating infection ", line))
                infection_bite_ids_today[person_id].append([infection_id, bite_id])
            elif "cleared." in line:
                filtered_lines.append(line)
                infection_id_cleared = int(dtk_sft.get_val("'s infection ", line))
                if infection_id_cleared in infections_cleared_today:
                    raise ValueError("There shouldn't be multiple of the same infections cleared. \n")
                else:
                    infections_cleared_today.append(infection_id_cleared)

        # get a list of all the infection ids present in the day
        # get a list of bites for those ids (throwing out repeating ones)
        days = len(infections_bites_ids_per_day[2])  # length of data
        expected_bite_ids = {2: [], 3: [], 4: [], 5: [], 6: []}
        expected_infection_ids = {2: [], 3: [], 4: [], 5: [], 6: []}
        list_ids_current_infections = {2: [], 3: [], 4: [], 5: [], 6: []}
        for day in range(days):
            for person in range(2, 7):
                person_list_ids_current_infections = list_ids_current_infections[person]
                person_list_ids_current_infections.extend(infections_bites_ids_per_day[person][day])
                if person_list_ids_current_infections:
                    if infections_cleared_per_day[day]:
                        remove = []
                        for [inf, bite] in person_list_ids_current_infections:
                            if inf in infections_cleared_per_day[day]:
                                remove.append([inf, bite])
                        if remove:
                            person_list_ids_current_infections = [inf_bite for inf_bite in
                                                                  person_list_ids_current_infections if
                                                                  inf_bite not in remove]
                    bite_ids_today = [bite for [inf, bite] in person_list_ids_current_infections]
                    list_ids_current_infections_person = [inf for [inf, bite] in person_list_ids_current_infections]
                    list_ids_current_infections[person] = person_list_ids_current_infections
                    # multibites = [item for item, count in collections.Counter(just_bites_today).items() if count > 1]
                    expected_bite_ids[person].append(bite_ids_today)
                    expected_infection_ids[person].append(list_ids_current_infections_person)

                else:
                    expected_bite_ids[person].append([])
                    expected_infection_ids[person].append([])

    if debug:
        pd.DataFrame.from_dict(expected_bite_ids).to_csv("Bites_from_logging.csv")
        pd.DataFrame.from_dict(expected_infection_ids).to_csv("Infections_from_logging.csv")
        with open("DEBUG_filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    return [expected_bite_ids, expected_infection_ids]


def create_report_file(report_under_test, logging_data, output_folder, param_obj, report_name, debug):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj['Config_Name']}: \n")
        with open("custom_reports.json") as custom_reports:
            fpg_report_config = json.load(custom_reports)["Reports"][1]
            if fpg_report_config["class"] != "ReportFpgOutputForObservationalModel":
                fpg_report_config = json.load(custom_reports)["Reports"][0]
        fpg_report_start_day = fpg_report_config["Start_Day"]
        fpg_report_sampling_period = fpg_report_config["Sampling_Period"]
        report_df = pd.read_csv(os.path.join(output_folder, report_under_test))
        report_data = {}
        for column in report_columns:
            report_data[column] = report_df[column]
        data_length = len(report_data[sim_day])
        possible_report_days = [fpg_report_start_day + x * fpg_report_sampling_period for x in range(0, 100)]
        for row in range(data_length):
            #  verify that year/month make sense based on day
            current_day = report_data[sim_day][row]
            expected_month = int(current_day / 30) % 12
            expected_year = int(current_day / 365)
            if current_day not in possible_report_days:
                success = False
                outfile.write(f"BAD: At {inf_index} row {row}, for {sim_day} we have {current_day}, "
                              f"which is not in report day possible for Start_Day {fpg_report_start_day} and"
                              f"Sampling_Period {fpg_report_sampling_period}.\n")
            if row != report_data[inf_index][row]:
                success = False
                outfile.write(f"BAD: At {inf_index} row {row}, we expected index {row}, but got "
                              f"{report_data[inf_index][row]}.\n")
            if expected_month != report_data[month][row]:
                success = False
                outfile.write(f"BAD: At {inf_index} {row}, day is {current_day} and expected month was "
                              f"{expected_month}, but is {report_data[month][row]}.\n")
            if expected_year != report_data[year][row]:
                success = False
                outfile.write(f"BAD: At infIndex {row}, day is {current_day} and expected year was "
                              f"{expected_year}, but is {report_data[year][row]}.\n")
        if success:
            outfile.write(f"GOOD: Columns {inf_index}, {sim_day}, {month}, {year} are all correct.")

        # verify bite and infection data is good

        bites_dict = logging_data[0]
        infections_dict = logging_data[1]
        little_success = True
        for row in range(data_length):
            day = report_data[sim_day][row]
            person = report_data[individual_id][row]
            report_data[infection_ids] = report_df[infection_ids].apply(lambda x: ast.literal_eval(x))
            report_data[bite_ids] = report_df[bite_ids].apply(lambda x: ast.literal_eval(x))
            list_infection_id = report_data[infection_ids][row]
            list_bite_id = report_data[bite_ids][row]
            # list_infection_id = [int(i) for i in report_data[infection_ids][row].split(',')]
            # list_bite_id = [int(i) for i in report_data[bite_ids][row].split(',')]
            if bites_dict[person][day] != list_bite_id:
                little_success = False
                outfile.write(f"BAD: At infIndex {row}, day {day} , person {person} - we expected to see "
                              f"bites {bites_dict[person][day]}, but we saw {list_bite_id}.\n")
            if infections_dict[person][day] != list_infection_id:
                little_success = False
                outfile.write(f"BAD: At infIndex {row}, day {day} , person {person} - we expected to see "
                              f"bites {infections_dict[person][day]}, but we saw {list_infection_id}.\n")
        if little_success:
            outfile.write(f"GOOD: {infection_ids} and {bite_ids} columns have correct data for the entire "
                          f"report.\n")
        else:
            success = False
        outfile.write(dtk_sft.format_success_msg(success))


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json", inset_chart="InsetChart.json",
                report_name=dtk_sft.sft_output_filename,
                report_under_test="infIndexRecursive-genomes-df.csv",
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
    logging_data = parse_stdout_file()
    create_report_file(report_under_test, logging_data, output_folder, param_obj, report_name, debug)


if __name__ == "__main__":
    application()
