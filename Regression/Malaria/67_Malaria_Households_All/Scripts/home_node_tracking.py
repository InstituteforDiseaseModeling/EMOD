import sys, os, json, collections
import matplotlib.pyplot as plt
from operator import attrgetter
from human_tracking_common import *

def PlotData( home_node_id, time_data, location_data, legend_data ):
    for i in range(len(time_data)):
        plt.plot( time_data[i], location_data[i])
    plt.title( "Migration of People With Home Node = " + str( home_node_id ) )
    plt.legend( legend_data )
    plt.show()


def ExtractData( data_map, home_node_id, specific_human_id, time_data, location_data, legend_data ):
    human_id_list = data_map.keys()
    human_id_list.sort()

    for human_id in human_id_list:
        if( (specific_human_id == -1) or (specific_human_id == human_id) ):
            human_data = data_map[ human_id ]
            if( human_data[0].home_node == home_node_id ):
                first = True
                human_time_data = []
                human_location_data = []
                for data in human_data:
                    if( first ):
                        first = False
                        human_time_data.append( 0 )
                        human_location_data.append( data.from_node )

                    human_time_data.append( data.time )
                    human_location_data.append( data.from_node )
                    human_time_data.append( data.time )
                    human_location_data.append( data.to_node )
                time_data.append( human_time_data )
                location_data.append( human_location_data )
                legend_data.append( "Human_ID = " + str(human_id) )


if __name__ == "__main__":
    if( (len(sys.argv) < 2) or (len(sys.argv) > 3) ):
        print ('\nUsage: %s [Home_Node_ID] [optional human_id]' % os.path.basename(sys.argv[0]))        
        exit(0)

    home_node_id = int( sys.argv[1] )
    specific_human_id = -1
    if( len(sys.argv) == 3 ):
        specific_human_id = int( sys.argv[2] )

    input_fn  = "ReportHumanMigrationTracking.csv"

    data_map = ReadData( input_fn )

    time_data = []
    location_data = []
    legend_data = []

    ExtractData( data_map, home_node_id, specific_human_id, time_data, location_data, legend_data )

    PlotData( home_node_id, time_data, location_data, legend_data )

