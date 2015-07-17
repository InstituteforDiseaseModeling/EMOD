import matplotlib.pyplot as plt
import numpy as np
import os
import json

def load_json_file(filename):
    with open(filename) as json_file:
        return json.loads(json_file.read())

class SimpleInsetChartAnalyzer():

    def __init__(self, report_range=None, channels=['Statistical Population', 'Rainfall', 'Adult Vectors', 'Daily EIR', 'Infected', 'Parasite Prevalence']):
        self.filename = 'InsetChart.json'
        self.channels = channels
        self.data = {}
        if report_range:
            self.report_range = report_range
        else:
            def return_all(x):
                return x
            self.report_range = return_all
        for channel in self.channels:
            self.data[channel] = {}

    def read_data(self):
        for channel in self.channels:
            data = load_json_file(self.filename)["Channels"][channel]["Data"]
            self.data[channel] = self.report_range(data)

    def plot(self):
        plt.figure('InsetChart1')
        ncol = 1+len(self.channels)/4
        for (i,channel) in enumerate(self.channels):
            plt.subplot(np.ceil(float(len(self.channels))/ncol), ncol, i+1)
            plt.plot(self.data[channel], color='b')
            plt.title(channel)
        plt.tight_layout()

if __name__ == '__main__':
    a=SimpleInsetChartAnalyzer()
    a.read_data()
    a.plot()
    plt.show()