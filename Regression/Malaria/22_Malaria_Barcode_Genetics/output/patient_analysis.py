import os
import json
import warnings
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import matplotlib.cm as cm

def load_json_file(filename):
    with open(filename) as json_file:
        return json.loads(json_file.read())

class MalariaPatientAnalyzer():

    def __init__(self, filename='MalariaSurveyJSONAnalyzer_Day0_0.json'):
        self.filename = filename
        self.patient_array = []
        self.ntsteps = 0

    def read_data(self):
        json_data=load_json_file(self.filename)
        self.ntsteps=1 #json_data["ntsteps"]
        self.patient_array = json_data["patient_array"]

    def plot_hist(self, s, data, ymax=6, orientation='vertical', color='b'):
        s.clear()
        s.hist(data, np.arange(-2,ymax,0.5), alpha=0.2, color=color, orientation=orientation)
        if orientation == 'vertical':
            s.get_xaxis().set_ticklabels([])
            s.locator_params(axis='y', tight=True, nbins=4)
        else:
            s.get_yaxis().set_ticklabels([])
            s.locator_params(axis='x', tight=True, nbins=4)

    def plot(self):
        tstep=0
        par=[]
        gam=[]
        cat=[]
        for t in range(self.ntsteps):
            aa=[]
            gg=[]
            cc=[]
            for p in self.patient_array:
                a=p['true_asexual_parasites'][0]
                g=p['true_gametocytes'][0]
                f=p['temps'][0]
                r=p['rdt'][0]
                aa.append(np.nan if t>=len(a) else np.log10(a[t]) if a[t] > 1e-2 else -2)
                gg.append(np.nan if t>=len(g) else np.log10(g[t]) if g[t] > 1e-2 else -2)
                cc.append(0 if t>=len(f) else (f[t]-37) if f[t] > 38.5 else -1 if r[t]==0 else -2 if g[t]==0 else 0) # categorize: infected, detected, symptoms
            par.append(aa)
            gam.append(gg)
            cat.append(cc)

        fig, ax = plt.subplots(figsize=(8,6))
        fig.subplots_adjust(left=0.1, bottom=0.05, top=0.9, right=0.95)
        gs = gridspec.GridSpec(2,2,width_ratios=[4,1],height_ratios=[4,1])

        txt = fig.text(0.38, 0.9, 'Day %d' % tstep, fontweight='bold')

        ax1=plt.subplot(gs[0])
        scat1=plt.scatter(par[tstep], gam[tstep], c=cat[tstep], s=60, alpha=0.2, cmap=cm.jet, vmin=-1, vmax=3);
        plt.xlim([-2.3,6])
        plt.ylim([-2.3,5])
        plt.ylabel('log10 gametocytes', fontweight='bold')
        plt.xlabel('log10 asexual parasites', fontweight='bold')

        ax2=plt.subplot(gs[1])
        hgam=self.plot_hist(ax2, gam[tstep], ymax=5, orientation='horizontal', color='g')

        ax3=plt.subplot(gs[2])
        hpar=self.plot_hist(ax3, par[tstep])

        plt.tight_layout()

if __name__ == '__main__':
    a=MalariaPatientAnalyzer('MalariaSurveyJSONAnalyzer_Day0_3.json')
    a.read_data()
    a.plot()
    plt.show()