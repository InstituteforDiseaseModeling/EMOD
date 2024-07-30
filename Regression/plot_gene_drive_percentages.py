import sys, os, json, collections, struct, datetime
import argparse
import math
import matplotlib.pyplot as plt

def ReadData( filename ):
    data_map = {}
    prev_time = 0.0
    with open( filename, "r" ) as rs:
        header = rs.readline() # chew up header
        header = header.replace('\n','')
        header_array = header.split( ',' )
        
        for name in header_array:
            data_map[ name ] = []
        
        for line in rs:
            line = line.replace('\n','')
            line_array = line.split( ',' )

            for i in range(len(header_array)):
                data_map[ header_array[i] ].append( float( line_array[ i ] ) )

    return data_map

def GetColor( i ):
    color = "-r"
    if i == 0:
        color = "-r"
    elif i == 1:
        color = "-b"
    elif i == 2:
        color = "-g"
    return color
    
def PlotData( data_list ):
    title = data_list[0]["FileName"]
    name_set = set()
    for data in data_list:
        if title != data["FileName"]:
            title += " vs " + data["FileName"]
        tmp_names_set = set(data["Data"].keys())
        name_set = name_set.union( tmp_names_set )
    
    names_list = list( name_set )
    names_list.remove("Time")
    num_names = len( names_list )
    
    square_root = math.ceil(math.sqrt(num_names))

    n_figures_x = square_root
    n_figures_y = math.ceil(float(num_names)/float(square_root)) #Explicitly perform a float division here as integer division floors in Python 2.x

    try:
        idx = 1
        for name in names_list:
            subplot = plt.subplot( n_figures_x, n_figures_y, idx ) 
            
            for i in range(len(data_list)):
                color = GetColor(i)
                if name in data_list[i]["Data"].keys():
                    subplot.plot( data_list[i]["Data"][ name ], color )
                
            plt.setp( subplot.get_xticklabels(), fontsize='6' )
            plt.title( name, fontsize='10' )
            idx += 1
    except Exception as ex:
        print( str(ex) )

    plt.suptitle( title )
    plt.subplots_adjust( bottom=0.05, hspace=0.4 )
    plt.show()


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('file_1', help='Name of one file with data')
    parser.add_argument('file_2', help='Name of one file with data')
    #parser.add_argument('file_3', help='Name of one file with data')
    args = parser.parse_args()

    data_1 = {}
    data_1[ "FileName" ] = args.file_1
    data_1[ "Data"     ] = ReadData( args.file_1 )
    data_2 = {}
    data_2[ "FileName" ] = args.file_2
    data_2[ "Data"     ] = ReadData( args.file_2 )
    #data_3 = {}
    #data_3[ "FileName" ] = args.file_3
    #data_3[ "Data"     ] = ReadData( args.file_3 )
    
    data_list = []
    data_list.append( data_1 )
    data_list.append( data_2 )
    #data_list.append( data_3 )
    
    PlotData( data_list )


