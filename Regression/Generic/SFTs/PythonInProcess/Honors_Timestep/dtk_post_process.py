#!/usr/bin/python

import dtk_test.dtk_sft as sft
import json
with open("config.json") as infile:
    run_number=json.load(infile)['parameters']['Run_Number']

"""
Adds an outbreak event to the simulation at the day
specified by the 'Build_Campaign_Event_At' parameter in config.json
1. dtk_pre_process parses config.json and 
1.1 adds the event_template to the config.json file
1.2 reads the 'Build_Campaign_Event_At' parameter out of config.json and
1.2.1 writes this as a constant into dtk_in_process.py
2. dtk_in_process receives a timestep string from Eradication and
2.1 Checks to see if this is equal to the constant that dtk_pre_process wrote into here
2.1.1 And if it is, reads the config.json to write a campaign event for an outbreak on the next day
3. dtk_post_process parses the InsetChart.json, and
3.1 Makes sure that there are no infections before the event day and
3.2 There are some infected on the event day
"""


def application(output_folder="output", config_filename="config.json",
                jsonreport_name="InsetChart.json", stdout_filename="test.txt",
                report_name=sft.sft_output_filename, debug=False):
    if debug:
        print("output_folder: " + output_folder + "\n")
        print("config_filename: " + config_filename + "\n")
        print("insetchart_name: " + jsonreport_name + "\n")
        print("stdout filename:" + stdout_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    config_params = None
    with open(config_filename) as infile:
        config_params = json.load(infile)['parameters']
        pass

    start_day = config_params['Build_Campaign_Event_At']
    last_timestep = config_params['Simulation_Duration']
    timestep_size = config_params['Simulation_Timestep']
    start_day_index = start_day // timestep_size

    import os.path as path

    # Verify that there are no infections until start_day
    inset_path = path.join(output_folder, jsonreport_name)
    with open (inset_path) as infile:
        inset_json = json.load(infile)
        pass
    infected_channel = inset_json["Channels"]["Infected"]["Data"]
    total_days = len(infected_channel)
    messages = []
    it_works = False
    first_infection_day = start_day//timestep_size + 1 + 1
    messages.append(f"TEST: this should all add up to zero: {infected_channel[0:first_infection_day]}\n")
    if sum(infected_channel[0:first_infection_day]) == 0:
        it_works = True  # should be no infections
        pass
    messages.append(f"Expected 0 infections until day {first_infection_day}. This worked: {it_works}.\n")
    outbreak_day_infected = infected_channel[first_infection_day]
    got_infections = False
    if outbreak_day_infected> 0:
        got_infections = True
        pass
    messages.append(f"Expected infections on day {start_day + 1 + timestep_size}. Infected channel had {outbreak_day_infected}.\n")
    if not got_infections:
        it_works = False
        pass

    # Verify that simulation timestep honored
    update_prefix = "Update(): Time: "
    inproc_prefix = "Hello from timestep "
    with open (stdout_filename) as infile:
        stdout = infile.readlines()
        pass
    last_update = 0
    last_hello = None

    for line in stdout:
        if update_prefix in line:
            last_update = int(float(line.split(update_prefix)[1].split()[0]))
            pass
        elif inproc_prefix in line:
            last_hello = int(line.split(inproc_prefix)[1])
            if last_hello != last_update-timestep_size:
                messages.append(f"BAD: after timestep {last_update}, dtk_in_proc get time {last_hello}\n")
                it_works = False
            if last_hello % timestep_size != 0:
                messages.append(f"BAD: dtk_in_proc got timestep {last_hello} which is not divisible by timestep size {timestep_size}\n")
                it_works = False


    with open(report_name,'w') as outfile:
        outfile.writelines(messages)
        outfile.write(sft.format_success_msg(it_works))
    pass


if __name__ == "__main__":
    # execute only if run as a script
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-j', '--jsonreport', default="InsetChart.json", help="Json report to load (InsetChart.json)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output, config_filename=args.config,
                jsonreport_name=args.jsonreport,
                report_name=args.reportname, debug=args.debug)
    pass

