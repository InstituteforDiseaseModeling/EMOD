#!/usr/bin/python

import json
import dtk_test.dtk_sft as sft
import math
import numpy as np
import dtk_test.dtk_TBHIV_Multicore_Support as tms


"""
Immunity_Natural_Immunity_Decay:

This test is design to run with multinode and multicore setup.

This is the test setup for the immunity decay test:
    Similar idea as for Natural immunity above except we allow for immune decay by setting
    'Acquisiton_Blocking_Immunity_Decay_Rate' = 100 and an 'Post_Infection_Acquisition_Multiplier' = 0.5
    
    Statistical Test:  As for other immunity tests this reduces to a chi-squared test, where we have to account for the
    decay so here if t is time since infection to clearance the expected number of infections cleared in a bin
    (tinitial, tfinal]  will be (note time is given in years here)
    
    E[tfinal-tinital] = B *(tf-ti) - B(1.0 - Post_Infection_Acquisition_Multiplier)/A * exp(-A* ti) * (1.0 - exp(-A* (tf-ti) )
    Where A is the 'Acquisition_Blocking_Immunity_Decay_Rate' represented in 1/years.
    
This test also make sure that the following test conditions are met:
    1. multi nodes
    2. multi cores
    3. human migration is happening
    4. make sure that the stdout file has each core reporting in every time step.

"""
# campaign parameter
KEY_START_DAY = "Start_Day"
# config parameter
KEY_CONFIG_NAME = "Config_Name"
KEY_BASE_INFECTIVITY = "Base_Infectivity"
KEY_IMMUNITY_ACQUISITION_FACTOR = "Post_Infection_Acquisition_Multiplier"
KEY_DECAY_RATE = "Acquisition_Blocking_Immunity_Decay_Rate"
KEY_DEMOGRAPHICS_FILENAMES = "Demographics_Filenames"
KEY_NUM_CORES = "Num_Cores"
KEY_SIMULATION_TIMESTEP = "Simulation_Timestep"
# insetchart constant
KEY_NEW_INFECTION = "New Infections"


matches = ["Time: ",
           "Rank: "
           ]


def load_campaign_file(campaign_filename="campaign.json", debug=False):
    """reads campaign file

    :param campaign_filename: campaign.json file
    :returns: campaign_obj structure, dictionary with KEY_INITIAL_EFFECT, KEY_START_DAY, etc., keys (e.g.)
    """
    with open(campaign_filename) as infile:
        cf = json.load(infile)
    campaign_obj = {}
    start_day = cf["Events"][-1][KEY_START_DAY]
    campaign_obj[KEY_START_DAY] = start_day

    if debug:
        with open("DEBUG_campaign_object.json", 'w') as outfile:
            json.dump(campaign_obj, outfile, indent=4)

    return campaign_obj


def parse_output_file(output_filename="test.txt", debug=False):
    """
    creates a dictionary to store filtered information for each time step
    :param output_filename: file to parse (test.txt)
    :return:                output_dict
    """
    filtered_lines = []
    with open(output_filename) as logfile:
        for line in logfile:
            if sft.has_match(line,[matches[0]]):
                filtered_lines.append(line)
    if debug:
        with open("DEBUG_filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    # initialize variables
    time_step = 0
    core = None
    output_dict = {}
    for line in filtered_lines:
        time_step = sft.get_val(matches[0], line)
        core = sft.get_val(matches[1], line)
        if time_step not in output_dict:
            output_dict[time_step] = [core]
        else:
            output_dict[time_step].append(core)

    if debug:
        res_path = r'./DEBUG_core_per_time_step.json'
        with open(res_path, "w") as file:
            json.dump(output_dict, file, indent=4)
    return output_dict


def create_report_file(param_obj, node_list, campaign_obj, migration_df, report_data_obj, stdout_filename, report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[KEY_CONFIG_NAME]
        outfile.write("Config_name = {}\n".format(config_name))
        success = True
        base_infectivity = param_obj[KEY_BASE_INFECTIVITY]
        start_day = campaign_obj[KEY_START_DAY]
        new_infection = report_data_obj[KEY_NEW_INFECTION]
        immunity_acquisition_factor = param_obj[KEY_IMMUNITY_ACQUISITION_FACTOR]
        decay_rate = param_obj[KEY_DECAY_RATE]

        outfile.write("checking some test conditions:\n")
        outfile.write("  -- simulation duration: {} days\n".format(len(new_infection)))
        if len(new_infection) < start_day + 1 + sft.DAYS_IN_YEAR:
            success = False
            outfile.write("BAD: the simulation duration is too short, please make sure it's at least {} days.\n".format(
                start_day + 1 + sft.DAYS_IN_YEAR))

        result = tms.check_test_condition(param_obj[KEY_NUM_CORES], node_list,  migration_df, outfile)
        if not result:
            success = False
            # summary message is writen to the report file in the check_test_condition function

        number_of_month = 1
        outfile.write("calculate expected number of infections for a time period of {} month("
                      "unit is 1/years):\n".format((number_of_month)))
        t_initial = 0
        expected = []
        decay_rate *= sft.DAYS_IN_YEAR
        base_infectivity *= sft.DAYS_IN_YEAR
        step = number_of_month / sft.MONTHS_IN_YEAR
        for t_final in np.arange(step, 1.01, step):
            expected_new_infection = base_infectivity * (t_final - t_initial) - base_infectivity * (
                                    1.0 - immunity_acquisition_factor) / decay_rate * math.exp(
                                    -1 * decay_rate * t_initial) * (1.0 - math.exp(-1 * decay_rate * (t_final - t_initial)))
            expected_new_infection *= len(node_list)
            expected.append(expected_new_infection)
            t_initial = t_final
        # group new infections for every month:
        value_to_test = []
        outfile.write("running chi-squared test for actual vs expected new infections for {0} {1}-months time bins: \n"
                      "base_infectivity = {2}, immunity_acquisition_factor = {3}, decay rate = {4}.(unit is 1/years)\n".format(
            sft.MONTHS_IN_YEAR // number_of_month, number_of_month, base_infectivity, immunity_acquisition_factor, decay_rate))
        actual_new_infection = 0
        i = 0
        for t in range(start_day + 1, len(new_infection)):
            actual_new_infection += new_infection[t]
            i += 1
            if not i % (number_of_month * sft.DAYS_IN_MONTH):
                value_to_test.append(actual_new_infection)
                actual_new_infection = 0
        sft.plot_data(value_to_test, dist2=expected, label1="actual_new_infections",
                                   label2="expected_new_infection",
                                   title="actual vs. expected new infection for every {} month".format(number_of_month),
                                   xlabel="month", ylabel="# of new infections", category='actual_vs_expected_new_infections',
                                   show=True, line=False)
        result = sft.test_multinomial(dist=value_to_test, proportions=expected, report_file=outfile, prob_flag=False)

        if not result:
            success = False
            outfile.write(
                "BAD: The Chi-squared test for number of new infections in every {} months failed.\n".format(number_of_month))
        else:
            outfile.write( "GOOD: The Chi-squared test for number of new infections in every {} months passed.\n".format(
                    number_of_month))

        output_dict = parse_output_file(stdout_filename, debug)
        outfile.write("checking if all cores are reporting in every time step in stdout file:\n")
        core_list = [str(n) for n in (range(param_obj[KEY_NUM_CORES]))]
        for t, cores in output_dict.items():
            if core_list != sorted(cores):
                success = False
                outfile.write("BAD: at time step {0}, these cores reported to stdout.txt are: {1}, while "
                              "expected cores are: {2}.\n".format(t, cores, core_list))

        outfile.write(sft.format_success_msg(success))

    if debug:
        print( "SUMMARY: Success={0}\n".format(success) )
    return success

def application( output_folder="output", stdout_filename="test.txt", insetchart_name="InsetChart.json",
                 migration_report_filename="ReportHumanMigrationTracking.csv",
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
    param_obj = tms.load_emod_parameters([KEY_CONFIG_NAME, KEY_BASE_INFECTIVITY, KEY_IMMUNITY_ACQUISITION_FACTOR, KEY_DECAY_RATE,
                             KEY_DEMOGRAPHICS_FILENAMES, KEY_NUM_CORES, KEY_SIMULATION_TIMESTEP], config_filename, debug)

    demog_filename = param_obj[KEY_DEMOGRAPHICS_FILENAMES][-1]
    node_list = tms.get_node_counts(demog_filename)

    campaign_obj = load_campaign_file(campaign_filename, debug)

    migration_df = tms.load_migration_report(migration_report_filename, output_folder)

    report_data_obj = tms.parse_json_report([KEY_NEW_INFECTION], insetchart_name, output_folder, debug)

    create_report_file(param_obj, node_list, campaign_obj, migration_df, report_data_obj, stdout_filename, report_name, debug)

if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-j', '--jsonreport', default="InsetChart.json", help="Json report to load (InsetChart.json)")
    parser.add_argument('-m', '--migrationreport', default="ReportHumanMigrationTracking.csv", help="migration report to load (ReportHumanMigrationTracking.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-C', '--campaign', default="campaign.json", help="campaign name to load (campaign.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', default=False, help="Debug flag")

    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout, insetchart_name=args.jsonreport,
                migration_report_filename=args.migrationreport,
                config_filename=args.config, campaign_filename=args.campaign,
                report_name=args.reportname, debug=args.debug)
