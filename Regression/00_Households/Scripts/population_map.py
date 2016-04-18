import sys, os, json, collections
import numpy as np
import matplotlib.pyplot as plt
from population_common import *


def ShowUsage():
    print ('\nUsage: %s [demog_fn] [ReportVectorStats.csv] [IsVectorPopulation: 0 or 1]' % os.path.basename(sys.argv[0]))

def ReportVectorStats_AvgPopPerNode( data_list, is_vectors ):
    node_avg_pop_map = collections.OrderedDict([])

    num_timesteps = 0
    prev_time = -1
    for data in data_list:
        if( not node_avg_pop_map.has_key( data.node_id ) ):
            node_avg_pop_map[ data.node_id ] = float(0)
        
        if( is_vectors ):
            node_avg_pop_map[ data.node_id ] += float(data.vector_population)
        else:
            node_avg_pop_map[ data.node_id ] += float(data.population)

        if( prev_time != data.time ):
            prev_time = data.time
            num_timesteps += 1

    if( num_timesteps > 0 ):
        for node_id in node_avg_pop_map.keys():
            node_avg_pop_map[ node_id ] = node_avg_pop_map[ node_id ] / float( num_timesteps )

    return node_avg_pop_map

def PlotContour( lon, lat, pop, is_vectors ):
    plt.xlabel('Longitude')
    plt.ylabel('Latitude')

    UpdateTitle( is_vectors, -1.0 )

    levels = GetLevels( is_vectors )

    cp = plt.contourf( lon, lat, pop, levels, extend='both' )
    plt.colorbar( cp )
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) != 4:
        ShowUsage()
        exit(0)

    demog_fn               =      sys.argv[1]
    report_vector_stats_fn =      sys.argv[2]
    is_vectors           = int( sys.argv[3] ) == 1

    node_data_map = GetInitialNodeData( demog_fn )
    lon = []
    lat = []
    ExtractLongitudeLatitude( node_data_map, lon, lat )

    all_data_list = ReportVectorStats_Read( report_vector_stats_fn, node_data_map )

    node_avg_pop_map = ReportVectorStats_AvgPopPerNode( all_data_list, is_vectors )

    pop = [[float(0.0) for x in range(len(lat))] for x in range(len(lon))] 

    node_id_list = node_avg_pop_map.keys()
    node_id_list.sort()
    for node_id in node_id_list:
        i_lat = lat.index( node_data_map[ node_id ].latitude )
        j_lon = lon.index( node_data_map[ node_id ].longitude )
        pop[ i_lat ][ j_lon ] = node_avg_pop_map[ node_id ]

    PlotContour( lon, lat, pop, is_vectors )

