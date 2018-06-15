#!/usr/bin/python

import json
import os.path as path
import dtk_sft
import pdb

matches = ["Demo coverage for individual"]

KEY_TOTAL_TIMESTEPS = "Simulation_Duration"

"""
GroupEventCoordinatorHIV can restrict coverage of HIVCoInfections based on 3 axes of 
demographic properties (including sim time): by age and sex.
"""

def has_match(target):
    for match in matches:
        if match in target:
            return True
    return False


def load_emod_parameters(config_filename="config.json"):
    """reads config file and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_START_TIMESTEP, etc., keys (e.g.)
    """
    cdj = None
    with open(config_filename) as infile:
        cdj = json.load(infile)["Events"]
    param_obj = {}
    param_obj["Start_Day"] = cdj[0]["Start_Day"]
    param_obj["Period"] = cdj[0]["Event_Coordinator_Config"]["Timesteps_Between_Repetitions"]
    return param_obj


def parse_stdout_file(curr_timestep, stdout_filename="test.txt", debug=False):
    """creates a dictionary of simday-to-infection probs for males and females.

    :param curr_timestep:   first timestep from config
    :param stdout_filename: file to parse (test.txt)
    :return:                infected_mosquitoes_per_day dictionary, total_infections int
    """

    filtered_lines = []
    with open(stdout_filename) as logfile:
        for line in logfile:
            if has_match(line):
                filtered_lines.append(line)
    if debug:
        with open("filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)


    # initialize variables
    male_probs = {}
    female_probs = {}

    if len( filtered_lines ) == 0:
        print( "ERROR: no matches found in logfile for lines of interest. Configuration or regex failure." )

    for line in filtered_lines:
        # 00:00:33 [0] [D] [GroupEventCoordinatorHIV] demo coverage for individual sex 1, time 366.000000, age 13025.565430 from draw = 0.000000. 
        #pdb.set_trace()
        split_line = line.split()
        sex = int(split_line[ 9 ].strip(','))
        time = float( split_line[ 14 ].strip(',') )
        #age = line[ split_13 ]
        prob = float(split_line[ 16 ].strip('.') )
        if sex == 0:
            if time not in male_probs:
                male_probs[ time ] = []
            male_probs[ time ].append( prob )
        else:
            if time not in female_probs:
                female_probs[ time ] = []
            female_probs[ time ].append( prob )
    return male_probs, female_probs

def parse_json_report(start_time, output_folder="output", insetchart_name="InsetChart.json", debug=False):
    """creates inset_days structure with "Infectivity" and "Bites" keys

    :param insetchart_name: InsetChart.json file with location (output/InsetChart.json)
    :returns: inset_days structure
    """
    insetchart_path = path.join(output_folder, insetchart_name)
    with open(insetchart_path) as infile:
        icj = json.load(infile)["Channels"]

    prevalence = icj["Infected"]["Data"]
    end_time = start_time + len(prevalence)
    inset_days = {}
    for x in range(start_time, end_time):
        inset_days[x] = x

    if debug:
        with open("inset_days.json", "w") as outfile:
            json.dump(inset_days, outfile, indent=4)

    return inset_days


def create_report_file( male_probs,
                        female_probs, 
                        param_obj, 
                        report_name):

    # Let's make sure all male probs for first tstep are 0 and all for
    # second tstep are 1. All females should be 0. TBD: Read & calc these from demog!
    error = False
    for tstep in male_probs:
        for val in male_probs[tstep]:
            if val != 0:
                print( "Found any non-zero prob for males." )
                error = True

    start_day = param_obj[ "Start_Day" ]
    period = param_obj[ "Period" ]

    if start_day in female_probs:
        for val in female_probs[ start_day ]:
            if val != 0:
                print( "Found non-zero prob for females at start_day. Should be 0 then." )
                error = True
    else:
        print( "Found no values for females at start_day." )
        error = True

    if error == False:
        if (start_day+period) in male_probs:
            for val in male_probs[ start_day + period ]:
                if val != 0:
                    error = True
        if (start_day+period) in female_probs:
            for val in female_probs[ start_day + period ]:
                if val != 1:
                    error = True
        else:
            error = True

    with open(report_name, "w") as outfile:
        #outfile.write("Total infections: {0} \n".format(44444))
        #expected_infections = {}
        #actual_infections = {}
        total_error = 0.0

        success = True

        error_tolerance = 0.00# * total_timesteps
        #outfile.write("Error tolerance is {0} with {1} timesteps\n".format(error_tolerance, total_timesteps))
        if error:
            success = False
            outfile.write("BAD: Total error is {0}, error tolerance is {1}\n".format(error, error_tolerance))

        outfile.write("SUMMARY: Success={0}\n".format(success))


def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json",
                 insetchart_name="InsetChart.json",
                 report_name=dtk_sft.sft_output_filename,
                 debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "insetchart_name: " + insetchart_name + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )
    dtk_sft.wait_for_done()
    #param_obj = load_emod_parameters("HIVMaleOverlay.json")
    param_obj = load_emod_parameters("campaign.json")
    #total_timesteps = param_obj[KEY_TOTAL_TIMESTEPS]

    start_timestep = 0 # get from config

    # Now process log output (probably) and compare to theory (not in this example) or to another report.
    male_probs, female_probs = parse_stdout_file(start_timestep, stdout_filename, debug)
    inset_days = parse_json_report(start_timestep, output_folder, insetchart_name, debug)
    create_report_file( male_probs, female_probs, param_obj, report_name )


if __name__ == "__main__":
    # execute only if run as a script
    application( "output" )
