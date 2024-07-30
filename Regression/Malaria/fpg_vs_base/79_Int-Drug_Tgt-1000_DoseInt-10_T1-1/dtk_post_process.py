import json, sys, os, shutil
import dtk_vector_stats_support as dvs

def application( output_path ):
    given_minus_received = dvs.ConvertReportVectorStats( output_path )
    
    # Only look at the points after the net was distributed
    given_minus_received = given_minus_received[201:]    
    
    dvs.CheckInfectiousBites( given_minus_received, output_path )
    
    print("Done testing results")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("need output path")
        exit(0)

    output_path = sys.argv[1]

    application( output_path )
