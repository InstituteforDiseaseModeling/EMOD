import os
import struct
import matplotlib.pyplot as plt
import numpy as np

# Path to output directory containing spatial report files
output_path = "../A_BaselineOutbreak"
#output_path = "../B_TargetVaccineRefusers"

# Channel name
channel_name = "New_Infections"
#channel_name = "Prevalence"

# Nodes names
node_names = [ "Adams", "Asotin", "Benton", "Chelan", "Clallam", "Clark", "Columbia",
               "Cowlitz", "Douglas", "Ferry", "Franklin", "Garfield", "Grant", "Grays Harbor",
               "Island", "Jefferson", "King", "Kitsap", "Kittitas", "Klickitat", "Lewis",
               "Lincoln", "Mason", "Okanogan", "Pacific", "Pend Oreille", "Pierce", "San Juan", "Skagit", 
               "Skamania", "Snohomish", "Spokane", "Stevens", "Thurston", "Wahkiakum", "Walla Walla",
               "Whatcom", "Whitman", "Yakima" ]

# Open binary file for reading
f = open( os.path.join( output_path, "output", "SpatialReport_" + channel_name + ".bin" ), "rb" )
data = f.read(8)

# Unpack metadata
n_nodes, = struct.unpack( 'i', data[0:4] )
n_tstep, = struct.unpack( 'i', data[4:8] )
print( "There are %d nodes and %d time steps" % (n_nodes, n_tstep) )

nodeids_dtype = np.dtype( [ ( 'ids', '<i4', (1, n_nodes ) ) ] )
nodeids = np.fromfile( f, dtype=nodeids_dtype, count=1 )
print( "node IDs: " )
print( nodeids['ids'][:,:,:].ravel() )

# Unpack data
channel_dtype = np.dtype( [ ( 'data', '<f4', (1, n_nodes ) ) ] )
channel_data = np.fromfile( f, dtype=channel_dtype )
channel_data = channel_data['data'].reshape(n_tstep, n_nodes)
#print( "channel data: ")
#print( channel_data )

# Plot channel data time series for a set of highlighted counties
plt.figure(1, figsize=(12,10))
highlighted_counties = {"King":3, "Skagit":1, "Spokane":6, "Snohomish":2, "Clark":4, "Franklin":5}
for i in range(0,n_nodes):
    print(i, node_names[i])
    if node_names[i] in highlighted_counties.keys():
        plt.subplot(len(highlighted_counties),1,highlighted_counties[node_names[i]])
        plt.plot( channel_data[:,i] )
        plt.xlim([10*365, 13*365])
        plt.ylabel(channel_name)
        plt.title(node_names[i] + ' County')
plt.subplot(len(highlighted_counties),1,len(highlighted_counties))
plt.xlabel('Day')
plt.tight_layout()

# Save figure to file
plt.savefig(os.path.join(output_path, 'output', 'SpatialOutput_' + channel_name + '.png'))
plt.show()
