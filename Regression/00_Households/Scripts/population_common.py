import sys, os, json, collections
import numpy as np
import matplotlib.pyplot as plt


#Time, NodeID, Population, VectorPopulation, AdultCount, InfectedCount, InfectiousCount, MaleCount, NewEggsCount, IndoorBitesCount, OutdoorBitesCount, VectorNotMigrateCount, VectorNotMigrateProb, MigrationFromCountLocal, MigrationFromCountRegional, arabiensis_AvailableHabitat, funestus_AvailableHabitat, gambiae_AvailableHabitat, arabiensis_EggCrowdingCorrection, funestus_EggCrowdingCorrection, gambiae_EggCrowdingCorrection

class NodeData:
    def __init__(self):
        self.time      = -1
        self.node_id  = -1
        self.population = -1
        self.vector_population = -1
        self.longitude = 0.0
        self.latitude = 0.0

def ReportVectorStats_Read( input_fn, node_data_map ):
    input_file = open(input_fn,"r")

    header = input_file.readline()

    data_list = []

    for line in input_file:
        line = line.replace('\n','')
        line_array = line.split( ',' )

        data = NodeData()
        data.time              = float( line_array[0] )
        data.node_id           = int(   line_array[1] )
        data.population        = int(   line_array[2] )
        data.vector_population = int(   line_array[3] )

        data.longitude = node_data_map[ data.node_id ].longitude
        data.latitude  = node_data_map[ data.node_id ].latitude

        data_list.append( data )

    input_file.close()

    return data_list

def GetInitialNodeData( demog_fn ):
    demog_file = open(demog_fn,'r')
    demogjson = json.load( demog_file )
    demog_file.close()

    node_data_map = collections.OrderedDict([])
    for node in demogjson['Nodes']:
        data = NodeData()
        data.node_id           = int( node['NodeID'] )
        data.longitude         = float( node['NodeAttributes']['Longitude'         ] )
        data.latitude          = float( node['NodeAttributes']['Latitude'          ] )
        data.population        = int(   node['NodeAttributes']['InitialPopulation' ] )
        data.vector_population = int(   node['NodeAttributes']['InitialVectors'    ] )
        node_data_map[ data.node_id ] = data
    return node_data_map

def ExtractLongitudeLatitude( node_data_map, lon, lat ):
    lon_map = collections.OrderedDict([])
    lat_map = collections.OrderedDict([])

    for node_id in node_data_map.keys():
        node_data = node_data_map[ node_id ]

        if( not lon_map.has_key( node_data.longitude ) ):
            lon_map[ node_data.longitude ] = node_data.longitude

        if( not lat_map.has_key( node_data.latitude ) ):
            lat_map[ node_data.latitude ] = node_data.latitude

    tmp_lon = lon_map.keys()
    tmp_lat = lat_map.keys()
    tmp_lon.sort()
    tmp_lat.sort()

    for val in tmp_lon:
        lon.append( val )

    for val in tmp_lat:
        lat.append( val )


def UpdateTitle( is_vectors, time ):
    title_str = ""
    if( is_vectors ):
        title_str = "Average Vector Population Map"
    else:
        title_str = "Average Population Map"
    if( time >= 0.0 ):
        title_str += " - Time = " + str(time)
    plt.title( title_str )

def GetLevels( is_vectors ):
    levels = []
    if( is_vectors ):
        levels = [0, 2500, 5000, 7500, 10000, 12500, 15000, 17500, 20000, 22500, 25000, 27500, 30000 ]
    else:
        levels = [0, 5, 10, 15, 20]
    return levels

