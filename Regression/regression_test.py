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
    parser.add_argument('--local', default=False, action='store_true', help='Run all simulations locally.')

    args = parser.parse_args()

    params = regression_runtime_params.RuntimeParameters(args)
    return params



def main():
    params = setup()
    runner = regression_runner.MyRegressionRunner(params)

    reglistjson = None
    if(str.isdigit(params.suite)):
        # Special shortcut if you just want to run a single regression with a number prefix
        dirs = glob.glob(params.suite + "_*")
        for dir in dirs:
            if os.path.isdir(dir):
                print("Executing single test: " + dir)
                reglistjson = { "tests" : [ { "path" : dir } ] }
    elif os.path.isdir( params.suite ) and os.path.exists( os.path.join( params.suite, "param_overrides.json" ) ):
        print( "You specified a directory." )
        reglistjson = { "tests" : [ { "path" : params.suite } ] }
    else:
        if params.suite.endswith(".json"):
            # regular case
            params.suite = params.suite.replace(".json", "")
        # handle comma-separated list of suites
        reglistjson = json.loads( open( params.suite.split(',')[0] + ".json" ).read() )
        if "tests" in reglistjson and len( params.suite.split(',') ) > 1:
            for suite in params.suite.split(',')[1:]:
                data = json.loads( open( suite + ".json" ).read() )
                if "tests" in data:
                    reglistjson[ "tests" ].extend( data["tests"] )
                else:
                    print( suite + " does not appear to be a suite, missing key 'tests'" )

    report = None
    regression_id = None
    if "tests" in reglistjson:
        p = subprocess.Popen( (params.executable_path + " -v").split(), shell=False, stdout=subprocess.PIPE )
        [pipe_stdout, pipe_stderr] = p.communicate()
        ru.version_string = re.search('[0-9]+.[0-9]+.[0-9]+.[0-9]+', pipe_stdout).group(0)

        starttime = datetime.datetime.now()
        report = regression_report.Report(params, ru.version_string)

        print( "Running regression...\n" )
        for simcfg in reglistjson["tests"]:
            os.chdir(ru.cache_cwd)
            sim_timestamp = str(datetime.datetime.now()).replace('-', '_' ).replace( ' ', '_' ).replace( ':', '_' ).replace( '.', '_' )
            if regression_id == None:
                regression_id = sim_timestamp

            try:
                #print "flatten config: " + os.path.join( simcfg["path"],"param_overrides.json" )
                configjson = ru.flattenConfig( os.path.join( simcfg["path"],"param_overrides.json" ) )
            except:
                report.addErroringTest(simcfg["path"], "Error flattening config.", "(no simulation directory created).")
                configjson = None

            campaign_override_fn = os.path.join( simcfg["path"],"campaign_overrides.json" )

            try:
                #print "flatten campaign: " + campaign_override_fn
                campjson = ru.flattenCampaign( campaign_override_fn, False )
            except:
                print "failed to flatten campaign: " + campaign_override_fn
                report.addErroringTest(simcfg["path"], "Failed flattening campaign.", "(no simulation directory created).")
                campjson = None

            if configjson is None:
                print("Error flattening config.  Skipping " + simcfg["path"])
                ru.final_warnings += "Error flattening config.  Skipped " + simcfg["path"] + "\n"
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

            thread = runner.commissionFromConfigJson( sim_timestamp, configjson, simcfg["path"], report )
            ru.reg_threads.append( thread )
        # do a schema test also
        if params.dts == True:
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

            thread = runner.commissionFromConfigJson( sim_timestamp, configjson, reglistjson["sweep"]["path"], None, False )
            ru.reg_threads.append( thread )
    else:
        print "Unknown state"
        sys.exit(0)

    # stay alive until done
    for thr in ru.reg_threads:
        thr.join()
        #print str(thr.sim_timestamp) + " is done."

    if report is not None:
        endtime = datetime.datetime.now()
        report.write(os.path.join("reports", "report_" + regression_id + ".xml"), endtime - starttime)
        print '========================================'
        print 'Elapsed time: ', endtime - starttime
        print '%(tests)3d tests total, %(passed)3d passed, %(failed)3d failed, %(errors)3d errors, schema: %(schema)s.' % report.Summary

    if ru.final_warnings is not "":
        print("----------------\n" + ru.final_warnings)
        #raw_input("Press Enter to continue...")

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
        time.sleep(1)
        
    return


if __name__ == "__main__":
    # 'twould be nice to ditch this (keeping for legacy reasons) anyone actually use this?
    if len(sys.argv) > 1 and sys.argv[1] == "--flatten":
        ru.flattenConfig( sys.argv[2] )
        sys.exit(0)

    main()
