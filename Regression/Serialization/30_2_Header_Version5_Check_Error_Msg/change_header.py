import sys
import os
from emod_api.serialization import dtkFileTools as dft 
from emod_api.serialization import dtkFileSupport as support
import pathlib


if __name__ == "__main__":
    path = os.path.dirname(os.path.abspath(__file__))
    path_serialized_file = os.path.join(path, "testing", "state-00002.dtk")

    # load population, change major version, save population
    ser_pop = dft.read(path_serialized_file)
    header5 = ser_pop.header
    header5['emod_info'].ser_pop_major_version = 0 # major version in SP must be different from version written by emod
    dft.write(ser_pop, path_serialized_file)