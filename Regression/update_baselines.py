#!/usr/bin/python

import sys
import xml.dom.minidom as xml
import json
import shutil
import os

if __name__ == '__main__':
    file_index = 1
    copy_all = False
    bad_args = True

    if len(sys.argv) == 2:
        file_index = 1
        bad_args = False
    elif len(sys.argv) == 3 and sys.argv[1] == "-copy_all":
        file_index = 2
        copy_all = True
        bad_args = False
    
    if bad_args:
        print( 'usage: %s <-copy_all> xmlReportFile' % (sys.argv[0]) )
    else:
        report_xml = xml.parse( sys.argv[ file_index ] )
        print("copy_all="+str(copy_all))
        for node in report_xml.getElementsByTagName( 'failure' ):
            message = node.getAttribute( 'message' )
            (regression_name, icj_path) = message.split()[0], message.split()[9]

            if not copy_all:
                reg_path = os.path.join( regression_name, 'output' )
                if os.path.exists( message.split()[9] ):
                    print( "copy "+message.split()[9]+" to " + reg_path )
                    shutil.copy( message.split()[9], reg_path )
            else:
                source_dir = os.path.dirname(icj_path)
                print( 'Copying *.json and *.csv from {0} to {1}'.format( source_dir, regression_name ))
                for item in os.listdir(source_dir):
                    if item.split('.')[-1].lower() == 'json' or item.split('.')[-1].lower() == 'csv':
                        if os.name == "posix" and item != "InsetChart.json":
                            continue
                        source_file = os.path.join( source_dir, item )
                        if os.path.exists( source_file ):
                            dest_file = os.path.join( regression_name, 'output', item )
                            if os.name == "posix":
                                dest_file = dest_file.replace( "InsetChart.json", "InsetChart.linux.json" )
                            print( '\tCopying {0} to {1} ...'.format( source_file, dest_file ) )
                            shutil.copy( source_file, dest_file )
