from __future__ import division #allows you to do non-integer (ie floating point) division
import numpy
import subprocess
import json
import os
from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.pyplot as plt
from pylab import *

def modifyConfigAndCampaigns():
#open config file, modify and write out
    configjson = json.loads( open( config_name).read() )        
    #save config to the output dir directory so you can call it
    with open( os.path.join( output_dir, config_name), 'w') as f:
        f.write( json.dumps( configjson, sort_keys=True, indent=4 ) )
        f.close()


    #copy over the campaign file
    campaignjson = json.loads( open( campaign_name).read() )
    if output_dir == "AcquiredMDR":
        campaignjson["Events"][0]["Event_Coordinator_Config"]["Intervention_Config"]["Genome"] = 0
        campaignjson["Events"][0]["Event_Coordinator_Config"]["Demographic_Coverage"] = 1
        campaignjson["Events"][0]["Event_Name"] = "Outbreak of DS TB to all"
        campaignjson["Events"][1]["Start_Day"] = 1 #acquire MDR
    elif output_dir == "TransmittedMDR":
        print "TransmittedMDR not doing anything to the campaign file"
    elif output_dir == "AcquiredandTransmittedMDR":
        campaignjson["Events"][0]["Event_Coordinator_Config"]["Intervention_Config"]["Genome"] = 0
        campaignjson["Events"][0]["Event_Coordinator_Config"]["Demographic_Coverage"] = 0.5
        campaignjson["Events"][0]["Event_Name"] = "Outbreak of DS TB to half of population"
        campaignjson["Events"][1]["Start_Day"] = 1 #acquire MDR
    elif output_dir == "IneffectiveRetreatmentforMDRTB":
        campaignjson["Events"][0]["Event_Coordinator_Config"]["Intervention_Config"]["Genome"] = 0
        campaignjson["Events"][0]["Event_Coordinator_Config"]["Demographic_Coverage"] = 1
        campaignjson["Events"][0]["Event_Name"] = "Outbreak of DS TB to all"
        campaignjson["Events"][1]["Start_Day"] = 1 #acquire MDR
        campaignjson["Events"][2]["Start_Day"] = 1 #ineffective tx for MDR
    elif output_dir == "MDRDiagnosticandSecondLineDrugsforMDRTB":
        campaignjson["Events"][0]["Event_Coordinator_Config"]["Intervention_Config"]["Genome"] = 0
        campaignjson["Events"][0]["Event_Coordinator_Config"]["Demographic_Coverage"] = 1
        campaignjson["Events"][0]["Event_Name"] = "Outbreak of DS TB to all"
        campaignjson["Events"][1]["Start_Day"] = 1 #acquire MDR
        campaignjson["Events"][3]["Start_Day"] = 1 #MDR test
        campaignjson["Events"][4]["Start_Day"] = 1 #MDRTestNegative gets ineffective tx
        campaignjson["Events"][5]["Start_Day"] = 1 #MDRTestPositive gets effective tx
    else:
        print "please modify this script to tell me what to do with this output_dir ", output_dir

    campaignjson_file = open( os.path.join( output_dir, campaign_name), "w" )
    campaignjson_file.write( json.dumps( campaignjson, sort_keys=True, indent=4 ) )
    campaignjson_file.close()

def plotChannelOverTime(channel_name, channel_name_normalization, output_data, subplot):
    channel_group = output_data["Channels"][channel_name]["Data"]
    if channel_name_normalization != '':
        channel_group = [x/y if y else 0 for x, y in zip(channel_group, output_data["Channels"][channel_name_normalization]["Data"])]
    if 'Num' in channel_name:
        channel_group = numpy.cumsum(channel_group)
    xvalues = [x*5 for x in range(1, len(channel_group)+1, 1)]
    plt.xlabel("Simulation Time", fontsize = 8)
    subplot.plot(xvalues, channel_group, color = colors_for_plotting[0])
    subplot.tick_params(axis = 'both', which= 'major', labelsize=7)

    subplot.legend(fontsize = 'xx-small', loc = 1)
    plt.title( channel_name, fontsize = 8 )
    plt.ylim(0, 10100)
    plt.xlim(0, 300)

def plotAddlFigures():
    os.chdir(main_dir)
    os.chdir(output_dir)
    os.chdir("output")
    output_data = json.loads( open( "Report_Scenarios.json" ).read() )
        
    num_chans = output_data["Header"]["Channels"]
    pp = PdfPages('figures2.pdf')
     
    plt.figure()
    fig = gcf()
    fig.suptitle(output_dir)

    #graph of the population
    subplot = plt.subplot( 2,3, 1)
    plotChannelOverTime('Active TB Population', '', output_data, subplot)
    plt.title( 'Active TB Population', fontsize = 8 )    
    plt.ylabel("Population", fontsize = 8)
    #disease state of adults and children over time
    subplot = plt.subplot( 2,3, 2)
    plotChannelOverTime('Active TB Population MDR', '', output_data, subplot)
    plt.title( 'Active TB Population MDR', fontsize = 8 )    
    plt.ylabel("Population", fontsize = 8)      
    subplot = plt.subplot( 2,3, 3)
    plotChannelOverTime('Active TB Population Acquired MDR', 'Active TB Population MDR', output_data, subplot)
    plt.title( 'Fraction Active MDR TB that is Acquired', fontsize = 8 )    
    plt.ylabel("Fraction", fontsize = 8)      
    plt.ylim(-0.01, 1.01)

    subplot = plt.subplot( 2,3, 4)
    plotChannelOverTime('Active TB Population OnEmpiricTreatment', '', output_data, subplot)
    plt.title("Population OnEmpiricTreatment", fontsize = 8)      
    plt.ylabel("Population", fontsize = 8)
    subplot = plt.subplot( 2,3, 5)
    plotChannelOverTime('Population OnSecondLineCombo', '', output_data, subplot)
    plt.title("Population OnSecondLineCombo", fontsize = 8)      
    plt.ylabel("Population", fontsize = 8)
    #save the last figure
    plt.tight_layout( rect = [0, 0, 1, 0.95])
    pp.savefig(fig)
    pp.close()
def plotCompareToAcquiredOnly():
    os.chdir(main_dir)
    os.chdir(output_dirs[0])
    os.chdir("output")
    baseline_output_data = json.loads( open( "InsetChart.json" ).read() )

    os.chdir(main_dir)
    os.chdir(output_dir)
    os.chdir("output")
    new_output_data = json.loads( open( "InsetChart.json" ).read() )
    
    num_chans = new_output_data["Header"]["Channels"]
    pp = PdfPages('figures.pdf')
    for plot_idx, chan_title in enumerate(sorted(baseline_output_data["Channels"])):
        num_rows = 4
        num_cols = 4
        num_graphs = num_rows*num_cols
        if plot_idx % num_graphs ==0:
            if plot_idx != 0:
                plt.tight_layout( rect = [0, 0, 1, 0.95])
                pp.savefig(fig)
            figure_num = plot_idx/num_graphs
            plt.figure()
            fig = gcf()
            fig.suptitle("Red = AcquiredMDR, Blue = " + output_dir)
        subplot = plt.subplot( num_rows,num_cols, plot_idx +1 - figure_num*num_graphs)
        subplot.plot( baseline_output_data["Channels"][chan_title]["Data"], 'r')
        subplot.plot( new_output_data["Channels"][chan_title]["Data"], 'b')
        subplot.tick_params(axis = 'both', which= 'major', labelsize=8)
        plt.xticks(range(0, baseline_output_data["Header"]["Timesteps"] + 1, 100))
        plt.title( chan_title, fontsize = 8 )
    #save the last figure
    plt.tight_layout( rect = [0, 0, 1, 0.95])
    pp.savefig(fig)
    pp.close()
            


###this is the main code
    
# specify paths
binary_path = "C:\\src\\dtk\\TBRelease\\Eradication\\x64\\Release\\Eradication.exe"
input_path  = "C:\\src\\inputs\\TB_single_node"
dll_path = "C:\\src\\dtk\\TBRelease\\Eradication\\x64\\Release"
main_dir = "C:\\src\\dtk\\TBRelease\\Regression\\TB_55_scenariotutorial_MDR"
output_dirs = [ "AcquiredMDR", "TransmittedMDR","AcquiredandTransmittedMDR", "IneffectiveRetreatmentforMDRTB", "MDRDiagnosticandSecondLineDrugsforMDRTB"]


colors_for_plotting = ['r', 'b', 'g', 'y', 'm']

os.chdir(main_dir)

config_name   = "config.json"
campaign_name = "campaign.json"

for output_dir in output_dirs:
    os.chdir(main_dir)    
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    modifyConfigAndCampaigns()
    
    #call dtk
    os.chdir(output_dir)
#    logfile = open("stdout.txt", "w")
#   subprocess.call( [ binary_path, "-C", config_name, "-I", input_path, "-D", dll_path] )

    #plot if it is anything other than baseline
    if output_dir != "AcquiredMDR":
        plotCompareToAcquiredOnly()
    plotAddlFigures()

plt.show()