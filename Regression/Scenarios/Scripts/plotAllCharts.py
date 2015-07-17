#!/usr/bin/python

import matplotlib.pyplot as plt
import json
import sys
import pylab
from math import ceil, sqrt

def plotOneFromDisk( label = "" ):
    ref_sim = open( sys.argv[1] )
    ref_data = json.loads( ref_sim.read() )

    num_chans = ref_data["Header"]["Channels"]

    square_root = ceil(sqrt(num_chans))

    n_figures_x = square_root
    n_figures_y = ceil(float(num_chans)/float(square_root))

    idx = 1
    plt.figure(1, figsize=(14.0, 7.5))
    for chan_title in sorted(ref_data["Channels"]):
        try:
            plt.subplot(n_figures_x, n_figures_y, idx)
            plt.tick_params(axis='both', which='major', labelsize=7)
            plt.tick_params(axis='both', which='minor', labelsize=5)
            plt.plot( ref_data["Channels"][chan_title]["Data"])
            plt.title( chan_title, fontsize = 9 )
            idx += 1
        except Exception as ex:
            print str(ex)


    plt.suptitle( label, fontsize=18 )
    plt.subplots_adjust( left=0.05, right=0.95, bottom=0.035, top=0.915, wspace=0.4, hspace=0.5 )
    plt.show()

def main():
    if len( sys.argv ) != 3:
        print( "Usage: plotAllCharts.py <inset_chart_json_path> <label>" )
        sys.exit(0)

    plotOneFromDisk( sys.argv[2] )

if __name__ == "__main__":
    main()
