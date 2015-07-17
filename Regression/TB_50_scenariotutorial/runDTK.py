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
    if output_dir == "baseline":
        print "using baseline"
    elif output_dir == "LatentFastAndSlow":
        configjson["parameters"]["TB_Fast_Progressor_Fraction_Adult"] = 0
        configjson["parameters"]["TB_Fast_Progressor_Fraction_Child"] = 0
        configjson["parameters"]["TB_Slow_Progressor_Rate"]= 0.02
    elif output_dir == "ActivePresymptomatic":
        configjson["parameters"]["TB_Active_Cure_Rate"] = 1
        configjson["parameters"]["TB_Base_Infectivity"] = 0
        configjson["parameters"]["TB_Base_Infectivity_Presymptomatic"] = 0.25
        configjson["parameters"]["TB_Presymptomatic_Cure_Rate"]= 0
        configjson["parameters"]["TB_Presymptomatic_Rate"]= 0.02
    elif output_dir == "ActiveSymptomaticSmearNeg" or output_dir == "ActiveSymptomaticSmearNegReducedR0":
        configjson["parameters"]["TB_Smear_Positive_Fraction_Adult"] = 0
        configjson["parameters"]["TB_Smear_Positive_Fraction_Child"] = 0
        configjson["parameters"]["TB_Extrapulmonary_Fraction_Adult"]= 0
        configjson["parameters"]["TB_Extrapulmonary_Fraction_Child"]= 0
        configjson["parameters"]["TB_Smear_Negative_Infectious_Multiplier"]= 1
        if output_dir == "ActiveSymptomaticSmearNegReducedR0":
            configjson["parameters"]["TB_Smear_Negative_Infectious_Multiplier"]= 0.5
    elif output_dir == "ActiveSymptomaticExtrapulm":
        configjson["parameters"]["TB_Smear_Positive_Fraction_Adult"] = 0
        configjson["parameters"]["TB_Smear_Positive_Fraction_Child"] = 0
        configjson["parameters"]["TB_Extrapulmonary_Fraction_Adult"]= 1
        configjson["parameters"]["TB_Extrapulmonary_Fraction_Child"]= 1
    elif output_dir == "DiseaseDuration":
        configjson["parameters"]["TB_Active_Period_Distribution"] = "GAUSSIAN_DURATION"
        configjson["parameters"]["TB_Active_Period_Std_Dev"] = 1
    else:
        print "please modify this script to tell me what to do with this output_dir ", output_dir

    #save config to the output dir directory so you can call it
    with open( os.path.join( output_dir, config_name), 'w') as f:
        f.write( json.dumps( configjson, sort_keys=True, indent=4 ) )
        f.close()
    #copy over the campaign file
    campaignjson = json.loads( open( campaign_name).read() )
    campaignjson_file = open( os.path.join( output_dir, campaign_name), "w" )
    campaignjson_file.write( json.dumps( campaignjson, sort_keys=True, indent=4 ) )
    campaignjson_file.close()

def plot_SEIR():
    os.chdir(main_dir)
    os.chdir(output_dirs[0])
    os.chdir("output")
    baseline_output_data = json.loads( open( "InsetChart.json" ).read() )

    num_chans = baseline_output_data["Header"]["Channels"]
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
            fig.suptitle("Red = baseline, Blue = " + output_dir)
        subplot = plt.subplot( num_rows,num_cols, plot_idx +1 - figure_num*num_graphs)
        subplot.plot( baseline_output_data["Channels"][chan_title]["Data"], 'r')
        subplot.tick_params(axis = 'both', which= 'major', labelsize=8)
        plt.xticks(range(0, baseline_output_data["Header"]["Timesteps"] + 1, 100))
        plt.title( chan_title, fontsize = 8 )
    plt.figure()
    fig = gcf()
    fig.suptitle(output_dir)
    subplot = plt.subplot( 1,1, 1)
    susceptible = [x -x*(y+z+a) for x, y, z, a in zip(baseline_output_data["Channels"]["Statistical Population"]["Data"],
                                                baseline_output_data["Channels"]["TB Immune Fraction"]["Data"],
                                                baseline_output_data["Channels"]["Latent TB Prevalence"]["Data"],
                                                baseline_output_data["Channels"]["Active TB Prevalence"]["Data"])]
    latent_pop = [x*y for x, y in zip(baseline_output_data["Channels"]["Statistical Population"]["Data"],
                                                 baseline_output_data["Channels"]["Latent TB Prevalence"]["Data"])]
    active_pop = [x*y for x, y in zip(baseline_output_data["Channels"]["Statistical Population"]["Data"],
                                                 baseline_output_data["Channels"]["Active TB Prevalence"]["Data"])]
    recovered_pop = [x*y for x, y in zip(baseline_output_data["Channels"]["Statistical Population"]["Data"],
                                         baseline_output_data["Channels"]["TB Immune Fraction"]["Data"])]
    subplot.plot( susceptible, 'b', label='Susceptible')
    subplot.plot( latent_pop, 'g', label='Latent')
    subplot.plot( active_pop, 'r', label='Active')
    subplot.plot( recovered_pop, 'c', label='Recovered')
    subplot.legend(loc = 7)
    plt.xlabel("Simulation Time")
    plt.ylabel("Number of people")

    #save the last figure
    plt.tight_layout( rect = [0, 0, 1, 0.95])
    pp.savefig(fig)
    pp.close()

def compareBaselineToNew():
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
            fig.suptitle("Red = baseline, Blue = " + output_dir)
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
binary_path = "C:\\src\\dtk\\trunk\\Eradication\\x64\\Release\\Eradication.exe"
input_path  = "C:\\src\\inputs\\TB_single_node"
main_dir = "C:\\src\\dtk\\trunk\\Regression\\TB_50_scenariotutorial"
output_dirs = ["LatentFastAndSlow", "ActivePresymptomatic",
               "ActiveSymptomaticSmearNeg", "ActiveSymptomaticSmearNegReducedR0", "ActiveSymptomaticExtrapulm", "DiseaseDuration"]


#always run baseline in addition to the edited ones
output_dirs.insert(0, "baseline")
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
    subprocess.call( [ binary_path, "-C", config_name, "-I", input_path] )

    #plot if it is anything other than baseline
    if output_dir != output_dirs[0]:
        compareBaselineToNew()
    if output_dir == output_dirs[0]:
        plot_SEIR()
plt.show()