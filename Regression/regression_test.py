#!/usr/bin/python

"""
This file is the root of regression. Almost everything here is about copying files around.
"""

# These imports are from original version of regression_test.py that ran as a REST-ful web service 
# inside mod_wsgi/apache setup. Was a good idea then and still might be...
#import BaseHTTPServer
#import SimpleHTTPServer
#import SocketServer
#import cgi
#import httplib
#import urllib
#import urlparse

import argparse
import datetime
import glob
import json
import os # e.g., mkdir
import re
import subprocess
import sys # for stdout.flush
import threading
import pdb

import regression_runner
import regression_utils as ru
import regression_runtime_params
import regression_local_monitor
import regression_hpc_monitor
import regression_report


def setup():

    # non-main module code starts here
    parser = argparse.ArgumentParser()
    parser.add_argument("suite", help="JSON test-suite to run - e.g. full.json, sanity (converted to sanity.json) - one or more comma separated values")
    parser.add_argument("exe_path", metavar="exe-path", nargs="?", default="", help="Path to the Eradication.exe binary to run.  Default is where the executable is normally built depending on --scons, --debug, and the OS.")
    parser.add_argument("--perf", action="store_true", default=False, help="Run for performance measurement purposes")
    parser.add_argument("--hidegraphs", action="store_true", default=False, help="Suppress pop-up graphs in case of validation failure")
    parser.add_argument("--debug", action="store_true", default=False, help="Use debug path for emodules")
    parser.add_argument("--quick-start", action="store_true", default=False, help="Use QuickStart path for emodules")
    parser.add_argument("--label", help="Custom suffix for HPC job name")
    parser.add_argument("--config", default="regression_test.cfg", help="Regression test configuration [regression_test.cfg]")
    parser.add_argument("--disable-schema-test", action="store_true", default=False, help="Disable schema test (testing is on by default, use to suppress schema testing)")
    parser.add_argument("--component-tests", action="store_true", default=False, help="Run the componentTests if the executable exists")
    parser.add_argument("--component-tests-show-output", action="store_true", default=False, help="Show the output of the componentTests")
    parser.add_argument("--use-dlls", action="store_true", default=False, help="Use emodules/DLLs when running tests")
    parser.add_argument("--all-outputs", action="store_true", default=False, help="Use all output .json files for validation, not just InsetChart.json")
    parser.add_argument("--dll-path", help="Path to the root directory of the DLLs to use (e.g. contains reporter_plugins)")
    parser.add_argument("--skip-emodule-check", action="store_true", default=False, help="Use this to skip the sometimes slow check that EMODules on cluster are up to date.")
    parser.add_argument("--config-constraints", nargs='+', help="key:value pair(s) which are used to filter the scenario list (the given key and value must be in the config.json), pairs are separated by commas")
    parser.add_argument("--scons", action="store_true", default=False, help="Indicates scons build so look for custom DLLs in the build/64/Release directory.")
    parser.add_argument('--local', default=False, action='store_true', help='Run all simulations locally.')

    args = parser.parse_args()

    params = regression_runtime_params.RuntimeParameters(args)
    return params

def main():
    params = setup()
    runner = regression_runner.MyRegressionRunner(params)

    reglistjson = None
    entries = params.suite.split( "," )
    reglistjson = { }
    for entry in entries: 
        if os.path.isdir( entry ) and os.path.exists( os.path.join( entry, "param_overrides.json" ) ):
            print( "You specified a directory." )
            if len(reglistjson) == 0:
                reglistjson = { "tests" : [ ] }
            reglistjson[ "tests" ].extend( [ { "path" : entry } ] )
        else:
            if entry.endswith(".json"):
                # regular case
                entry = entry.replace(".json", "")

            # handle comma-separated list of suites
            reglistjson_new = json.loads( open( entry + ".json" ).read() )
            if "science" in reglistjson_new.keys():
                #reglistjson = json.loads( open( entry + ".json" ).read() )
                data = json.loads( open( entry + ".json" ).read() )
                if "science" in data:
                    if len(reglistjson) == 0:
                        reglistjson = { "science" : [ ] }
                    reglistjson[ "science" ].extend( data["science"] )
                else:
                    print( suite + " does not appear to be a suite, missing key 'science'" )
            
            if "tests" in reglistjson_new: # and len( entry.split(',') ) > 1:
                data = json.loads( open( entry + ".json" ).read() )
                if "tests" in data:
                    if len(reglistjson) == 0:
                        reglistjson = { "tests" : [ ] }
                    reglistjson[ "tests" ].extend( data["tests"] )
                else:
                    print( suite + " does not appear to be a suite, missing key 'tests'" )
            elif "sweep" in reglistjson_new:
                data = json.loads( open( entry + ".json" ).read() )
                if "sweep" in data:
                    reglistjson[ "sweep" ] = data["sweep"]

    report = None
    regression_id = None
    if "tests" in reglistjson or "science" in reglistjson:
        p = subprocess.Popen( (params.executable_path + " -v").split(), shell=False, stdout=subprocess.PIPE )
        [pipe_stdout, pipe_stderr] = p.communicate()
        ru.version_string = re.search('[0-9]+.[0-9]+.[0-9]+.[0-9]+', pipe_stdout).group(0)

        starttime = datetime.datetime.now()
        report = regression_report.Report(params, ru.version_string)

        suite_type = "tests"
        if "science" in reglistjson:
            suite_type = "science"
            if os.environ.has_key( "HOME" ):
                homepath = os.getenv( "HOME" )
            elif params.local_execution:
                homepath = os.path.join( os.getenv( "HOMEDRIVE" ), os.getenv( "HOMEPATH" ) )
            else: #cluster/HPC
                homepath = os.path.join( params.sim_root, ".." )

            flag = os.path.join( homepath, ".rt_show.sft" )
            if os.path.exists( flag ):
                os.remove( flag )
            if params.hide_graphs == False:
                #print ("Should find way to tell SFT's not to display graphs." )
                open(flag, 'w').close()  # this seems like lame way to touch a file, but works on windows. blech.

        print( "Running regression...\n" )
        for simcfg in reglistjson[ suite_type ]:
            configjson = None
            os.chdir(ru.cache_cwd)
            sim_timestamp = str(datetime.datetime.now()).replace('-', '_' ).replace( ' ', '_' ).replace( ':', '_' ).replace( '.', '_' )
            if regression_id == None:
                regression_id = sim_timestamp

            try:
                #print "flatten config: " + os.path.join( simcfg["path"],"param_overrides.json" )
                if os.path.exists( os.path.join( simcfg["path"],"param_overrides.json" ) ):
                    configjson = ru.flattenConfig( os.path.join( simcfg["path"],"param_overrides.json" ) )
                else:
                    configjson = json.loads( open( os.path.join( simcfg["path"],"config.json" ) ).read() )
            except Exception as ex:
                print( str( ex ) )
                report.addErroringTest(simcfg["path"], "Error flattening config.", "(no simulation directory created).", suite_type)
                configjson = None

            campaign_override_fn = os.path.join( simcfg["path"],"campaign_overrides.json" )

            try:
                #print "flatten campaign: " + campaign_override_fn
                campjson = ru.flattenCampaign( campaign_override_fn, False )
            except:
                print "failed to flatten campaign: " + campaign_override_fn
                report.addErroringTest(simcfg["path"], "Failed flattening campaign.", "(no simulation directory created).", suite_type)
                campjson = None

            if configjson is None:
                print("Error flattening config.  Skipping " + simcfg["path"])
                ru.final_warnings += "Error flattening config.  Skipped " + simcfg["path"] + "\n"
                continue

            constraints_satisfied = True
            if len(params.constraints_dict) != 0:
                real_params = configjson["parameters"]
                constraints = params.constraints_dict
                for constraint_param in constraints:
                    constraint_value = constraints[constraint_param]
                    if constraint_param not in real_params.keys() or str(real_params[ constraint_param ]) != constraint_value:
                        if constraint_param in real_params.keys():
                            print( "Scenario configuration did not satisfy constraint: {0} == {1} but must == {2}.".format( constraint_param, str(real_params[ constraint_param ]), constraint_value ) )
                        else:
                            print( "Scenario configuration did not satisfy constraint(s). Key not present." )
                        constraints_satisfied = False
                        continue

            if constraints_satisfied == False:
                continue

            if campjson is None:
                if configjson["parameters"]["Enable_Interventions"] == 1:
                    # Try loading directly from file
                    alt = glob.glob(os.path.join(simcfg["path"], "campaign*.json"))[0]
                    runner.campaign_filename = os.path.basename(alt)
                    campjson_file = open(os.path.join(simcfg["path"], runner.campaign_filename))
                    campjson = json.loads( campjson_file.read().replace( "u'", "'" ).replace( "'", '"' ).strip( '"' ) )
                    campjson_file.close()
                else:
                    campjson = dict()
                    campjson["Events"] = []

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

            thread = runner.commissionFromConfigJson( sim_timestamp, configjson, simcfg["path"], report, suite_type )
            ru.reg_threads.append( thread )

        # do a schema test also
        if not params.disable_schema_test:
            report.schema = runner.doSchemaTest()

    elif "sweep" in reglistjson:
        print( "Running sweep...\n" )
        param_name = reglistjson["sweep"]["param_name"]
        # NOTE: most code below was copy-pasted from 'tests' (regression) case above.
        # I could factor this and generalize now but future extensions of sweep capability may involve
        # a greater departure from this code path so that might be a premature optimization.
        for param_value in reglistjson["sweep"]["param_values"]:
            os.chdir(ru.cache_cwd)
            sim_timestamp = str(datetime.datetime.now()).replace('-', '_' ).replace( ' ', '_' ).replace( ':', '_' ).replace( '.', '_' )
            if regression_id == None:
                regression_id = sim_timestamp
            # atrophied? configjson_filename = reglistjson["sweep"]["path"]
            # atrophied? configjson_path = str( os.path.join( reglistjson["sweep"]["path"], "config.json" ) )

            configjson = ru.flattenConfig( os.path.join( reglistjson["sweep"]["path"], "param_overrides.json" ) )
            if configjson is None:
                print("Error flattening config.  Skipping " + simcfg["path"])
                ru.final_warnings += "Error flattening config.  Skipped " + simcfg["path"] + "\n"
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

            thread = runner.commissionFromConfigJson( sim_timestamp, configjson, reglistjson["sweep"]["path"], None, 'sweep' )
            ru.reg_threads.append( thread )
    else:
        print "Unknown state"
        sys.exit(0)

    # stay alive until done
    for thr in ru.reg_threads:
        thr.join()
        #print str(thr.sim_timestamp) + " is done."

    component_tests_passed = True
    if( params.component_tests ):
        if( params.scons or (os.sys.platform != 'win32') ):
            component_test_path = "../build/x64/Release/componentTests/componentTests"
        else:
            component_test_path = "../x64/Release/componentTests"

        if os.sys.platform == 'win32':
            component_test_path += ".exe"

        os.chdir( "../componentTests" )
        if( os.path.exists( component_test_path ) ):
            #print("+++++++++++++++++++++++ componentTests - Begin +++++++++++++++++++++++")
            sys.stdout.flush()

            ret = None
            if( params.component_tests_show_output ):
                stderr_file = open("StdErr.txt","w")
                ret = subprocess.call( [ component_test_path ], stderr=stderr_file )
                stderr_file.close()
                os.remove("StdErr.txt")
            else:
                stdout_file = open("StdOut.txt","w")
                ret = subprocess.call( [ component_test_path ], stdout=stdout_file )
                stdout_file.close()
                if( ret == 0 ):
                    os.remove("StdOut.txt")

            if ret != 0:
                component_tests_passed = False
                print >> sys.stderr, "Component tests failed!"

            #print("+++++++++++++++++++++++ componentTests - End +++++++++++++++++++++++")
            sys.stdout.flush()

    if report is not None:
        endtime = datetime.datetime.now()
        report.write(os.path.join("reports", "report_" + regression_id + ".xml"), endtime - starttime)
        print '========================================'
        print 'Elapsed time: ', endtime - starttime
        print '%(tests)3d tests total, %(passed)3d passed, %(failed)3d failed, %(errors)3d errors, schema: %(schema)s.' % report.Summary
        print '========================================'

    if ru.final_warnings is not "":
        print("----------------")
        print(ru.final_warnings)
        print("----------------")
        #raw_input("Press Enter to continue...")

    if not component_tests_passed:
        sys.exit(1)
        
    # if doing sweep, call plotAllCharts.py with all sim_timestamps on command line.
    if "sweep" in reglistjson:
        print( "Plot sweep results...\n" )
        all_data = []
        all_data_prop = []
        
        ref_path_prop = os.path.join( str(reglistjson["sweep"]["path"]), os.path.join( "output", "PropertyReport.json" ) )
        ref_json_prop = {}
        if os.path.exists( ref_path_prop ) == True:
            ref_json_prop = json.loads( open( os.path.join( ru.cache_cwd, ref_path_prop ) ).read() )

        ref_path = os.path.join( str(reglistjson["sweep"]["path"]), os.path.join( "output", "InsetChart.json" ) )
        ref_json = json.loads( open( os.path.join( ru.cache_cwd, ref_path ) ).read() )

        for thr in ru.reg_threads:
            sim_dir = os.path.join( thr.sim_root, thr.sim_timestamp )
            icj_filename = os.path.join( sim_dir, os.path.join( "output", "InsetChart.json" ) )
            icj_json = json.loads( open( icj_filename ).read() )
            all_data.append( icj_json )
            if os.path.exists( ref_path_prop ) == True:
                prj_filename = os.path.join( sim_dir, os.path.join( "output", "PropertyReport.json" ) )
                prj_json = json.loads( open( prj_filename ).read() )
                all_data_prop.append( prj_json )
        plot_title = "Sweep over " + reglistjson["sweep"]["param_name"] + " (" + str(len(reglistjson["sweep"]["param_values"])) + " values)"
        os.chdir( ru.cache_cwd )
        import plotAllCharts
        plotAllCharts.plotBunch( all_data, plot_title, ref_json )
        if os.path.exists( ref_path_prop ) == True:
            plotAllCharts.plotBunch( all_data_prop, plot_title, ref_json_prop )
        
    return


if __name__ == "__main__":
    # 'twould be nice to ditch this (keeping for legacy reasons) anyone actually use this?
    if len(sys.argv) > 1 and sys.argv[1] == "--flatten":
        ru.flattenConfig( sys.argv[2] )
        sys.exit(0)

    main()
