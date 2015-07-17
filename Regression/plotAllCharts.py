#!/usr/bin/python

#import pymongo
import matplotlib.pyplot as plt
import json
import sys
import pylab
from math import sqrt, ceil

#mc = pymongo.Connection("ingolstadt").test.things

def plotOneFromDisk():
    #random_sim = mc.find()[mc.find().count()-1]
    #ref_sim = open( "/tmp/ic.new_regress.1.old.json" )
    ref_sim = open( sys.argv[1] )
    ref_data = json.loads( ref_sim.read() )

    num_chans = ref_data["Header"]["Channels"]
    #print num_chans
    idx = 0
    for chan_title in sorted(ref_data["Channels"]):
        #print idx
        #idx_x = idx%5
        #idx_y = int(idx/3)
        #print str(idx_x) + "," + str(idx_y)
        try:
            #subplot = plt.subplot2grid( (5,3), (idx_x,idx_y)  ) 
            subplot = plt.subplot( 4,5, idx  ) 
            subplot.plot( ref_data["Channels"][chan_title]["Data"], 'r-' )
            plt.title( chan_title )
        except Exception as ex:
            print str(ex) + ", idx = " + str(idx)
        if idx == 4*5:
            break

    plt.show()

def plotCompareFromMongo():
    #random_sim = mc.find()[mc.find().count()-1]
    random_sim = mc.find_one( { "sim.sim_id" : "2011_11_09_06_09_22_418000" } )
    icj_data = json.loads( random_sim["sim"]["inset_chart_data"] )

    ref_sim = json.loads( open( "regression.vectorgarki.reference.json" ).read() )
    ref_data = ref_sim["sim"]["inset_chart_data"]

    #print str(icj_data) 
    num_chans = ref_data["Header"]["Channels"]
    #print num_chans
    
    idx = 0
    for chan_title in sorted(ref_data["Channels"]):
        #print idx
        idx_x = idx%4
        idx_y = int(idx/4)
        #print str(idx_x) + "," + str(idx_y)
        try:
            subplot = plt.subplot2grid( (4,4), (idx_x,idx_y)  ) 
            subplot.plot( ref_data["Channels"][chan_title]["Data"], 'r-', icj_data["Channels"][chan_title]["Data"], 'b-' )
            plt.title( chan_title )
        except Exception as ex:
            print str(ex)
        if idx == 15:
            break

    plt.show()

def plotCompareFromDisk( label = "" ):
    #random_sim = mc.find()[mc.find().count()-1]
    #ref_sim = open( "/tmp/InsetChart.pb.bloedow.json" )
    #print sys.argv[1]
    ref_sim = open( sys.argv[1] )
    ref_data = json.loads( ref_sim.read() )

    #test_sim = open( "/tmp/InsetChart.pb.jeff.json" )
    #print sys.argv[2]
    test_sim = open( sys.argv[2] )
    test_data = json.loads( test_sim.read() )

    num_chans = ref_data["Header"]["Channels"]

    square_root = ceil(sqrt(num_chans))

    n_figures_x = square_root
    n_figures_y = ceil(float(num_chans)/float(square_root)) #Explicitly perform a float division here as integer division floors in Python 2.x

    figure = plt.figure(label.split('/')[-1])   # label includes the full (relative) path to the scenario, take just the final directory

    idx = 1
    for chan_title in sorted(ref_data["Channels"]):
        #print idx

        if chan_title not in test_data["Channels"]:
            continue

        try:
            subplot = plt.subplot( n_figures_x, n_figures_y, idx ) 
            subplot.plot( ref_data["Channels"][chan_title]["Data"], 'r-', test_data["Channels"][chan_title]["Data"], 'b-' )
            plt.setp( subplot.get_xticklabels(), fontsize='4' )
            plt.title( chan_title, fontsize='6' )
            idx += 1
        except Exception as ex:
            print str(ex)

    plt.suptitle( label + " (red=reference, blue=test)")
    plt.subplots_adjust( bottom=0.05 )
    plt.show()

def plotBunchFromMongo():
    mysims = mc.find( { "parameters.ConfigName":"Polio Bihar Regression Test", "user":"jbloedow", "sim.status":"Finished", "parameters.Run_Number":{ "$gt":40 }} )
# calculate mean of first 10 and mean of second 10: later!
    all_data = []
    for sim in mysims: #[0:9]:
        icj_data = json.loads( sim["sim"]["inset_chart_data"] )
        all_data.append( icj_data )
    plotBunch( all_data )

def plotBunch( all_data, plot_name, baseline_data=None ):
    num_chans = all_data[0]["Header"]["Channels"]
    plt.suptitle( plot_name )
    square_root = 4
    if num_chans > 30:
        square_root = 6
    elif num_chans > 16:
        square_root = 5
    plots = []
    labels = []

    idx = 0
    for chan_title in sorted(all_data[0]["Channels"]):
        idx_x = idx%square_root
        idx_y = int(idx/square_root)

        try:
            subplot = plt.subplot2grid( (square_root,square_root), (idx_y,idx_x)  ) 
            colors = [ 'b', 'g', 'c', 'm', 'y', 'k' ]

            if baseline_data is not None:
                plots.append( subplot.plot( baseline_data["Channels"][chan_title]["Data"], 'r-', linewidth=2 ) )

            for sim_idx in range(0,len(all_data)):
                labels.append(str(sim_idx))
                plots.append( subplot.plot( all_data[sim_idx]["Channels"][chan_title]["Data"], colors[sim_idx%len(colors)] + '-' ) )

            plt.title( chan_title )
        except Exception as ex:
            print str(ex)
        if idx == (square_root*square_root)-1:
            break

        idx += 1

    #plt.legend( plots, labels )

    #plt.set_size( 'xx-small' )
    plt.subplots_adjust( left=0.04, right=0.99, bottom=0.02, top =0.9, wspace=0.3, hspace=0.3 )
    pylab.savefig( plot_name.replace( " ", "_" ) + ".png", bbox_inches='tight', orientation='landscape' )
    plt.show()
    # print( "Exiting from plotBunch.\n" )

def main():
    #plotOneFromDisk()
    if len( sys.argv ) < 3:
        print( "Usage: plotAllCharts.py <ref_inset_chart_json_path> <test_inset_chart_json_path> <label>" )
        sys.exit(0)

    plotCompareFromDisk( sys.argv[3] )
    #plotBunch()

if __name__ == "__main__":
    main()
