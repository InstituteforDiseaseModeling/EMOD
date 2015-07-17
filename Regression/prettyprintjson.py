#!/usr/bin/python

import json
import os
import sys

inputfile = sys.argv[1]
outputfile  = sys.argv[2]

orig = json.loads( open( inputfile ).read() )
open( outputfile, "w" ).write( json.dumps( orig, indent=4 ) )
