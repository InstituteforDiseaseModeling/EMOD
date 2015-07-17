import matplotlib.pyplot as plt
import numpy as np
import os
import json

def load_json_file(filename):
    with open(filename) as json_file:
        return json.loads(json_file.read())

class MalariaSummaryAnalyzer():

    def __init__(self, report_range=None, semilogAge=False, saveOutput=False):
        self.filename = 'MalariaSummaryReport_AnnualAverage.json'
        self.channels = ['Annual EIR', 'Average Population by Age Bin', 'PfPR_2to10', 'PfPR by Age Bin', 'RDT PfPR by Age Bin', 'Annual Clinical Incidence by Age Bin', 'Annual Severe Incidence by Age Bin']
        self.semilogAge = semilogAge
        self.agebins = []
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
        json_data=load_json_file(self.filename)
        self.agebins=json_data["Age Bins"]
        for channel in self.channels:
            data = json_data[channel]
            self.data[channel] = self.report_range(data)

    def plot(self):
        bincenters = (np.array([0] + self.agebins[:-1]) + np.array(self.agebins[:-1] + [80]))/2
        detection_tech = 'slide microscopy' #  'RDT'  'slide microscopy'
        pfpr_channel = ('RDT ' if detection_tech == 'RDT' else '') + 'PfPR by Age Bin'
        plt.figure('PfPR Summary')
        legend_entries = []
        for pfpr in self.data[pfpr_channel]:
            plt.plot(bincenters,pfpr)
        plt.title('PfPR by age')
        plt.xlabel('Age (years)')
        plt.ylabel('Parasite Prevalence (%s)' % detection_tech)
        plt.ylim([0.0, 1.0])
        if self.semilogAge:
            plt.xscale('log')

        plt.figure('Clinical Incidence Summary')
        for clinical in self.data['Annual Clinical Incidence by Age Bin']:
            plt.plot(bincenters,clinical)
        plt.title('Clinical Incidence by age')
        plt.xlabel('Age (years)')
        plt.ylabel('Clinical Incidence per Year')
        plt.ylim([0.0, 10.0])
        if self.semilogAge:
            plt.xscale('log')
                
        plt.figure('Severe Disease Summary')
        for severe in self.data['Annual Severe Incidence by Age Bin']:
            plt.plot(bincenters,severe)
        plt.title('Severe Incidence by age')
        plt.xlabel('Age (years)')
        plt.ylabel('Severe Incidence per Year')
        plt.ylim([0.0, 1.0])
        if self.semilogAge:
            plt.xscale('log')

        plt.figure('Transmission Dependences')
        plt.subplot(131)
        plt.scatter(self.data['Annual EIR'], self.data['PfPR_2to10'])
        plt.xlabel('Annual EIR') 
        plt.ylabel('PfPR 2-10 (slide microscopy)')
        plt.ylim([0,1])

        plt.subplot(132)
        clinical=self.data['Annual Clinical Incidence by Age Bin']
        population = self.data['Average Population by Age Bin']
        weighted_clinical_mean = [sum([m*w for m,w in zip(clinical[y], population[y])])/sum(population[y]) for y in range(len(clinical))]
        plt.scatter(self.data['PfPR_2to10'], weighted_clinical_mean)
        plt.ylabel('Annual clinical incidence per person')
        plt.xlabel('PfPR 2-10 (slide microscopy)')
        #plt.ylim([0,2])

        plt.subplot(133)
        severe=self.data['Annual Severe Incidence by Age Bin']
        population = self.data['Average Population by Age Bin']
        weighted_severe_mean = [sum([m*w for m,w in zip(severe[y], population[y])])/sum(population[y]) for y in range(len(severe))]
        plt.scatter(self.data['PfPR_2to10'], weighted_severe_mean)
        plt.ylabel('Annual severe incidence per person')
        plt.xlabel('PfPR 2-10 (slide microscopy)')
        #plt.ylim([0,0.2])


if __name__ == '__main__':
    a=MalariaSummaryAnalyzer()
    a.filename='MalariaSummaryReport_.json' #no tag like "AnnualAverage" was passed in the campaign file
    a.read_data()
    a.plot()
    plt.show()