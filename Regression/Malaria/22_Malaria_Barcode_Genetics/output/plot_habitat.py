import os
import json
import glob
import numpy as np
import matplotlib.pyplot as plt

colors = [[0.5,0.5,0.5],
          [0.6,0,0],
          [0.3,0,0],
          'k',
          [0.3,0.3,1.0],
          [0.2,0.4,0.3]]

output_dir = '.'

with open(os.path.join(output_dir, "InsetChart.json")) as ic:
    ic_json = json.loads(ic.read())

with open(os.path.join(output_dir, "VectorSpeciesReport.json")) as vsr:
    vsr_json = json.loads(vsr.read())

species_names = vsr_json["Header"]["Subchannel_Metadata"]["MeaningPerAxis"][0][0]
print(species_names)

plt.figure("Larval Habitat", figsize=(16,10))
plt.subplot(221)
plt.plot(ic_json["Channels"]["Rainfall"]["Data"], label="Rainfall (mm)")
plt.xlim([0, 365*2])
plt.title('Adult Vectors (thousands)')

for (i,channel_data) in enumerate(vsr_json["Channels"]["Adult Vectors"]["Data"]):
    plt.plot([c/1000.0 for c in channel_data], label=species_names[i], color=colors[i], linewidth=2)

plt.legend()

with open(os.path.join(output_dir, "VectorHabitatReport.json")) as vhr:
    vhr_json = json.loads(vhr.read())

habitat_types = vhr_json["Header"]["Subchannel_Metadata"]["MeaningPerAxis"][0][0]
print(habitat_types)

for i,(channel_name, channel_data) in enumerate(vhr_json["Channels"].items()):
    for j,habitat_data in enumerate(channel_data["Data"]):
        habitat_name = habitat_types[j]
        if 'funestus' in habitat_name:
            plt.subplot(222)
            plt.title('funestus')
        elif 'arabiensis' in habitat_name:
            plt.subplot(223)
            plt.title('arabiensis')
        elif 'gambiae' in habitat_name:
            plt.subplot(224)
            plt.title('gambiae')
        plt.semilogy(habitat_data, label=channel_name, color=colors[i])
        plt.xlim([0, 365*2])
        plt.ylim([1e-4,1e6])

plt.subplot(222)
leg=plt.legend(loc="lower right", ncol=2)

# set the linewidth of each legend object
for legobj in leg.legendHandles:
    legobj.set_linewidth(2.0)

plt.tight_layout()
plt.show()
