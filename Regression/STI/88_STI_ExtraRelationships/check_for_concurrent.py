#!/usr/bin/python

# --------------------------------------------------------------------------------------
# The purpose of the script is to use the RelationshipStart.csv and RelationshipEnd.csv
# output files to determine if there are concurrent relationships. In particular,
# it looks for ones where the Risk property of the both individuals is LOW.
# --------------------------------------------------------------------------------------

import os
import json
import argparse

REL_ID_INDEX = 0
START_INDEX = 1
TYPE_INDEX =3
MALE_INDEX = 6
FEMALE_INDEX = 21
MALE_IP_INDEX = 36
FEMALE_IP_INDEX = 37

def GetRiskValue( ip_string ):
    kv_pairs = ip_string.split( ';' )
    for kv in kv_pairs:
        key_value = kv.split( '-' )
        if( key_value[0] == "Risk" ):
            return key_value[1]
    raise exception( "Risk not found in: " + ip_string )

class Relationship:

    def __init__(self,line_data):
        self.rel_id      = int(line_data[ 0])
        self.rel_type    = int(line_data[ 3])
        self.male_id     = int(line_data[ 6])
        self.female_id   = int(line_data[21])
        self.start       = float(line_data[1])
        self.end         = -1.0
        self.male_risk   = GetRiskValue( line_data[36] )
        self.female_risk = GetRiskValue( line_data[37] )
        
    def ReadEndData( self, line_data ):
        self.end = float(line_data[4])

        
def ReadStartFile( output_dir ):
    filename = os.path.join( output_dir, "RelationshipStart.csv" )
    if( os.path.isfile( filename ) == False ):
        raise exception( filename + " not found" )
    
    rel_map = {}
    
    with open( filename ) as start_file:
        start_file.readline() # skip header
        for line in start_file:
            line_data = line.split(',')
            rel = Relationship( line_data )
            rel_map[rel.rel_id] = rel
    
    return rel_map
            
def ReadEndFile( output_dir, rel_map ):
    filename = os.path.join( output_dir, "RelationshipEnd.csv" )
    if( os.path.isfile( filename ) == False ):
        raise exception( filename + " not found" )
    
    with open( filename ) as end_file:
        end_file.readline() # skip header
        for line in end_file:
            line_data = line.split(',')
            rel_id = int(line_data[0])
            if( rel_id in rel_map ):
                rel_map[ rel_id ].ReadEndData( line_data )
            else:
                raise excpetion( str(rel_id) + " from end file not found in start" )

def CheckForConcurrent( rel_map ):

    # find all of the relationships for each individual
    rels_by_id = {}
    for rel in rel_map.values():
        if( rel.male_id not in rels_by_id ):
            rels_by_id[ rel.male_id ] = []
        rels_by_id[ rel.male_id ].append( rel )

        if( rel.female_id not in rels_by_id ):
            rels_by_id[ rel.female_id ] = []
        rels_by_id[ rel.female_id ].append( rel )
        
    # go through each person and see if they have a relationship that overlaps with another
    num_concurrent = 0
    for human_id in rels_by_id.keys():
        rel_list = rels_by_id[ human_id ]
        
        prev_rel = None
        for rel in rel_list:
            if( prev_rel == None ):
                prev_rel = rel
            elif( prev_rel.start > rel.start ):
                raise exception( "rels not sorted" )
            elif( prev_rel.end > rel.start ):
                if( ((prev_rel.male_risk == "LOW") and (prev_rel.female_risk == "LOW")) and ((rel.male_risk == "LOW") and (rel.female_risk == "LOW")) ):
                    #if( prev_rel.rel_type == rel.rel_type ):
                    #if( prev_rel.start > 40000 ):
                    print "Overlap between relationships " + str(prev_rel.rel_id) + " and " + str(rel.rel_id) + ":" + prev_rel.male_risk +"-"+ prev_rel.female_risk +"|"+ rel.male_risk +"-"+ rel.female_risk +"||"+str(prev_rel.end)+">"+str(rel.start)+"||"+str(prev_rel.rel_type)+","+str(rel.rel_type)
                    num_concurrent += 1
    
    print "num_concurrent = "+str(num_concurrent)
    
            
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('output_dir', help='Directory with start and relationship files')
    args = parser.parse_args()

    print "---- Read Start ----"
    rel_map = ReadStartFile( args.output_dir )
    
    print "---- Read End ----"
    ReadEndFile( args.output_dir, rel_map )
    print "Num rels="+str(len(rel_map))
    
    print "---- Check For Concurrent Relationships ----"
    CheckForConcurrent( rel_map )
    