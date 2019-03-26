#!/usr/bin/python

import json
import dtk_test.dtk_sft as sft
import math
import numpy as np

"""
SEIR_Death:

SEIR dynamics outbreak only. Outbreak TB to all individuals. All will progress to symptomatic active TB within 2
timesteps.  Infectivity is set to 0 to look only at progression.

Active TB will progress to death according to rate 'TB_Active_Mortality_Rate'.

Statistical test: K-S test for time to death, exponentially distributed with rate of 'TB_Active_Mortality_Rate'.

"""

KEY_CONFIG_NAME = "Config_Name"
KEY_SIMULATION_TIMESTEP = "Simulation_Timestep"
KEY_DURATION = "Simulation_Duration"
KEY_DEATH = "Death"
KEY_SYMPTOMATIC = "symptomatic active"
KEY_DEATH_RATE = "TB_Active_Mortality_Rate"
KEY_INDIVIDUAL = "Individual "
KEY_TIMER = "timer "

matches = ["Update(): Time: ",
           "moved from Active Symptomatic to Death",
           "moved from Active Presymptomatic to Active Symptomatic"
           ]

def load_emod_parameters(config_filename="config.json", debug=False):
    """reads config file and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_CONFIG_NAME, etc., keys (e.g.)
    """
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {}
    param_obj[KEY_CONFIG_NAME] = cdj[KEY_CONFIG_NAME]
    param_obj[KEY_DEATH_RATE] = cdj[KEY_DEATH_RATE]
    param_obj[KEY_SIMULATION_TIMESTEP] = cdj[KEY_SIMULATION_TIMESTEP]
    param_obj[KEY_DURATION] = cdj[KEY_DURATION]
    if debug:
        with open("DEBUG_param_object.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj

def parse_output_file(output_filename="test.txt", debug=False):
    """
    creates a dictionary to store filtered information for each time step
    :param output_filename: file to parse (test.txt)
    :return:                output_dict
    """
    filtered_lines = []
    with open(output_filename) as logfile:
        for line in logfile:
            if sft.has_match(line,matches):
                filtered_lines.append(line)
    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    # initialize variables
    time_step = 0
    simulation_timestep = 1
    output_dict = {}
    for line in filtered_lines:
        if matches[0] in line:
            time_step += simulation_timestep
        if matches[1] in line:
            individual_id = sft.get_val(KEY_INDIVIDUAL, line)
            if individual_id in output_dict:
                output_dict[individual_id][KEY_DEATH] = time_step
            else:
                output_dict[individual_id] = {KEY_DEATH: time_step}
        if matches[2] in line:
            individual_id = sft.get_val(KEY_INDIVIDUAL, line)
            timer = float(sft.get_val(KEY_TIMER, line))
            if individual_id in output_dict:
                output_dict[individual_id][KEY_SYMPTOMATIC] = [time_step, timer]
            else:
                output_dict[individual_id] = {KEY_SYMPTOMATIC:[time_step, timer]}
    res_path = r'./SEIR_Death_from_logging.json'
    with open(res_path, "w") as file:
        json.dump(output_dict, file, indent=4)
    return output_dict

def create_report_file(param_obj, output_dict, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[KEY_CONFIG_NAME]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        death_rate = param_obj[KEY_DEATH_RATE]
        simulation_duration = param_obj[KEY_DURATION]
        if not len(output_dict):
            success = False
            outfile.write(sft.sft_no_test_data)
        actual_timer = []
        outfile.write("collecting the actual timestep between active and death:\n")
        for id in output_dict:
            death_time = timer = active_time = None
            if KEY_DEATH in output_dict[id]:
                death_time = output_dict[id][KEY_DEATH]
            if KEY_SYMPTOMATIC in output_dict[id]:
                active_time = output_dict[id][KEY_SYMPTOMATIC][0]
            if active_time:
                if death_time: # some individual may not die yet at the end of the simulation
                    actual_timer.append(death_time - active_time)
                else:
                    outfile.write("Individual {0} moved to symptomatic active at timestep {1} and is not dead yet at "
                                  "the end of simulation (duration = {2}).\n".format(id, active_time, simulation_duration))
            else:
                success = False
                outfile.write("BAD: individual {0} died before entering active symptomatic state.\n".format(id))
        if not len(actual_timer):
            success = False
            outfile.write("BAD: There is no death in this test, please fix the test.\n")

        outfile.write("Running ks test for death time and numpy exponential distribution: \n")
        size = len(actual_timer)
        scale = 1.0 / death_rate
        dist_exponential_np = np.random.exponential(scale, size)
        sft.plot_data_sorted(actual_timer, dist2=np.array(dist_exponential_np), label1="death timer",
                                   label2="numpy exponential",
                                   title="exponential rate = {}".format(death_rate),
                                   xlabel="data point", ylabel="death timer", category='Death_timer',
                                   show=True, line=False, overlap=True)
        result = sft.test_exponential(actual_timer, p1=death_rate, report_file=outfile, integers=True, roundup=True,
                                          round_nearest=False)
        outfile.write("ks test result is {0}, exponential rate = {1}, # of data point = {2}.\n".format(result, death_rate, size))
        if not result:
            success = False
            outfile.write("BAD: test exponential for death timer failed with death rate = {}.\n".format(death_rate))

        outfile.write(sft.format_success_msg(success))
    if debug:
        print( "SUMMARY: Success={0}\n".format(success) )
    return success

def application( output_folder="output", stdout_filename="test.txt", insetchart_name="InsetChart.json",
                 config_filename="config.json", campaign_filename="campaign.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "insetchart_name: " + insetchart_name+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "campaign_filename: " + campaign_filename + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )

    sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename, debug)
    output_dict = parse_output_file(stdout_filename, debug)
    create_report_file(param_obj, output_dict, report_name, debug)

if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-j', '--jsonreport', default="InsetChart.json", help="Json report to load (InsetChart.json)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-C', '--campaign', default="campaign.json", help="campaign name to load (campaign.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout, insetchart_name=args.jsonreport,
                config_filename=args.config, campaign_filename=args.campaign,
                report_name=args.reportname, debug=False)
