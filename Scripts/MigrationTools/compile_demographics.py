import os
import sys
import subprocess

input_path        = "C:\\src\\inputs\\Bihar_Like_2_2x2_Properties"

uncompiled_files  = [ "Bihar_2_2x2_Properties_torus_demographics.json" ]

compileScript     = "C:\\src\\dtk\\Tuberculosis-2013-current\\Scripts\\InputFiles\\compiledemog.py"
migrationScript   = "C:\\src\\dtk\\Tuberculosis-2013-current\\Scripts\\InputFiles\\createmigrationheader.py"

# compile the demographics and mixing-pool files (single- and multi-node)
for uncompiled_file in uncompiled_files:
    subprocess.call([sys.executable, compileScript, os.path.join(input_path, uncompiled_file)])

# create the multi-node local migration header
subprocess.call([sys.executable, migrationScript, os.path.join(input_path, "Bihar_2_2x2_Properties_torus_demographics.compiled.json"), "local"])
