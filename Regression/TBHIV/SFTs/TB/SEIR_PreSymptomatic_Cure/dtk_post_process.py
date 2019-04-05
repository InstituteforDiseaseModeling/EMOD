#!/usr/bin/python

import json
import dtk_test.dtk_sft as sft
import math
import numpy as np

"""
SEIR_PreSymptomatic_Cure:

SEIR dynamics outbreak only. Outbreak TB to all individuals. All will progress to presymptomatic active TB within 1
timestep.  Infectivity is set to 0 to look only at progression.

Individuals in presymptomatic class will more from presymptomatic active state to cured according to rate
'TB_Presymptomatic_Cure_Rate'

Statistical test: K-S test for time to cured infection, exponentially distributed according to rate 'TB_Presymptomatic_Cure_Rate'.


"""

KEY_CONFIG_NAME = "Config_Name"
KEY_SIMULATION_TIMESTEP = "Simulation_Timestep"
KEY_DURATION = "Simulation_Duration"
KEY_PRESYMPTOMATIC = "presymptomatic active"
KEY_CLEARED = 'cleared'
KEY_SLOW_PROGRESSOR_RATE = "TB_Slow_Progressor_Rate"
KEY_LATENT_CURE_RATE = "TB_Latent_Cure_Rate"
KEY_PRESYMPTOMATIC_CURE_RATE = "TB_Presymptomatic_Cure_Rate"
KEY_PRESYMPTOMATIC_RATE = "TB_Presymptomatic_Rate"
KEY_BASE_INFECTIVITY = "Base_Infectivity"
KEY_INDIVIDUAL = "Individual "
KEY_TIMER = "timer "

matches = ["Update(): Time: ",
           "moved from Active Presymptomatic to Cleared",
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
    param_obj[KEY_SLOW_PROGRESSOR_RATE] = cdj[KEY_SLOW_PROGRESSOR_RATE]
    param_obj[KEY_LATENT_CURE_RATE] = cdj[KEY_LATENT_CURE_RATE]
    param_obj[KEY_SIMULATION_TIMESTEP] = cdj[KEY_SIMULATION_TIMESTEP]
    param_obj[KEY_PRESYMPTOMATIC_CURE_RATE] = cdj[KEY_PRESYMPTOMATIC_CURE_RATE]
    param_obj[KEY_PRESYMPTOMATIC_RATE] = cdj[KEY_PRESYMPTOMATIC_RATE]
    param_obj[KEY_BASE_INFECTIVITY] = cdj[KEY_BASE_INFECTIVITY]
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
        elif matches[1] in line: # this individual is Cleared
            individual_id = sft.get_val(KEY_INDIVIDUAL, line)
            timer = float(sft.get_val(KEY_TIMER, line))
            if individual_id in output_dict:
                output_dict[individual_id][KEY_CLEARED] = [time_step, timer]
            else:
                output_dict[individual_id] = {KEY_CLEARED: [time_step, timer]}
        elif matches[2] in line: # this individual is PreSymptomatic active
            individual_id = sft.get_val(KEY_INDIVIDUAL, line)
            timer = float(sft.get_val(KEY_TIMER, line))
            if individual_id in output_dict:
                output_dict[individual_id][KEY_PRESYMPTOMATIC] = [time_step, timer]
            else:
                output_dict[individual_id] = {KEY_PRESYMPTOMATIC: [time_step, timer]}
    res_path = r'./SEIR_PreSymptomatic_Cure_from_logging.json'
    with open(res_path, "w") as file:
        json.dump(output_dict, file, indent=4)
    return output_dict

def create_report_file(param_obj, output_dict, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[KEY_CONFIG_NAME]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        slow_progressor_rate = param_obj[KEY_SLOW_PROGRESSOR_RATE]
        latent_cure_rate = param_obj[KEY_LATENT_CURE_RATE]
        presymptomatic_cure_rate = param_obj[KEY_PRESYMPTOMATIC_CURE_RATE]
        presymptomatic_rate = param_obj[KEY_PRESYMPTOMATIC_RATE]
        base_infectivity = param_obj[KEY_BASE_INFECTIVITY]
        simulation_duration = param_obj[KEY_DURATION]
        if not len(output_dict):
            success = False
            outfile.write(sft.sft_no_test_data)

        outfile.write("checking test conditions: \n")
        dist_exponential_np_slow = np.random.exponential(1 / slow_progressor_rate, 100)
        if min(dist_exponential_np_slow) < simulation_duration:
            success = False
            outfile.write("BAD: expected a small {0} to avoid moving individual to active disease state, got {1} from config.json. Please "
                          "fix the test.\n".format(KEY_SLOW_PROGRESSOR_RATE, slow_progressor_rate))
        dist_exponential_np_latent_cure = np.random.exponential(1 / latent_cure_rate, 100)
        if min(dist_exponential_np_latent_cure) < simulation_duration:
            success = False
            outfile.write(
                "BAD: expected a small {0} to avoid Latent to Cleared state transition(all Latent state will progress to "
                "PreSymptomatic), got {1} from config.json. Please fix the test.\n".format(KEY_LATENT_CURE_RATE, latent_cure_rate))
        dist_exponential_np_presymptomatic = np.random.exponential(1 / presymptomatic_rate, 100)
        if min(dist_exponential_np_presymptomatic) < simulation_duration:
            success = False
            outfile.write(
                "BAD: expected a small {0} to avoid PreSymptomatic to Symptomatic state transition(all PreSymptomatic "
                "state will progress to Cleared), got {1} from config.json. Please fix the test.\n".format(
                    KEY_PRESYMPTOMATIC_RATE, presymptomatic_rate))
        if base_infectivity:
            success = False
            outfile.write("BAD: expected {0} = 0 to look only at progression, got {1} from config.json. Please fix"
                          "the test.\n".format( KEY_BASE_INFECTIVITY, base_infectivity))
        outfile.write("conditional check result is {}.\n".format(success))

        actual_timer = []
        internal_timer = []
        outfile.write("collecting the actual timestep between PreSymptomatic and Cleared:\n")
        outfile.write("checking if the internal timer matches the PreSymptomatic to Cleared duration:\n")
        for id in output_dict:
            cleared_time = presymptomatic_time = timer = None
            if KEY_CLEARED in output_dict[id]:
                cleared_time = output_dict[id][KEY_CLEARED][0]
                timer = output_dict[id][KEY_CLEARED][1]
                internal_timer.append(timer)
            if KEY_PRESYMPTOMATIC in output_dict[id]:
                presymptomatic_time = output_dict[id][KEY_PRESYMPTOMATIC][0]
            if presymptomatic_time:
                if cleared_time: # some individual may not move to cleared state at the end of the simulation
                    actual_timer.append(cleared_time - presymptomatic_time)
                    if cleared_time - presymptomatic_time != math.ceil(timer):
                        success = False
                        outfile.write("BAD: individual {0} has internal timer = {1} but the actual timer is {2} (enter "
                                      "PreSymptomatic state at timestep {3}, moved to Cleared state at "
                                      "timestep {4}).\n".format(id, timer, cleared_time - presymptomatic_time,
                                                                presymptomatic_time, cleared_time))
                else:
                    outfile.write("Individual {0} moved to PreSymptomatic state at timestep {1} and is not cleared yet at the "
                                  "end of simulation (duration = {2})."
                                  "\n".format(id, presymptomatic_time, simulation_duration))
            else:
                success = False
                outfile.write("BAD: individual {0} moved to cleared state at timerstep {1} before entering "
                              "PreSymptomatic state.\n".format(id, cleared_time))

        if not len(actual_timer):
            success = False
            outfile.write("BAD: There is no PreSymptomatic to cleared transition in this test, please fix the test.\n")

        outfile.write("Running ks test for PreSymptomatic to cleared internal timer and numpy exponential distribution: \n")
        size = len(internal_timer)
        scale = 1.0 / presymptomatic_cure_rate
        dist_exponential_np = np.random.exponential(scale, size)
        sft.plot_data_sorted(internal_timer, dist2=np.array(dist_exponential_np), label1="PreSymptomatic to cleared duration",
                          label2="numpy exponential", title="exponential rate = {}".format(presymptomatic_cure_rate),
                          xlabel="data point", ylabel="PreSymptomatic to cleared duration",
                          category='PreSymptomatic_to_cleared_duration', show=True, line=True, overlap=True)
        result = sft.test_exponential(internal_timer, p1=presymptomatic_cure_rate, report_file=outfile, integers=False, roundup=False,
                                          round_nearest=False)
        outfile.write("ks test result is {0}, exponential rate = {1}, # of data point = {2}.\n".format(result, presymptomatic_cure_rate, size))
        if not result:
            success = False
            outfile.write("BAD: test exponential for PreSymptomatic to cleared duration failed with {0} "
                          "= {1}.\n".format(KEY_PRESYMPTOMATIC_CURE_RATE, presymptomatic_cure_rate))
        else:
            outfile.write("GOOD: test exponential for PreSymptomatic to cleared duration passed with {0} "
                          "= {1}.\n".format(KEY_PRESYMPTOMATIC_CURE_RATE, presymptomatic_cure_rate))

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

