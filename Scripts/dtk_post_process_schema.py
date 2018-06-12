#!/usr/bin/python

import json
import sys
import os

idm_type_schemas = {}

def recurser( in_json ):
    #print( str(in_json) )
    list_of_arrays_found = []
    #for a, v in in_json.iteritems():
    nuke_these = []
    # iterate over all the keys (for object) or items (for arrays)
    for a in in_json:

        value = None
        if isinstance( in_json, list ):
            value = a
        else:
            value = in_json[a]

        #if not isinstance( a, dict ):
            if a.startswith( "idmType:" ): # and isinstance( in_json[a], dict ) and str(in_json[a].keys()[0]) != "base":
                # Don't append this if it's a 'base' type thing
                if not isinstance( in_json[a], dict ) or ( isinstance(in_json[a],dict) and 'base' not in in_json[a].keys() ):
                    idm_type_schemas[a] = in_json[a]
                    #print( "Found an idmType to extract with key = " + str(a) + ": " )
                    #print( str( in_json[a] ) )
                    #print( json.dumps( idm_type_schemas[a], sort_keys=True, indent=4 ) )
                nuke_these.append( a )
        #else:
            #print( "key is a dict: " + str( a ) )

        if json.dumps( value ).startswith( "{" ):
            #print( "Object" )
            v = recurser( value )
        elif json.dumps( value ).startswith( "[" ):
            #print( "Array" )
            #print( str( a ) )
            if a != "enum":
                try:
                    key_to_add = str(a) + ".Metadata"
                    list_of_arrays_found.append( key_to_add )
                    #print( key_to_add )
                except Exception as ex:
                    print( str( ex ) )
                v = recurser( value )
        else:
            #print( "Leaf:" + a )
            if isinstance( in_json, dict ):
                if in_json[a] == "idmType:Intervention":
                    in_json[a] = "idmAbstractType:Intervention"
                if in_json[a] == "idmType:IndividualIntervention":
                    in_json[a] = "idmAbstractType:IndividualIntervention"
                if in_json[a] == "idmType:NodeIntervention":
                    in_json[a] = "idmAbstractType:NodeIntervention"
                if in_json[a] == "idmType:EventCoordinator":
                    in_json[a] = "idmAbstractType:EventCoordinator"
                if in_json[a] == "idmType:CampaignEvent":
                    in_json[a] = "idmAbstractType:CampaignEvent"
                elif in_json[a] == "idmType:NodeSet":
                    in_json[a] = "idmAbstractType:NodeSet"

    # purge the idmType nodes from their original locations all over the place
    #for array_key in list_of_arrays_found:
        #in_json[ array_key ] = "IsArray=true"

    for dead in nuke_these:
        del( in_json[dead] )

    return in_json

def application( schema_file ):
    print( schema_file )
    with open( schema_file ) as f:
        schema = json.loads( f.read() )
    #print( str(schema) )
    for a, v in schema.items():
        print( str(a) )
        if json.dumps( schema[a] ).startswith( "{" ):
            print( "is Object" )
            #recurser( schema[a] )
            v = recurser( v )
        elif json.dumps( schema[a] ).startswith( "[" ):
            print( "is (top level) Array" )
            schema[ str(a) + ".Metadata"] = "IsArray=true"
            for a,v in v.items():
                v = recurser( v )
        else:
            print( "Leaf" )
        #print( str(a), str(v) )

    # go through interventions and sort by node vs individual
    node_interventions = {}
    individual_interventions = {}

    schema["idmTypes"] = idm_type_schemas

    #print( str( schema["idmTypes"].keys() ) )
    interventions_json = schema["idmTypes"]["idmType:Intervention"]
    for iv_class_name in interventions_json:
        #print( str( interventions_json[ iv_class_name ][ "iv_type" ] ) )
        if interventions_json[ iv_class_name ][ "iv_type" ] == "NodeTargeted":
            node_interventions[ iv_class_name ] = interventions_json[ iv_class_name ]
        else:
            individual_interventions[ iv_class_name ] = interventions_json[ iv_class_name ]
    del( schema["idmTypes"]["idmType:Intervention"] )
    schema["idmTypes"]["idmAbstractType:Intervention"] = {}
    schema["idmTypes"]["idmAbstractType:Intervention"]["idmAbstractType:IndividualIntervention"] = individual_interventions
    schema["idmTypes"]["idmAbstractType:Intervention"]["idmAbstractType:NodeIntervention"] = node_interventions

    schema["idmTypes"]["idmAbstractType:EventCoordinator"] = schema["idmTypes"]["idmType:EventCoordinator"]
    del( schema["idmTypes"]["idmType:EventCoordinator"] )

    schema["idmTypes"]["idmAbstractType:CampaignEvent"] = schema["idmTypes"]["idmType:CampaignEvent"]
    del( schema["idmTypes"]["idmType:CampaignEvent"] )

    schema["idmTypes"]["idmAbstractType:NodeSet"] = schema["idmTypes"]["idmType:NodeSet"]
    del( schema["idmTypes"]["idmType:NodeSet"] )

    os.remove( schema_file )
    with open( schema_file, "w" ) as f:
        f.write( json.dumps( schema, sort_keys=True, indent=4 ) )

#application( sys.argv[1] )

#print( str( idm_type_schemas ) )
