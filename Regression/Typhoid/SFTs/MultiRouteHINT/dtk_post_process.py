#!/usr/bin/python

import re
import json
import math
import dtk_test.dtk_sft as sft

# global variables

def find_first( this_list ):
    for idx in range( len( this_list ) ):
        if this_list[idx]>0:
            return idx
    return len( this_list )

def application(report_file):
    sft.wait_for_done()

    # get params from config.json
    cdj = json.loads(open("config.json").read())["parameters"] 

    # I don't use most of these yet, but they're pretty standard boilerplate for SFTs
    start_time = cdj["Start_Time"]
    lines = []
    timestep = start_time
    with open("test.txt") as logfile:
        for line in logfile:
            if "Update(): Time:" in line:
                # calculate time step
                timestep += 1

    # This test uses the PropertyReport not stdout.
    prt_json = json.loads( open( "output/PropertyReportTyphoid.json" ).read() )
    geographic_zones = [ "A", "B", "C", "D", "E", "F", "G", "H", "I" ]
<<<<<<< HEAD
    # File looks like: "Channels": { "Contagion (Contact): Geographic:A": { "Data": [
    contact_chans = {}
    enviro_chans = {}
    for zone in geographic_zones:
        contact_chans[ zone ] = prt_json[ "Channels" ][ "Contagion (Contact): Geographic:" + zone ][ "Data" ]
        enviro_chans[ zone ] = prt_json[ "Channels" ][ "Contagion (Environment): Geographic:" + zone ][ "Data" ]
=======
    # File looks like: "Channels": { "Contagion (Contact):Geographic:A": { "Data": [
    contact_chans = {}
    enviro_chans = {}
    for zone in geographic_zones:
        contact_chans[ zone ] = prt_json[ "Channels" ][ "Contagion (Contact):Geographic:" + zone ][ "Data" ]
        enviro_chans[ zone ] = prt_json[ "Channels" ][ "Contagion (Environment):Geographic:" + zone ][ "Data" ]
>>>>>>> origin/Typhoid-Ongoing

    success = True
    # This test assumes a very specific scenario setup: Down the River from A to B.
    # Contact matrix is an identity matrix, local mixing only
    # Enviro matrix is A->B, B->C, C->D, etc.
    # Now let's check that the following qualitative facts are true:
    # All contact channels should have non-zero values, and start day should be in order
    # All but A enviro channels should have non-zero values, and start day should be in order
    if max( enviro_chans["A"] ) > 0:
        print( "We found some (enviro) contagion in zone A. Bad." )
        success = False
    
    last_cont_contagion_appearance_tstep = 0
    last_env_contagion_appearance_tstep = 0
    for zone in geographic_zones:
        if max( contact_chans[ zone ] ) == 0:
            print( "We didn't find any contagion in this zone. Bad." )
            success = False
        first_contagion_timestep = find_first( contact_chans[ zone ] )
        if first_contagion_timestep <= last_cont_contagion_appearance_tstep:
            print( "Contagion (contact) appeared in zone too early. Bad." )
            success = False
        last_cont_contagion_appearance_tstep = first_contagion_timestep 

    for zone in geographic_zones[1:]:
        if max( enviro_chans[ zone ] ) == 0:
            print( "We didn't find any contagion in this zone. Bad." )
            success = False
        first_contagion_timestep = find_first( enviro_chans[ zone ] )
        if first_contagion_timestep <= last_env_contagion_appearance_tstep:
            print( "Contagion (enviro) appeared in zone too early. Bad." )
            success = False
        last_env_contagion_appearance_tstep = first_contagion_timestep 

    with open(sft.sft_output_filename, "w") as report_file:
        #if len(lines) == 0:
            #success = False
            #report_file.write("BAD: Found no data matching test case.\n")
        #else:
        report_file.write(sft.format_success_msg(success))

        # I'm a big advocate of always-plot-somethign from SFT but I admit I can't figure out 
        # what to plot yet.
        #sft.plot_data(actual_infectiousness_data, expected_infectiousness_data, label1="TBD",
                      #label2="TBD", title="TBD",
                      #xlabel="TBD",
                      #ylabel="TBD",
                      #category='TBD')


if __name__ == "__main__":
    # execute only if run as a script
    application("")
