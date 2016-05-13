import os
import struct
import matplotlib.pyplot as plt
import numpy as np

# Output directory
output_path = "C:\\Users\\ghuynh\\Desktop\\fromsabalcore"

# Channel name
channel_name = "Active_TB_Prevalence"
#channel_name = "New_Infections"

# Nodes names
node_names = [ "1", "2", "3", "4",
              "5", "6", "7", "8" ]

# Open binary file for reading
f = open( os.path.join( output_path, "output", "SpatialReport_" + channel_name + ".bin" ), "rb" )
data = f.read(8)

n_nodes, = struct.unpack( 'i', data[0:4] )
n_tstep, = struct.unpack( 'i', data[4:8] )

print( "There are %d nodes and %d time steps" % (n_nodes, n_tstep) )

nodeids_dtype = np.dtype( [ ( 'ids', '<i4', (1, n_nodes ) ) ] )
nodeids = np.fromfile( f, dtype=nodeids_dtype, count=1 )

print( "node IDs: " )
print( nodeids['ids'][:,:,:].ravel() )

channel_dtype = np.dtype( [ ( 'data', '<f4', (1, n_nodes ) ) ] )
channel_data = np.fromfile( f, dtype=channel_dtype )
channel_data = channel_data['data'].reshape(n_tstep, n_nodes)

print( "channel data: ")
print( channel_data )

node_legend = []
for i in range(0,n_nodes):
    if node_names[i] == "IMPORT": 
        continue
    node_legend.append(node_names[i])
    plt.plot( channel_data[:,i] )

plt.legend( node_legend )

plt.title( channel_name )
plt.show()
