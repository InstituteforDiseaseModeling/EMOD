#!/usr/bin/python

import json
import dtk_test.dtk_sft as sft
import math
import numpy as np

"""
SEIR_Fast_Progress_HIV::

SEIR dynamics,  outbreak only.  Outbreak HIV and TB to all individuals in successive time  steps.  50% of HIV positives
should progress via Fast progression according to params. Since fast progession is set to happen in 1 timestep this
means that the fast progressor fraction  of population will progress from latent to active in 1 timestep

'TB_Slow_Progressor_Rate' was set to 1e-09, so the individual that fall in this category will progress to active after
1e8 days, which will not happen before the simulation ends.

Statistical Test: Binomial test with p = Fast_Progressor_Fraction_Adult (adults > 15 years of age),
Fast_Porgressor_Fraction_Child (child) for the number of individuals progressing from latent to active disease in 1 timestep.

"""

KEY_CONFIG_NAME = "Config_Name"
KEY_SIMULATION_TIMESTEP = "Simulation_Timestep"
KEY_DURATION = "Simulation_Duration"
KEY_PRESYMPTOMATIC = "presymptomatic active"
KEY_LATENT = "latent"
KEY_SLOW_PROGRESSOR_RATE = "TB_Slow_Progressor_Rate"
KEY_FAST_PROGRESSOR_RATE = "TB_Fast_Progressor_Rate"
KEY_ADULT_FRACTION = "TB_Fast_Progressor_Fraction_Adult"
KEY_CHILD_FRACTION = "TB_Fast_Progressor_Fraction_Child"
KEY_CD4_PROGRESSION_MULTIPLIER = "TB_CD4_Primary_Progression"
KEY_INDIVIDUAL = "Individual "
KEY_TIMER = "timer "

matches = ["Update(): Time: ",
           "moved from Latent to Active Presymptomatic",
           "now has latent infection"
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
    param_obj[KEY_FAST_PROGRESSOR_RATE] = cdj[KEY_FAST_PROGRESSOR_RATE]
    param_obj[KEY_ADULT_FRACTION] = cdj[KEY_ADULT_FRACTION]
    param_obj[KEY_CHILD_FRACTION] = cdj[KEY_CHILD_FRACTION]
    param_obj[KEY_CD4_PROGRESSION_MULTIPLIER] = cdj[KEY_CD4_PROGRESSION_MULTIPLIER]
    param_obj[KEY_SIMULATION_TIMESTEP] = cdj[KEY_SIMULATION_TIMESTEP]
    param_obj[KEY_DURATION] = cdj[KEY_DURATION]
    if debug:
        with open("DEBUG_param_object.json", 'w') as outfile:
            json.dump(param_obj, outfile, indent=4)
    return param_obj

def parse_output_file(output_filename="test.txt", simulation_timestep=1, debug=False):
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
    output_dict = {}
    for line in filtered_lines:
        if matches[0] in line:
            time_step += simulation_timestep
        elif matches[1] in line: # this individual is PreSymptomatic active
            individual_id = sft.get_val(KEY_INDIVIDUAL, line)
            timer = float(sft.get_val(KEY_TIMER, line))
            if individual_id in output_dict:
                output_dict[individual_id][KEY_PRESYMPTOMATIC] = [time_step, timer]
            else:
                output_dict[individual_id] = {KEY_PRESYMPTOMATIC: [time_step, timer]}
        elif matches[2] in line: # this individual is latent
            individual_id = sft.get_val(KEY_INDIVIDUAL, line)
            if individual_id in output_dict:
                output_dict[individual_id][KEY_LATENT] = time_step
            else:
                output_dict[individual_id] = {KEY_LATENT: time_step}
    res_path = r'./SEIR_Latent_Slow_from_logging.json'
    with open(res_path, "w") as file:
        json.dump(output_dict, file, indent=4)
    return output_dict

def create_report_file(param_obj, output_dict, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[KEY_CONFIG_NAME]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        slow_progressor_rate = param_obj[KEY_SLOW_PROGRESSOR_RATE]
        fast_progressor_rate = param_obj[KEY_FAST_PROGRESSOR_RATE]
        child_fast_fraction = param_obj[KEY_CHILD_FRACTION]
        adult_fast_fraction = param_obj[KEY_ADULT_FRACTION]
        simulation_duration = param_obj[KEY_DURATION]
        progression_multiplier = np.mean(param_obj[KEY_CD4_PROGRESSION_MULTIPLIER])
        if not len(output_dict):
            success = False
            outfile.write(sft.sft_no_test_data)

        outfile.write("checking test conditions: \n")
        if adult_fast_fraction:
            success = False
            outfile.write("BAD: expected {0} = 0, got {1} from config.json. "
                          "Please fix the test.\n".format(KEY_ADULT_FRACTION, adult_fast_fraction))
        dist_exponential_np_slow = np.random.exponential(1/slow_progressor_rate, 100)
        if min(dist_exponential_np_slow) < simulation_duration:
            success = False
            outfile.write("BAD: expected a small {0} to distinguish fast and slow progress TB, got {1} from config.json. Please "
                          "fix the test.\n".format(KEY_SLOW_PROGRESSOR_RATE, slow_progressor_rate))
        outfile.write("conditional check result is {}.\n".format(success))

        actual_timer = []
        internal_timer = []
        slow_count = 0
        outfile.write("collecting the actual timestep between latent and presymptomatic:\n")
        outfile.write("checking if the internal timer matches the PreSymptomatic to Cleared duration:\n")

        for id in output_dict:
            presymptomatic_time = timer = latent_time = None
            if KEY_PRESYMPTOMATIC in output_dict[id]:
                presymptomatic_time = output_dict[id][KEY_PRESYMPTOMATIC][0]
                timer = output_dict[id][KEY_PRESYMPTOMATIC][1]
                internal_timer.append(timer)
            if KEY_LATENT in output_dict[id]:
                latent_time = output_dict[id][KEY_LATENT]
            if latent_time:
                if presymptomatic_time: # some individual may not move to presymptomatic state at the end of the simulation
                    actual_timer.append(presymptomatic_time - latent_time)
                    if presymptomatic_time - latent_time != math.ceil(timer):
                        success = False
                        outfile.write("BAD: individual {0} has internal timer = {1} but the actual timer is {2} (enter "
                                      "latent state at timestep {3}, enter presymptomatic active state at "
                                      "timestep {4}).\n".format(id, timer, presymptomatic_time - latent_time,
                                                                latent_time, presymptomatic_time))
                else:
                    slow_count += 1
                    if debug:
                        outfile.write("Individual {0} moved to latent state at timestep {1} and is not move to "
                                      "presymptomatic active yet at the end of simulation (duration = {2})."
                                      "\n".format(id, latent_time, simulation_duration))
            else:
                success = False
                outfile.write("BAD: individual {0} moved to presymptomatic active state at timerstep {1} before entering "
                              "latent state.\n".format(id, presymptomatic_time))
        if not len(actual_timer):
            success = False
            outfile.write("BAD: There is no latent to presymptomatic state transition in this test, please fix the test.\n")

        outfile.write("Running ks test for latent to presymptomatic internal timer and numpy exponential distribution: \n")
        size = len(internal_timer)
        scale = 1.0 / fast_progressor_rate
        dist_exponential_np = np.random.exponential(scale, size)
        sft.plot_data_sorted(internal_timer, dist2=np.array(dist_exponential_np), label1="internal timer",
                          label2="numpy exponential", title="exponential rate = {}".format(fast_progressor_rate),
                          xlabel="data point", ylabel="latent to presymptomatic internal timer",
                          category='latent_to_presymptomatic_internal_timer', show=True, line=False, overlap=True)
        result = sft.test_exponential(internal_timer, p1=fast_progressor_rate, report_file=outfile, integers=True, roundup=True,
                                          round_nearest=False)
        outfile.write("ks test result is {0}, exponential rate = {1}, # of data point = {2}.\n".format(result, fast_progressor_rate, size))
        if not result:
            success = False
            outfile.write("BAD: test exponential for latent to presymptomatic duration failed with fast_progressor_rate "
                          "= {}.\n".format(fast_progressor_rate))
        else:
            outfile.write(
                "GOOD: test exponential for latent to presymptomatic duration passed with fast_progressor_rate "
                "= {}.\n".format(fast_progressor_rate))

        outfile.write("running binomial test with 95% confidence for Fast_Porgressor_Fraction_Child:\n")
        result2 = sft.test_binomial_95ci(num_success=len(internal_timer), num_trials=len(internal_timer) + slow_count,
                                             prob=child_fast_fraction * progression_multiplier, report_file=outfile,
                                             category="Fast_Progressor_Fraction_Child")
        outfile.write("# of slow progressor is {0} and # of fast progressor is {1}.\n".format(slow_count, len(internal_timer)))
        if not result2:
            success = False
            outfile.write("BAD: binomial test for Fast_Progressor_Fraction_Child = {0} and TB_CD4_Primary_Progression= {1} failed"
                          ".\n".format(child_fast_fraction, progression_multiplier))
        else:
            outfile.write("GOOD: binomial test for Fast_Progressor_Fraction_Child = {0} and TB_CD4_Primary_Progression= {1} passed"
                          ".\n".format(child_fast_fraction, progression_multiplier))
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
    simulation_timestep = param_obj[KEY_SIMULATION_TIMESTEP]
    output_dict = parse_output_file(stdout_filename, simulation_timestep, debug)
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
