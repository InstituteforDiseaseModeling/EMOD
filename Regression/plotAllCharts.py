#!/usr/bin/python

"""
This module contains methods for plotting channel reports (i.e. InsetChart).
"""

import argparse
import matplotlib.pyplot as plt
import numpy as np
import json
import sys
import os
import pylab
from math import sqrt, ceil


def getColor( idx ):
    """
    Return color code for plotting choosing from a fixed set of colors.
    Needs to be synchronized with getColorName()
    
    Args:
        idx: index of the plot used to select color
        
    Returns:
        Matplotlib basic color to use for plotting.
    """
    colors = [ 'b', 'g', 'c', 'm', 'y', 'k' ]
    return colors[ idx % len(colors) ] + "-"


def getColorName( idx ):
    """
    Return name of color that should be returned by getColor() given the same input value.
    Needs to be synchronized with getColor().
    
    Args:
        idx: index of the plot used to select color
        
    Returns:
        Name of the basic color to use for plotting in matplotlib.
    """
    color_names = [ 'blue', 'green', 'cyan', 'magenta', 'yellow', 'black' ]
    return color_names[ idx % len(color_names) ]


def getListOfChannels( ref_data, test_data ):
    """
    Returns a list of the unique channel names used in both the reference data
    and the test data.  This should enable the display of all of the channels
    even when both reports do not have the same channels.
    
    Args:
        ref_data: channel report, json dictionary consider to contain the baseline data
        test_data: a list of channel reports (dictionaries) containing data to compare to
        
    Returns:
        Unique list of channels from all the channels in the input
    """

    channel_titles_list = []
    if ref_data != None:
        channel_titles_list = list( ref_data["Channels"].keys() )

    for data in test_data:
        channel_titles_list = channel_titles_list + list ( data["Channels"].keys() )

    channel_titles_set  = set( channel_titles_list )
    channel_titles_list = sorted( list( channel_titles_set ) )

    return channel_titles_list


def createTitleString( reference, data_filenames ):
    """
    Returns a string that contains the input file names where the color used
    in plotting is included in the name.  This can be used as the title of the plot.
    
    Args:
        reference: name of the reference data file
        data_filenames: a list of the test data file names
        
    Returns:
        A string where each file name is on its own line and includes the color
        to be used in plotting in the name.
    """
    title = ""
    if reference != None:
        title = "reference(red)=" + reference + "\n"
    
    for i in range(0,len(data_filenames)):
        color_name = getColorName( i )
        filename = data_filenames[ i ]
        title = title + "test(" + color_name + ")=" + filename
        if i < len(data_filenames)-1:
            title = title + "\n"
    
    return title


def getTestDataFromDirectory( directory ):
    """
    Gets the JSON files from the input directory and return the names of the files
    and the dictionaries of data.  The idea is to allow the user to put several
    channel reports into one directory and plot them all by just giving the name
    of the directory.
    
    Args:
        directory: a path to a directory that contains only a collection of channel reports
        
    Returns:
        Return the list of file names in the directory AND the list of dictionaries
        containing the data of those files
    """
    dir_data = []
    dir_data_filenames = []
    for file in os.listdir( directory ):
        file = os.path.join( directory, file )
        if file.endswith(".json"):
            dir_data_filenames.append( file )

    dir_data_filenames = sorted( dir_data_filenames )
    for file in dir_data_filenames:
        with open( file ) as dir_sim:
            dir_data.append( json.loads( dir_sim.read() ) )
            
    return dir_data_filenames, dir_data

    
def plotData( title, ref_data, test_data, test_filenames=[], savefig=False, plot_name=None ):
    """
    Plot the data such that there is a grid of subplots with each subplot representing
    a "channel" of data.  Each subplot will have time on the x-axis and the units of
    that channel on the y-axis.
    
    Args:
        title: The string to put at the top of the page
        ref_data: A channel report dictionary whose data will be plotted in red
        test_data: A list of channel report dictionaries whose data will be plotted
                   in colors other than red
        test_file_names: The list of file names in parallel to the test_data.
        savefig: If true, an PNG file will be saved of the plot.
        plot_name: If provided the name of the file for the saved image.
        
    Returns:
        Nothing
    """
    channel_titles_list = getListOfChannels( ref_data, test_data )
    
    num_chans = len( channel_titles_list )
    square_root = ceil(sqrt(num_chans))
    #Explicitly perform a float division here as integer division floors in Python 2.x
    n_figures_y = ceil(float(num_chans)/float(square_root))
    n_figures_x = square_root
    
    ref_color = "r-"
    if len(test_data) == 0:
        ref_color = "b-"
    
    for idx in range(0,len(channel_titles_list)):
        chan_title = channel_titles_list[ idx ]
        
        idx_x = idx%square_root
        idx_y = int(idx/square_root)
        
        try:
            idx_x = idx%square_root
            idx_y = int(idx/square_root)
            subplot = plt.subplot2grid( (square_root,square_root), (idx_y,idx_x)  ) 

            if ref_data != None:
                if chan_title in ref_data["Channels"]:
                    ref_tstep = 1
                    if( "Simulation_Timestep" in ref_data["Header"] ):
                        ref_tstep = ref_data["Header"]["Simulation_Timestep"]
                    ref_x_len = len( ref_data["Channels"][chan_title]["Data"] )
                    ref_x_data = np.arange( 0, ref_x_len*ref_tstep, ref_tstep )
                    ref_y_data = ref_data["Channels"][chan_title]["Data"]
                    subplot.plot( ref_x_data, ref_y_data, ref_color, linewidth=2 )
                else:
                    print("Reference missing channel = "+chan_title)

            for test_idx in range(0,len(test_data)):
                if chan_title in test_data[test_idx][ "Channels" ]:
                    tst_tstep = 1
                    if( "Simulation_Timestep" in test_data[test_idx]["Header"] ):
                        tst_tstep = test_data[test_idx]["Header"]["Simulation_Timestep"]
                    tst_x_len = len( test_data[ test_idx ]["Channels"][chan_title]["Data"] )
                    tst_x_data = np.arange( 0, tst_x_len*tst_tstep, tst_tstep )
                    tst_y_data = test_data[test_idx]["Channels"][chan_title]["Data"]
                    tst_color = getColor( test_idx )
                    subplot.plot( tst_x_data, tst_y_data, tst_color )
                else:
                    missing_msg = "Test Data"
                    if len(test_filenames) > 0:
                        missing_msg = test_filenames[ test_idx ]
                    missing_msg = missing_msg + " missing channel = "+chan_title
                    print( missing_msg )

            plt.setp( subplot.get_xticklabels(), fontsize='7' )
            plt.title( chan_title, fontsize='9' )
        except Exception as ex:
            print( "Exception: " + str(ex) )
    
    plt.suptitle( title )

    plt.subplots_adjust( left=0.04, right=0.99, bottom=0.02, top =0.9, wspace=0.3, hspace=0.3 )
    
    mng = plt.get_current_fig_manager()
    #mng.full_screen_toggle()
    #if os.name == "nt":
    #    mng.window.state('zoomed')
    #else:
    #    mng.frame.Maximize(True)
    
    if savefig:
        if os.name == "nt":
            mng.window.state('zoomed')
        else:
            mng.frame.Maximize(True)
        plot_name = plot_name.replace( " ", "_" )
        plot_name = plot_name.replace( "\n", "_" )
        pylab.savefig( plot_name + ".png", orientation='landscape' )
    
    plt.show()
    return


def plotBunch( all_data, plot_name, baseline_data=None ):
    """
    Plot several sets of data.  This is used by regression for parameter sweeps.
    
    Args:
        all_data: A list of channel report dictionaries to compare.
        plot_name: The name of the saved image.
        baseline_data: The reference data to compare "all_data" to.
        
    Returns:
        Nothing
    """
    plotData( plot_name, baseline_data, all_data, savefig=True, plot_name=plot_name )
    return


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('reference',   default=None, nargs='?', help='Reference channel report filename')
    parser.add_argument('comparison1', default=None, nargs='?', help='Comparison1 channel report filename')
    parser.add_argument('comparison2', default=None, nargs='?', help='Comparison2 channel report filename')
    parser.add_argument('comparison3', default=None, nargs='?', help='Comparison3 channel report filename')
    parser.add_argument('--dir',       default=None, nargs='?', help='directory of channel reports with .json extension')
    parser.add_argument('--title',     default=None, nargs='?', help='Title of Plot')
    parser.add_argument('--savefig',   action='store_true',     help='save plot to image')
    
    args = parser.parse_args()
    
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit()
    
    test_filenames = []
    test_data = []
    if args.dir != None:
        test_filenames, test_data = getTestDataFromDirectory( args.dir )

    if args.comparison1 != None:
        test_filenames.append( args.comparison1 )
        with open( args.comparison1 ) as test_sim:
            test_data.append( json.loads( test_sim.read() ) )
        
    if args.comparison2 != None:
        test_filenames.append( args.comparison2 )
        with open( args.comparison2 ) as test_sim:
            test_data.append( json.loads( test_sim.read() ) )
        
    if args.comparison3 != None:
        test_filenames.append( args.comparison3 )
        with open( args.comparison3 ) as test_sim:
            test_data.append( json.loads( test_sim.read() ) )
        
    ref_data = None
    if args.reference != None:
        with open( args.reference ) as ref_file:
            ref_data = json.loads( ref_file.read() )
        
    title = args.title
    plot_name = args.title
    if plot_name == None:
        plot_name = "TestPlot"
        title = createTitleString( args.reference, test_filenames )

    plotData( title, ref_data, test_data, test_filenames, savefig=args.savefig, plot_name=plot_name )
