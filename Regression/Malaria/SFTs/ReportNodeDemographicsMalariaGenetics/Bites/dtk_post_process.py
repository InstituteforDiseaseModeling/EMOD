import os
import dtk_test.dtk_malaria_support as mal_sup
import dtk_test.dtk_sft as dtk_sft
import pandas as pd
import collections

"""
In this test we will "NumBitesDeliveredCurrentInfections" "NumBitesDeliveredMultipleCurrentInfections"

NumBitesDeliveredCurrentInfections - The number of bites that delivered the CurrentNumInfections. This includes bites 
that delivered multiple infections. This is not the number of infectious bites made this time step but those that were 
required to deliver the current number of infection objects in the population. This should mimic the bite data in the
 ReportFpgOutputForObservationalModel report.
 
NumBitesDeliveredMultipleCurrentInfections - The number of bites that delivered more than one of the 
CurrentNumInfections. A bite that delivered more than one infection object is on one person and due to the mosquito 
having sporozoites with different genomes. One should note that this is not an accurate statistic of multi-infection 
bites since when an infections clear, a multi-bite infection can become a single bite infection. However, this will 
mimic the data in the ReportFpgOutputForObservationalModel report.


"""

bites_that_delivered_current_infections = "NumBitesDeliveredCurrentInfections"
bites_that_delivered_multiple_current_infections = "NumBitesDeliveredMultipleCurrentInfections"
infections_cleared = "NumInfectionsCleared"

report_columns = [bites_that_delivered_current_infections, bites_that_delivered_multiple_current_infections,
                  infections_cleared]


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
        bite_ids_today = []
        bites_per_day = []
        infection_ids_today = []
        infections_bites_ids_per_day = []
        multi_infection_bites_today = []
        multi_infection_bites_per_day = []
        infections_cleared_today = []
        infections_cleared_per_day = []
        for line in log_file:
            if "Update(): Time:" in line:
                filtered_lines.append(line)
                multi_infection_bites_per_day.append(multi_infection_bites_today)
                bites_per_day.append(len(bite_ids_today))
                infections_bites_ids_per_day.append(infection_ids_today)
                infections_cleared_per_day.append(infections_cleared_today)
                bite_ids_today = []
                infection_ids_today = []
                multi_infection_bites_today = []
                infections_cleared_today = []
            elif "got bite" in line:
                filtered_lines.append(line)
                bite_id = int(dtk_sft.get_val("got bite ", line))
                infection_id = int(dtk_sft.get_val("creating infection ", line))
                if bite_id in bite_ids_today and bite_id not in multi_infection_bites_today:  # and infection_id not in infection_ids_today guaranteed
                    multi_infection_bites_today.append(bite_id)
                    infection_ids_today.append([infection_id, bite_id])
                else:
                    bite_ids_today.append(bite_id)
                    infection_ids_today.append([infection_id, bite_id])
            elif "cleared." in line:
                filtered_lines.append(line)
                infection_id_cleared = int(dtk_sft.get_val("'s infection ", line))
                if infection_id_cleared in infections_cleared_today:
                    raise ValueError("There shouldn't be multiple of the same infections cleared. \n")
                else:
                    infections_cleared_today.append(infection_id_cleared)

        # get a list of all the infection ids present in the day
        # get a list of bites for those ids (throwing out repeating ones)
        expected_bites_that_delivered_current_infections = []
        expected_bites_that_delivered_multiple_current_infections = []
        list_ids_current_infections = []
        for day in range(len(multi_infection_bites_per_day)):
            list_ids_current_infections.extend(infections_bites_ids_per_day[day])
            if list_ids_current_infections:
                if infections_cleared_per_day[day]:
                    remove = []
                    for [inf, bite] in list_ids_current_infections:
                        if inf in infections_cleared_per_day[day]:
                            remove.append([inf, bite])
                    list_ids_current_infections = [inf_bite for inf_bite in list_ids_current_infections if inf_bite not in remove]
                just_bites_today =[bite for [inf, bite] in list_ids_current_infections]
                multibites = [item for item, count in collections.Counter(just_bites_today).items() if count > 1]
                just_bites_today = list(set(just_bites_today))  # removing duplicates
                expected_bites_that_delivered_current_infections.append(len(just_bites_today))
                expected_bites_that_delivered_multiple_current_infections.append(len(multibites))
            else:
                expected_bites_that_delivered_current_infections.append(0)
                expected_bites_that_delivered_multiple_current_infections.append(0)

    bites_dict = {"bites_per_day": bites_per_day,
                  "multi_infection_bites_per_day": multi_infection_bites_per_day,
                  "infections_cleared_per_day": infections_cleared_per_day,
                  "infections_bites_ids_per_day": infections_bites_ids_per_day,
                  "expected_bites_that_delivered_current_infections": expected_bites_that_delivered_current_infections,
                  "expected_bites_that_delivered_multiple_current_infections": expected_bites_that_delivered_multiple_current_infections}

    if debug:
        pd.DataFrame.from_dict(bites_dict).to_csv("Bites_from_logging.csv")
        with open("DEBUG_filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    return bites_dict


def create_report_file(report_under_test, bites_dict, output_folder, param_obj, report_name, debug):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj['Config_Name']}: \n")
        report_df = pd.read_csv(os.path.join(output_folder, report_under_test))
        report_data = {}
        for column in report_columns:
            report_data[column] = report_df[column]
        data_length = len(report_data[bites_that_delivered_multiple_current_infections])
        for day in range(data_length):
            if bites_dict["expected_bites_that_delivered_multiple_current_infections"][day] != \
                    report_data[bites_that_delivered_multiple_current_infections][day]:
                success = False
                outfile.write(f"BAD: On day {day} (0-index), expected "
                              f"{bites_that_delivered_multiple_current_infections} "
                              f"were {bites_dict['expected_bites_that_delivered_multiple_current_infections'][day]}, "
                              f"but {report_under_test} "
                              f"has {report_data[bites_that_delivered_multiple_current_infections][day]}.\n")
            if bites_dict["expected_bites_that_delivered_current_infections"][day] != \
                    report_data[bites_that_delivered_current_infections][day]:
                success = False
                outfile.write(f"BAD: On day {day} (0-index), expected {bites_that_delivered_current_infections} "
                              f"were {bites_dict['expected_bites_that_delivered_current_infections'][day]}, "
                              f"but {report_under_test} "
                              f"has {report_data[bites_that_delivered_current_infections][day]}.\n")
        if success:
            outfile.write(f"GOOD: For all days, {bites_that_delivered_multiple_current_infections} and "
                          f"{bites_that_delivered_multiple_current_infections} values were as expected.\n")
        outfile.write(dtk_sft.format_success_msg(success))


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json", inset_chart="InsetChart.json",
                report_name=dtk_sft.sft_output_filename,
                report_under_test="ReportNodeDemographicsMalariaGenetics.csv",
                debug=False):
    if debug:
        print("output_folder: " + output_folder + "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("report_under_test: " + report_under_test + "\n")
        print("inset_chart: " + inset_chart + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done()
    bites_dict = parse_stdout_file()
    param_obj = mal_sup.load_genetics_parameters(config_filename, debug)
    create_report_file(report_under_test, bites_dict, output_folder, param_obj, report_name, debug)


if __name__ == "__main__":
    application()
