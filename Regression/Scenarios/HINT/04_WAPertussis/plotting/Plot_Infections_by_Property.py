import os
import json
import matplotlib.pyplot as plt
import numpy as np

# Path to output directory containing property report files
output_path = "../A_BaselineOutbreak"
#output_path = "../B_TargetVaccineRefusers"

# Open and parse PropertyReport.json
pr_json = json.loads( open( os.path.join( output_path, "output", "PropertyReport.json" ) ).read() )
pr_json_data = pr_json["Channels"]

# Get sorted list of channel names
channels = sorted(pr_json_data.keys())
print('Channels: ')
print(json.dumps(channels, indent=2, ))

# Channel name
channel_name = "New Infections"
#channel_name = "Statistical Population"

# Property type to group on
group_by = "Accessibility"
#group_by = "Age_Bin"

channel_values = {}
stat_pop_values = {}

# Collect channel data and population normalization channel
for channel in channels:
    if channel_name in channel:
        print(channel)
        if group_by not in channel:
            raise Exception('Cannot group by ' + group_by + ' property.')
        else:
            property_string = channel[len(channel_name)+1:]
            #print(property_string)
            properties = property_string.split(',')
            #print(properties)
            for p in properties:
                property_pair = p.split(':')
                if property_pair[0] == group_by:
                    print(property_pair[0] + ' = ' + property_pair[1])

                    if property_pair[1] not in channel_values.keys():
                        channel_values[property_pair[1]] = np.array(pr_json_data[ channel ][ "Data" ], dtype='float')
                        stat_pop_values[property_pair[1]] = np.array(pr_json_data[ channel.replace(channel_name, "Statistical Population") ][ "Data" ], dtype='float')
                    else:
                        channel_values[property_pair[1]] += pr_json_data[ channel ][ "Data" ]
                        stat_pop_values[property_pair[1]] += pr_json_data[ channel.replace(channel_name, "Statistical Population") ][ "Data" ]
                    break

# Plot channel data by property group and population-normalized rates
plt.figure(1, figsize=(10,10))
plt.subplot(211)
for (group_name, values) in channel_values.items():
    plt.plot(values, label=group_name)
plt.ylabel( channel_name )
plt.legend()
plt.xlim([10*365, 13*365])

plt.subplot(212)
for (group_name, values) in channel_values.items():
    plt.plot(1000*values/stat_pop_values[group_name], label=group_name)
plt.ylabel( channel_name + ' per 1,000 individuals' )
plt.xlim([10*365, 13*365])
plt.xlabel( 'Days')

# Save figure to file
plt.savefig(os.path.join(output_path, 'output', channel_name.replace(' ', '_') + '_by_' + group_by + '.png'))
plt.show()
