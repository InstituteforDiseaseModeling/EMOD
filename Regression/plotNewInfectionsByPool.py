#!/usr/bin/python

import json
import matplotlib.pyplot as plt
import sys

if len( sys.argv ) < 2:
    print( "Usage: plotNewInfectionsByPool <output/PropertyReport.json> <number of New Infection pools>" )
    sys.exit(0)

data = open( sys.argv[1] ).read()
data_json = json.loads( data )
num_pools = int(sys.argv[2])
f, axes = plt.subplots( num_pools, sharex=True, sharey=True )
counter = 0
for data in data_json[ "Channels" ]:
    print data
    if "New Infections" in data:
        axes[ counter ].plot( data_json[ "Channels" ][ data ][ "Data" ] )
	axes[ counter ].set_title( data )
        counter = counter + 1

plt.show()
