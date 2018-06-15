#!/usr/bin/python

import json
import dtk_sft
import math
import numpy as np

"""
SEIR_Cure:
SEIR dynamics outbreak only. Outbreak TB to all individuals. All will progress to symptomatic active TB within 2
timesteps.  Infectivity is set to 0 to look only at progression.

Active TB will resolve (self cure) with rate of 1.0/365 = TB_Active_Cure_Rate. Note TB Death rate has been set to 0
to test each rate individually.

Statistical Test: K-S test on time to resolved infections (might need to add broadcast here) exponentially distributed
with rate 'TB_Actve_Cure_Rate'
"""

KEY_CONFIG_NAME = "Config_Name"
KEY_SIMULATION_TIMESTEP = "Simulation_Timestep"
KEY_DURATION = "Simulation_Duration"
KEY_CLEARED = "cleared"
KEY_SYMPTOMATIC = "symptomatic active"
KEY_CURE_RATE = "TB_Active_Cure_Rate"
KEY_INDIVIDUAL = "Individual "
KEY_TIMER = "timer "

matches = ["Update(): Time: ",
           "moved from Active Symptomatic to Cleared",
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
    param_obj[KEY_CURE_RATE] = cdj[KEY_CURE_RATE]
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
            if dtk_sft.has_match(line,matches):
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
            individual_id = dtk_sft.get_val(KEY_INDIVIDUAL, line)
            timer = float(dtk_sft.get_val(KEY_TIMER, line))
            if individual_id in output_dict:
                output_dict[individual_id][KEY_CLEARED] = [time_step, timer]
            else:
                output_dict[individual_id] = {KEY_CLEARED:[time_step, timer]}
        if matches[2] in line:
            individual_id = dtk_sft.get_val(KEY_INDIVIDUAL, line)
            timer = float(dtk_sft.get_val(KEY_TIMER, line))
            if individual_id in output_dict:
                output_dict[individual_id][KEY_SYMPTOMATIC] = [time_step, timer]
            else:
                output_dict[individual_id] = {KEY_SYMPTOMATIC:[time_step, timer]}
    res_path = r'./SEIR_Cure_from_logging.json'
    with open(res_path, "w") as file:
        json.dump(output_dict, file, indent=4)
    return output_dict

def create_report_file(param_obj, output_dict, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[KEY_CONFIG_NAME]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        cure_rate = param_obj[KEY_CURE_RATE]
        simulation_duration = param_obj[KEY_DURATION]
        if not len(output_dict):
            success = False
            outfile.write(dtk_sft.sft_no_test_data)
        cure_timer = []
        actual_timer = []
        outfile.write("Checking the timer and actual timestep between active and cure:\n")
        outfile.write("checking if the internal timer matches the PreSymptomatic to Cleared duration:\n")

        for id in output_dict:
            cure_time = timer = active_time = None
            if KEY_CLEARED in output_dict[id]:
                cure_time = output_dict[id][KEY_CLEARED][0]
                timer = output_dict[id][KEY_CLEARED][1]
                cure_timer.append(timer)
            if KEY_SYMPTOMATIC in output_dict[id]:
                active_time = output_dict[id][KEY_SYMPTOMATIC][0]
            if active_time:
                if cure_time: # some individual may not be cleared at the end of the simulation
                    actual_timer.append(cure_time - active_time)
                    if cure_time - active_time != math.ceil(timer):
                        success = False
                        outfile.write("BAD: individual {0} has cure timer = {1} but the actual cure time is {2} (enter "
                                      "symptomatic active state at timestep {3}, cleared at timestep {4}).\n".format(id, timer,
                                      cure_time - active_time, active_time, cure_time))
                else:
                    outfile.write("Individual {0} moved to symptomatic active at timestep {1} and is not cleared yet at "
                                  "the end of simulation (duration = {2}).\n".format(id, active_time, simulation_duration))
            else:
                success = False
                outfile.write("BAD: individual {0} is cleared before entering active symptomatic state.\n".format(id))
        if not len(actual_timer):
            success = False
            outfile.write("BAD: There is no recovered individual in this test, please fix the test.\n")
        outfile.write("Result is {0}. # of recovered individual = {1}\n".format(success, len(actual_timer)))

        outfile.write("Running ks test for timer and numpy exponential distribution: \n")
        size = len(cure_timer)
        scale = 1.0 / cure_rate
        dist_exponential_np = np.random.exponential(scale, size)
        dtk_sft.plot_data_sorted(cure_timer, dist2=np.array(dist_exponential_np), label1="cure timer",
                                   label2="numpy exponential",
                                   title="exponential rate = {}".format(cure_rate),
                                   xlabel="data point", ylabel="cure timer", category='Cure_timer',
                                   show=True, line=False, overlap=True)
        result = dtk_sft.test_exponential(cure_timer, p1=cure_rate, report_file=outfile, integers=False, roundup=False,
                                          round_nearest=False)
        outfile.write("ks test result is {0}, exponential rate = {1}, # of data point = {2}.\n".format(result, cure_rate, size))
        if not result:
            success = False
            outfile.write("BAD: test exponential for cure timer failed with cure rate = {}.\n".format(cure_rate))

        outfile.write(dtk_sft.format_success_msg(success))
    if debug:
        print( "SUMMARY: Success={0}\n".format(success) )
    return success

def application( output_folder="output", stdout_filename="test.txt", insetchart_name="InsetChart.json",
                 config_filename="config.json", campaign_filename="campaign.json",
                 report_name=dtk_sft.sft_output_filename,
                 debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "insetchart_name: " + insetchart_name+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "campaign_filename: " + campaign_filename + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )

    dtk_sft.wait_for_done()
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
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout, insetchart_name=args.jsonreport,
                config_filename=args.config, campaign_filename=args.campaign,
                report_name=args.reportname, debug=False)
