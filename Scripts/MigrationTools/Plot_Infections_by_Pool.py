import os
import json
import matplotlib.pyplot as plt

# test
#output_path = "mixing_pool_test"
output_path = "multicore_mixing_pool_test"

# open and parse InsetChart.json
ic_json = json.loads( open( os.path.join( output_path, "output", "InsetChart.json" ) ).read() )
ic_json_data = ic_json["Channels"]
 
# Channel name
#channel_name = "Total Contagion"
channel_name = "New Infections"

# plot "New Infections" channels by time step (for each pool)
nonOrthodox = ic_json_data[ "Pool non-orthodox " + channel_name ][ "Data" ]
orthodox    = ic_json_data[  "Pool orthodox " + channel_name ][ "Data" ]
total       = map(sum,zip(nonOrthodox,orthodox))

plt.semilogy( nonOrthodox, 'r-' )
plt.semilogy( orthodox,    'g-' )
plt.legend( ('non-Orthodox', 'Orthodox') )

plt.title( channel_name )
plt.show()
