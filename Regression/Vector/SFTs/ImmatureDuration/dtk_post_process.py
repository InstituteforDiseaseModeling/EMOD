import json
import dtk_test.dtk_sft as dtk_sft
import math

"""
Immature Duration:
This test verifies that immature queues progress as expected. 1/Immature_Duration is the rate of progression, which is
added every time step. We are verifying that each Immature_Queue_Update the queue progress is increased by 1/Immature_Duration
and that every time step there are less or same number of vectors, they don't change their sex and are not present after
their progress reaches 1.0

Please note this doesn't work for TRACK_ALL_VECTORS as the test.txt becomes really really big really really fast

Sweep suggestions:
Run_Number,  Immature_Duration, Adult_Life_Expectancy, Enable_Vector_Aging
"""

KEY_CONFIG_NAME = "Config_Name"
VECTOR_SPECIES_PARAMS = "Vector_Species_Params"
IMMATURE_DURATION = "Immature_Duration"
SIMULATION_DURATION = "Simulation_Duration"


def load_emod_parameters(config_filename="config.json", debug=False):
    """reads config file and populates params_obj
    :param config_filename: name of config file (config.json)
    :param debug: write out parsed data on true
    :returns param_obj:     dictionary with KEY_CONFIG_NAME, etc., keys (e.g.)
    """
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {}
    param_obj[KEY_CONFIG_NAME] = cdj[KEY_CONFIG_NAME]
    param_obj[IMMATURE_DURATION] = float(cdj[VECTOR_SPECIES_PARAMS][0][IMMATURE_DURATION])
    param_obj[SIMULATION_DURATION] = int(cdj[SIMULATION_DURATION])

    if debug:
        with open("DEBUG_param_object.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj


def parse_output_file(param_obj, output_filename="test.txt", debug=False):
    """
    creates a dictionary to store filtered information for each time step
    :param param_obj: parameters passed in as an object, created in load_emod_parameters()
    :param output_filename: file to parse (test.txt)
    :param debug: write out parsed data on true
    :return:                output_dict
    """
    filtered_lines = []
    time = 0
    immature_duration = param_obj[IMMATURE_DURATION]
    simulation_duration = param_obj[SIMULATION_DURATION]
    immature_queues_dictionary = {}
    with open(output_filename) as logfile:
        for line in logfile:
            if "Time:" in line:
                new_time = int(float(dtk_sft.get_val("Time: ", line)))
                if new_time < time:
                    break
                time = new_time
                filtered_lines.append(line)
            elif "Update_Immature_Queue" in line:
                filtered_lines.append(line)
                cohort_id = int(dtk_sft.get_val("cohort_id=", line))
                progress = float(dtk_sft.get_val("progress=", line))
                sex = int(dtk_sft.get_val("sex=", line))
                population = int(dtk_sft.get_val("population=", line).rstrip("."))
                # not adding new cohorts for the last steps of the sim because there won't be enough
                print("{}\n".format(cohort_id))
                if cohort_id not in immature_queues_dictionary and time <= (
                        simulation_duration - (immature_duration + 1)):
                    immature_queues_dictionary[cohort_id] = [[progress, population, sex]]
                elif cohort_id in immature_queues_dictionary:
                    immature_queues_dictionary[cohort_id].append([progress, population, sex])

    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    return immature_queues_dictionary


def create_report_file(param_obj, std_out_dict, report_name, debug):
    with open(report_name, "w") as outfile:
        success_details = []
        failure_details = []
        immature_duration = param_obj[IMMATURE_DURATION]
        immature_progress = 1 / float(immature_duration)
        config_name = param_obj[KEY_CONFIG_NAME]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        if not std_out_dict:
            success = False
            outfile.write(dtk_sft.sft_no_test_data)
        else:
            for key, value in std_out_dict.items():
                if len(value) != math.ceil(immature_duration):
                    if value[-1][1] != 0:  # if last population is 0, then it makes sense that it is shorter
                        success = False
                        failure_details.append("BAD: There are {} entries for cohort {}, but expected {}."
                                               "\n".format(len(value), key, immature_duration))
                    else:
                        success_details.append("GOOD: There are {} entries for cohort {}, and expected {}."
                                               "\n".format(len(value), key, immature_duration))
                else:
                    epsilon = 0.000005  # big immature_duration (ex: 501) have rounding errors in dtk
                    # setting up the initial values from the first occurrence of the cohort id
                    if abs(value[0][0] - immature_progress) > epsilon:  # stdout is after the increment
                        success = False
                        failure_details.append("BAD: Cohort {}, {} progress is expected, got {}."
                                               "\n".format(key, round(immature_progress, 6), value[0][0]))
                    else:
                        success_details.append("GOOD: Cohort {}, {} progress is expected, got {}."
                                               "\n".format(key, round(immature_progress, 6), value[0][0]))
                    population = value[0][1]
                    sex = value[0][2]
                    if value[-1][0] > 1:
                        success = False
                        failure_details.append("BAD: Cohort {} is aged {}, but should be 1 or less."
                                               "\n".format(key, value[-1][0]))
                    else:
                        success_details.append("GOOD: Cohort {} is aged {}, and should be 1 or less."
                                               "\n".format(key, value[-1][0]))
                    for day in range(1, len(value)):
                        today_progress = immature_progress * (day + 1)
                        if today_progress > 1:
                            today_progress = 1
                        if abs(value[day][0] - today_progress) > epsilon:  # stdout is after the increment
                            success = False
                            failure_details.append("BAD: Cohort {}, {} progress is expected, got {}, a difference of"
                                                   " {}.\n".format(key, today_progress, value[day][0],
                                                                   abs(value[day][0] - today_progress)))
                        else:
                            success_details.append(
                                "GOOD: Cohort {}, {} progress is expected, got {}, a difference of {}."
                                "\n".format(key, today_progress, value[day][0],
                                            abs(value[day][0] - today_progress)))
                        if value[day][2] != sex:
                            success = False
                            failure_details.append("BAD: Cohort {}, sex changed from {} to {} ."
                                                   "\n".format(key, sex, value[day][2]))
                        else:
                            success_details.append("GOOD: Cohort {}, sex did not change."
                                                   "\n".format(key, sex, value[day][2]))
                        if value[day][1] > population:
                            success = False
                            failure_details.append("BAD: Cohort {}, expecting population to drop or remain the same,"
                                                   " but it rose: previous {}, current {}."
                                                   "\n".format(key, population, value[day][1]))
                        else:
                            success_details.append("GOOD: Cohort {}, expecting population to drop or remain the same,"
                                                   " and: previous {}, current {}."
                                                   "\n".format(key, population, value[day][1]))
                            population = value[day][1]

        if debug:
            with open("success_details_debug.txt", "w") as details:
                for line in success_details:
                    details.writelines(line)
            with open("failure_details_debug.txt", "w") as details:
                for line in failure_details:
                    details.writelines(line)

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
    param_obj = load_emod_parameters(config_filename, debug)
    std_out_dict = parse_output_file(param_obj, stdout_filename, debug)
    create_report_file(param_obj, std_out_dict, report_name, debug)


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
