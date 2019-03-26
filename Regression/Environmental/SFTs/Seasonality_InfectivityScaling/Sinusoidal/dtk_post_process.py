#!/usr/bin/python

if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as sft
import math
import pandas as pd
from dtk_test.dtk_General_Support import ConfigKeys, DemographicsKeys, InsetKeys
import dtk_test.dtk_General_Support as General_Support
import dtk_test.dtk_HINT_Support as HINT_Support
import dtk_test.dtk_InfectivityScalingSinusoidal_Support as Sinusoidal_Support

"""

This SFT is testing the environmental seasonality with "Infectivity_Scale_Type": "SINUSOIDAL_FUNCTION_OF_TIME"(
environmental feature 2.4 Seasonality and Seasonal forcing). The  seasonality attenuation is only applies to 
environmental contagion pool and infectivity scaling is applies to only contact contagion pool.

Test data is loaded from StdOut.txt and PropertyReportEnvironmental.json.

Suggested sweep parameters: Environmental_Peak_Start, Environmental_Ramp_Down_Duration, Environmental_Cutoff_Days
                            Environmental_Ramp_Up_Duration, Infectivity_Sinusoidal_Forcing_Amplitude,
                            Infectivity_Sinusoidal_Forcing_Phase
 

"""

config_keys = [ConfigKeys.Config_Name, ConfigKeys.Simulation_Timestep,
               ConfigKeys.Simulation_Duration, ConfigKeys.Base_Infectivity,
               ConfigKeys.Run_Number,
               ConfigKeys.Environmental_Cutoff_Days,
               ConfigKeys.Environmental_Peak_Start,
               ConfigKeys.Environmental_Ramp_Down_Duration,
               ConfigKeys.Environmental_Ramp_Up_Duration,
               ConfigKeys.Demographics_Filenames,
               ConfigKeys.Infectivity_Sinusoidal_Forcing_Amplitude,
               ConfigKeys.Infectivity_Sinusoidal_Forcing_Phase]

channels = [InsetKeys.ChannelsKeys.Infected,
            "New Infections By Route (ENVIRONMENT)",
            "New Infections By Route (CONTACT)",
            "Contagion (Environment)",
            "Contagion (Contact)",
            InsetKeys.ChannelsKeys.Statistical_Population]

matches = ["Update(): Time: ",
           "amplification calculated as "
           ]

routes = [
    "environmental",
    "contact"
]


class Stdout:
    amplification = "amplification"
    day_of_year = "day of year="
    stat_pop = "StatPop: "
    infected = "Infected: "
    start = "start="
    end = "end="
    ramp_up = "ramp_up="
    ramp_down = "ramp_down="
    cutoff = "cutoff="


def parse_stdout_file(stdout_filename="StdOut.txt", simulation_timestep=1, debug=False):
    """
    creates a dictionary to store filtered information for each time step
    :param output_filename: file to parse (StdOut.txt)
    :return:                stdout_df
    """
    filtered_lines = []
    with open(stdout_filename) as logfile:
        for line in logfile:
            if sft.has_match(line,matches):
                filtered_lines.append(line)
    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    # initialize variables
    time_step = index = 0
    stdout_df = pd.DataFrame(columns=[ConfigKeys.Simulation_Timestep, Stdout.amplification, Stdout.day_of_year,
                                      Stdout.infected])
    stdout_df.index.name = 'index'
    amplification = day_of_year = infected = None
    for line in filtered_lines:
        if matches[0] in line:
            stdout_df.loc[index] = [time_step, amplification, day_of_year, infected]
            index += 1
            time_step += simulation_timestep
            infected = int(sft.get_val(Stdout.infected, line))

        elif matches[1] in line:
            day_of_year = int(sft.get_val(Stdout.day_of_year, line))
            amplification = float(sft.get_val(matches[1], line))

    if debug:
        res_path = r'./DEBUG_filtered_from_logging.csv'
        stdout_df.to_csv(res_path)
    return stdout_df


def create_report_file(param_obj, stdout_df, property_obj, property_df, report_name, debug):
    with open(report_name, "w") as outfile:
        for name, param in param_obj.items():
            outfile.write("{0} = {1}\n".format(name, param))
        success = True
        start_day = param_obj[ConfigKeys.Environmental_Peak_Start]
        ramp_up = param_obj[ConfigKeys.Environmental_Ramp_Up_Duration]
        ramp_down = param_obj[ConfigKeys.Environmental_Ramp_Down_Duration]
        cut_off = param_obj[ConfigKeys.Environmental_Cutoff_Days]
        duration = param_obj[ConfigKeys.Simulation_Duration]

        peak_duration = sft.DAYS_IN_YEAR - ramp_up - ramp_down - cut_off
        peak_starttime = start_day % sft.DAYS_IN_YEAR
        peak_endtime = peak_starttime + peak_duration
        cutoff_starttime = peak_starttime + peak_duration + ramp_down
        cutoff_endtime = peak_starttime + peak_duration + ramp_down + cut_off

        amp = []
        expected_amp = []
        expected_contagion_e_list = []
        expected_contagion_c_list = []
        # adjust the times so that the ramp up starts at time 0, which means the cut off ends at time 0 too.
        adjust_time = peak_starttime - ramp_up
        peak_starttime -= adjust_time
        peak_endtime -= adjust_time
        cutoff_starttime -= adjust_time
        cutoff_endtime -= adjust_time
        cutoff_endtime %= sft.DAYS_IN_YEAR

        contagion_channels_e = [c for c in property_df.columns if channels[3] in c]
        contagion_channels_c = [c for c in property_df.columns if channels[4] in c]
        property_df[channels[3]] = property_df[contagion_channels_e].sum(axis=1)
        property_df[channels[4]] = property_df[contagion_channels_c].sum(axis=1)

        for t in range(1, duration):
            amplification = stdout_df[Stdout.amplification].iloc[t]
            day_in_year = stdout_df[Stdout.day_of_year].iloc[t]
            if day_in_year != t % sft.DAYS_IN_YEAR:
                success = False
                outfile.write("BAD: at time step {0}, day_in_year is {1}, it should be {2}.\n".format(t, day_in_year,
                                                                                                      t % sft.DAYS_IN_YEAR))
                day_in_year = t % sft.DAYS_IN_YEAR
            day_in_year -= adjust_time
            day_in_year %= sft.DAYS_IN_YEAR
            # Environment Ramp Up
            if cutoff_endtime < day_in_year < peak_starttime:
                expected_amplification = day_in_year / ramp_up
            # Environment peak
            elif peak_starttime <= day_in_year <= peak_endtime:
                expected_amplification = 1
            # Environment Ramp Down
            elif peak_endtime < day_in_year < cutoff_starttime:
                expected_amplification = (cutoff_starttime - day_in_year) / ramp_down
            # Environment cutoff
            else:
                expected_amplification = 0
            if not math.fabs(amplification - expected_amplification) <= 5e-2 * expected_amplification:
                success = False
                outfile.write(
                    "BAD: at time {0}, day of year = {1}, the environmental amplification is {2}, expected {3}.\n".format(
                        t, t % sft.DAYS_IN_YEAR, amplification, expected_amplification))
            amp.append(amplification)
            expected_amp.append(expected_amplification)

            # Sinusoidal scaling
            sinusoidal_multiplier = Sinusoidal_Support.calculate_infectiousness(
                infected_pop=1, index=t, simulation_timestep=param_obj[ConfigKeys.Simulation_Timestep],
                phase=param_obj[ConfigKeys.Infectivity_Sinusoidal_Forcing_Phase],
                base_infectivity=1,
                amplitude=param_obj[ConfigKeys.Infectivity_Sinusoidal_Forcing_Amplitude], debug=debug)
            expected_contagion_e = param_obj[ConfigKeys.Base_Infectivity] * \
                                   expected_amplification * \
                                  (len(property_obj[DemographicsKeys.PropertyKeys.Values]) ** 2)

            # infectivity scaling should only applies to contact route.
            # expected_contagion_e *= sinusoidal_multiplier

            expected_contagion_c = sinusoidal_multiplier * param_obj[ConfigKeys.Base_Infectivity] * \
                                   len(property_obj[DemographicsKeys.PropertyKeys.Values])
            actual_contagion_e = property_df[channels[3]][t]
            actual_contagion_c = property_df[channels[4]][t]
            for expected_contagion, actual_contagion, column_name in [
                (expected_contagion_e, actual_contagion_e, channels[3]),
                (expected_contagion_c, actual_contagion_c, channels[4])]:
                if not math.fabs(actual_contagion - expected_contagion) <= 5e-2 * expected_contagion:
                    success = False
                    outfile.write(
                        "BAD: at time {0}, day of year = {1}, the {2} is {3}, expected {4}.\n".format(
                            t, t % sft.DAYS_IN_YEAR, column_name, actual_contagion, expected_contagion))
            expected_contagion_e_list.append(expected_contagion_e)
            expected_contagion_c_list.append(expected_contagion_c)

        sft.plot_data(amp, expected_amp,
                      label1="Actual amplification",
                      label2="Expected amplification",
                      title="Seasonality", xlabel="Day",
                      ylabel="Environmental amplification",
                      category='seasonal_attenuation', overlap=True, alpha=0.5)

        sft.plot_data(property_df[channels[3]][1:], expected_contagion_e_list[1:],
                      label1="Actual",
                      label2="Expected",
                      title="Environmental Contagion\nSeasonality_Sinusoidal", xlabel="Day",
                      ylabel="Environmental Contagion",
                      category='Environmental_Contagion', overlap=True, alpha=0.5)

        sft.plot_data(property_df[channels[4]][1:], expected_contagion_c_list[1:],
                      label1="Actual",
                      label2="Expected",
                      title="Contact Contagion\nSeasonality_Sinusoidal", xlabel="Day",
                      ylabel="Contact Contagion",
                      category='Contact_Contagion', overlap=True, alpha=0.5)

        outfile.write(sft.format_success_msg(success))
        if debug:
            print(sft.format_success_msg(success))
        return success


def application( output_folder="output", stdout_filename="test.txt", config_filename="config.json",
                 property_report_name="PropertyReportEnvironmental.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename+ "\n")
        print("property_report_name: " + property_report_name+ "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done(stdout_filename)

    config_obj = General_Support.load_config_parameters(config_filename, config_keys, debug)
    stdout_df = parse_stdout_file(stdout_filename, config_obj[ConfigKeys.Simulation_Timestep], debug)

    demo_path = "Assets" if stdout_filename == "StdOut.txt" else ""
    property_list = HINT_Support.load_demo_mr_overlay_file(
        config_obj[ConfigKeys.Demographics_Filenames][-1], demo_path, debug)
    property_keys = HINT_Support.build_channel_string_for_property(property_list, channels, debug)
    property_df = HINT_Support.parse_property_report_json(property_report_name, output_folder, property_keys, debug)
    property_obj = property_list[0]  # this test only has one property object

    create_report_file(config_obj, stdout_df, property_obj, property_df, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-p', '--propertyreport', default="PropertyReportEnvironmental.json",
                        help="Property report to load (PropertyReportEnvironmental.json)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                property_report_name=args.propertyreport,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

