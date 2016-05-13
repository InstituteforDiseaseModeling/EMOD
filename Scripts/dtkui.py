#!/usr/bin/python

import Tkinter as tk
import subprocess
import json
import os
import sys
import signal
import numpy as np  

import matplotlib
matplotlib.use( "TkAgg" )

import matplotlib.pyplot as plt  

plt.ion()

dtk_run = None

#runBtn = None
wth = "WTH?"

#from ttk import Frame, Button, Style

class MyApp( tk.Frame ):
    def __init__(self, parent):
        tk.Frame.__init__( self, parent, background="white" )
        self.parent = parent
        self.button_height = 10
        self.editor_exe = "gedit"
        #self.editor_exe = [ "cmd.exe", "/c", "start", "/max", "notepad.exe" ]
        self.eradication_exe = "Eradication"
        #self.eradication_exe = "B:/Eradication/x64/Release/Eradication.exe"
        self.chart_plotter = "plotAllCharts.py"
        #self.chart_plotter = "python P:/Regression/plotAllCharts.py"
        self.input_dir = "/home/jbloedow/EMOD/InputFiles/"
        #self.input_dir = "n:/public/input/TIP/SamplesInput"
        #self.input_dir = "n:/home/jbloedow/input/auto-generated"
        #self.input_dir = "."
        self.initUI()

    def initUI(self):
        self.config_name = "config_flat.json"
        if( os.path.exists( "config_flat.json" ) == False ):
            self.config_name = "config.json"

        self.parent.title( "DTK: " + json.loads( open( self.config_name ).read() )[ "parameters" ][ "Config_Name" ] )
        self.pack( fill=tk.BOTH, expand=1 )

        configJsonBtn = tk.Button( self, text="Open config.json", command=self.config_dot_json )
        configJsonBtn.pack( side=tk.LEFT )
        
        campaignJsonBtn = tk.Button( self, text="Open campaign.json", command=self.campaign_dot_json ).pack( side=tk.LEFT )
        #campaignJsonBtn.place( x=150,y=self.button_height )

        self.runBtn = tk.Button( self, text="Run DTK", command=self.run_dtk )
        self.runBtn.pack( side=tk.LEFT )
        #print( self.runBtn )
        #runBtn.place( x=300,y=self.button_height )

        insetChartJsonBtn = tk.Button( self, text="Plot Output", command=self.insetChart_dot_json ).pack( side=tk.LEFT )
        #insetChartJsonBtn.place( x=400,y=self.button_height )

        quitButton = tk.Button( self, text="Quit", command=self.quit ).pack( side=tk.RIGHT )
        #quitButton.place(x=550,y=self.button_height)

    def config_dot_json(self):
        config_params = []
        #config_params.extend( self.editor_exe )
        config_params.append( self.editor_exe )
        config_params.append( self.config_name )
        subprocess.call( 
            config_params
        ) 

    def campaign_dot_json(self):
        campaign_params = []
        #campaign_params.extend( self.editor_exe )
        campaign_params.append( self.editor_exe )
        campaign_params.append( "campaign.json" )
        print( campaign_params )
        subprocess.call( 
            campaign_params
        ) 

    def run_dtk(self):
        #print( self.runBtn )
        self.runBtn[ "text" ] = "Running..."

        config_name = "config_flat.json"
        if( os.path.exists( "config_flat.json" ) == False ):
            config_name = "config.json"
        sim_duration = int(json.loads( open( config_name ).read() )["parameters"]["Simulation_Duration"])

        fig = plt.figure()
        mng = plt.get_current_fig_manager()
        mng.resize(*mng.window.maxsize())
        #mng.window.state('zoomed')
        ax = fig.add_subplot(111)
        plt.suptitle( "Prevalence (red=reference, blue=test)" )

        line1, = ax.plot([], [],'-r')

        ref_output = "output/InsetChart.json"
        if os.path.exists( ref_output ):
            ref_data = json.loads( open( ref_output ).read() )["Channels"]["Infected"]["Data"]
            line1.set_ydata( ref_data )
            line1.set_xdata( range( len( ref_data ) ) )
        ax.axis( [ 0, sim_duration, 0, 1.0 ] )

        line2, = ax.plot([], [],'-b')
        data_values = []
        plt.draw()

        params = [
            self.eradication_exe,
            "--config",
            config_name,
            "--input",
            self.input_dir,
            "--output",
            "test"
        ]
        print( params )
        global dtk_run
        #dtk_run = subprocess.Popen( params, shell=True, stdout=subprocess.PIPE )
        dtk_run = subprocess.Popen( params, shell=False, stdout=subprocess.PIPE )
        counter = 0
        for line in dtk_run.stdout:
            if( "Exception" in line ):
                print( line )
            elif( "Infected" in line ):
                counter = counter + 1
#                print( line.rstrip() )
                #tstep = int( line.split()[6].rstrip( '.0') )
                tstep = float(line.split()[6])
                prog_msg = ""
                #print( str( tstep ) )
                if tstep>0:
                    prog_msg = str( int( 100*tstep/sim_duration ) )
                    #sys.stdout.write( "Progress: " + prog_msg + "%\r" )
                infected = float(line.split()[len(line.split())-1])
                pop = float(line.split()[10])
                #print( str(infected), str(pop) )
                if pop > 0:
                    data_values.append( infected/pop )
                else:
                    data_values.append( 0 )
                line2.set_ydata( data_values )
                line2.set_xdata( range( len( data_values ) ) )
                if counter % 5 == 0:
                    self.runBtn["text"] = prog_msg +"%"
                    self.runBtn.update()
                    plt.draw()
        self.runBtn["text"] = "Run DTK"

    def insetChart_dot_json(self):
        second_file = "output/InsetChart.json"
        test_output = "test/InsetChart.json"
        if( os.path.exists( test_output ) ):
            second_file = test_output
        sim_name = None
        if( os.path.exists( "config_flat.json" ) ):
            sim_name = json.loads( open( "config_flat.json" ).read() )["parameters"]["Config_Name"]
        else:
            sim_name = json.loads( open( "config.json" ).read() )["parameters"]["Config_Name"]
        params = [
            self.chart_plotter,
            #"python",
            #"P:/Regression/plotAllCharts.py",
            "output/InsetChart.json",
            second_file,
            sim_name
        ]
        #print( params )
        subprocess.call( params ) 

def ctrl_c_handler( signal, frame ):
    global dtk_run
    try:
        dtk_run.terminate()
    except Exception as ex:
        pass # I'm a bad boy

signal.signal(signal.SIGINT, ctrl_c_handler)

root = tk.Tk()
myapp = MyApp( root )
#root.title( "DTK" )
#root.geometry( "640x50" )
root.geometry( "640x30" )

root.mainloop()
