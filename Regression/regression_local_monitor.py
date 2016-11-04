#!/usr/bin/python

import sys
import os
import subprocess
import datetime
import threading
import json
import tempfile
from hashlib import md5
import pdb

import regression_utils as ru

MAX_ACTIVE_JOBS=20
class Monitor(threading.Thread):
    sems = threading.Semaphore( MAX_ACTIVE_JOBS )
    completed = 0

    def __init__(self, sim_id, config_id, report, params, config_json=None, compare_results_to_baseline=True):
        threading.Thread.__init__( self )
        #print "Running DTK execution and monitor thread."
        sys.stdout.flush()
        self.sim_timestamp = sim_id
        self.config_id = config_id
        self.report = report
        self.config_json = config_json
        self.duration = None
        # can I make this static?
        self.params = params
        self.compare_results_to_baseline = compare_results_to_baseline

    def run(self):
        self.__class__.sems.acquire()
        self.sim_root = self.params.local_sim_root
        sim_dir = os.path.join( self.sim_root, self.sim_timestamp )
        #os.chdir( sim_dir )    # NOT THREAD SAFE!

        starttime = datetime.datetime.now()

        with open(os.path.join(sim_dir, "stdout.txt"), "w") as stdout, open(os.path.join(sim_dir, "stderr.txt"), "w") as stderr:
            actual_input_dir = os.path.join( self.params.input_path, self.config_json["parameters"]["Geography"] )
            cmd = None
            # python-script-path is optional parameter.
            if "PSP" in self.config_json:
                cmd = [self.config_json["bin_path"], "-C", "config.json", "--input-path", actual_input_dir, "--python-script-path", self.config_json["PSP"]]
            else:
                cmd = [self.config_json["bin_path"], "-C", "config.json", "--input-path", actual_input_dir ]
            #print( "Calling '" + str(cmd) + "' from " + sim_dir + "\n" )
            print( "Running '" + str(self.config_json["parameters"]["Config_Name"]) + "' in " + sim_dir + "\n" )
            proc = subprocess.Popen( cmd, stdout=stdout, stderr=stderr, cwd=sim_dir )
            proc.wait()
        # JPS - do we want to append config_json["parameters"]["Geography"] to the input_path here too like we do in the HPC case?
        endtime = datetime.datetime.now()
        self.duration = endtime - starttime
        os.chdir( ru.cache_cwd )
        self.__class__.completed = self.__class__.completed + 1
        print( str(self.__class__.completed) + " out of " + str(len(ru.reg_threads)) + " completed." )
        # JPS - should check here and only do the verification if it passed... ?
        if self.compare_results_to_baseline:
            if self.params.all_outputs == False:
            # Following line is for InsetChart.json only
                self.verify(sim_dir)
            else:
                # Every .json file in output (not hidden with . prefix) will be used for validation
                for file in os.listdir( os.path.join( self.config_id, "output" ) ):
                    if ( file.endswith( ".json" ) or file.endswith( ".csv" ) ) and file[0] != ".":
                        self.verify( sim_dir, file, "Channels" )
        self.__class__.sems.release()

    def get_json_data_hash( self, data ):
        #json_data = collections.OrderedDict([])
        #json_data["Data"] = data
        with tempfile.TemporaryFile() as handle:
            json.dump( data, handle )
            hash = ru.md5_hash( handle )
        return hash

    def compareJsonOutputs( self, sim_dir, report_name, ref_path, test_path, failures ):
        fail_validation = False
        failure_txt = ""

        ref_json = json.loads( open( os.path.join( ru.cache_cwd, ref_path ) ).read() )
        if "Channels" not in ref_json.keys():
            ref_md5  = ru.md5_hash_of_file( ref_path )
            test_md5 = ru.md5_hash_of_file( test_path )
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
                ru.final_warnings += self.config_id + " - New channels not found in reference:\n  " + '\n  '.join(new_channels) + "\nPlease update reference from " + os.path.join( sim_dir, os.path.join( "output", "InsetChart.json" ) ) + "!\n"
                self.report.addFailingTest( self.config_id, failure_txt, os.path.join( sim_dir, ( "output/" + report_name ) ) )

            if "Header" in ref_json.keys() and ref_json["Header"]["Timesteps"] != test_json["Header"]["Timesteps"]:
                warning_msg = "WARNING: test "+report_name+" has timesteps " + str(test_json["Header"]["Timesteps"])  + " DIFFERRING from ref "+report_name+" timesteps " + str(ref_json["Header"]["Timesteps"]) + "!\n"
                if self.params.hide_graphs:
                    # This is treated as automated running mode (or bamboo nightly build mode)
                    fail_validation = True
                    failure_txt += warning_msg
                else:
                    # This is treated as manual running mode
                    ru.final_warnings += warning_msg
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
        ref_md5 = ru.md5_hash_of_file( ref_path )
        test_md5 = ru.md5_hash_of_file( test_path )
        if ref_md5 == test_md5:
            # print( "CSV files passed MD5 comparison test." )
            return False, ""

        fail_validation = False
        err_msg = ""

        # print( "CSV files failed MD5 comparison test." )
        # First (md5) test failed. Do line length, then line-by-line
        ref_length = ru.file_len( ref_path )
        test_length = ru.file_len( test_path )
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
        ref_md5 = ru.md5_hash_of_file( ref_path )
        test_md5 = ru.md5_hash_of_file( test_path )
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
        fail_validation = False
        failures = []
        failure_txt = ""

        test_path = os.path.join( sim_dir, os.path.join( "output", report_name ) )
        ref_path = os.path.join( ru.cache_cwd, os.path.join( str(self.config_id), os.path.join( "output", report_name ) ) )

        # if on linux, use alternate InsetChart.json, but only if exists
        if os.name != "nt" and report_name == "InsetChart.json":
            report_name = "InsetChart.linux.json" 
            alt_ref_path = os.path.join( ru.cache_cwd, os.path.join( str(self.config_id), os.path.join( "output", report_name ) ) )
            if os.path.exists( alt_ref_path ):
                ref_path = alt_ref_path


        if os.name == "nt" and report_name == "InsetChart.linux.json":
            return True

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

            if len(failures) > 0 and not self.params.hide_graphs and report_name.startswith( "InsetChart" ):
                #print( "Plotting charts for failure deep dive." )  
                # Note: Use python version 2 for plotAllCharts.py
                subprocess.Popen( ["python", "plotAllCharts.py", ref_path, test_path, self.config_id ] )
        else:
            print( self.config_id + " passed (" + str(self.duration) + ") - " + report_name )
            self.report.addPassingTest(self.config_id, self.duration)
            
            if ru.version_string is not None:
                try:
                    timefile = open( os.path.join( self.config_id, "time.txt" ), 'a' )
                    timefile.write(ru.version_string + '\t' + str(self.duration) + '\n')
                    timefile.close()
                except Exception as e:
                    print("Problem writing time.txt file (repeat of error Jonathan was seeing on linux?)\n")
                    print(str(e))

        

