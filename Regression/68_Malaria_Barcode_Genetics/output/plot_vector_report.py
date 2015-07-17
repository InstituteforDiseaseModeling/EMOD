import matplotlib.pyplot as plt
import numpy as np
import json

colors = ['b', 'g', 'k', 'r', 'm', 'c', 'y']

def load_json_file(filename):
    with open(filename) as json_file:
        return json.loads(json_file.read())

class SimpleVectorAnalyzer():

    def __init__(self, channels = ['Adult Vectors', 'Infectious Vectors', 'Daily EIR']):
        self.filename = 'VectorSpeciesReport.json'
        self.channels = channels
        self.species_names=[]
        self.data = {}
        for channel in self.channels:
            self.data[channel] = {}

    def read_data(self):
        json_data=load_json_file(self.filename)
        self.species_names=json_data["Header"]["Subchannel_Metadata"]["MeaningPerAxis"][0][0]
        for channel in self.channels:
            self.data[channel] = load_json_file(self.filename)["Channels"][channel]["Data"]

    def plot(self):
        plt.figure('VectorReport1')
        ncol = 1+len(self.channels)/4
        for (i,channel) in enumerate(self.channels):
            plt.subplot(len(self.channels)/ncol, ncol, i+1)
            for k,species in enumerate(self.species_names):
                color = colors[k%len(colors)]
                plt.plot(self.data[channel][k], color=color, label=species)
            plt.title(channel)
            if not i:
                plt.legend()

        plt.tight_layout()

if __name__ == '__main__':
    a=SimpleVectorAnalyzer()
    a.read_data()
    a.plot()
    plt.show()