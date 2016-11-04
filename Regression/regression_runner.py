#!/usr/bin/python

import os
import json
import threading
import subprocess
import glob
import shutil # copyfile
import glob
import regression_local_monitor
import regression_hpc_monitor
import regression_utils as ru

class MyRegressionRunner():

    # static variables
    emodules_map = {}

    def __init__(self, params):
        self.params = params
        self.dtk_hash = ru.md5_hash_of_file( self.params.executable_path )
        self.sim_dir_sem = threading.Semaphore()
        self.emodules_map[ "interventions" ] = []
        self.emodules_map[ "disease_plugins" ] = []
        self.emodules_map[ "reporter_plugins" ] = []
        if params.dll_root is not None and params.use_dlls is True:
            #print( "dll_root (remote) = " + params.dll_root )
            self.copyEModulesOver(params)
        else:
            print( "Not using DLLs" )

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
                    raise Exception('Overlay with same basename has already been copied to remote simulation directory.')
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
                    if not ru.areTheseJsonFilesTheSame( remote_source, dest ):
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
        
    # Copy just build dlls to deployed places based on commandline argument 
    # - The default is to use all of the DLLs found in the location the DLL projects
    #   place the DLLs (<trunk>\x64\Release).
    # - --dll-path allows the user to override this default path
    def copyEModulesOver( self, params ):

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
                dll_hash = ru.md5_hash_of_file( dll )
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

                    self.emodules_map[ dll_subdir ].append( os.path.join( target_dir, os.path.basename( dll ) ) )
        
                except IOError:
                    print "Failed to copy dll " + dll + " to " + os.path.join( os.path.join( params.dll_root, dll_dirs[1] ), os.path.basename( dll ) ) 
                    ru.final_warnings += "Failed to copy dll " + dll + " to " + os.path.join( os.path.join( params.dll_root, dll_dirs[1] ), os.path.basename( dll )) + "\n"
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
            if os.name == "posix":
                return True

            return self.params.local_execution 

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
                    self.copy_sim_file( config_id, sim_dir, os.path.basename( py_file ) )
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
        f.write( json.dumps( self.emodules_map, sort_keys=True, indent=4 ) )
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
            self.copy_sim_file( config_id, sim_dir, "dtk_post_process.py" )

        monitorThread = None # need scoped here

        #print "Creating run & monitor thread."
        if is_local_simulation(reply_json, config_id):
            monitorThread = regression_local_monitor.Monitor( sim_id, config_id, report, self.params, reply_json, compare_results_to_baseline )
        else:
            monitorThread = regression_hpc_monitor.HpcMonitor( sim_id, config_id, report, self.params, self.params.label, reply_json, compare_results_to_baseline )

        #monitorThread.daemon = True
        monitorThread.daemon = False
        #print "Starting run & monitor thread."
        monitorThread.start()

        #print "Monitor thread started, notify data service, and return."
        return monitorThread

    def doSchemaTest( self ):
        #print( "Testing schema generation..." )
        test_schema_path = "test-schema.json"
        subprocess.call( [ self.params.executable_path, "--get-schema", "--schema-path", test_schema_path ], stdout=open(os.devnull) )
        try:
            schema = json.loads( open( test_schema_path ).read() )
            print( "schema works." )
            os.remove( test_schema_path )
            return "pass"
        except Exception as ex:
            print( str(ex) )
