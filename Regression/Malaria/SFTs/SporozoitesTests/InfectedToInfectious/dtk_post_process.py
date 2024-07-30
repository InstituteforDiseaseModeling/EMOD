#!/usr/bin/python

import dtk_test.dtk_sft as sft
import dtk_test.dtk_Vector_Support as veds
import pandas as pd
import numpy as np

"""
Spec: 
The amount of time to progress from oocyst to sporozoite will be the same logic that is used in the 
base Malaria model that determines when a vector goes from infected to infectious.  At the beginning 
of each time step, the amount of progress that is achieved for that day is calculated.  All 
ParasiteCohorts that are in the oocyst stage will use this value to update their timer.  When the timer 
is greater than or equal to 1.0, the ParasiteCohort will transition to sporozoites.  The number of 
oocysts will be converted to sporozoites and the state changed to sporozoites.

The amount of progress to achieve each day is based on the following Arrhenius equation:
    progress = dt * Infected_Arrhenius_1 * exp( -Infected_Arrhenius_2 / temperature )

where temperature is the current temperature in that node in Kelvin.

    Relevant parameters:
    Infected_Arrhenius_1	117000000000
    Infected_Arrhenius_2	8336

In this test, we will be recording the daily calculations and verifying they care correct.

Verify that oocysts progress the same 
"""


def parse_output_file(stdout, debug=False):
    """
    Parses following lines, example:

    Mean gonotrophic cycle duration = 10.54870 days at 15.77 degrees C.

    Args:
        stdout: the logging file to parse
        debug: flag, if true writes out the lines that were used to collect the data

    Returns: data frame

    """
    time = 0
    last_time = 0
    progress_df = pd.DataFrame(columns='time temperature_k progress state id'.split())
    cohort_total_progress_df = pd.DataFrame(columns='time state progress id age'.split())
    with open(stdout) as logfile:
        for line in logfile:
            if "Time: " in line:
                time += 1
            if "infected progress this timestep" in line:
                if last_time != time:
                    temperature_k = float(sft.get_val("temperature ", line)) + 273.15
                    progress = float(sft.get_val("this timestep ", line))
                    state = int(sft.get_val("state ", line))
                    progress_df = progress_df.append({'time': time, 'temperature_k': temperature_k,
                                                      'progress': progress, 'state': state},
                                                     ignore_index=True)
                    last_time = time
            if "state 8 progress" in line or "state 0 progress" in line:
                state = int(sft.get_val("state ", line))
                progress = float(sft.get_val("progress ", line))
                id_ = int(sft.get_val("id ", line))
                age = int(float(sft.get_val("age ", line)))
                # looking only at oocysts over progress = 1, they should show up only once
                # looking only at sporozoite cohorts whose progress is less than 1, there shouldn't be any of those
                if state == 8 and progress > 1:
                    cohort_total_progress_df = cohort_total_progress_df.append({'time': time, 'state': state,
                                                                                'progress': progress, 'id': id_,
                                                                                'age': age},
                                                                               ignore_index=True)
                elif state == 0 and progress < 1:
                    cohort_total_progress_df = cohort_total_progress_df.append({'time': time, 'state': state,
                                                                                'progress': progress, 'id': id_,
                                                                                'age': age},
                                                                               ignore_index=True)

    if debug:
        progress_df.to_csv("progress_df_DEBUG.csv")
        cohort_total_progress_df.to_csv("cohort_total_progress_df.csv")

    return progress_df, cohort_total_progress_df


def create_report_file(param_obj, report_name, progress_df, cohort_total_progress_df, debug=False):
    success = True
    error_epsilon = 5e-2
    with open(report_name, "w") as outfile:
        outfile.write(f"Test name: {str(param_obj[veds.ConfigKeys.CONFIG_NAME])} \n"
                      f"Run number: {str(param_obj[veds.ConfigKeys.RUN_NUMBER])} \n")
        if not len(progress_df):
            success = False
            outfile.write(sft.sft_no_test_data)
            outfile.write("No data in progress_df!\n")
        if not len(cohort_total_progress_df):
            success = False
            outfile.write(sft.sft_no_test_data)
            outfile.write("No data in cohort_total_progress_df!\n")

        progress_df["theoretical"] = None
        for index in progress_df.index:
            temperature_k = float(progress_df.at[index, "temperature_k"])
            theoretical_progress = veds.infected_progress_calc(temperature_k, param_obj)
            progress_df.at[index, "theoretical"] = theoretical_progress
            actual_progress = float(progress_df.at[index, "progress"])
            if abs(actual_progress - theoretical_progress) > error_epsilon * theoretical_progress:
                low_acceptance_bound = theoretical_progress - error_epsilon * theoretical_progress
                high_acceptance_bound = theoretical_progress + error_epsilon * theoretical_progress
                success = False
                outfile.write(f"BAD: At temperature {temperature_k}, infected (oocyst) progress = {actual_progress},\n"
                              f"which is not within acceptance range of ({low_acceptance_bound}, "
                              f"{high_acceptance_bound}).\nExpected {theoretical_progress}.\n")
        if debug:
            progress_df.to_csv("progress_with_theoretic_df_DEBUG.csv")

        # check if we have any sporozoite cohorts that are progressed less than 1, there shouldn't be able
        if cohort_total_progress_df[cohort_total_progress_df["state"] == 0].size:
            success = False
            outfile.write(f"There were sporozoite cohorts with progress of less than 1, that shouldn't happen. \n"
                          f"please see state_0_DEBUG.csv")
            state_0_df = cohort_total_progress_df[cohort_total_progress_df['state'] == 0]
            state_0_df.to_csv("state_0_DEBUG.csv")

        # check if there are oocyst cohorts that are over progree of 1 for more than one timestep
        state_8_df = cohort_total_progress_df[cohort_total_progress_df['state'] == 8]
        if state_8_df['id'].duplicated().any():
            success = False
            outfile.write(f"There were some oocyst cohorts that appear again after reaching progress of 1.\n"
                          f"Check that state_8_df.csv\n")
            state_8_df.to_csv("state_8_df.csv")

        if success:
            outfile.write(f"GOOD: For all temperatures, progress was within {error_epsilon * 100}% of "
                          "theoretical\nand oocysts correctly progress to sporozoites once progress reaches 1.\n")

        outfile.write(sft.format_success_msg(success))


def application(output_folder="output",
                config_filename="config.json",
                stdout="test.txt",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("stdout_filename: " + stdout + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    param_obj = veds.load_emod_parameters(config_filename)
    progress_df, cohort_total_progress_df = parse_output_file(stdout, debug)
    create_report_file(param_obj, report_name, progress_df, cohort_total_progress_df, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)
