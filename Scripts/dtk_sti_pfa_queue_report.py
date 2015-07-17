#!/usr/bin/python

import json
import re

def init_variables( age_bins ):
    report = {}
    report["Channels"] = {}
    for sex in ( "Males", "Females" ):
        report["Channels"][ sex ] = {}
        report["Channels"][ sex ]["transitory"] = {}
        report["Channels"][ sex ]["informal"] = {}
        report["Channels"][ sex ]["marital"] = {}
        for age_bin in age_bins:
            report["Channels"][ sex ][ "transitory" ][ age_bin ] = []
            report["Channels"][ sex ][ "informal" ][ age_bin ] = []
            report["Channels"][ sex ][ "marital" ][ age_bin ] = []

    return report

def application():
    time = 1
    age_bins = []
    age_bins.append( "15.0-17.5" )
    age_bins.append( "17.5-20.0" )
    age_bins.append( "20.0-22.5" )
    age_bins.append( "22.5-25.0" )
    age_bins.append( "25.0-27.5" )
    age_bins.append( "27.5-30.0" )
    age_bins.append( "30.0-32.5" )
    age_bins.append( "32.5-35.0" )
    age_bins.append( "35.0-37.5" )
    age_bins.append( "37.5-40.0" )
    age_bins.append( "40.0-42.5" )
    age_bins.append( "42.5-45.0" )
    age_bins.append( "45.0-47.5" )
    age_bins.append( "47.5-50.0" )
    age_bins.append( "50.0-52.5" )
    age_bins.append( "52.5-55.0" )
    age_bins.append( "55.0-57.5" )
    age_bins.append( "57.5-60.0" )
    age_bins.append( "60.0-62.5" )
    age_bins.append( "62.5-65.0" )

    report = init_variables( age_bins )

    with open( "stdout.txt" ) as logfile:
        for line in logfile:
            if "Time:" in line:
                time = time + 1
            elif "queue lengths" in line: 
                type = line.split()[5]
                if "female" in line:
                    data = re.sub( '.*:.*\[', '', line )
                    data = re.sub( '\]', '', data )
                    for idx in range(len(data.split())):
                        report["Channels"]["Females"][ type ][ age_bins[idx] ].append( int( data.split()[idx] ) )
                elif "male" in line:
                    data = re.sub( '.*:.*\[', '', line )
                    data = re.sub( '\]', '', data )
                    for idx in range(len(data.split())):
                        report["Channels"]["Males"][ type ][ age_bins[idx] ].append( int( data.split()[idx] ) )

    #print( json.dumps( report, indent=4, sort_keys = True ) )
    with open( "output/pfa_lengths.json", "w" ) as pfa_out:
        pfa_out.write( json.dumps( report, indent=4, sort_keys = True ) )
