import sys, os, json, collections
import numpy as np
import matplotlib.pyplot as plt


def ShowUsage():
    print ('\nUsage: %s [ReportEventRecorder.csv]' % os.path.basename(sys.argv[0]))


class ReportEventReporter_Data:
    def __init__(self):
        self.year      = -1
        self.time      = -1
        self.event_name = ""
        self.node_id    = -1
        self.human_id   = -1
        self.age_years  = -1
        self.gender     = ""
        self.infections = -1

def ReportEventReporter_Read( input_fn ):
    input_file = open(input_fn,"r")

    header = input_file.readline()

    data_list = []

    for line in input_file:
        line = line.replace('\n','')
        line_array = line.split( ',' )

        isAdult = False
        if( line_array[4] == "T" ):
            isAdult = True

        data = ReportEventReporter_Data()
        data.year       = float( line_array[0] )
        data.time       = float( line_array[1] )
        data.event_name =        line_array[2]
        data.node_id    = int(   line_array[3] )
        data.human_id   = int(   line_array[4] )
        data.age_years  = float( line_array[5] )
        data.gender     =        line_array[6]
        data.infectious = float( line_array[7] )


        data_list.append( data )

    input_file.close()

    return data_list

def ReportEventRecoder_ExtractEventsPerNode( data_list ):
    node_event_count_map = collections.OrderedDict([])

    for data in data_list:
        if( not node_event_count_map.has_key( data.node_id ) ):
            node_event_count_map[ data.node_id ] = collections.OrderedDict([])

        if( not node_event_count_map[ data.node_id ].has_key( data.event_name ) ):
            node_event_count_map[ data.node_id ][ data.event_name ] = collections.OrderedDict([])
        
        if( not node_event_count_map[ data.node_id ][ data.event_name ].has_key( data.human_id ) ):
            node_event_count_map[ data.node_id ][ data.event_name ][ data.human_id ] = 0

        node_event_count_map[ data.node_id ][ data.event_name ][ data.human_id ] += 1

    return node_event_count_map


def PlotBarChart( title, y_label, x_labels, bar_values ):
    fig, chart = plt.subplots()

    bar_width = 0.35
    ind = np.arange( len( x_labels ) )

    bars = chart.bar( ind+bar_width, bar_values, bar_width, color='r' )

    chart.set_ylabel( y_label )
    chart.set_title( title )
    chart.set_xticks( ind+bar_width )
    chart.set_xticklabels( x_labels )

    plt.show()


def ExtractData( node_event_count_map, event_name, node_id_labels, event_count ):
    node_id_list = node_event_count_map.keys()
    node_id_list.sort()

    for node_id in node_id_list:
        if( node_event_count_map[ node_id ].has_key( event_name ) ):
            node_id_labels.append( str( node_id ) )
            event_count.append( len(node_event_count_map[ node_id ][ event_name ]) )


if __name__ == "__main__":
    if len(sys.argv) != 2:
        ShowUsage()
        exit(0)

    report_event_recorder_fn = sys.argv[1]

    data_list = ReportEventReporter_Read( report_event_recorder_fn )

    node_event_count_map = ReportEventRecoder_ExtractEventsPerNode( data_list )

    node_id_labels = []
    event_count = []

    ExtractData( node_event_count_map, "NewClinicalCase", node_id_labels, event_count )

    PlotBarChart( "# of Occurances of 'NewClinicalCase' Per Node", "# Occurances", node_id_labels, event_count )

