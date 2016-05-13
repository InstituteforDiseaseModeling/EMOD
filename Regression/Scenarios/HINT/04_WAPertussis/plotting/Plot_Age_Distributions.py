import os
import json
import matplotlib.pyplot as plt
import numpy as np

# Path to output directory containing property report files
output_path = "../A_BaselineOutbreak"
#output_path = "../B_TargetVaccineRefusers"

# Open and parse BinnedReport.json
br_json = json.loads( open( os.path.join( output_path, "output", "BinnedReport.json" ) ).read() )
br_json_data = br_json["Channels"]

# Get sorted list of channel names
channels = sorted(br_json_data.keys())
print('Channels: ')
print(json.dumps(channels, indent=2, ))

# Population by age group
#plt.figure('Population')
#for i,agebin in enumerate(br_json_data[ "Population" ][ "Data" ]):
#    plt.plot(agebin, label=br_json["Header"]["Subchannel_Metadata"]["MeaningPerAxis"][0][0][i])

#plt.legend(ncol=2, title='Age (yr)')
#plt.ylabel('Population by age group')
#plt.xlabel('Day of simulation')
#plt.tight_layout()

# Cumulative incidence by age group
plt.figure('Cumulative Incidence')
fraction_infected = []
n_agebins = br_json["Header"]["Subchannel_Metadata"]["NumBinsPerAxis"][0][0]
for i in range(n_agebins):
    sum_incidence = sum(br_json_data[ "New Infections" ][ "Data" ][i])
    avg_population = np.mean(np.array([br_json_data[ "Population" ][ "Data" ][i][t] for t in range(len(br_json_data[ "Population" ][ "Data" ][i])) if br_json_data[ "New Infections" ][ "Data" ][i][t] > 0]))
    fraction_infected.append(sum_incidence / avg_population)
plt.bar(range(n_agebins), fraction_infected)
plt.xticks(np.array(range(n_agebins)) + 0.5, br_json["Header"]["Subchannel_Metadata"]["MeaningPerAxis"][0][0])
plt.xlabel('Age (yr)')
plt.ylabel('Cumulative incidence')

plt.show()
