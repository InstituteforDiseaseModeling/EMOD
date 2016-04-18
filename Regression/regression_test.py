#!/usr/bin/python

from hashlib import md5
from regression_utils import *
import BaseHTTPServer
import ConfigParser
import SimpleHTTPServer
import SocketServer
import argparse
import cgi
import datetime
import glob
import httplib
import json
import os # e.g., mkdir
import plotAllCharts
import re
import shutil # copyfile
import subprocess
import sys # for stdout.flush
import tempfile
import threading
import time # for sleep
import urllib
import urlparse
import xml.dom.minidom
import pdb

# global variable >:p -- actually, it's a module variable :) 
params = None
MAX_ACTIVE_JOBS=20

class RuntimeParameters:
    def __init__(self, args):
        print( "os = " + os.name )
        self.args = args
        if not os.path.exists(args.config) :
            print("Couldn't find configuration-file \"" + args.config + "\"")
            sys.exit()

        user = None
        username_key = None
        if os.name == "posix":
            self.os_type = "POSIX"
            username_key = "USER"
            user = ""
        else:
            self.os_type = "WINDOWS"
            username_key = "USERNAME"
            user = os.environ["USERDOMAIN"] + '\\'
            
        user += os.environ[username_key]
        self.config = ConfigParser.SafeConfigParser({'password':'', 'username':user})
        self.config.read(args.config)
        self.config.set('ENVIRONMENT', 'username', os.environ[username_key])
        self._use_user_input_root = False
        self.PSP = None
    
    @property
    def suite(self):
        return self.args.suite
        
    @property
    def executable_path(self):
        return self.args.exe_path
    
    @property
    def measure_perf(self):
        return self.args.perf
    
    @property
    def hide_graphs(self):
        return self.args.hidegraphs
    
    @property
    def debug(self):
        return self.args.debug
    
    @property
    def quick_start(self):
        return self.args.quick_start
    
    @property
    def use_dlls(self):
        if self.args.dll_path is not None:
            return True
        else:
            return self.args.use_dlls
    
    @property
    def scons(self):
        return self.args.scons
    
    @property
    def label(self):
        return self.args.label

    @property
    def regression_config(self):
        return self.args.config
        
    @property
    def config(self):
        return self.config

    @property
    def hpc_head_node(self):
        return self.config.get('HPC', 'head_node')
        
    @property
    def hpc_node_group(self):
        return self.config.get('HPC', 'node_group')
        
    @property
    def hpc_user(self):
        return self.config.get('HPC', 'username')
        
    @property
    def hpc_password(self):
        return self.config.get('HPC', 'password')
        
    @property
    def cores_per_socket(self):
        return self.config.getint('HPC', 'num_cores_per_socket')
        
    @property
    def cores_per_node(self):
        return self.config.getint('HPC', 'num_cores_per_node')

    @property
    def local_sim_root(self):
        return self.config.get(self.os_type, 'local_sim_root')
        
    @property
    def input_path(self):
        return self.config.get(self.os_type, 'local_input_root')
        
    @property
    def local_bin_root(self):
        return self.config.get(self.os_type, 'local_bin_root')

    @property
    def sim_root(self):
        return self.config.get('ENVIRONMENT', 'sim_root')
        
    @property
    def shared_input(self):
        return self.config.get('ENVIRONMENT', 'input_root')
        
    @property
    def bin_root(self):
        return self.config.get('ENVIRONMENT', 'bin_root')
        
    @property
    def user_input(self):
        return self.config.get('ENVIRONMENT', 'home_input')
        
    @property
    def py_input(self):
        return self.config.get('ENVIRONMENT', 'py_input')
        
    @property
    def use_user_input_root(self):
        return self._use_user_input_root
        
    @use_user_input_root.setter
    def use_user_input_root(self, value):
        self._use_user_input_root = value
        
    @property
    def input_root(self):
        if not self.use_user_input_root:
            return self.shared_input
        else:
            return self.user_input
        
    @property
    def dll_path(self):
        return self.args.dll_path

    @property
    def dll_root(self):
        try:
            dll_root = self.config.get('ENVIRONMENT', 'dll_root')
        except Exception as ex:
            dll_root = self.config.get('ENVIRONMENT', 'bin_root')
        return dll_root

    @property
    def src_root(self):
        try:
            src_root = self.config.get('LOCAL-ENVIRONMENT', 'src_root')
        except Exception as ex:
            src_root = ".\\.."
        return src_root

    @property
    def all_outputs(self):
        return self.args.all_outputs
        
    @property
    def dts(self):
        return self.args.disable_schema_test
        
    @property
    def sec(self):
        return self.args.skip_emodule_check

    @property
    def constraints_dict(self):
        constraints_list = self.args.config_constraints
        constraints_dict = {}
        for raw_nvp in constraints_list:
            nvp = raw_nvp.split(":")
            constraints_dict[ nvp[0] ] = nvp[1]
        return constraints_dict
        
class Monitor(threading.Thread):
    def __init__(self, sim_id, config_id, report, config_json=None, compare_results_to_baseline=True):
        threading.Thread.__init__( self )
        #print "Running DTK execution and monitor thread."
        sys.stdout.flush()
        self.sim_timestamp = sim_id
        self.config_id = config_id
        self.report = report
        self.config_json = config_json
        self.duration = None
        # can I make this static?
        self.sim_root = params.local_sim_root
        self.compare_results_to_baseline = compare_results_to_baseline

    def run(self):
        MyRegressionRunner.sems.acquire()
        sim_dir = os.path.join( self.sim_root, self.sim_timestamp )
        #os.chdir( sim_dir )    # NOT THREAD SAFE!

        starttime = datetime.datetime.now()

        with open(os.path.join(sim_dir, "stdout.txt"), "w") as stdout, open(os.path.join(sim_dir, "stderr.txt"), "w") as stderr:
            actual_input_dir = os.path.join( params.input_path, self.config_json["parameters"]["Geography"] )
            cmd = None
            # python-script-path is optional parameter.
            if "PSP" in self.config_json:
                cmd = [self.config_json["bin_path"], "-C", "config.json", "--input-path", actual_input_dir, "--python-script-path", self.config_json["PSP"]]
            else:
                cmd = [self.config_json["bin_path"], "-C", "config.json", "--input-path", actual_input_dir ]
            print( "Calling '" + str(cmd) + "' from " + sim_dir + "\n" )
            proc = subprocess.Popen( cmd, stdout=stdout, stderr=stderr, cwd=sim_dir )
            proc.wait()
        # JPS - do we want to append config_json["parameters"]["Geography"] to the input_path here too like we do in the HPC case?
        endtime = datetime.datetime.now()
        self.duration = endtime - starttime
        os.chdir( cache_cwd )
        global completed
        completed = completed + 1
        print( str(completed) + " out of " + str(len(reg_threads)) + " completed." )
        # JPS - should check here and only do the verification if it passed... ?
        if self.compare_results_to_baseline:
            if params.all_outputs == False:
            # Following line is for InsetChart.json only
                self.verify(sim_dir)
            else:
                # Every .json file in output (not hidden with . prefix) will be used for validation
                for file in os.listdir( os.path.join( self.config_id, "output" ) ):
                    if ( file.endswith( ".json" ) or file.endswith( ".csv" ) ) and file[0] != ".":
                        self.verify( sim_dir, file, "Channels" )
        MyRegressionRunner.sems.release()

    def get_json_data_hash( self, data ):
        #json_data = collections.OrderedDict([])
        #json_data["Data"] = data
        with tempfile.TemporaryFile() as handle:
            json.dump( data, handle )
            hash = md5_hash( handle )
        return hash

    def compareJsonOutputs( self, sim_dir, report_name, ref_path, test_path, failures ):
        fail_validation = False
        failure_txt = ""

        ref_json = json.loads( open( os.path.join( cache_cwd, ref_path ) ).read() )
        if "Channels" not in ref_json.keys():
            ref_md5  = md5_hash_of_file( ref_path )
            test_md5 = md5_hash_of_file( test_path )
            if ref_md5 == test_md5:
                return False, ""
            else:
                print( self.config_id + " completed but did not match reference! (" + str(self.duration) + ") - " + report_name )
                return True, "Non-Channel JSON failed MD5."
        else:
            test_json = json.loads( open( os.path.join( sim_dir, test_path ) ).read() )

            if "Channels" not in test_json.keys():
                return True, "Reference has Channel data and Test file does not."

            ref_md5  = self.get_json_data_hash( ref_json["Channels"] )
            test_md5 = self.get_json_data_hash( test_json["Channels"] )

            ref_channels = set(ref_json["Channels"])
            test_channels = set(test_json["Channels"])

            if ref_md5 == test_md5:
                return False, ""

            missing_channels = ref_channels - test_channels
            new_channels = test_channels - ref_channels

            if len(missing_channels) > 0:
                fail_validation = True
                print("ERROR: Missing channels - " + ', '.join(missing_channels))
                failure_txt += "Missing channels:\n" + '\n'.join(missing_channels) + "\n"
                self.report.addFailingTest( self.config_id, failure_txt, os.path.join( sim_dir, ( "output/" + report_name ) ) )

            if len(new_channels) > 0:
                print("WARNING: The test "+report_name+" has " + str(len(new_channels)) + " channels not found in the reference.  Please update the reference "+report_name+".")
                global final_warnings
                final_warnings += self.config_id + " - New channels not found in reference:\n  " + '\n  '.join(new_channels) + "\nPlease update reference from " + os.path.join( sim_dir, os.path.join( "output", "InsetChart.json" ) ) + "!\n"
                self.report.addFailingTest( self.config_id, failure_txt, os.path.join( sim_dir, ( "output/" + report_name ) ) )

            if "Header" in ref_json.keys() and ref_json["Header"]["Timesteps"] != test_json["Header"]["Timesteps"]:
                warning_msg = "WARNING: test "+report_name+" has timesteps " + str(test_json["Header"]["Timesteps"])  + " DIFFERRING from ref "+report_name+" timesteps " + str(ref_json["Header"]["Timesteps"]) + "!\n"
                if params.hide_graphs:
                    # This is treated as automated running mode (or bamboo nightly build mode)
                    fail_validation = True
                    failure_txt += warning_msg
                else:
                    # This is treated as manual running mode
                    final_warnings += warning_msg
                    print(warning_msg)

            if not fail_validation:
                #print( "Hasn't failed validation on second level review. Time to look channel by channel, timestep by timestep." )
                # BinnedReport and its derived classes have "Subchannel_Metadata" in the header
                if "Header" in ref_json.keys() and "Subchannel_Metadata" in ref_json["Header"].keys():
                    self.compareBinnedReportType( ref_json, test_json, failures )
                elif "Header" in ref_json.keys() and "Report_Type" in ref_json["Header"].keys() and ref_json["Header"]["Report_Type"] =="InsetChart":
                    # Assuming a BaseChannelReport
                    self.compareChannelReportType( ref_json, test_json, failures )
                else:
                    fail_validation = True
                    failures.append(report_name + " - Files are different but cannot do deep dive.")

            if len(failures) > 0:
                fail_validation = True
                failure_txt += "Channel Timestep Reference_Value Test_Value\n" + ''.join(failures)
                print( self.config_id + " completed but did not match reference! (" + str(self.duration) + ") - " + report_name )

        return fail_validation, failure_txt

    def compareCsvOutputs( self, ref_path, test_path, failures ):
        # print( "Comparing CSV files: ref = " + ref_path + ", test = " + test_path )
        # Do Md5 comp first.
        ref_md5 = md5_hash_of_file( ref_path )
        test_md5 = md5_hash_of_file( test_path )
        if ref_md5 == test_md5:
            # print( "CSV files passed MD5 comparison test." )
            return False, ""

        fail_validation = False
        err_msg = ""

        # print( "CSV files failed MD5 comparison test." )
        # First (md5) test failed. Do line length, then line-by-line
        ref_length = file_len( ref_path )
        test_length = file_len( test_path )
        if ref_length != test_length:
            fail_validation = True
            err_msg = "Reference output {0} has {1} lines but test output {2} has {3} lines".format( ref_path, ref_length, test_path, test_length )

        else:
            ref_file = open( ref_path )
            test_file = open( test_path )
            line_num = 0
            for ref_line in ref_file:
                line_num = line_num + 1
                test_line = test_file.readline()
                if ref_line != test_line:
                    ref_line_tokens = ref_line.split(',')
                    test_line_tokens = test_line.split(',')
                    for col_idx in range( len( ref_line_tokens) ):
                        if ref_line_tokens[col_idx] != test_line_tokens[col_idx]:
                            break
                    err_msg = "First mismatch at line {0} of {1} column {2}: reference line...\n{3}vs test line...\n{4}{5} vs {6}".format( line_num, ref_path, col_idx, ref_line, test_line, ref_line_tokens[col_idx], test_line_tokens[col_idx] ) 
                    fail_validation = True
                    ref_file.close()
                    test_file.close()
                    break

        print( err_msg )
        failure_txt = err_msg
        #self.report.addFailingTest( self.config_id, failure_txt, test_path )
        return fail_validation, failure_txt

    def compareOtherOutputs( self, report_name, ref_path, test_path, failures ):
        ref_md5 = md5_hash_of_file( ref_path )
        test_md5 = md5_hash_of_file( test_path )
        if ref_md5 == test_md5:
            # print( "CSV files passed MD5 comparison test." )
            return False, ""
        else:
            print( self.config_id + " completed but did not match reference! (" + str(self.duration) + ") - " + report_name )
            return True, "Failes MD5 check."

    # Compare Binned Report Types
    def compareBinnedReportType( self, ref_json, test_json, failures ):
        num_bins_ref  = ref_json[ "Header"]["Subchannel_Metadata"]["NumBinsPerAxis"][0][0]
        num_bins_test = test_json["Header"]["Subchannel_Metadata"]["NumBinsPerAxis"][0][0]

        if num_bins_ref != num_bins_test:
            error_txt = report_name + ": Reference(NumBinsPerAxis=" + str(num_bins_ref) +") != Test(NumBinsPerAxis="+ str(num_bins_test) +")"
            print( error_txt )
            failures.append( error_txt )
        else:
            ref_channels  = set(ref_json[ "Channels"])
            test_channels = set(test_json["Channels"])

            min_tstep_ind = min(ref_json["Header"]["Timesteps"], test_json["Header"]["Timesteps"])

            for chan_title in (ref_channels & test_channels):
                for bin_idx in range( 0, num_bins_ref ):
                    for tstep_idx in range( 0, min_tstep_ind ):
                        val_ref  = ref_json[ "Channels"][chan_title]["Data"][bin_idx][tstep_idx]
                        val_test = test_json["Channels"][chan_title]["Data"][bin_idx][tstep_idx]
                        if val_ref != val_test:
                            failures.append(chan_title + " " + str(bin_idx) + " " + str(tstep_idx) + " " + str( val_ref ) + " " + str( val_test ) + "\n")
        return

    # Compare Channel Report Types
    def compareChannelReportType( self, ref_json, test_json, failures ):
        ref_channels  = set(ref_json[ "Channels"])
        test_channels = set(test_json["Channels"])

        min_tstep_ind = min(ref_json["Header"]["Timesteps"], test_json["Header"]["Timesteps"])

        for chan_title in (ref_channels & test_channels):
            #print( "Looking at channel {0}".format( chan_title ) )
            num_steps_ref  = len(ref_json["Channels"][chan_title]["Data"])
            num_steps_test = len(test_json["Channels"][chan_title]["Data"])
            if( (min_tstep_ind > num_steps_ref) or (min_tstep_ind > num_steps_test) ):
                failures.append("Reference has "+str(num_steps_ref) + " steps and test has "+str(num_steps_test)+" steps, but the header says the min Timesteps is "+str(min_tstep_ind))
                print("!!!! Reference has "+str(num_steps_ref) + " steps and test has "+str(num_steps_test)+" steps, but the header says the min Timesteps is "+str(min_tstep_ind))
                return
            for tstep_idx in range( 0, min_tstep_ind ):
                if test_json["Channels"][chan_title]["Data"][tstep_idx] != ref_json["Channels"][chan_title]["Data"][tstep_idx]:
                    failures.append(chan_title + " " + str(tstep_idx) + " " + str( ref_json["Channels"][chan_title]["Data"][tstep_idx] ) + " " + str( test_json["Channels"][chan_title]["Data"][tstep_idx] ) + "\n")
        return

    # Adding optional report_name parameter, defaults to InsetChart
    def verify(self, sim_dir, report_name="InsetChart.json", key="Channels" ):
        #print( "Checking if report " + report_name + " based on key " + key + " matches reference..." )
        # check if insetchart matched
        # since ICJ now has header, just calculate md5 on data section
        # Read whole file and write channel data to temp file. Calculate md5 on that.
        
        # different reports obviously have different structure. The only report we currently have defailt 
        # structural knowledge about is InsetChart, but we should know the top level keys. PolioPatientSurvey
        # has "patient_array" as the top level key.
        global cache_cwd
        global regression_id, completed, reg_threads

        fail_validation = False
        failures = []
        failure_txt = ""

        test_path = os.path.join( sim_dir, os.path.join( "output", report_name ) )
        ref_path = os.path.join( cache_cwd, os.path.join( str(self.config_id), os.path.join( "output", report_name ) ) )

        # if on linux, use alternate InsetChart.json, but only if exists
        if os.name != "nt" and report_name == "InsetChart.json":
            report_name = "InsetChart.linux.json" 
            alt_ref_path = os.path.join( cache_cwd, os.path.join( str(self.config_id), os.path.join( "output", report_name ) ) )
            if os.path.exists( alt_ref_path ):
                ref_path = alt_ref_path

        #if test_hash != ref_hash:
        if os.path.exists( test_path ) == False:
            print( "Test file \"" + test_path + "\" -- for " + self.config_id + " -- does not exist." )
            failure_txt = "Report not generated by executable."
            self.report.addFailingTest( self.config_id, failure_txt, os.path.join( sim_dir, ( "output/" + report_name ) ) )
            return False

        if test_path.endswith( ".csv" ):
            fail_validation, failure_txt = self.compareCsvOutputs( ref_path, test_path, failures )

        elif test_path.endswith( ".json" ):
            fail_validation, failure_txt = self.compareJsonOutputs( sim_dir, report_name, ref_path, test_path, failures )

        elif test_path.endswith( ".kml" ) or test_path.endswith( ".bin" ):
            fail_validation, failure_txt = self.compareOtherOutputs( report_name, ref_path, test_path, failures )

        if fail_validation:
            #print( "Validation failed, add to failing tests report." )
            self.report.addFailingTest( self.config_id, failure_txt, os.path.join( sim_dir, ( "output/" + report_name ) ) )

            if len(failures) > 0 and not params.hide_graphs and report_name.startswith( "InsetChart" ):
                #print( "Plotting charts for failure deep dive." )  
                # Note: Use python version 2 for plotAllCharts.py
                subprocess.Popen( ["python", "plotAllCharts.py", ref_path, test_path, self.config_id ] )
        else:
            print( self.config_id + " passed (" + str(self.duration) + ") - " + report_name )
            self.report.addPassingTest(self.config_id, self.duration)
            
            global version_string
            if version_string is not None:
                try:
                    timefile = open( os.path.join( self.config_id, "time.txt" ), 'a' )
                    timefile.write(version_string + '\t' + str(self.duration) + '\n')
                    timefile.close()
                except Exception as e:
                    print("Problem writing time.txt file (repeat of error Jonathan was seeing on linux?)\n")
                    print(str(e))

        
class CommandlineGenerator(object):
    def __init__(self, exe_path, options, params):
        self._exe_path = exe_path
        self._options  = options
        self._params   = params
        
    @property
    def Executable(self):
        return self._exe_path
    
    @property
    def Options(self):
        options = []
        for k,v in self._options.items():
            if k[-1] == ':':
                options.append(k + v)   # if the option ends in ':', don't insert a space
            else:
                options.extend([k,v])   # otherwise let join (below) add a space

        return ' '.join(options)
    
    @property
    def Params(self):
        return ' '.join(self._params)
    
    @property
    def Commandline(self):
        return ' '.join([self.Executable, self.Options, self.Params])

        
class HpcMonitor(Monitor):
    def __init__(self, sim_id, config_id, report, suffix, config_json=None, compare_results_to_baseline=True):
        Monitor.__init__( self, sim_id, config_id, report, config_json, compare_results_to_baseline )
        #print "Running DTK execution and monitor thread for HPC commissioning."
        self.sim_root = params.sim_root # override base Monitor which uses local sim directory
        self.config_json = config_json
        self.config_id = config_id
        self.suffix = suffix

    def run(self):
    
        MyRegressionRunner.sems.acquire()
        def get_num_cores( some_json ):
            num_cores = 1
            if ('parameters' in some_json) and ('Num_Cores' in some_json['parameters']):
                num_cores = some_json['parameters']['Num_Cores']
            else:
               print( "Didn't find key 'parameters/Num_Cores' in '{0}'. Using 1.".format( self.config_id ) )
               
            return int(num_cores)
    
        input_dir = params.input_root + self.config_json["parameters"]["Geography"] + "\\"
        sim_dir = self.sim_root + "\\" + self.sim_timestamp   # can't use os.path.join() here because on linux it'll give us the wrong dir-separator...
        if self.suffix is not None:
            job_name = self.config_json["parameters"]["Config_Name"].replace( ' ', '_' ) + "_" + self.suffix + "_(" + self.sim_timestamp + ")"
        else:
            job_name = self.config_json["parameters"]["Config_Name"].replace( ' ', '_' ) + "_(" + self.sim_timestamp + ")"
        job_name = job_name[:79]

        numcores = get_num_cores( self.config_json )

        hpc_resource_option = '/numcores:'
        hpc_resource_count  = str(numcores)
        mpi_core_option = None
        mpi_core_count  = ''

        if params.measure_perf:
            if numcores % params.cores_per_node == 0:
                hpc_resource_option = '/numnodes:'
                hpc_resource_count  = str(numcores / params.cores_per_node)
                mpi_core_option = '-c'
                mpi_core_count  = str(params.cores_per_node)
            elif numcores == params.cores_per_socket:
                hpc_resource_option = '/numsockets:'
                hpc_resource_count  = '1'
                mpi_core_option = '-c'
                mpi_core_count  = str(params.cores_per_socket)
            # "bail" here, we don't have a multiple of cores per node nor can we fit on a single socket

        #eradication.exe commandline
        eradication_bin = self.config_json['bin_path']
        eradication_options = { '--config':'config.json', '--input-path':input_dir, '--progress':' ' }

        # python-script-path is optional parameter.
        if "PSP" in self.config_json:
            eradication_options[ "--python-script-path" ] = self.config_json["PSP"]
        #if params.dll_root is not None and params.use_dlls is True:
        #    eradication_options['--dll-path'] = params.dll_root
        eradication_params = []
        eradication_command = CommandlineGenerator(eradication_bin, eradication_options, eradication_params)

        #mpiexec commandline
        mpi_bin = 'mpiexec'
        mpi_options = {}
        if mpi_core_option is not None:
            mpi_options[mpi_core_option] = mpi_core_count
        mpi_params = [eradication_command.Commandline]
        mpi_command = CommandlineGenerator(mpi_bin, mpi_options, mpi_params)
        
        #job submit commandline
        jobsubmit_bin = 'job submit'
        jobsubmit_options = {}
        jobsubmit_options['/workdir:'] = sim_dir
        jobsubmit_options['/scheduler:'] = params.hpc_head_node
        jobsubmit_options['/nodegroup:'] = params.hpc_node_group
        jobsubmit_options['/user:'] = params.hpc_user
        if params.hpc_password != '':
            jobsubmit_options['/password:'] = params.hpc_password
        jobsubmit_options['/jobname:'] = job_name
        jobsubmit_options[hpc_resource_option] = hpc_resource_count
        if params.measure_perf:
            jobsubmit_options['/exclusive'] = ' '
        jobsubmit_options['/stdout:'] = 'StdOut.txt'
        jobsubmit_options['/stderr:'] = 'StdErr.txt'
        jobsubmit_options['/priority:'] = 'Lowest'
        jobsubmit_params = [mpi_command.Commandline]
        jobsubmit_command = CommandlineGenerator(jobsubmit_bin, jobsubmit_options, jobsubmit_params)

        #print 'simulation command line:', eradication_command.Commandline
        #print 'mpiexec command line:   ', mpi_command.Commandline
        #print 'job submit command line:', jobsubmit_command.Commandline

        hpc_command_line = jobsubmit_command.Commandline

        job_id = -1
        num_retries = -1

        while job_id == -1:
            num_retries += 1
            #print "executing hpc_command_line: " + hpc_command_line + "\n"

            p = subprocess.Popen( hpc_command_line.split(), shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE )
            [hpc_pipe_stdout, hpc_pipe_stderr] = p.communicate()
            #print "Trying to read hpc response..."
            #print hpc_pipe_stdout
            line = hpc_pipe_stdout

            if p.returncode == 0:
                job_id = line.split( ' ' )[-1].strip().rstrip('.')
                if str.isdigit(job_id) and job_id > 0:
                    print( self.config_id + " submitted (as job_id " + str(job_id) + ")\n" )
                else:
                    print( "ERROR: What happened here?  Please send this to Jeff:\n" )
                    print( hpc_pipe_stdout )
                    print( hpc_pipe_stderr )
                    job_id = -1
            else:
                print( "ERROR: job submit of " + self.config_id + " failed!" )
                print( hpc_pipe_stdout )
                print( hpc_pipe_stderr )

            if job_id == -1 and num_retries >= 5 and params.hide_graphs:
                print( "Job submission failed multiple times for " + self.config_id + ".  Aborting this test and logging error." )
                self.report.addErroringTest( self.config_id, "", sim_dir )
                return

        monitor_cmd_line = "job view /scheduler:" + params.hpc_head_node + " " + str(job_id)

        check_status = True
        while check_status:
            #print "executing hpc_command_line: " + monitor_cmd_line
            #print "Checking status of job " + str(job_id)
            #hpc_pipe = os.popen( monitor_cmd_line )
            hpc_pipe = subprocess.Popen( monitor_cmd_line.split(), shell=False, stdout=subprocess.PIPE )
            [hpc_pipe_stdout, hpc_pipe_stderr] = hpc_pipe.communicate()
            lines = hpc_pipe_stdout
            #for line in hpc_pipe.readlines():
            #print lines
            for line in lines.split('\n'):
                res = line.split( ':' )
                #print "DEBUG: " + str(res[0])
                if res[0].strip() == "State":
                    global completed
                    state = res[1].strip()
                    if state == "Failed":
                        completed = completed + 1
                        print( self.config_id + " FAILED!" )
                        check_status = False
                        global regression_id
                        self.report.addErroringTest( self.config_id, "", sim_dir )
                        #self.finish(sim_dir, False)
                    if state == "Canceled":
                        completed = completed + 1
                        print( "Canceled!" )
                        check_status = False
                        #self.finish(sim_dir, False)
                    elif state == "Completed" or state == "Finished":
                        completed = completed + 1
                        print( str(completed) + " out of " + str(len(reg_threads)) + " completed." )
                        check_status = False

                        status_file = open(os.path.join(sim_dir, "status.txt"))
                        for status_line in status_file.readlines():
                            if status_line.startswith("Done"):
                                time_split = status_line.split('-')[1].strip().split(':')
                                self.duration = datetime.timedelta(hours=int(time_split[0]), minutes=int(time_split[1]), seconds=int(time_split[2]))
                                break

                        if self.compare_results_to_baseline:
                            if params.all_outputs == False:
                            # Following line is for InsetChart.json only
                                self.verify(sim_dir)
                            else:
                                # Every .json file in output (not hidden with . prefix) will be used for validation
                                for file in os.listdir( os.path.join( self.config_id, "output" ) ):
                                    if ( file.endswith( ".json" ) or file.endswith( ".csv" ) or file.endswith( ".kml" ) or file.endswith( ".bin" ) ) and file[0] != "." and file != "transitions.json" and "linux" not in file:
                                        self.verify( sim_dir, file, "Channels" )
                    break
            time.sleep(5)
        MyRegressionRunner.sems.release()

            
# placeholder, I miss these.
class SimpleReport:
    def __init__(self, params):
        print( "Writing a human-readable report" )
        self.params = params

        
class Report:
    def __init__(self, params, version_string):
        self.num_tests = 0
        self.num_failures = 0
        self.num_errors = 0
        
        self.params = params
    
        self.doc = xml.dom.minidom.Document()
        
        self.suite_el = self.doc.createElement("testsuite")
        self.doc.appendChild(self.suite_el)
        
        prop_el = self.doc.createElement("property")
        prop_el.setAttribute("name", "Version string")
        prop_el.setAttribute("value", version_string)

        headnode_el = self.doc.createElement("property")
        headnode_el.setAttribute("name", "HPC Headnode")
        headnode_el.setAttribute("value", params.hpc_head_node)

        nodegroup_el = self.doc.createElement("property")
        nodegroup_el.setAttribute("name", "HPC Nodegroup")
        nodegroup_el.setAttribute("value", params.hpc_node_group)

        prop_els = self.doc.createElement("properties")
        prop_els.appendChild(prop_el)
        prop_els.appendChild(headnode_el)
        prop_els.appendChild(nodegroup_el)

        self.suite_el.appendChild(prop_els)
        self.schema = "not done"
        
    def addPassingTest(self, name, time):
        testcase_el = self.doc.createElement("testcase")
        testcase_el.setAttribute("name", name)
        testcase_el.setAttribute("time", str(time.total_seconds()))
        
        self.suite_el.appendChild(testcase_el)
        
        self.num_tests += 1
    
    def addFailingTest(self, name, failure_txt, insetchart_path):
        testcase_el = self.doc.createElement("testcase")
        testcase_el.setAttribute("name", name)
        testcase_el.setAttribute("time", "-1")
        
        failure_el = self.doc.createElement("failure")
        failure_el.setAttribute("type", "Validation failure")
        failure_el.setAttribute("message", name + " failed validation!  Result data can be found at " + insetchart_path)
        
        #failure_txt_el = self.doc.createTextNode(failure_txt)
        #failure_el.appendChild(failure_txt_el)
        
        sysout_el = self.doc.createElement("system-out")
        sysout_txt = self.doc.createTextNode("n/a") # could fill this out more in the future, but not right now...
        sysout_el.appendChild(sysout_txt)

        syserr_el = self.doc.createElement("system-err")
        syserr_txt = self.doc.createTextNode("n/a") # could fill this out more in the future, but not right now...
        syserr_el.appendChild(syserr_txt)

        testcase_el.appendChild(failure_el)
        testcase_el.appendChild(sysout_el)
        testcase_el.appendChild(syserr_el)

        self.suite_el.appendChild(testcase_el)
        
        self.num_tests += 1
        self.num_failures += 1

    def addErroringTest(self, name, error_txt, simulation_path):
        testcase_el = self.doc.createElement("testcase")
        testcase_el.setAttribute("name", name)
        testcase_el.setAttribute("time", "-1")
        
        error_el = self.doc.createElement("error")
        error_el.setAttribute("type", "Functional failure")
        error_el.setAttribute("message", name + " exited unexpectedly!  Simulation output can be found at " + simulation_path)
        
        error_txt_el = self.doc.createTextNode(error_txt)
        error_el.appendChild(error_txt_el)
        
        sysout_el = self.doc.createElement("system-out")
        sysout_txt = self.doc.createTextNode("n/a") # could fill this out more in the future, but not right now...
        sysout_el.appendChild(sysout_txt)

        syserr_el = self.doc.createElement("system-err")
        syserr_txt = self.doc.createTextNode("n/a") # could fill this out more in the future, but not right now...
        syserr_el.appendChild(syserr_txt)

        testcase_el.appendChild(error_el)
        testcase_el.appendChild(sysout_el)
        testcase_el.appendChild(syserr_el)

        self.suite_el.appendChild(testcase_el)
        
        self.num_tests += 1
        self.num_errors += 1
    
    def write(self, filename, time):
        self.suite_el.setAttribute("name", self.params.suite + " regression suite")
        self.suite_el.setAttribute("tests", str(self.num_tests))
        self.suite_el.setAttribute("failures", str(self.num_failures))
        self.suite_el.setAttribute("errors", str(self.num_errors))
        self.suite_el.setAttribute("time", str(time.total_seconds()))
        
        if not os.path.exists("reports"):
            os.makedirs("reports")
        report_file = open( filename, "a" )
        report_file.write(self.doc.toprettyxml())

    @property
    def Summary(self):
        return { "tests" : self.num_tests, "passed" : (self.num_tests - self.num_errors - self.num_failures), "failed" : self.num_failures, "errors" : self.num_errors, "schema": self.schema }


class MyRegressionRunner():
    sems = threading.Semaphore( MAX_ACTIVE_JOBS )
    def __init__(self, params):
        self.params = params
        self.dtk_hash = md5_hash_of_file( self.params.executable_path )
        self.sim_dir_sem = threading.Semaphore()
        # print( "md5 of executable = " + self.dtk_hash )
        
    def copy_demographics_files_to_user_input(self, sim_id, reply_json, actual_input_dir, local_input_dir, remote_input_dir):
        input_files = reply_json['parameters'].get('Demographics_Filenames',[])
        if not input_files:
            input_files = reply_json['parameters'].get('Demographics_Filename','').split(';')
        reply_json["parameters"]["Demographics_Filenames"] = []
        for filename in input_files:
            #print( filename )
            if not filename or len( filename.strip(' ') ) == 0:
                #print( "file is empty. Skip" )
                continue
            do_copy = False

            local_source = os.path.join( local_input_dir, filename )
            remote_source = os.path.join( remote_input_dir, filename )
            dest = os.path.join( actual_input_dir, os.path.basename( filename ) )

            # For any demographics overlays WITHIN regression folder:
            # Copy directly to remote simulation working directory
            if os.path.exists(local_source):
                #print('Copying %s to remote working directory'%filename)
                sim_dir = os.path.join( self.params.sim_root, sim_id )
                do_copy = False # do my own copy here, since
                remote_path = os.path.join(sim_dir,os.path.basename(filename))
                if os.path.exists(remote_path):
                    raise Exception('Overlay of same basename has already been copied to remote simulation directory.')
                shutil.copy(local_source, remote_path)

            # Cases:
            # 0 - source file exists and is found at destination
            #   NO COPY: nothing to do if the same
            # 1 - source file exists and is found at destination
            #   COPY source to destination if _not_ the same
            # 2 - source file exists and is _not_ found at destination
            #   COPY source to destination
            # 3 - source file not found but _is_ found at destination
            #   NO COPY: use destination file
            # 4 - source file not found and _not_ found at destination
            #   NO COPY: give up

            elif os.path.exists( remote_source ):
                if os.path.exists( dest ):
                    if not areTheseJsonFilesTheSame( remote_source, dest ):
                        # Case #1
                        print( "Source file '{0}' not the same as the destination file '{1}', updating.".format( remote_source, dest ) )
                        do_copy = True
                    else:
                        # Case #0: they're the same, don't copy
                        pass
                else:
                    # Case #2
                    print( "File '{0}' doesn't exist in destination directory, copying.".format( dest ) )
                    do_copy = True

            else:
                if os.path.exists( dest ):
                    # Case #3: use copy cached at destination
                    pass
                else:
                    # Case #4
                    print( "Couldn't find source file '{0}' locally ({1}) or remotely ({2})! Exiting.".format( filename, local_source, remote_source ) )
                    reply_json["parameters"]["Demographics_Filename"] = "input file ({0}) not found".format( filename )
                    return

            if do_copy:
                print( "Copy input files from " + local_input_dir + " and " + remote_input_dir + " to " + actual_input_dir + "." )
                shutil.copy( remote_source, dest )

            reply_json["parameters"]["Demographics_Filenames"].append(os.path.basename( filename ) )

        if "Demographics_Filename" in reply_json["parameters"]:
            del( reply_json["parameters"]["Demographics_Filename"] )

    def copy_climate_and_migration_files_to_user_input(self, reply_json, remote_input_dir, actual_input_dir):
        # Copy climate and migration files also
        for key in reply_json["parameters"]:
            if( "_Filename" in key and "Demographics_Filename" not in key and key != "Campaign_Filename" and key != "Custom_Reports_Filename" ):
                filename = reply_json["parameters"][key]
                if( len( filename ) == 0 ):
                    continue
                source = os.path.join( remote_input_dir, filename )
                dest = os.path.join( actual_input_dir, os.path.basename( filename ) )
                if os.path.exists( source ) and not os.path.exists( dest ):
                    #print( "Copying " + file )
                    shutil.copy( source, dest )
                dest = dest + ".json"
                source = source + ".json"
                if os.path.exists( source ) and not os.path.exists( dest ):
                    #print( "Copying " + file )
                    shutil.copy( source, dest )
        
    def copy_input_files_to_user_input(self, sim_id, config_id, reply_json, is_local):
        # Copy local demographics/input file(s) and remote base input file into user-local input directory 
        # E.g., //diamonds-hn/EMOD/home/jbloedow/input/Bihar, where "//diamonds-hn/EMOD/home/jbloedow/input/"
        # is gotten from config["home"] and "Bihar" is from config_json["Geography"]
        # Then use that directory as the input.
        local_input_dir = config_id
        remote_input_dir = os.path.join( self.params.shared_input, reply_json["parameters"]["Geography"] )
        # print( "remote_input_dir = " + remote_input_dir )
        actual_input_dir = os.path.join( self.params.user_input, reply_json["parameters"]["Geography"] )

        if os.path.exists( actual_input_dir ) == False:
            print( "Creating " + actual_input_dir )
            os.makedirs( actual_input_dir )

        self.copy_demographics_files_to_user_input(sim_id, reply_json, actual_input_dir, local_input_dir, remote_input_dir)
        self.copy_climate_and_migration_files_to_user_input(reply_json, remote_input_dir, actual_input_dir)
        self.params.use_user_input_root = True
        
    def copy_sim_file( self, config_id, sim_dir, filename ):
        if( len( filename ) != 0 ):
            filename = os.path.join( config_id, filename )
            if( os.path.exists( filename ) ) :
                #print( "Copying " + filename )
                shutil.copy( filename, sim_dir )
            else:
                print( "ERROR: Failed to find file to copy: " + filename )
        return

    def commissionFromConfigJson( self, sim_id, reply_json, config_id, report, compare_results_to_baseline=True ):
        # compare_results_to_baseline means we're running regression, compare results to reference.
        # opposite/alternative is sweep, which means we don't do comparison check at end
        # now we have the config_json, find out if we're commissioning locally or on HPC

        def is_local_simulation( some_json, id ):
            is_local = False
            if ('parameters' in reply_json) and ('Local_Simulation' in reply_json['parameters']):
                if (reply_json['parameters']['Local_Simulation'] == 1):
                    is_local = True
            else:
                print( "Didn't find key 'parameters/Local_Simulation' in json '{0}'.".format( id ) )

            return is_local

        sim_dir = os.path.join( self.params.sim_root, sim_id )
        bin_dir = os.path.join( self.params.bin_root, self.dtk_hash ) # may not exist yet

        is_local = is_local_simulation(reply_json, config_id)
        if is_local:
            print( "Commissioning locally (not on cluster)!" )
            sim_dir = os.path.join( self.params.local_sim_root, sim_id )
            bin_dir = os.path.join( self.params.local_bin_root, self.dtk_hash ) # may not exist yet
        # else:
            # print( "HPC!" )

        # create unique simulation directory
        self.sim_dir_sem.acquire()
        os.makedirs(sim_dir)
        self.sim_dir_sem.release()

        # only copy binary if new to us; copy to bin/<md5>/Eradication.exe and run from there
        # Would like to create a symlink and run from the sim dir, but can't do that on cluster; no permissions!

        # JPS - can't we just check for existence of that file?  This seems overly complicated...

        # check in bin_dir to see if our binary exists there...
        foundit = False
        bin_path = os.path.join( bin_dir, "Eradication" if os.name == "posix" else "Eradication.exe" )
        if os.path.exists(bin_dir):
            if os.path.exists(bin_path):
                foundit = True
        else:
            os.makedirs(bin_dir)
        
        if not foundit:
            print( "We didn't have it, copy it up..." )
            shutil.copy( self.params.executable_path, bin_path )
            print( "Copied!" )

        reply_json["bin_path"] = bin_path
        reply_json["executable_hash"] = self.dtk_hash

        # JPS - not sure what some of the rest of this stuff does
        # campaign_json is non, and config.json contains double the stuff it needs in the sim dir... :-(
        
        # tease out campaign json, save separately
        campaign_json = json.dumps(reply_json["campaign_json"]).replace( "u'", "'" ).replace( "'", '"' ).strip( '"' )
        reply_json["campaign_json"] = None

        # tease out custom_reports json, save separately
        if reply_json["custom_reports_json"] is not None:
            reports_json = json.dumps(reply_json["custom_reports_json"]).replace( "u'", "'" ).replace( "'", '"' ).strip( '"' )
            reply_json["custom_reports_json"] = None
            # save custom_reports.json
            f = open( sim_dir + "/custom_reports.json", 'w' )
            #f.write( json.dumps( reports_json, sort_keys=True, indent=4 ) )
            f.write( str( reports_json ) )
            f.close()

        # Use a local variable here because we don't want the PSP in the config.json that gets written out to disk
        # but we need it passed through to the monitor thread execution in the reply_json/config_json.
        py_input = None
        if "Python_Script_Path" in reply_json["parameters"]:
            psp_param = reply_json["parameters"]["Python_Script_Path"]
            if psp_param == "LOCAL":
                py_input = "."
                for py_file in glob.glob( os.path.join( config_id, "dtk_*.py" ) ):
                    regression_runner.copy_sim_file( config_id, sim_dir, os.path.basename( py_file ) )
            elif psp_param == "SHARED":
                py_input = params.py_input
            elif psp_param != "NO":
                print( psp_param + " is not a valid value for Python_Script_Path. Valid values are NO, LOCAL, SHARED. Exiting." )
                sys.exit() 
            del( reply_json["parameters"]["Python_Script_Path"] )

        self.copy_input_files_to_user_input(sim_id, config_id, reply_json, is_local)

        #print "Writing out config and campaign.json."
        # save config.json
        f = open( sim_dir + "/config.json", 'w' )
        f.write( json.dumps( reply_json, sort_keys=True, indent=4 ) )
        f.close()

        # now that config.json is written out, add Py Script Path back (if non-empty)
        if py_input is not None:
            reply_json["PSP"] = py_input

        # save campaign.json
        f = open( sim_dir + "/campaign.json", 'w' )
        #f.write( json.dumps( campaign_json, sort_keys=True, indent=4 ) )
        f.write( str( campaign_json ) )
        f.close()

        f = open( sim_dir + "/emodules_map.json", 'w' )
        f.write( json.dumps( emodules_map, sort_keys=True, indent=4 ) )
        f.close()
        
        # ------------------------------------------------------------------
        # If you uncomment the following line, it will copy the program database
        # file to the directory where a simulation will run (i.e. with the config.json file).
        # This will help you get a stack trace with files and line numbers.
        # ------------------------------------------------------------------
        #print( "Copying PDB file...." )
        #shutil.copy( "../Eradication/x64/Release/Eradication.pdb", sim_dir )
        # ------------------------------------------------------------------

        if os.path.exists( os.path.join( config_id, "dtk_post_process.py" ) ):
            regression_runner.copy_sim_file( config_id, sim_dir, "dtk_post_process.py" )

        monitorThread = None # need scoped here

        #print "Creating run & monitor thread."
        if is_local_simulation(reply_json, config_id):
            monitorThread = Monitor( sim_id, config_id, report, reply_json, compare_results_to_baseline )
        else:
            monitorThread = HpcMonitor( sim_id, config_id, report, params.label, reply_json, compare_results_to_baseline )

        #monitorThread.daemon = True
        monitorThread.daemon = False
        #print "Starting run & monitor thread."
        monitorThread.start()

        #print "Monitor thread started, notify data service, and return."
        return monitorThread

    def doSchemaTest( self ):
        #print( "Testing schema generation..." )
        test_schema_path = "test-schema.json"
        subprocess.call( [ params.executable_path, "--get-schema", "--schema-path", test_schema_path ], stdout=open(os.devnull) )
        try:
            schema = json.loads( open( test_schema_path ).read() )
            print( "schema works." )
            os.remove( test_schema_path )
            return "pass"
        except Exception as ex:
            print( "schema failed!" )
            return "fail"

# Copy just build dlls to deployed places based on commandline argument 
# - The default is to use all of the DLLs found in the location the DLL projects
#   place the DLLs (<trunk>\x64\Release).
# - --dll-path allows the user to override this default path
def copyEModulesOver( params ):

    print "src_root = " + params.src_root

    if params.dll_path is not None:
        emodule_dir = params.dll_path
    else:
        if params.scons:
            emodule_dir = os.path.join( params.src_root, "build" )
            emodule_dir = os.path.join( emodule_dir, "x64" )
        else:
            emodule_dir = os.path.join( params.src_root, "x64" )
        if params.debug == True:
            emodule_dir = os.path.join( emodule_dir, "Debug" )
        elif params.quick_start == True:
            emodule_dir = os.path.join( emodule_dir, "QuickStart" )
        else:
            emodule_dir = os.path.join( emodule_dir, "Release" )

    print( 'Assuming emodules (dlls) are in local directory: ' + emodule_dir )

    if os.path.exists( emodule_dir ) == False:
        print( "Except that directory does not exist!  Not copying emodules." )
        return

    #print "dll_root = " + params.dll_root

    dll_dirs = [ "disease_plugins",  "reporter_plugins", "interventions"]

    for dll_subdir in dll_dirs:
        dlls = glob.glob( os.path.join( os.path.join( emodule_dir, dll_subdir ), "*.dll" ) )
        for dll in dlls:
            dll_hash = md5_hash_of_file( dll )
            #print( dll_hash )
            # 1) calc md5 of dll
            # 2) check for existence of rivendell (or whatever) for <root>/emodules/<subdir>/<md5>
            # 3) if no exist, create and copy
            # 4) put full path in emodules_json
            # 5) write out emodules_json when done to target sim dir
            try:
                target_dir = os.path.join( params.dll_root, dll_subdir )
                target_dir = os.path.join( target_dir, dll_hash )

                if params.sec:
                    print( dll + " will be used without checking 'new-ness'." )
                elif not (os.path.isdir( target_dir ) ):
                    print( dll + ": copying to cluster" )
                else:
                    print( dll + ": Already on cluster" )

                if not (os.path.isdir( target_dir ) ) and params.sec == False: # sec = command-line option to skip this
                    os.makedirs( target_dir )
                    shutil.copy( dll, os.path.join( target_dir, os.path.basename( dll ) ) )

                emodules_map[ dll_subdir ].append( os.path.join( target_dir, os.path.basename( dll ) ) )
    
            except IOError:
                print "Failed to copy dll " + dll + " to " + os.path.join( os.path.join( params.dll_root, dll_dirs[1] ), os.path.basename( dll ) ) 
                final_warnings += "Failed to copy dll " + dll + " to " + os.path.join( os.path.join( params.dll_root, dll_dirs[1] ), os.path.basename( dll )) + "\n"

def main():
    global regression_id, params, final_warnings
    report = None
    reglistjson = None

    if(str.isdigit(params.suite)):
        dirs = glob.glob(params.suite + "_*")
        for dir in dirs:
            if os.path.isdir(dir):
                print("Executing single test: " + dir)
                reglistjson = { "tests" : [ { "path" : dir } ] }
    else:
        if params.suite.endswith(".json"):
            params.suite = params.suite.replace(".json", "")
        reglistjson = json.loads( open( params.suite.split(',')[0] + ".json" ).read() )
        if "tests" in reglistjson and len( params.suite.split(',') ) > 1:
            for suite in params.suite.split(',')[1:]:
                data = json.loads( open( suite + ".json" ).read() )
                if "tests" in data:
                    reglistjson[ "tests" ].extend( data["tests"] )
                else:
                    print( suite + " does not appear to be a suite, missing key 'tests'" )

    if "tests" in reglistjson:
        p = subprocess.Popen( (params.executable_path + " -v").split(), shell=False, stdout=subprocess.PIPE )
        [pipe_stdout, pipe_stderr] = p.communicate()
        global version_string
        version_string = re.search('[0-9]+.[0-9]+.[0-9]+.[0-9]+', pipe_stdout).group(0)

        starttime = datetime.datetime.now()
        report = Report(params, version_string)

        print( "Running regression...\n" )
        for simcfg in reglistjson["tests"]:
            os.chdir(cache_cwd)
            sim_timestamp = str(datetime.datetime.now()).replace('-', '_' ).replace( ' ', '_' ).replace( ':', '_' ).replace( '.', '_' )
            if regression_id == None:
                regression_id = sim_timestamp

            try:
                #print "flatten config: " + os.path.join( simcfg["path"],"param_overrides.json" )
                configjson = flattenConfig( os.path.join( simcfg["path"],"param_overrides.json" ) )
            except:
                report.addErroringTest(simcfg["path"], "Error flattening config.", "(no simulation directory created).")
                configjson = None

            campaign_override_fn = os.path.join( simcfg["path"],"campaign_overrides.json" )

            try:
                #print "flatten campaign: " + campaign_override_fn
                campjson = flattenCampaign( campaign_override_fn, False )
            except:
                print "failed to flatten campaign: " + campaign_override_fn
                report.addErroringTest(simcfg["path"], "Failed flattening campaign.", "(no simulation directory created).")
                campjson = None

            if configjson is None:
                print("Error flattening config.  Skipping " + simcfg["path"])
                final_warnings += "Error flattening config.  Skipped " + simcfg["path"] + "\n"
                continue

            constraints_satisfied = True
            if len(params.constraints_dict) != 0:
                real_params = configjson["parameters"]
                cons = params.constraints_dict
                for key in cons:
                    val = cons[key]
                    if key not in real_params.keys() or str(real_params[ key ]) != val:
                        print( "Scenario configuration did not satisfy constraint: {0} == {1} but must == {2}.".format( key, str(real_params[ key ]), val ) )
                        constraints_satisfied = False
                        continue

            if constraints_satisfied == False:
                continue

            if campjson is None:
                # Try loading directly from file
                campjson_file = open( os.path.join( simcfg["path"],"campaign.json" ) )
                campjson = json.loads( campjson_file.read().replace( "u'", "'" ).replace( "'", '"' ).strip( '"' ) )
                campjson_file.close()

            # add campaign to config
            configjson["campaign_json"] = str(campjson)

            # add custom_reports to config
            report_fn = os.path.join( simcfg["path"],"custom_reports.json" )
            if os.path.exists( report_fn ) == True:
                reportjson_file = open( report_fn )
                reportjson = json.loads( reportjson_file.read().replace( "u'", "'" ).replace( "'", '"' ).strip( '"' ) )
                reportjson_file.close()
                configjson["custom_reports_json"] = str(reportjson)
            else:
                configjson["custom_reports_json"] = None

            thread = regression_runner.commissionFromConfigJson( sim_timestamp, configjson, simcfg["path"], report )
            reg_threads.append( thread )
        # do a schema test also
        if params.dts == True:
            report.schema = regression_runner.doSchemaTest()

    elif "sweep" in reglistjson:
        print( "Running sweep...\n" )
        param_name = reglistjson["sweep"]["param_name"]
        # NOTE: most code below was copy-pasted from 'tests' (regression) case above.
        # I could factor this and generalize now but future extensions of sweep capability may involve
        # a greater departure from this code path so that might be a premature optimization.
        for param_value in reglistjson["sweep"]["param_values"]:
            os.chdir(cache_cwd)
            sim_timestamp = str(datetime.datetime.now()).replace('-', '_' ).replace( ' ', '_' ).replace( ':', '_' ).replace( '.', '_' )
            if regression_id == None:
                regression_id = sim_timestamp
            # atrophied? configjson_filename = reglistjson["sweep"]["path"]
            # atrophied? configjson_path = str( os.path.join( reglistjson["sweep"]["path"], "config.json" ) )

            configjson = flattenConfig( os.path.join( reglistjson["sweep"]["path"], "param_overrides.json" ) )
            if configjson is None:
                print("Error flattening config.  Skipping " + simcfg["path"])
                final_warnings += "Error flattening config.  Skipped " + simcfg["path"] + "\n"
                continue

            # override sweep parameter
            configjson["parameters"][param_name] = param_value

            campjson_file = open( os.path.join( reglistjson["sweep"]["path"],"campaign.json" ) )
            campjson = json.loads( campjson_file.read().replace( "u'", "'" ).replace( "'", '"' ).strip( '"' ) )
            campjson_file.close()
            configjson["campaign_json"] = str(campjson)

            report_fn = os.path.join( reglistjson["sweep"]["path"],"custom_reports.json" )
            if os.path.exists( report_fn ) == True:
                reportjson_file = open( report_fn )
                reportjson = json.loads( reportjson_file.read().replace( "u'", "'" ).replace( "'", '"' ).strip( '"' ) )
                reportjson_file.close()
                configjson["custom_reports_json"] = str(reportjson)
            else:
                configjson["custom_reports_json"] = None

            thread = regression_runner.commissionFromConfigJson( sim_timestamp, configjson, reglistjson["sweep"]["path"], None, False )
            reg_threads.append( thread )
    else:
        print "Unknown state"
        sys.exit(0)

    # stay alive until done
    for thr in reg_threads:
        thr.join()
        #print str(thr.sim_timestamp) + " is done."

    if report is not None:
        endtime = datetime.datetime.now()
        report.write(os.path.join("reports", "report_" + regression_id + ".xml"), endtime - starttime)
        print '========================================'
        print 'Elapsed time: ', endtime - starttime
        print '%(tests)3d tests total, %(passed)3d passed, %(failed)3d failed, %(errors)3d errors, schema: %(schema)s.' % report.Summary

    if final_warnings is not "":
        print("----------------\n" + final_warnings)
        #raw_input("Press Enter to continue...")

    # if doing sweep, call plotAllCharts.py with all sim_timestamps on command line.
    if "sweep" in reglistjson:
        print( "Plot sweep results...\n" )
        all_data = []
        all_data_prop = []
        
        ref_path_prop = os.path.join( str(reglistjson["sweep"]["path"]), os.path.join( "output", "PropertyReport.json" ) )
        ref_json_prop = {}
        if os.path.exists( ref_path_prop ) == True:
            ref_json_prop = json.loads( open( os.path.join( cache_cwd, ref_path_prop ) ).read() )

        ref_path = os.path.join( str(reglistjson["sweep"]["path"]), os.path.join( "output", "InsetChart.json" ) )
        ref_json = json.loads( open( os.path.join( cache_cwd, ref_path ) ).read() )

        for thr in reg_threads:
            sim_dir = os.path.join( thr.sim_root, thr.sim_timestamp )
            icj_filename = os.path.join( sim_dir, os.path.join( "output", "InsetChart.json" ) )
            icj_json = json.loads( open( icj_filename ).read() )
            all_data.append( icj_json )
            if os.path.exists( ref_path_prop ) == True:
                prj_filename = os.path.join( sim_dir, os.path.join( "output", "PropertyReport.json" ) )
                prj_json = json.loads( open( prj_filename ).read() )
                all_data_prop.append( prj_json )
        plot_title = "Sweep over " + reglistjson["sweep"]["param_name"] + " (" + str(len(reglistjson["sweep"]["param_values"])) + " values)"
        os.chdir( cache_cwd )
        plotAllCharts.plotBunch( all_data, plot_title, ref_json )
        if os.path.exists( ref_path_prop ) == True:
            plotAllCharts.plotBunch( all_data_prop, plot_title, ref_json_prop )
        time.sleep(1)
        
    return


def setup():

    # non-main global code starts here
    parser = argparse.ArgumentParser()
    parser.add_argument("suite", help="JSON test-suite to run - e.g. full.json, sanity (converted to sanity.json), 25 (just run 25_Vector_Madagascar)")
    parser.add_argument("exe_path", metavar="exe-path", help="Path to the Eradication.exe binary to run")
    parser.add_argument("--perf", action="store_true", default=False, help="Run for performance measurement purposes")
    parser.add_argument("--hidegraphs", action="store_true", default=False, help="Suppress pop-up graphs in case of validation failure")
    parser.add_argument("--debug", action="store_true", default=False, help="Use debug path for emodules")
    parser.add_argument("--quick-start", action="store_true", default=False, help="Use QuickStart path for emodules")
    parser.add_argument("--label", help="Custom suffix for HPC job name")
    parser.add_argument("--config", default="regression_test.cfg", help="Regression test configuration [regression_test.cfg]")
    parser.add_argument("--disable-schema-test", action="store_false", default=True, help="Test schema (true by default, use to suppress schema testing)")
    parser.add_argument("--use-dlls", action="store_true", default=False, help="Use emodules/DLLs when running tests")
    parser.add_argument("--all-outputs", action="store_true", default=False, help="Use all output .json files for validation, not just InsetChart.json")
    parser.add_argument("--dll-path", help="Path to the root directory of the DLLs to use (e.g. contains reporter_plugins)")
    parser.add_argument("--skip-emodule-check", action="store_true", default=False, help="Use this to skip sometimes slow check that EMODules on cluster are up-to-date.")
    parser.add_argument("--config-constraints", default=[], action="append", help="Use this to skip sometimes slow check that EMODules on cluster are up-to-date.")
    parser.add_argument("--scons", action="store_true", default=False, help="Indicates scons build so look for custom DLLs in the build/64/Release directory.")
    args = parser.parse_args()

    global params
    params = RuntimeParameters(args)

    global emodules_map, regression_runner, version_string, completed, cache_cwd, regression_id, reg_threads, final_warnings
    emodules_map = {}
    emodules_map[ "interventions" ] = []
    emodules_map[ "disease_plugins" ] = []
    emodules_map[ "reporter_plugins" ] = []

    regression_runner = MyRegressionRunner(params)
    version_string = None
    completed = 0
    cache_cwd = os.getcwd()
    regression_id = None
    reg_threads = []
    final_warnings = ""

    if params.dll_root is not None and params.use_dlls is True:
        #print( "dll_root (remote) = " + params.dll_root )
        copyEModulesOver(params)
    else:
        print( "Not using DLLs" )

if __name__ == "__main__":
    # 'twould be nice to ditch this (keeping for legacy reasons) anyone actually use this?
    if len(sys.argv) > 1 and sys.argv[1] == "--flatten":
        flattenConfig( sys.argv[2] )
        sys.exit(0)

    setup()
    main()
