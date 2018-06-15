#!/usr/bin/python

import json
import dtk_sft
import math
import numpy as np

"""
SEIR_PreSymptomatic:

SEIR dynamics outbreak only. Outbreak TB to all individuals. All will progress to presymtomatic active TB within 1
timestep.  Infectivity is set to 0 to look only at progression.

Individuals in presymptomatic class will progress to symptomatic TB (in DTK broadcast of 'TBActivation') according to
rate 'TB_Presymptomatic_Rate'

Statistical test: K-S test for time to 'TBActivation' (onset of symptomatic TB) exponentially distributed according to
rate 'TB_Presymptomatic_Rate'.

"""

KEY_CONFIG_NAME = "Config_Name"
KEY_SIMULATION_TIMESTEP = "Simulation_Timestep"
KEY_DURATION = "Simulation_Duration"
KEY_PRESYMPTOMATIC = "presymptomatic active"
KEY_SYMPTOMATIC = "symptomatic active"
KEY_PRESYMPTOMATIC_RATE = "TB_Presymptomatic_Rate"
KEY_INDIVIDUAL = "Individual "
KEY_TIMER = "timer "

matches = ["Update(): Time: ",
           "moved from Latent to Active Presymptomatic",
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
    param_obj[KEY_PRESYMPTOMATIC_RATE] = cdj[KEY_PRESYMPTOMATIC_RATE]
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
        elif matches[1] in line: # this individual is PreSymptomatic active
            individual_id = dtk_sft.get_val(KEY_INDIVIDUAL, line)
            timer = float(dtk_sft.get_val(KEY_TIMER, line))
            if individual_id in output_dict:
                output_dict[individual_id][KEY_PRESYMPTOMATIC] = [time_step, timer]
            else:
                output_dict[individual_id] = {KEY_PRESYMPTOMATIC: [time_step, timer]}
        elif matches[2] in line: # this individual is Symptomatic active
            individual_id = dtk_sft.get_val(KEY_INDIVIDUAL, line)
            timer = float(dtk_sft.get_val(KEY_TIMER, line))
            if individual_id in output_dict:
                output_dict[individual_id][KEY_SYMPTOMATIC] = [time_step, timer]
            else:
                output_dict[individual_id] = {KEY_SYMPTOMATIC:[time_step, timer]}
    res_path = r'./SEIR_Presymptomatic_from_logging.json'
    with open(res_path, "w") as file:
        json.dump(output_dict, file, indent=4)
    return output_dict

def create_report_file(param_obj, output_dict, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[KEY_CONFIG_NAME]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        presymptomatic_rate = param_obj[KEY_PRESYMPTOMATIC_RATE]
        simulation_duration = param_obj[KEY_DURATION]
        if not len(output_dict):
            success = False
            outfile.write(dtk_sft.sft_no_test_data)
        actual_timer = []
        internal_timer = []
        outfile.write("collecting the actual timestep between presymptomatic and symptomatic:\n")
        outfile.write("Testing the internal timer and the actual timer:\n")
        for id in output_dict:
            presymptomatic_time = timer = symptomatic_time = None
            if KEY_PRESYMPTOMATIC in output_dict[id]:
                presymptomatic_time = output_dict[id][KEY_PRESYMPTOMATIC][0]
            if KEY_SYMPTOMATIC in output_dict[id]:
                symptomatic_time = output_dict[id][KEY_SYMPTOMATIC][0]
                timer = output_dict[id][KEY_SYMPTOMATIC][1]
                internal_timer.append(timer)
            if presymptomatic_time:
                if symptomatic_time: # some individual may not die yet at the end of the simulation
                    actual_timer.append(symptomatic_time - presymptomatic_time)
                    if symptomatic_time - presymptomatic_time != math.ceil(timer):
                        success = False
                        outfile.write("BAD: individual {0} has internal timer = {1} but the actual timer is {2} (enter "
                                      "presymptomatic active state at timestep {3}, enter symptomatic active state at "
                                      "timestep {4}).\n".format(id, timer, symptomatic_time - presymptomatic_time,
                                                                presymptomatic_time, symptomatic_time))
                else:
                    outfile.write("Individual {0} moved to presymptomatic active at timestep {1} and is not move to "
                                  "symptomatic active yet at the end of simulation (duration = {2})."
                                  "\n".format(id, presymptomatic_time, simulation_duration))
            else:
                success = False
                outfile.write("BAD: individual {0} moved to symptomatic active state at timerstep {1} before entering "
                              "active presymptomatic state.\n".format(id, symptomatic_time))
        if not len(actual_timer):
            success = False
            outfile.write("BAD: There is no presymptomatic to symptomatic state transition in this test, please fix the test.\n")

        outfile.write("Running ks test for internal and numpy exponential distribution: \n")
        size = len(internal_timer)
        scale = 1.0 / presymptomatic_rate
        dist_exponential_np = np.random.exponential(scale, size)
        dtk_sft.plot_data_sorted(internal_timer, dist2=np.array(dist_exponential_np), label1="internal timer",
                                   label2="numpy exponential",
                                   title="exponential rate = {}".format(presymptomatic_rate),
                                   xlabel="data point", ylabel="internal timer", category='Internal_timer',
                                   show=True, line=True, overlap=True)
        result = dtk_sft.test_exponential(internal_timer, p1=presymptomatic_rate, report_file=outfile, integers=False, roundup=False,
                                          round_nearest=False)
        outfile.write("ks test result is {0}, exponential rate = {1}, # of data point = {2}.\n".format(result, presymptomatic_rate, size))
        if not result:
            success = False
            outfile.write("BAD: test exponential for internal timer failed with presymptomatic_rate = {}.\n".format(presymptomatic_rate))

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
