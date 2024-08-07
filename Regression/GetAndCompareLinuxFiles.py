# GetAndCompareLinuxFiles.py
# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------

import sys, os, json, collections, struct
import matplotlib.pyplot as plt
from math import sqrt, ceil
import shutil # copyfile
import xml.etree.ElementTree as ET
import argparse

def ReadXML( xml_fn ):
    
    tree = ET.parse(xml_fn)
    root = tree.getroot()
  
    failures = set()
  
    for xml_failure in root.findall('./testcase/failure'):
        msg = xml_failure.attrib["message"]
        msg_elements = msg.split(' ')
        test_dir = msg_elements[0]
        sim_dir = msg_elements[10]
        sim_dir = sim_dir.replace("/mnt/iazdvfil05/home/", "//iazdvfil05.idmhpc.azr/IDM/home/")
        #print(sim_dir)

        test_failure = ( test_dir, sim_dir )
        failures.add( test_failure )

    return failures

def ReadJson( json_fn ):
    with open( json_fn,'r') as json_file:
        json_data = json.load( json_file )
    return json_data

def isTestSameAsRef( test_name, ref_fn, test_fn ):
    ref_json  = ReadJson( ref_fn )
    test_json = ReadJson( test_fn )

    num_chans = ref_json["Header"]["Channels"]

    test_diff_ref = False
    for chan_title in sorted(ref_json["Channels"]):
        try:
            num_steps_1 = len(ref_json["Channels"][chan_title]["Data"])
            num_steps_2 = len(test_json["Channels"][chan_title]["Data"])

            if( num_steps_1 != num_steps_2 ):
                print("\nChannels dont have same number of data-data1="+num_steps_1+"  data2="+num_steps_2 + " for test="+test_name)
                return False

            for tstep_idx in range( 0, num_steps_1 ):
               diff = ref_json["Channels"][chan_title]["Data"][tstep_idx] - test_json["Channels"][chan_title]["Data"][tstep_idx]
               if( diff != 0.0 ):
                    return False

        except Exception as ex:
            print( str(ex) )

    return True


def PlotData( test_name, ref_fn, test_fn ):
    ref_json  = ReadJson( ref_fn )
    test_json = ReadJson( test_fn )

    num_chans = ref_json["Header"]["Channels"]

    square_root = ceil(sqrt(num_chans))

    n_figures_x = square_root
    n_figures_y = ceil(float(num_chans)/float(square_root)) #Explicitly perform a float division here as integer division floors in Python 2.x

    idx = 1
    for chan_title in sorted(ref_json["Channels"]):
        try:
            num_steps_1 = len(ref_json["Channels"][chan_title]["Data"])
            num_steps_2 = len(test_json["Channels"][chan_title]["Data"])

            if( num_steps_1 != num_steps_2 ):
                print("\nChannels dont have same number of data-data1="+num_steps_1+"  data2="+num_steps_2 + " for test="+test_name)
                exit(0)

            subplot = plt.subplot( n_figures_x, n_figures_y, idx ) 
            subplot.plot( ref_json["Channels"][chan_title]["Data"], 'r-', test_json["Channels"][chan_title]["Data"], 'b-' )
            plt.setp( subplot.get_xticklabels(), fontsize='6' )
            plt.title( chan_title, fontsize='8' )
            idx += 1
        except Exception as ex:
            print( str(ex) )

    plt.suptitle( test_name )
    plt.subplots_adjust( bottom=0.05 )
    plt.show()



if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('test_report',   default=None, nargs='?', help='The XML file with the results of the test suite (failures)')
    parser.add_argument('replace', default=None, nargs='?', help='Do not show plots but just update the XXX.linux.json files')

    args = parser.parse_args()
    
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit()

    replace = (args.replace == "replace")

    failures = ReadXML( sys.argv[1] )

    results_same = 0
    results_error = 0
    results_copied = 0
    
    for test_name, sim_dir in failures:
        filename = ""
        if "InsetChart" in sim_dir:
            filename  = sim_dir.replace( ".linux.json", ".json" )
        copy_to_fn = "./" + test_name + "/output/InsetChart.linux.json.new"
        linux_fn = "./" + test_name + "/output/InsetChart.linux.json"
        ref_fn =  "./" + test_name + "/output/InsetChart.json"
        
        if (len(filename) > 0) and os.path.exists( filename ):
            if isTestSameAsRef( test_name, ref_fn, filename ):
                print( ">>>> " + test_name + " had same results as in Windows" )
                results_same += 1
                if not replace and os.path.exists( linux_fn ):
                    os.rename( linux_fn, linux_fn+".old" )
            else:
                print( "Copying file for " + test_name )
                shutil.copy( filename, copy_to_fn )
                results_copied += 1
                if( replace ):
                    if( os.path.exists( linux_fn ) ):
                        os.rename( linux_fn, linux_fn+".old" )
                    os.rename( copy_to_fn, linux_fn )
                else:
                    PlotData( test_name, ref_fn, copy_to_fn )
        else:
            print( "!!!!!!!!!! File not found for test: " + test_name  )
            results_error += 1

    print("  ")
    print("copied = " + str(results_copied))
    print("same = " + str(results_same))
    print("error = " + str(results_error))
    
    exit(0)
