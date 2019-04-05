import json
import numpy as np
import json
import os.path as path
import dtk_test.dtk_sft as sft
import re
import pandas as pd
import os
import math

matches = ["Update(): Time: ",
           "Infected: ",
           "total contagion = ",
           "StatPop: "]

KEY_TOTAL_TIMESTEPS = "Simulation_Duration"
KEY_SIMULATION_TIMESTEP = "Simulation_Timestep"
KEY_BASE_INFECTIVITY = "Base_Infectivity"
KEY_AMPLITUDE = "Infectivity_Sinusoidal_Forcing_Amplitude"
KEY_PHASE = "Infectivity_Sinusoidal_Forcing_Phase"
KEY_INFECTED = "infected"
KEY_INFECTIOUSNESS = "infectiousness"
KEY_NEW_INFECTIONS= "New Infections"
KEY_CUMULATIVE_INFECTIONS = "Cumulative Infections"
KEY_STAT_POP = "Stat Pop"

class Constants:
    DAYS_IN_YEAR = 365

"""
The purpose of this ENUM is to modulate the infectivity of the disease,
using a sine function with a period of 1 year.  The epidemiological relevance
is that many diseases are observed to have a high season and a low season,
and often it is not clear whether this is due to social behavior, climate,
or other factors that vary with an annual period.  In the literature, changing
disease infectivity from a constant to a sine function is a common way to handle this.

The implementation is intended to reproduce the following behavior:

beta(t)=beta_0 [1+A* sin((2* pi*(t-t_0))/365)]

Where beta_0= Base_Infectivity, A = Infectivity_Sinusoidal_Forcing_Amplitude,
and t0 = Infectivity_Sinusoidal_Forcing_Phase, and t is time in days, within the simulation.

A note - it would appear that time is assumed to be in days, and it should be checked
whether changing the simulation timestep to multiple days or fractional days changes the
frequency of this sine wave, which should stay at one year.
"""
# region post_process support
def load_emod_parameters(config_filename="config.json"):
    """
    reads config file and populates params_obj
    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_TOTAL_TIMESTEPS, etc., keys (e.g.)
    """
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {}
    param_obj[KEY_TOTAL_TIMESTEPS] = cdj[KEY_TOTAL_TIMESTEPS]
    param_obj[KEY_BASE_INFECTIVITY] = cdj[KEY_BASE_INFECTIVITY]
    param_obj[KEY_AMPLITUDE] = cdj[KEY_AMPLITUDE]
    param_obj[KEY_PHASE] = cdj[KEY_PHASE]
    param_obj[KEY_SIMULATION_TIMESTEP] = cdj[KEY_SIMULATION_TIMESTEP]
    return param_obj

def parse_output_file(output_filename="test.txt", simulation_timestep = 1, debug=False):
    """
    creates a dataframe of time step and infected,  infectiouness and stat populations
    :param output_filename: file to parse (test.txt)
    :return:                output_df:  # of infected population and infectiousness per person at each time step
    """
    filtered_lines = []
    with open(output_filename) as logfile:
        for line in logfile:
            if sft.has_match(line, matches):
                filtered_lines.append(line)
    if debug:
        with open("filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    # initialize variables
    time_step = 0
    infectiousness = 0
    infected = 0
    statpop = 0
    output_df = pd.DataFrame( columns = [KEY_SIMULATION_TIMESTEP, KEY_INFECTED, KEY_INFECTIOUSNESS, KEY_STAT_POP])
    output_df.index.name = "index"
    index = 0
    for line in filtered_lines:
        if matches[0] in line:
            infected = sft.get_val(matches[1], line)
            statpop = sft.get_val(matches[3], line)
            output_df.loc[index] = [time_step, infected, infectiousness, statpop]
            index += 1
            time_step += simulation_timestep
            infectiousness = 0
            continue
        if matches[2] in line:
            infectiousness = sft.get_val(matches[2], line)
            continue
    res_path = r'./infected_vs_infectiousness.csv'
    if not os.path.exists(os.path.dirname(res_path)):
        os.makedirs(os.path.dirname(res_path))
    output_df.to_csv(res_path)
    return output_df

def parse_json_report(output_folder="output", insetchart_name="InsetChart.json", debug=False):
    """
    creates report_df dataframe with "new_infections" and "cumulative_infections" columns
    :param insetchart_name: InsetChart.json file with location (output/InsetChart.json)
    :returns: report_df dataframe
    """
    insetchart_path = path.join(output_folder, insetchart_name)
    with open(insetchart_path) as infile:
        icj = json.load(infile)["Channels"]

    new_infections = icj[KEY_NEW_INFECTIONS]["Data"]
    cumulative_infections = np.cumsum(new_infections)

    report_dict = {KEY_NEW_INFECTIONS: new_infections, KEY_CUMULATIVE_INFECTIONS: cumulative_infections}
    report_df = pd.DataFrame(report_dict)
    report_df.index.name = "index"

    if debug:
        res_path = r'./new_infections_vs_cumulative_infections.csv'
        if not os.path.exists(os.path.dirname(res_path)):
            os.makedirs(os.path.dirname(res_path))
        report_df.to_csv(res_path)
    return report_df

def calculate_infectiousness(infected_pop, index, simulation_timestep, phase, base_infectivity, amplitude, debug = False):
    """
    calculate infectiousness at each time step
    :param infected_pop:
    :param index:
    :param simulation_timestep:
    :param start_time:
    :param end_time:
    :param base_infectivity:
    :param amplitude:
    :param debug:
    :return:
    """
    timestep = index * simulation_timestep
    day_in_year = float(timestep % Constants.DAYS_IN_YEAR)
    infectiousness = base_infectivity * ( 1.0 + amplitude * math.sin(2.0 * math.pi * (day_in_year - phase) / Constants.DAYS_IN_YEAR))
    infectiousness *= infected_pop
    if debug:
        print ("calculated infectiousness is {0} at time step {1} with {2} people infected.\n".format(infectiousness, timestep, infected_pop))
    return infectiousness

def create_report_file(param_obj, output_df, report_df, report_name, debug):
    total_timesteps = int(param_obj[KEY_TOTAL_TIMESTEPS])
    simulation_timestep = float(param_obj[KEY_SIMULATION_TIMESTEP])
    base_infectivity = float(param_obj[KEY_BASE_INFECTIVITY])
    amplitude = float(param_obj[KEY_AMPLITUDE])
    phase = float(param_obj[KEY_PHASE])
    infected = output_df[KEY_INFECTED]
    infectiousness = output_df[KEY_INFECTIOUSNESS]
    statpop = output_df[KEY_STAT_POP]
    new_infections = report_df[KEY_NEW_INFECTIONS]
    if debug:
        sft.plot_data(new_infections, label1="new infections", label2="NA",
                                   title="Phase: {0} day, amplitude: {1}, base_infectivity: {2}".format(phase, amplitude, base_infectivity),
                                   xlabel="Time_Step_{0}_Days".format(simulation_timestep), ylabel=None,
                                   category='New_infections',
                                   show=True, line = True)
    with open(report_name, "w") as outfile:
        expected_infectiousness = []
        for index in range(len(infected)):
            infected_pop = int(infected[index])
            expected_infectiousness.append(calculate_infectiousness(infected_pop, index, simulation_timestep,
                                                                    phase,
                                                                    base_infectivity, amplitude, debug))
        success = True
        actual_infectiousness_all = []
        calc_infectiousness_all = []
        for index in range(len(infectiousness)):
            timestep = index * simulation_timestep
            actual_infectiousness = float(infectiousness[index])
            calc_infectiousness = expected_infectiousness[index] / float(statpop[index])
            actual_infectiousness_all.append(actual_infectiousness)
            calc_infectiousness_all.append(calc_infectiousness)
            tolerance = 0 if calc_infectiousness == 0 else 5e-2 * calc_infectiousness
            if  math.fabs(actual_infectiousness - calc_infectiousness) > tolerance:
                success = False
                outfile.write("BAD: actual infectiousness at time step {0} is {1}, expected {2}.\n".format(timestep, actual_infectiousness, calc_infectiousness))
        outfile.write(sft.format_success_msg(success))
    sft.plot_data(actual_infectiousness_all, calc_infectiousness_all,
                               label1="actual infectiousness", label2="calc infectiousness",
                               title="Phase: {0} day, amplitude: {1}, base_infectivity: {2}".format(phase, amplitude, base_infectivity),
                               xlabel="Time_Step_{0}_Days".format(simulation_timestep), ylabel="Infectiousness",
                               category='Infectiousness',
                               show=True, line = True)
    return success
# endregion
