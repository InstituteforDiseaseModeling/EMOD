#!/usr/bin/python

import os
import json

def application():
    """Purpose:
    This script is a DTK post-processing validation test for STI_SIM and its 
    derivatives.

    Assumptions/Inputs:
    It assumes the existence of the RelationshipStart.csv, RelationshipEnd.csv, 
    and TransmissionReport.csv, all in the output directory. 

    Outputs:
    It attempts to ensure that a reported transmission of disease only occurs 
    between two individuals who are actually in a relationship at that timestep. 
    It reports its findings as a boolean in a small json-formatted file it writes 
    into the output directory, with the same name as the script itself.

    How-To Use:
    This script can be invoked directly from the DTK if it is renamed to 
    dtk_post_process.py, or can be imported from a script with that name and 
    invoked by calling this application method.
    """

    rels = {}
    current_timestep = 0
    formation_timeline = {}
    #print( "You are here! " + os.getcwd() )

    if os.path.isfile( "output/RelationshipStart.csv" ) == False or os.path.isfile( "output/RelationshipEnd.csv" ) == False or os.path.isfile( "output/TransmissionReport.csv" ) == False:
        return

    with open( "output/RelationshipStart.csv" ) as rs:
        rs.readline() # chew up header
        for line in rs:
            #awk -F, '{ print $2"a" ,":", $6, "(inf="$7") united with", $23,"(inf="$24")" }' RelationshipStart.csv >> epi_story 
            line_array = line.split( ',' )
            read_tstep = int(line_array[1])
            male = int(line_array[5])
            female = int(line_array[22])
            #print "At time {0} person {1} united with person {2}.".format( read_tstep, male, female )
            new_rel = {}
            new_rel[ male ] = female
            if read_tstep not in formation_timeline: 
                formation_timeline[ read_tstep ] = []
            formation_timeline[ read_tstep ].append( new_rel )

    dissolution_timeline = {}
    with open( "output/RelationshipEnd.csv" ) as re:
        re.readline() # chew up header
        for line in re:
            #awk -F, '{ print $4"c" ,":", $6, "broke up with", $7 }' RelationshipEnd.csv >> epi_story
            line_array = line.split( ',' )
            read_tstep = int(line_array[3])
            male = int(line_array[5])
            female = int(line_array[6])
            #print "At time {0} person {1} left person {2}.".format( read_tstep, male, female )
            dead_rel = {}
            dead_rel[ male ] = female
            if read_tstep not in dissolution_timeline: 
                dissolution_timeline[ read_tstep ] = []
            dissolution_timeline[ read_tstep ].append( dead_rel )

    pass_test = True
    with open( "output/TransmissionReport.csv" ) as tx:
        active_rels = {}
        tx.readline() # chew up  header
        for line in tx:
            #awk -F, '{ print $1"b" ,":", $3, "infected", $18 }' TransmissionReport.csv >> epi_story
            line_array = line.split( ',' )
            read_tstep = int(line_array[0])
            # fast-forward timeline to current tstep
            infector = int(line_array[2])
            infectee = int(line_array[17])
            #print "At time {0} person {1} infected person {2}.".format( read_tstep, infector, infectee )
            import pdb
            #pdb.set_trace()
            while current_timestep <= read_tstep:
                if current_timestep in formation_timeline:
                    for rel in formation_timeline[ current_timestep ]: # array
                        #print( rel )
                        male = rel.keys()[0]
                        female = rel[male]
                        if male not in active_rels:
                            active_rels[ male ] = []
                        active_rels[ male ].append( female )
                current_timestep = current_timestep + 1

            # check that infector is currently in relationship with infectee
            #print( "Check for valid transmission on active_rels of size " + str( len( active_rels ) ) )
            valid = False
            for male in active_rels:
                if male == infector or male == infectee:
                    females = active_rels[ male ]
                    for female in females:
                        if female == infector or female == infectee:
                            valid = True
                            break
                if valid == True:
                    break
            if valid == False:
                print( "FAIL! Person {0} does not seem to be in relationship with person {1} at this time.".format( infector, infectee ) )
                pass_test = False
            #else:
                #print( "PASSED.".format( infector, infectee ) )

            # Purge dead rels from active _rels
            for tstep in range( 0,current_timestep ):
                if tstep in dissolution_timeline:
                    for rel in dissolution_timeline[ tstep ]: # array
                        #print( "Found relationship to dissolve at time " + str( tstep ) )
                        ##print( rel )
                        male = rel.keys()[0]
                        female = rel[male]
                        #print( "Purge relationship between {0} and {1}.".format( male, female ) )
                        active_rels[ male ].remove( female )
                        if len( active_rels[ male ] ) == 0:
                            del( active_rels[ male ] )
                    del( dissolution_timeline[ tstep ] )

   
    print( "pass_test = {0}".format( pass_test ) )
    report = json.loads( "{}" )
    report_name = str( os.path.basename( __file__ ) ).split('.')[0]
    report["Channels"] = {} # works nicely with regression validation this way
    report["Channels"]["Report_Name"] = report_name
    report["Channels"]["Report_Status"] = "{0}".format( pass_test )
    with open( "output/" + report_name + ".json", "w" ) as output_json:
        output_json.write( json.dumps( report, sort_keys=True, indent=4 ) )
