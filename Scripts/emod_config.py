import os
import json
import regression_utils

def doSomething( filename ):
    print( "In doSomething!" )
    retVal = json.loads( open( filename ).read() ) 
    if retVal.has_key( "parameters" ): # is this a config.json?
        regression_utils.flattenConfig( filename ) # saves to config_flat.json
        retVal = json.loads( open( "config_flat.json" ).read() ) 
        retVal[ "parameters" ][ "logLevel_Pool" ] = "WARN"
        retVal[ "parameters" ][ "logLevel_NodeTB" ] = "DEBUG"
        retVal[ "parameters" ][ "logLevel_ReportTB" ] = "DEBUG"
        retVal[ "parameters" ][ "Simulation_Duration" ] = 20
        retVal[ "parameters" ][ "Simulation_Type" ] = "TB_SIM"
    return json.dumps( retVal )
