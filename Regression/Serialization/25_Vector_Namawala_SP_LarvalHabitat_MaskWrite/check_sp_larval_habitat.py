import sys
import os
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "Scripts", "serialization"))  # path to dtkFileTools
from emod_api.serialization import dtkFileTools as dft 
from emod_api.serialization import dtkFileSupport as support
import pathlib
import collections.abc
import difflib

counter = 0

def find(name, handle, currentlevel="ser_pop.nodes"):
    global counter
    found = False
    if type(handle) is str and difflib.get_close_matches(name, [handle], cutoff=1.0):
        print(counter, "  Found in: ", currentlevel)
        counter += 1
        return True

    if type(handle) is str or not isinstance(handle, collections.abc.Iterable):
        return False

    for idx, key in enumerate(handle):  # key can be a string or on dict/list/..
        level = currentlevel + "." + key if type(key) is str else currentlevel + "[" + str(idx) + "]"
        try:
            tmp = handle[key]
            if isinstance(tmp, collections.abc.Iterable):
                found = find(name, key, level + "[]") or found
            else:
                found = find(name, key, level) or found
        except:
            found = find(name, key, level) or found  # list or keys of a dict, works in all cases but misses objects in dicts
        if isinstance(handle, dict):
            found = find(name, handle[key], level) or found  # check if string is key for a dict
    return found


if __name__ == "__main__":
    path = os.path.dirname(os.path.abspath(__file__))
    path_serialized_file = os.path.join(path, "output", "state-00050.dtk")

    # opens and saves all nodes, suid
    ser_pop = dft.read(path_serialized_file)
    node = ser_pop.nodes[0]
    print("Check in generated file: m_larval_habitats must not exist, serializationFlags = 11")
    print("Ok! m_larval_habitats not found" if not find("m_larval_habitats", ser_pop.nodes) else "Test failed! Found m_larval_habitats.")
    if find("serializationFlags", ser_pop.nodes):
        print("Ok! found serializationFlags.")
        print("OK! serializationFlags == 11" if node["serializationFlags"] == 11 else "Test failed! serializationFlags != 11")
    else:
        print("Test failed! serializationFlags not found.")
    print("Ok! EggQueues found" if find("EggQueues", ser_pop.nodes) else "Test failed! EggQueues not found.")
