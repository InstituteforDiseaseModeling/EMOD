import subprocess
import json
import os
import matplotlib.pyplot as plt

def plot_extras():
    plt.margins( 0.05, 0.1 )        
    plt.autoscale()
    plt.rcParams.update( {'font.size': 7} )   # Make smaller font


def plotBinnedReport(output_data):
    # colors = [ 'm', 'r', 'Darkorange', 'y', 'g', 'c', 'b', 'MidnightBlue', 'DarkViolet', 'k' ] #somewhat rainbow for now, TODO figure out legend
    num_chans = output_data["Header"]["Channels"]
    num_age_bins = output_data["Header"]["Subchannel_Metadata"]["NumBinsPerAxis"]
    num_age_bins = int(num_age_bins[0] ) #convert the num_age_bins param from a list to an integer
    plt.figure()
    for chan_idx, chan_title in enumerate(sorted(output_data["Channels"])):
        try:
            num_rows = 3
            num_cols = 3
            subplot = plt.subplot( num_rows,num_cols, chan_idx)
            for age_idx in range(num_age_bins):
                subplot.plot( output_data["Channels"][chan_title]["Data"][age_idx])
            plt.title( chan_title )
            plot_extras()
            
        except Exception as ex:
            print str(ex) + ", idx = " + str(chan_idx)
        if chan_idx == (num_chans + 1):
            break


def plotReports(output_data,color='r'):

    num_chans = output_data["Header"]["Channels"]
    for plot_idx, chan_title in enumerate(sorted(output_data["Channels"])):
        try:
            num_rows = 3   
            num_cols = 4
            if plot_idx%12 == 0:
                plt.figure()
            subplot = plt.subplot( num_rows,num_cols, plot_idx%12 + 1)
            # subplot.plot( output_data["Channels"][chan_title]["Data"], 'r')
            subplot.plot( output_data["Channels"][chan_title]["Data"], color)
            plt.title( chan_title )        #plt.ylabel( output_data["Channels"][chan_title]["Units"] )
            plot_extras()
            
        except Exception as ex:
            print str(ex) + ", idx = " + str(plot_idx)
        if plot_idx == (num_chans + 1):
            break


# specify paths
binary_path = "C:\\src\\dtk\\\Tuberculosis-2013-current\\Eradication\\x64\\Release\\Eradication.exe"
#binary_path = "C:\\TB_current\\Eradication\\x64\\Release\\Eradication.exe"
# binary_path = "C:\\TB_HIV\\Eradication\\x64\Release\\Eradication.exe"
# input_path  = "C:\\src\\Research\\TB\\scripts\\regression_tests"
input_path  = "C:\\EMOD_Research_current\\Groups\\TB\\trunk\\scripts\\regression_tests\\6_OutbreakIndividual"
main_dir = "C:\\src\\Research\\TB\\scripts\\regression_tests\\6_OutbreakIndividual"
#main_dir = "C:\\EMOD_Research_current\\Groups\\TB\\trunk\\scripts\\regression_tests\\6_OutbreakIndividual"

os.chdir(main_dir)

config_name   = "config_flat.json"
# campaign_name = "campaign.json"

# commission job
logfile = open("stdout.txt", "w")
# p = subprocess.Popen( [ binary_path, "-C", "config_flat.json", "--input", input_path, "--output", "output" ], stdout=logfile )
# subprocess.call( [ binary_path, "-C", "config_flat.json", "--input", input_path, "--output", "output", ] )
subprocess.call( [ binary_path, "-C", config_name, "--input", input_path, "--output", "output", ] )


#plot the output
output_directory = os.path.join( main_dir, "output")
os.chdir(output_directory)

#InsetChart.json
output_data = json.loads( open( "InsetChart.json" ).read() )
plotReports(output_data)

#PropertyReportTB.json
output_data = json.loads( open( "PropertyReportTB.json" ).read() )
plotReports(output_data)

#DemographicsSummary.json
output_data = json.loads( open( "DemographicsSummary.json" ).read() )
plotReports(output_data)

#BinnedReport.json
output_data = json.loads( open( "BinnedReport.json" ).read() )
plotBinnedReport(output_data)

plt.show()

