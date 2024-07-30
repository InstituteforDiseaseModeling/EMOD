#!/usr/bin/python

import json


class DemographicsKeys:
    NODES = "Nodes"
    NODEID = "NodeID"
    DEFAULTS = "Defaults"
    NODE_ATTRIBUTES = "NodeAttributes"
    INDIVIDUAL_ATTRIBUTES = "IndividualAttributes"
    METADATA = "Metadata"
    NODECOUNT = "NodeCount"
    INITIAL_POPULATION = "InitialPopulation"


def speculative_json_load(json_obj, key_train=[]):
    """
    Utility method for extracting data from json objects, makes it easier to handle different types of data (lists,
    dicts, etc.) and to deeply index values without having to add specific tests for every key along the way existing.

    :param json_obj: json object to load data from
    :param key_train: list of keys to drill down through (e.g. ['a', 'b', 'c'] -> json_obj['a']['b']['c']
    :return: value retrieved or None if some key along the way doesn't exist
    """
    current = json_obj
    if not key_train or len(key_train) == 0:
        return None

    # make using a single key easier by helpfully casting it to a list
    if not isinstance(key_train, list):
        key_train = [key_train]

    # progress down the tree for each key as long as they exist
    for key in key_train:
        # you may not know what format each level of data is in (dictionary vs. array) in the json object, nor what
        # each index type should be, this code conveniently just "does the right thing for you" by figuring out what
        # data type is used and whether it's necessary to convert the key to an integer or not
        if isinstance(current, dict):
            if key in current:
                current = current[key]
            else:
                return None
        elif isinstance(current, list):
            if isinstance(key, int):
                index = key
            elif key.isdigit():
                index = int(key)
            else:
                # could have an error here I guess, but overall the point of this is to not throw errors
                return None

            if index < len(current):
                current = current[index]
            else:
                return None
        else:
            return None

    return current


class DemographicsFile(object):
    """
    Handle reading demographics file json in a more convenient interface.

    Specify node subset, node attributes, metadata, node defaults, and individual defaults to load into node_data
    """
    def __init__(self, demographics_filename, node_subset=[], collect_attributes=[], collect_metadata=[], collect_node_defaults={}, collect_individual_defaults={}):
        self.filename = demographics_filename
        self.node_subset = list(node_subset)
        self.attributes = list(collect_attributes)
        self.collect_metadata = list(collect_metadata)
        self.collect_node_defaults = dict(collect_node_defaults)
        self.collect_individual_defaults = dict(collect_individual_defaults)

        self.metadata = {}
        self.node_defaults = {}
        self.individual_defaults = {}
        self.node_data = {}

        self.load_demographics()

    def get_node_id(self, node):
        """
        Extract node-id from a node json object

        :param node: node json obj
        :return: node id
        """
        return speculative_json_load(node, [DemographicsKeys.NODEID])

    def load_attribute(self, node, attribute):
        """
        Load an attribute from a node json object into node_data

        :param node: node json object
        :param attribute: node attribute to retrieve
        """
        node_id = self.get_node_id(node)
        if len(self.node_subset) > 0 and node_id not in self.node_subset:
            return
        attrib_value = speculative_json_load(node, [DemographicsKeys.NODE_ATTRIBUTES, attribute])
        self.add_node_data(node_id, attribute, attrib_value)

    def add_node_data(self, node_id, attribute_name, value):
        """
        Add node data for a specific node id and attribute, create node data dictionary for node id if it doesn't exist.
        """
        if node_id not in self.node_data:
            self.node_data[node_id] = {}
        self.node_data[node_id][attribute_name] = value

    def load_metadata(self, demog_json, key):
        """
        Load metadata from demographics json for a specific property

        :param demog_json: demographics json object
        :param key: property name
        """
        metadata_val = speculative_json_load(demog_json, [DemographicsKeys.METADATA, key])
        if metadata_val:
            self.metadata[key] = metadata_val

    def get_default(self, demog_json, root_key, key_path):
        """
        Get default value from demographics json for a particular root (node or individual attribs) and a particular key path

        :param demog_json: demographics json object
        :param root_key: root key under defaults section, typically either node or individual attributes
        :param key_path: string path for value to collect, e.g. "MortalityDistribution/PopulationGroups/0/0"
        :return: value from demographics defaults (or None if some key is not found)
        """
        full_key_path = [DemographicsKeys.DEFAULTS, root_key] + key_path.split('/')
        return speculative_json_load(demog_json, full_key_path)

    def load_demographics(self):
        """
        Load demographics data from configured filename into self.metadata, self.node_defaults,
        self.individual_defaults, and self.node_data
        """
        with open(self.filename) as demofile:
            demog = json.load(demofile)

        for metadata_key in self.collect_metadata:
            self.load_metadata(demog, metadata_key)

        for node_def_key in self.collect_node_defaults:
            self.node_defaults[node_def_key] = self.get_default(demog, DemographicsKeys.NODE_ATTRIBUTES, node_def_key)

        for ind_def_key in self.collect_individual_defaults:
            self.individual_defaults[ind_def_key] = self.get_default(demog, DemographicsKeys.INDIVIDUAL_ATTRIBUTES, ind_def_key)

        nodes = speculative_json_load(demog, [DemographicsKeys.NODES])
        if nodes:
            for node in nodes:
                for attribute in self.attributes:
                    self.load_attribute(node, attribute)
