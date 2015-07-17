#!/usr/bin/python

from __future__ import print_function
import argparse
import json
import os
from struct import pack
import sys
import time

#valid migration types and their link counts
canonical_types = {'LOCAL': 8, 'REGIONAL': 30, 'SEA': 5, 'AIR': 60}


# this tool expects the input demographics file to contain the following objects/tags:
# {
#     "Metadata": {
#         ...,
#         "IdReference": id schema (e.g. "Legacy" or "Gridded world grump2.5arcmin"),
#         ...
#     },
#     ...,
#     "Nodes":[
#         {
#             "NodeID": nodeid
#         },
#         ...
#     ]
# }
#
# node ids are returned in a list to preserve the order of the demographics file Nodes array
#
# node migration data is written out in the order of the nodes in the demographics file
# each entry consists of a 32-bit/4-byte destination node id and 64-bit/8-byte double float migration rate
# see the canonical types above for the number of outbound links written per source node


class BuildException(Exception):
    def __init__(self, message):
        Exception.__init__(self, message)


class MigrationNetwork(object):
    def __init__(self, max_links):
        self.__node_ids = []
        self.__id_reference = "UNSPECIFIED"
        self.__links = {}
        self.__link_count = max_links

    def get_node_ids(self): return self.__node_ids
    def set_node_ids(self, value): self.__node_ids = value
    def del_node_ids(self): del self.__node_ids
    node_ids = property(get_node_ids, set_node_ids, del_node_ids, "Node Id List")

    def get_reference(self): return self.__id_reference
    def set_reference(self, value): self.__id_reference = value
    def del_reference(self): del self.__id_reference
    id_reference = property(get_reference, set_reference, del_reference, "Id Reference")

    def get_network(self): return self.__links
    def set_network(self, value): self.__links = value
    def del_network(self): del self.__links
    links = property(get_network, set_network, del_network, "Node Network")

    def get_link_count(self): return self.__link_count
    def set_link_count(self, value): self.__link_count = value
    def del_link_count(self): del self.__link_count
    link_count = property(get_link_count, set_link_count, del_link_count, "Per Node Link Count")

    def loadDemographicsFile(self, filename):
        def tryLoadJson(filename):
                handle = open(filename)
                try:
                    json_data = json.load(handle)
                except ValueError as e:
                    raise BuildException("Error '%s' loading JSON file" % e)
                finally:
                    handle.close()
                return json_data

        def tryExtractNodeIds(json_data):
            try:
                node_ids = [int(node['NodeID']) for node in json_data['Nodes']]
            except KeyError:
                raise BuildException("Couldn't find entry 'Nodes' in JSON data")
            except TypeError:
                raise BuildException("'Nodes' entry in JSON data doesn't appear to be an array")
            return node_ids

        def tryGetEntry(dictionary, key):
            try:
                entry = dictionary[key]
            except KeyError as ke:
                raise BuildException("Couldn't find entry '%s' in data" % ke)
            return entry

        json_data = tryLoadJson(filename)
        node_ids = tryExtractNodeIds(json_data)
        id_reference = tryGetEntry(tryGetEntry(json_data, 'Metadata'), 'IdReference')

        self.__node_ids = node_ids
        self.__id_reference = id_reference
        return self.__node_ids, self.__id_reference

    def loadMigrationRates(self, filename):
        def validateRateInformation(line, node_ids, line_number):
            def validateNodeId(node_id, label, node_ids):
                if not node_id in node_ids:
                    raise BuildException('%s node ID %s not found in demographics.' % (label, node_id))

            info = line.split('#')[0].strip().split()
            if len(info) != 3:
                raise BuildException("Poorly formed input in migration rates file (line %s): '%s'" % (line_number, line))
            source_id = int(info[0])
            destination_id = int(info[1])
            rate = float(info[2])
            validateNodeId(source_id,      'source',      node_ids)
            validateNodeId(destination_id, 'destination', node_ids)
            if destination_id == source_id:
                raise BuildException("Destination node == source node in migration rates file (line %s): '%s'" % (line_number, line))

            return source_id, destination_id, rate

        def addMigrationLink(source_id, destination_id, rate, links):
            if source_id not in links:
                links[source_id] = {}

            if destination_id in links[source_id]:
                raise BuildException('Link from %s to %s defined multiple times.' % (source_id, destination_id))

            links[source_id][destination_id] = rate

        links = {}
        migration = open(filename)

        line_number = 1
        for line in migration:
            (source_id, destination_id, rate) = validateRateInformation(line, self.node_ids, line_number)
            addMigrationLink(source_id, destination_id, rate, links)
            line_number += 1

        migration.close()
        self.links = links
        return self.links

    def validate(self):
        clean = True
        outbound = {}
        # look for nodes with too many outbound links
        # also look for nodes with no outbound links
        # keep track of destination ids for subsequent inbound check
        for source_id in self.node_ids:
            if source_id in self.links:
                if len(self.links[source_id]) > self.link_count:
                    print('WARNING: node %s has more than %s outbound links defined' % (source_id, self.link_count), file=sys.stderr)
                    clean = False
                for destination_id in self.links[source_id]:
                    outbound[destination_id] = True
            else:
                print('WARNING: node %s is an island|sink (there are no outbound migration links)' % source_id, file=sys.stderr)
                clean = False
                self.links[source_id] = {}
        # warn on nodes with no inbound links
        for source_id in self.node_ids:
            if not source_id in outbound:
                print('WARNING: node %s has no inbound links' % source_id, file=sys.stderr)
                clean = False
        return clean

    def writeMigrationBinaryFile(self, binary_filename):
        binary = open(binary_filename, 'wb')
        
        for source_id in self.node_ids:
            print('Processing source node %s' % source_id)
            destination_ids   = []
            destination_rates = []
            
            for i in xrange(self.link_count):
                destination_ids.append(0)
                destination_rates.append(0)
                
            if source_id in self.links:
                destination_index = 0
                for destination_id in self.links[source_id]:
                    print('\tAdding destination %s with rate %s in slot %s' % (destination_id, self.links[source_id][destination_id], destination_index))
                    destination_ids[destination_index]   = destination_id
                    destination_rates[destination_index] = self.links[source_id][destination_id]
                    destination_index += 1
    
            print('\tDestination ids %s' % destination_ids)
            print('\tRates %s' % destination_rates)
                
            s_destinations = pack('L'*len(destination_ids), *destination_ids)
            s_rates        = pack('d'*len(destination_rates), *destination_rates)
            
            binary.write(s_destinations)
            binary.write(s_rates)
            
        binary.close()
    
    def writeMigrationHeaderFile(self, header_filename):
        def buildNodeOffsets(node_ids, link_count):
            offsets = []
            offset  = 0
            delta   = (link_count * 4) + (link_count * 8)   # 32-bit (uint) ids and 64-bit (double) rates
            for node_id in node_ids:
                offsets.append('%0.8X%0.8X' % (node_id, offset))
                offset += delta

            return ''.join(offsets)

        metadata = {
            'DateCreated': time.asctime(),
            'Tool': os.path.basename(sys.argv[0]),
            'Author': os.environ['USERNAME'],
            'IdReference': self.id_reference,
            'NodeCount': len(self.node_ids),
            'DatavalueCount': self.link_count
        }
        node_offsets = buildNodeOffsets(self.node_ids, self.link_count)
        header_json = {'Metadata': metadata, 'NodeOffsets': node_offsets}
        header = open(header_filename, 'w')
        json.dump(header_json, header, indent=4, separators=(',', ': '), sort_keys=True)
        header.close()
        return header_json


def main(demographics_filename, rates_filename, migration_type, binary_filename, header_filename):
    network = MigrationNetwork(canonical_types[migration_type])
    network.loadDemographicsFile(demographics_filename)
    network.loadMigrationRates(rates_filename)
    network.validate()
    network.writeMigrationBinaryFile(binary_filename)
    network.writeMigrationHeaderFile(header_filename)
    

def canonicalTypeForMigration(user_type):
    for key in canonical_types.keys():
        if key.startswith(user_type.upper()):
            return key

    raise BuildException("Unknown migration type '%s'" % user_type)


def displayArguments(arguments):
    print('Input demographics file (JSON):       %s' % arguments.demographics)
    print('Input migration rates file (text):    %s' % arguments.rates)
    print('Output migration rates file (binary): %s' % arguments.binary)
    print('Output migration file header (JSON):  %s' % arguments.header)
    print('Migration type:                       %s' % arguments.type)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--demographics', help='master demographics filename (JSON)',          type=str, required=True)
    parser.add_argument('-r', '--rates',        help='migration rates filename (text)',              type=str, required=True)
    parser.add_argument('-b', '--binary',       help='destination migration filename (binary)',      type=str, required=True)
    parser.add_argument('-t', '--type',         help='migration type [LOCAL|REGIONAL|SEA|AIR]',      type=str, required=True)
    parser.add_argument(      '--header',       help='destination migration header filename (JSON)', nargs='?', type=str)
    args = parser.parse_args()
    if not args.header:
        args.header = args.binary + '.json'
    args.type = canonicalTypeForMigration(args.type)
    displayArguments(args)
 
    main(args.demographics, args.rates, args.type, args.binary, args.header)
