#!/usr/bin/python

import json
import dtk_test.dtk_sft as sft
import os


"""
TB_ONLY Exogenous TB Infection Test

"Enable_Superinfection": 1,
"TB_Enable_Exogenous": 1,
"Enable_Coinfection": 1,

The way Exogenous TB is modeled is when there is contagion in the area and a person gets exposed, 
if they are a slow progressor, who is latent and not pending relapse (no one is pending relapse in this sim), then we 
check if they were to get an infection, would they be a fast progressor (if not, it doesn't matter if they get a second
infection or not, so we drop it), if they are going to be a fast progressor, then we check if they will get the 
infection based on the contagion, interventions and susceptibility modifier (immunity). If so, their activation time 
gets recalculated for a fast progressor.

if the person is also HIV+, then CD4_Primary_Map is used to augment the probability of the person being a fast progressor
(tmp_primary * pISTB->GetFastProgressorFraction)

In this test we turned up the infectivity and turned off immunity and there are no interventions reducing the acquisition
of the second infection. This means that virtually everyone who is selected to be a fast progressor (about the fast 
progressor fraction of those that qualify) will also get the exogenous infection. 

We check that the proportion of people getting the exogenous infections is about the same as the fast progressors. 

This test is sensitive to fast_progressor fraction, as the simulation duration will need to be longer for the smaller 
fraction and the lower limit of "latent" people total would need to change to make sure there's enough data for the
binomial test. 

"""
# config parameter
KEY_CONFIG_NAME = "Config_Name"
KEY_FAST_PROGRESSOR_FRACTION = "TB_Fast_Progressor_Fraction_Adult"
KEY_CD4_PROGRESSION_MULTIPLIER = "TB_CD4_Primary_Progression"


def load_emod_parameters(config_filename="config.json", debug=False):
    """reads config file and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_CONFIG_NAME, etc., keys (e.g.)
    """
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {}
    param_obj[KEY_CONFIG_NAME] = cdj[KEY_CONFIG_NAME]
    param_obj[KEY_FAST_PROGRESSOR_FRACTION] = cdj[KEY_FAST_PROGRESSOR_FRACTION]
    param_obj[KEY_CD4_PROGRESSION_MULTIPLIER] = cdj[KEY_CD4_PROGRESSION_MULTIPLIER]
    if debug:
        with open("DEBUG_param_object.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj


def parse_stdout_file(initial_timestep=0, stdout_filename="test.txt", debug=False):
    """
    :param initial_timestep:   first timestep from config
    :param stdout_filename: file to parse (test.txt)
    :param debug:           whether or not we write an additional file that's full of the matched lines
    :return:                array of lines of interest
    """
    exogenous = "EXOGENOUS infected"  # "EXOGENOUS infected"
    infectiousness = "UpdateInfectiousness"
    state_latent = "state= Latent"
    filtered_lines = []
    exogenous_infected_count = 0
    latent_count = 0
    exogenous_infected_dict = {}
    exogenous_infected_list = []
    infections_start_time = 12 # this will vary depending when imported people will begin to be infectious.
    initial_population = 0  # placeholder for the first time step
    update_time = "Update(): Time: "
    time = initial_timestep
    with open(stdout_filename) as logfile:
        for line in logfile:
            if update_time in line:
                if time > infections_start_time:
                    exogenous_infected_list.append(exogenous_infected_count)
                    exogenous_infected_dict[time] = [exogenous_infected_count, latent_count]
                time += 1
                if time == 1:
                    initial_population = int(sft.get_val("StatPop: ", line))
                exogenous_infected_count = 0  # resetting for the next time step
                latent_count = 0
                filtered_lines.append(line)
            elif exogenous in line:
                ind_id = int(float(sft.get_val("Individual ", line)))
                if ind_id <= initial_population: # ignoring imported people
                    exogenous_infected_count += 1
                filtered_lines.append(line)
            elif infectiousness in line and state_latent in line:
                ind_id = int(float(sft.get_val("Individual ", line)))
                fast_progressor = int(float(sft.get_val("progressor=", line)))
                filtered_lines.append(line)
                if not fast_progressor and ind_id <= initial_population:  # ignoring imported people
                        latent_count += 1

    if debug:
        with open("filtered_lines.txt", "w") as outfile:
            for line in filtered_lines:
                outfile.write(line)

    return [exogenous_infected_dict, exogenous_infected_list]


def create_report_file(param_obj, stdout_data_obj, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[KEY_CONFIG_NAME]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        cd4_progressor_multiplier = param_obj[KEY_CD4_PROGRESSION_MULTIPLIER][0]  # all values should be the same
        fast_progressor_fraction = param_obj[KEY_FAST_PROGRESSOR_FRACTION]
        results = []
        for key, value in stdout_data_obj[0].items():
            if key > 2 and value[1] > 35:  # ignoring the smaller latent #s because there's not enough data for the test
                result = sft.test_binomial_95ci(value[0], value[1],
                                                    fast_progressor_fraction * cd4_progressor_multiplier, outfile,
                                                    "Time {} :{} new exogenous infections for {} latent infections."
                                                     "\n".format(key, value[0], value[1]))
                results.append(result)
        if not results:
            outfile.write("No results.\n")
            success = False
        elif results.count(False) > 2:  # not enough data for a binomial test on the binomial results,
            success = False           # so, the closest we can come is "less than two False is good".
            outfile.write("{} out of {} tests failed.\n".format(results.count(False), len(results)))
        outfile.write(sft.format_success_msg(success))
        sft.plot_data(stdout_data_obj[1], title="Exogenous infections for tracked time steps",
                          category="exogenous_infections_tb_hiv_art")

    if debug:
        print("SUMMARY: Success={0}\n".format(success))
    return success


def application(output_folder="output", stdout_filename="test.txt", initial_timestep=0,
                config_filename="config.json",  report_name=sft.sft_output_filename, debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename + "\n")
        print("initial_timestep: " + str( initial_timestep ) + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename, debug)
    stdout_data_obj = parse_stdout_file(initial_timestep, stdout_filename, debug)
    create_report_file(param_obj, stdout_data_obj, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--config', default="config.json", help="config.json param file")
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-t', '--time', default=0, help="initial timestep for filterting test.txt data(test.txt)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout, config_filename=args.config,
                initial_timestep=args.time, report_name=args.reportname, debug=True)

