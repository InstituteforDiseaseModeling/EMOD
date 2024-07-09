import json, sys, os, shutil
import numpy as np
import pandas as pd
#import dtk_test.dtk_sft as dtk_sft

#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#!!! I tried multiple things to get this script to work while running
#!!! regression tests and it kept failing.  The scripts runs on the data
#!!! correctly but there is something with access to the files when
#!!! running as a DTK post process.
#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

# The barcode ID calculated in EMOD uses this same algorithm.
def CreateBarcodeID( nucleotide_sequence ):
    hash_code = 17;
    for val in nucleotide_sequence:
        hash_code = 31 * hash_code + val
        
    return hash_code


def ReadBarodeData( output_path ):
    genome_data_filename = os.path.join( output_path, "variants24_afFPG.npy" )
    
    if not os.path.exists( genome_data_filename ):
        raise Exception("missing file="+genome_data_filename)
        
    genome_data_array = np.load( genome_data_filename )
    
    barcode_id_list = []
    for genome_data in genome_data_array:
        barcode_id = CreateBarcodeID( genome_data )
        barcode_id_list.append( barcode_id )
        
    return barcode_id_list
        

def ReadInfectionData( output_path ):
    infection_data_filename = os.path.join( output_path, "infIndexRecursive-genomes-df.csv" )

    if not os.path.exists( infection_data_filename ):
        raise Exception("missing file="+infection_data_filename)
        
    df = pd.read_csv( infection_data_filename )
    
    return df
    

def application( output_path ):
    #dtk_sft.wait_for_done( filename="stdout.txt")
    
    print("ReadBarodeData")
    npy_barcode_id_list = ReadBarodeData( output_path )
    
    print("ReadInfectionData")
    inf_data_df = ReadInfectionData( output_path )
    
    print("compare data")
    error_line_num = -1
    for line_num, row in inf_data_df.iterrows():
        inf_data_barcode_indexes_str = row["recursive_nid"  ]
        inf_data_inf_ids_str         = row["infection_ids"  ]
        inf_data_barcode_ids_str     = row["barcode_ids"    ]

        # remove last(']') and first ('[') characters
        #inf_data_barcode_indexes_str = inf_data_barcode_indexes_str[:-1]
        #inf_data_barcode_indexes_str = inf_data_barcode_indexes_str[1:]
        #
        #inf_data_inf_ids_str = inf_data_inf_ids_str[:-1]
        #inf_data_inf_ids_str = inf_data_inf_ids_str[1:]
        #
        #inf_data_barcode_ids_str = inf_data_barcode_ids_str[:-1]
        #inf_data_barcode_ids_str = inf_data_barcode_ids_str[1:]
        
        # convert to a list of numbers
        inf_data_barcode_indexes = [int(e) for e in inf_data_barcode_indexes_str.split(',')]
        inf_data_inf_ids         = [int(e) for e in inf_data_inf_ids_str.split(',')]
        inf_data_barcode_ids     = [int(e) for e in inf_data_barcode_ids_str.split(',')]
            
        inf_data_indexes_count         = row["recursive_count"]
        inf_data_barcode_indexes_count = len(inf_data_barcode_indexes)
        inf_data_inf_ids_count         = len(inf_data_inf_ids        )
        inf_data_barcode_ids_count     = len(inf_data_barcode_ids    )
        
        if (inf_data_indexes_count != inf_data_barcode_indexes_count) or\
           (inf_data_indexes_count != inf_data_inf_ids_count        ) or\
           (inf_data_indexes_count != inf_data_barcode_ids_count    ):
            print("BAD counting")
            print(inf_data_indexes_count)
            print(inf_data_barcode_indexes_count)
            print(inf_data_inf_ids_count)
            print(inf_data_barcode_ids_count)
            error_line_num = line_num
            break
           
        i = 0
        for barcode_index in inf_data_barcode_indexes:
            npy_barcode_id = npy_barcode_id_list[ barcode_index ]
            inf_data_barcode_id = inf_data_barcode_ids[ i ]
            if npy_barcode_id != inf_data_barcode_id:
                print("BAD barcode ID")
                print(npy_barcode_id)
                print(inf_data_barcode_id)
                error_line_num = line_num
                break
            i += 1
    
    results_filename = os.path.join( output_path, "results.txt" )
    with open(results_filename, "w") as results_file:
        if error_line_num == -1:
            results_file.write("GOOD: Data is all synchronized!!")
            print("GOOD: Data is all synchronized!!")
        else:
            results_file.write("BAD: Data not synchronized on line "+str(error_line_num))    
            print("BAD: Data not synchronized on line "+str(error_line_num))    
    
    print("Done checking FPG output files for the Genetic Workflow")
    
    
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("need output path")
        exit(0)

    output_path = sys.argv[1]

    application( output_path )
