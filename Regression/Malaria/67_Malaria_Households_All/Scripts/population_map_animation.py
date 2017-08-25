import sys, os, json, collections
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from population_common import *


def ShowUsage():
    print ('\nUsage: %s [demog.json] [ReportVectorStats.csv] [IsVectorPopulation: 0 or 1]' % os.path.basename(sys.argv[0]))


def ReportVectorStats_CreateTimeMap( all_data_list ):
    time_data_list = collections.OrderedDict([])
    prev_time = -1
    data_list = []
    for data in all_data_list:
        if( prev_time != data.time ):
            if( prev_time != -1 ):
                time_data_list[ prev_time ] = data_list
                data_list = []
            prev_time = data.time
        data_list.append( data )
    return time_data_list

class AnimationData:
    def __init__(self):
        self.time_list_index = 0
        self.time_list = []
        self.time_data_list = collections.OrderedDict([])
        self.is_vectors = False
        self.lon = []
        self.lat = []

    def NextTimeStep( self, pop ):
        if( self.time_list_index >= len(self.time_list) ):
            return -1.0

        time = self.time_list[ self.time_list_index ]
        self.time_list_index += 1

        for node_data in self.time_data_list[ time ]:
            i_lat = self.lat.index( node_data.latitude )
            j_lon = self.lon.index( node_data.longitude )
            if( self.is_vectors ):
                pop[ i_lat ][ j_lon ] = float(node_data.vector_population)
            else:
                pop[ i_lat ][ j_lon ] = float(node_data.population)
            #print time, node_data.node_id, i_lat, j_lon, pop[ i_lat ][ j_lon ]
        return time

def Update( i, animation_data, ax ):
    pop = [[float(0.0) for x in range(len(animation_data.lat))] for x in range(len(animation_data.lon))] 
    time = animation_data.NextTimeStep( pop )
    if( time == -1 ):
        return
    levels = GetLevels( animation_data.is_vectors )
    ax.cla()
    cp = ax.contourf( animation_data.lon, animation_data.lat, pop, levels, extend='both' )
    UpdateTitle( animation_data.is_vectors, time )
    return cp

def PlotContour( animation_data ):
    plt.xlabel('Longitude')
    plt.ylabel('Latitude')

    UpdateTitle( animation_data.is_vectors, 0.0 )

    levels = GetLevels( animation_data.is_vectors )

    pop = [[float(0.0) for x in range(len(animation_data.lat))] for x in range(len(animation_data.lon))] 

    fig = plt.figure()
    cp = plt.contourf( animation_data.lon, animation_data.lat, pop, levels, extend='both' )
    ax = fig.gca()
    ani = animation.FuncAnimation( fig, Update, fargs=(animation_data, ax), interval=50 )
    plt.colorbar( cp )
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) != 4:
        ShowUsage()
        exit(0)

    animation_data = AnimationData()

    demog_fn               =      sys.argv[1]
    report_vector_stats_fn =      sys.argv[2]
    is_vectors           = int( sys.argv[3] ) == 1

    animation_data.is_vectors = is_vectors

    node_data_map = GetInitialNodeData( demog_fn )
    ExtractLongitudeLatitude( node_data_map, animation_data.lon, animation_data.lat )

    all_data_list = ReportVectorStats_Read( report_vector_stats_fn, node_data_map )

    animation_data.time_data_list = ReportVectorStats_CreateTimeMap( all_data_list )

    animation_data.time_list = animation_data.time_data_list.keys()
    animation_data.time_list.sort()

    PlotContour( animation_data )

