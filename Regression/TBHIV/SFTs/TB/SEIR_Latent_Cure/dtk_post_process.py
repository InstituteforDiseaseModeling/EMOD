#!/usr/bin/python

import json
import dtk_sft
import math
import numpy as np

"""
SEIR_Latent_Cure:

SEIR dynamics. Outbreak HIV at time 0 and then TB at time 2 to everyone. Individuals will not progress to active
disease due to progression rate close to zero.

Individuals will clear latent infection according to   'TB_Latent_Cure_Rate' = 1 yr-1

Statistical test: K-S test on time to clearance of latent infection exponentially distributed with rate given by 'TB_Latent_Cure_Rate'

"""

KEY_CONFIG_NAME = "Config_Name"
KEY_SIMULATION_TIMESTEP = "Simulation_Timestep"
KEY_DURATION = "Simulation_Duration"
KEY_PRESYMPTOMATIC = "presymptomatic active"
KEY_LATENT = "latent"
KEY_CLEARED = 'cleared'
KEY_FAST_PROGRESSOR_RATE = "TB_Fast_Progressor_Rate"
KEY_ADULT_FRACTION = "TB_Fast_Progressor_Fraction_Adult"
KEY_CHILD_FRACTION = "TB_Fast_Progressor_Fraction_Child"
KEY_LATENT_CURE_RATE = "TB_Latent_Cure_Rate"
KEY_INDIVIDUAL = "Individual "
KEY_TIMER = "timer "

matches = ["Update(): Time: ",
           "moved from Latent to Cleared",
           "now has latent infection",
           "moved from Latent to Active Presymptomatic"
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
    param_obj[KEY_FAST_PROGRESSOR_RATE] = cdj[KEY_FAST_PROGRESSOR_RATE]
    param_obj[KEY_ADULT_FRACTION] = cdj[KEY_ADULT_FRACTION]
    param_obj[KEY_CHILD_FRACTION] = cdj[KEY_CHILD_FRACTION]
    param_obj[KEY_SIMULATION_TIMESTEP] = cdj[KEY_SIMULATION_TIMESTEP]
    param_obj[KEY_LATENT_CURE_RATE] = cdj[KEY_LATENT_CURE_RATE]
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
        elif matches[1] in line: # this individual is Cleared
            individual_id = dtk_sft.get_val(KEY_INDIVIDUAL, line)
            if individual_id in output_dict:
                output_dict[individual_id][KEY_CLEARED] = time_step
            else:
                output_dict[individual_id] = {KEY_CLEARED: time_step}
        elif matches[2] in line: # this individual is Latent
            individual_id = dtk_sft.get_val(KEY_INDIVIDUAL, line)
            if individual_id in output_dict:
                output_dict[individual_id][KEY_LATENT] = time_step
            else:
                output_dict[individual_id] = {KEY_LATENT: time_step}
        elif matches[3] in line: # this individual is PreSymptomatic active
            individual_id = dtk_sft.get_val(KEY_INDIVIDUAL, line)
            if individual_id in output_dict:
                output_dict[individual_id][KEY_PRESYMPTOMATIC] = time_step
            else:
                output_dict[individual_id] = {KEY_PRESYMPTOMATIC: time_step}
    res_path = r'./SEIR_Latent_Cure_from_logging.json'
    with open(res_path, "w") as file:
        json.dump(output_dict, file, indent=4)
    return output_dict

def create_report_file(param_obj, output_dict, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[KEY_CONFIG_NAME]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        fast_progressor_rate = param_obj[KEY_FAST_PROGRESSOR_RATE]
        latent_cure_rate = param_obj[KEY_LATENT_CURE_RATE]
        child_fast_fraction = param_obj[KEY_CHILD_FRACTION]
        adult_fast_fraction = param_obj[KEY_ADULT_FRACTION]
        simulation_duration = param_obj[KEY_DURATION]
        if not len(output_dict):
            success = False
            outfile.write(dtk_sft.sft_no_test_data)

        outfile.write("checking test conditions: \n")
        if not child_fast_fraction or not adult_fast_fraction:
            success = False
            outfile.write("BAD: expected {0} and {1} = 1, got {2} and {3} from config.json. "
                          "Please fix the test.\n".format(KEY_CHILD_FRACTION, KEY_ADULT_FRACTION, child_fast_fraction, adult_fast_fraction))
        dist_exponential_np_fast = np.random.exponential(1 / fast_progressor_rate, 100)
        if min(dist_exponential_np_fast) < simulation_duration:
            success = False
            outfile.write("BAD: expected a small {0} to avoid moving individual to active disease state, got {1} from config.json. Please "
                          "fix the test.\n".format(KEY_FAST_PROGRESSOR_RATE, fast_progressor_rate))
        outfile.write("conditional check result is {}.\n".format(success))

        actual_timer = []
        outfile.write("collecting the actual timestep between latent and cleared:\n")
        for id in output_dict:
            cleared_time = presymptomatic_time = latent_time = None
            if KEY_CLEARED in output_dict[id]:
                cleared_time = output_dict[id][KEY_CLEARED]
            if KEY_LATENT in output_dict[id]:
                latent_time = output_dict[id][KEY_LATENT]
            if KEY_PRESYMPTOMATIC in output_dict[id]:
                presymptomatic_time = output_dict[id][KEY_PRESYMPTOMATIC]
            if latent_time:
                if cleared_time: # some individual may not move to cleared state at the end of the simulation
                    actual_timer.append(cleared_time - latent_time)
                else:
                    outfile.write("Individual {0} moved to latent state at timestep {1} and is not cleared yet at the "
                                  "end of simulation (duration = {2})."
                                  "\n".format(id, latent_time, simulation_duration))
            else:
                success = False
                outfile.write("BAD: individual {0} moved to cleared state at timerstep {1} before entering "
                              "latent state.\n".format(id, cleared_time))
            if presymptomatic_time:
                success = False
                outfile.write("BAD: individual {0} moved to presymptomatic at timestep {1}, expected no active disease"
                              " in this simulation, please double check the config.\n".format(id, presymptomatic_time))
        if not len(actual_timer):
            success = False
            outfile.write("BAD: There is no latent to cleared transition in this test, please fix the test.\n")

        outfile.write("Running ks test for latent to cleared duration and numpy exponential distribution: \n")
        size = len(actual_timer)
        scale = 1.0 / latent_cure_rate
        dist_exponential_np = np.random.exponential(scale, size)
        dtk_sft.plot_data_sorted(actual_timer, dist2=np.array(dist_exponential_np), label1="latent to cleared duration",
                          label2="numpy exponential", title="exponential rate = {}".format(latent_cure_rate),
                          xlabel="data point", ylabel="latent to cleared duration",
                          category='latent_to_cleared_duration', show=True, line=False, overlap=True)
        result = dtk_sft.test_exponential(actual_timer, p1=latent_cure_rate, report_file=outfile, integers=True, roundup=True,
                                          round_nearest=False)
        outfile.write("ks test result is {0}, exponential rate = {1}, # of data point = {2}.\n".format(result, latent_cure_rate, size))
        if not result:
            success = False
            outfile.write("BAD: test exponential for latent to cleared duration failed with {0} "
                          "= {1}.\n".format(KEY_LATENT_CURE_RATE, latent_cure_rate))
        else:
            outfile.write("GOOD: test exponential for latent to cleared duration passed with {0} "
                          "= {1}.\n".format(KEY_LATENT_CURE_RATE, latent_cure_rate))

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

