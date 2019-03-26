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
from dtk_test.dtk_General_Support import ConfigKeys
import dtk_test.dtk_General_Support as General_Support

"""

This SFT is testing environmental feature 2.4 Seasonality:

"Seasonal attenuation. You can specify a trapezoidal-shaped infectivity multiplier on the environmental contagion. 
Seasonality and dose for environmental transmission: Exposure to the environmental reservoir varies seasonally, 
and is mediated by the parameter seasonality_factor (where fExposure = fEnvironment * seasonality_factor. Note that 
seasonality_factor represents the current value of the seasonality infectivity multiplier under Seasonal Forcing). 
The seasonality can start or end at any time during the year, with a periodicity of 365 days. The pattern is anchored 
at the value Environmental_Peak_Start, which indicates the day of year peak seasonality starts. At the highest point 
in the season, individuals are exposed to 100% of the environmental reservoir (seasonality_factor = 1). At low season, 
there is no dose through the environment and transmission is dependent on contact transmission (seasonality_factor = 0).

Seasonality follows a trapezoidal shape with the following parameters:

    Environmental_Peak_Start (t0 on Fig. 1)
    Environmental_Ramp_Down_Duration (t1 to t2 on Fig. 1)
    Environmental_Cutoff_Days (t2 to t3 on Fig. 1)
    Environmental_Ramp_Up_Duration (t3 to t4 on Fig. 1)
Note that the sum total of all durations will be < 365.

Specific days of the year act as transition points:

    Seasonal_factor max. Seasonal_factor remains at 1 (from t0 to t1 on Fig. 1)
    
        Seasonal_factor remains at 1 for the number of days determined by: 
        #Days = 365 – Environmental_Ramp_Down_Duration - Environmental_Cutoff_Days – Environmental_Ramp_Up_Duration
    
    Seasonal_factor decrease. Seasonal_factor linearly decreases with each day from 1 to 0 (from t1 to t2 on Fig. 1).
    
        A linear decrease occurs between seasonal_factor 1 and 0 with slope determined by Environmental_Ramp_Down_Duration.
    
    Seasonal_factor min. Seasonal_factor remains at 0 (from t2 to t3 on Fig. 1).
    
        Seasonal_factor remains at 0 for the duration of Environmental_Cutoff_Days.
    
    Seasonal_factor increase. Seasonal_factor linearly increases with each day from 0 - 1 (from t3 to t4 on Fig. 1).

        A linear increase occurs between seasonal_factor 0 and 1 with slope determined by Environmental_Ramp_Up_Duration.
"
Test data is loaded from StdOut.txt.

Suggested sweep parameters: Environmental_Peak_Start, Environmental_Ramp_Down_Duration, Environmental_Cutoff_Days
                            Environmental_Ramp_Up_Duration


"""

config_keys = [ConfigKeys.Config_Name, ConfigKeys.Simulation_Timestep,
               ConfigKeys.Simulation_Duration, ConfigKeys.Base_Infectivity,
               ConfigKeys.Run_Number,
               ConfigKeys.Environmental_Cutoff_Days,
               ConfigKeys.Environmental_Peak_Start,
               ConfigKeys.Environmental_Ramp_Down_Duration,
               ConfigKeys.Environmental_Ramp_Up_Duration]

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


def create_report_file(param_obj, stdout_df, report_name, debug):
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
        # adjust peak start time to day in year
        peak_starttime = start_day % sft.DAYS_IN_YEAR
        peak_endtime = peak_starttime + peak_duration
        cutoff_starttime = peak_starttime + peak_duration + ramp_down
        cutoff_endtime = peak_starttime + peak_duration + ramp_down + cut_off

        amp = []
        expected_amp = []
        # adjust the times so that the ramp up starts at time 0, which means the cut off ends at time 0 too.
        adjust_time = peak_starttime - ramp_up
        peak_starttime -= adjust_time
        peak_endtime -= adjust_time
        cutoff_starttime -= adjust_time
        cutoff_endtime -= adjust_time
        cutoff_endtime %= sft.DAYS_IN_YEAR

        for t in range(1, duration):
            amplification = stdout_df[Stdout.amplification].iloc[t]
            day_in_year = stdout_df[Stdout.day_of_year].iloc[t]
            if day_in_year != t % sft.DAYS_IN_YEAR:
                success = False
                outfile.write("BAD: at time step {0}, day_in_year is {1}, it should be {2}.\n".format(
                    t, day_in_year, t % sft.DAYS_IN_YEAR))
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
        sft.plot_data(amp, expected_amp,
                          label1="Actual amplification",
                          label2="Expected amplification",
                          title="Seasonality", xlabel="Day",
                          ylabel="Environmental amplification",
                          category='Environmental_amplification', overlap=True, alpha=0.5)
        outfile.write(sft.format_success_msg(success))
        if debug:
            print(sft.format_success_msg(success))
        return success


def application( output_folder="output", stdout_filename="test.txt", config_filename="config.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )

    sft.wait_for_done(stdout_filename)

    config_obj = General_Support.load_config_parameters(config_filename, config_keys, debug)
    stdout_df = parse_stdout_file(stdout_filename, config_obj[ConfigKeys.Simulation_Timestep], debug)
    create_report_file(config_obj, stdout_df, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

