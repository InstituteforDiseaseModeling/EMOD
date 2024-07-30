import sys
import os
from emod_api.serialization import dtkFileTools as dft 
from emod_api.serialization import dtkFileSupport as support
import pathlib


if __name__ == "__main__":
    path = os.path.dirname(os.path.abspath(__file__))
    path_serialized_file = os.path.join(path, "output", "state-00002.dtk")

    # opens and saves all nodes, suid
    ser_pop = dft.read(path_serialized_file)
    count_errors = 0
    
    keys_header5 = ['author', 
                    'bytecount',
                    'chunkcount',
                    'chunksizes',
                    'compression',
                    'date',
                    'engine',
                    'tool',
                    'version',
                    'emod_sccs_date',
                    'emod_major_version',
                    'emod_minor_version',
                    'emod_revision_number',
                    'ser_pop_major_version',
                    'ser_pop_minor_version',
                    'ser_pop_patch_version',
                    'emod_build_date',
                    'emod_builder_name',
                    'emod_sccs_branch',
                    'emod_sccs_date']
    header5 = ser_pop.header
    
    for key in keys_header5:
        if header5.get(key) is None:
            print("Error key '", key, "' is missing.")
            count_errors += 1 
    
    if count_errors > 0:
        print("There were", count_errors, "errors")
    else:
        print("Test passed")