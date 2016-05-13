#!/usr/bin/python

import compiledemog
import json
import os
import sys

def application(filename):
    compiled_filename = filename
    if os.path.exists( filename ) == False:
        print( "File not found: " + filename )
    try:
        demog_json = json.loads( open( filename ).read() )
        if demog_json.has_key( "NodeOffsets" ) == False or demog_json.has_key( "StringTable" ) == False:
            compiledemog.Compile( filename )
            compiled_filename = filename.replace( ".json", ".compiled.json" )
        else:
            print( "Already compiled." )
    except Exception as ex:
        print( str(ex) )

    print( "Returning " + compiled_filename + "\n" )
    sys.stdout.flush()
    sys.stderr.flush()
    return compiled_filename
