#!/usr/bin/python


"""
Purpose:
VerifyRcFile.py can be used to verify that the macros in the given RC file
are found in the cpp files.  It will search all of the subdirectories from
where it is called looking for cpp files.

Use:
python VerifyRcFile.py file.rc

"""

import sys
import os

def findInFile( param_text, fn ):
    file = open( fn, "r" )
    for line in file:
        if param_text in line:
            return True
    return False

def findParam( param_text, dir ):
    dir_list = os.listdir( dir )
    for entry in dir_list:
        if entry.endswith(".cpp"):
            fn = os.path.join( dir, entry )
            found = findInFile( param_text, fn )
            #print entry + "  found (" + param_text + ")="+str(found)
            if found:
                return found
        elif os.path.isdir( entry ):
            found = findParam( param_text, entry )
            if found:
                return found
    return False

def application( rc_fin ):

    print( "You are here! " + os.getcwd() )
    print( "Process RC File = " + rc_fn )

    rc_file = open( rc_fn, "r" )

    for line in rc_file:
        line = line.replace('\n','')
        line_array = line.split( ' ' )
        if len(line_array) > 2 and line_array[0] == "#define":
            param_text = line_array[1]
            if not findParam( param_text, os.getcwd() ):
                print param_text

if __name__ == "__main__":
    rc_fn = "utils/config_params.rc"
    if len(sys.argv) > 1:
        rc_fn = sys.argv[1]

    application( rc_fn )
