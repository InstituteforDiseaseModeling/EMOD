import os
import sys
import subprocess

input_path        = "C:\\Input\\Namawala_2_nodes"

input_file   = "migration_rates.txt"
output_file  = "Netherlands\\Netherlands_multi_node_local_migration.bin"

# write binary migration file from text input with lines like:
# Node1 index       Node2 index     rate
subprocess.call([sys.executable, "convert_txt_to_bin.py", input_file, os.path.join(input_path, output_file)])
