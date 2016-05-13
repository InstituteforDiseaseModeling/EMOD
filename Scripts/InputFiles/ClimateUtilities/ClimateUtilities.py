#!/usr/bin.python

# [X] dump node ids
# [X] validate node ids against demographics
# [X] check for node id
# [X] calculate MD5 hash
# [X] convert to ESRI for validation


from __future__ import print_function
from hashlib import md5
from sets import Set
import argparse
import json
import numpy as np
import os
import re
import sys

from EsriHeader import EsriHeader
import FormatException


_resolution = { '30arcsec': 30, '2.5arcmin': 150 }


def degrees_to_arcsec(value):
    return value * 3600


def arcsec_to_degrees(value):
    return value / 3600.0


def node_id_to_lat_long_arcsec(node_id, resolution):
    latitude_arcsec = (((node_id - 1) % 65536) * resolution) - degrees_to_arcsec(90) + (resolution / 2)
    longitude_arcsec = (((node_id - 1) >> 16) * resolution) - degrees_to_arcsec(180) + (resolution / 2)

    return latitude_arcsec, longitude_arcsec


def lat_long_arcsec_to_node_id(latitude_arcsec, longitude_arcsec, resolution):
    x_index = int((longitude_arcsec + degrees_to_arcsec(180)) / resolution)
    y_index = int((latitude_arcsec + degrees_to_arcsec(90)) / resolution)
    node_id = (x_index << 16) + y_index + 1

    return node_id

def convert_climate_file_to_bip(climate_file, json_header, offset_map):

    resolution = _resolution[json_header['Metadata']['Resolution']]
    lat_long = {}
    min_latitude_arcsec = degrees_to_arcsec(90)     # start at opposite limit
    max_latitude_arcsec = degrees_to_arcsec(-90)    # start at opposite limit
    min_longitude_arcsec = degrees_to_arcsec(180)   # start at opposite limit
    max_longitude_arcsec = degrees_to_arcsec(-180)  # start at opposite limit
    for node_id in offset_map:
        latitude_arcsec, longitude_arcsec = node_id_to_lat_long_arcsec(node_id, resolution)
        lat_long[node_id] = (offset_map[node_id], latitude_arcsec, longitude_arcsec)
        min_latitude_arcsec = min(min_latitude_arcsec, latitude_arcsec)
        max_latitude_arcsec = max(max_latitude_arcsec, latitude_arcsec)
        min_longitude_arcsec = min(min_longitude_arcsec, longitude_arcsec)
        max_longitude_arcsec = max(max_longitude_arcsec, longitude_arcsec)

    min_latitude_degrees = min_latitude_arcsec / 3600.0
    max_latitude_degrees = max_latitude_arcsec / 3600.0
    min_longitude_degrees = min_longitude_arcsec / 3600.0
    max_longitude_degrees = max_longitude_arcsec / 3600.0

    print('max_latitude  = {0}'.format(max_latitude_degrees), file=sys.stderr)
    print('min_longitude = {0}'.format(min_longitude_degrees), file=sys.stderr)
    print('min_latitude  = {0}'.format(min_latitude_degrees), file=sys.stderr)
    print('max_longitude = {0}'.format(max_longitude_degrees), file=sys.stderr)

    print('{0},{1},0'.format(min_longitude_degrees, max_latitude_degrees), file=sys.stderr)
    print('{0},{1},0'.format(max_longitude_degrees, max_latitude_degrees), file=sys.stderr)
    print('{0},{1},0'.format(max_longitude_degrees, min_latitude_degrees), file=sys.stderr)
    print('{0},{1},0'.format(min_longitude_degrees, min_latitude_degrees), file=sys.stderr)
    print('{0},{1},0'.format(min_longitude_degrees, max_latitude_degrees), file=sys.stderr)

    esri_header = EsriHeader()

    esri_header.number_of_rows = ((max_latitude_arcsec - min_latitude_arcsec) / resolution) + 1
    esri_header.number_of_columns = ((max_longitude_arcsec - min_longitude_arcsec) / resolution) + 1
    esri_header.number_of_bands = json_header['Metadata']['DatavalueCount']
    esri_header.number_of_bits = 32
    esri_header.upper_left_x = min_longitude_degrees
    esri_header.upper_left_y = max_latitude_degrees
    esri_header.x_dimension = arcsec_to_degrees(resolution)
    esri_header.y_dimension = arcsec_to_degrees(resolution)
    esri_header.total_row_bytes = 4 * esri_header.number_of_bands

    esri_header.print(climate_file + '.hdr')

    float_type = np.dtype([('value', np.float32, esri_header.number_of_bands)])
    climate_data = np.fromfile(climate_file, float_type)

    #integer_type = np.dtype([('value', np.int32, esri_header.number_of_bands)])
    #esri_data = np.empty([esri_header.number_of_rows, esri_header.number_of_columns], integer_type)
#    esri_data = np.empty([esri_header.number_of_rows, esri_header.number_of_columns, esri_header.number_of_bands], np.int32)
    esri_data = np.empty([esri_header.number_of_rows, esri_header.number_of_columns, esri_header.number_of_bands], 'f4')

    no_data = [ -40.0 for x in range(esri_header.number_of_bands) ]

    latitude_arcsec = max_latitude_arcsec
    for row in range(esri_header.number_of_rows):
        longitude_arcsec = min_longitude_arcsec
        for column in range(esri_header.number_of_columns):
            node_id = lat_long_arcsec_to_node_id(latitude_arcsec, longitude_arcsec, resolution)
            if node_id in offset_map:
                node_index = offset_map[node_id] / (esri_header.number_of_bands * 4)
                values = climate_data[node_index]
#                values[0] *= 10
#                esri_data[row, column] = values[0].astype(int)
                esri_data[row, column] = values
            else:
                esri_data[row, column] = no_data
            longitude_arcsec += resolution
            pass

        latitude_arcsec -= resolution
        pass

#    esri_data.tofile(climate_file + '.bip')
    esri_data.tofile(climate_file + '.flt')

    pass


def load_json(filename):
    data = {}
    with open(filename) as handle:
        data = json.load(handle)

    return data


def get_offset_map(node_count, offset_string):
    if len(offset_string) != (node_count * 16):
        raise FormatException, "Length of offset string ({0}) doesn't match number of nodes ({1} -> {2}).".format(len(offset_string), node_count, node_count * 8)

    offset_map = {}
    for index in range(node_count):
        char_offset = index * 16
        id_substring = offset_string[char_offset:char_offset+8]
        offset_substring = offset_string[char_offset+8:char_offset+16]
        node_id = int(id_substring, 16)
        offset = int(offset_substring, 16)
        offset_map[node_id] = offset

    return offset_map


def calculate_md5_hash(filename):
    hash = None
    with open(filename, 'rb') as handle:
        md5_calc = md5()
        while True:
            data = handle.read(10240)
            if len(data) == 0:
                break;
            md5_calc.update(data)

        hash = md5_calc.hexdigest()

    return hash


def extract_node_ids_from_demographics(demographics):
    node_ids = Set()
    nodes_array = demographics['Nodes']
    for entry in nodes_array:
        node_ids.add(entry['NodeID'])

    return node_ids


def compare_climate_and_demographic_nodes(climate_nodes, demographic_nodes):
    climate_only = climate_nodes - demographic_nodes
    climate_count = len(climate_only)
    demographic_only = demographic_nodes - climate_nodes
    demographic_count = len(demographic_only)

    return climate_count, demographic_count


def main(climate_file, bip, dump, find_node, calc_hash, demographics):
    print("Working with '{0}'.".format(climate_file), file=sys.stderr)
    header = load_json(climate_file + '.json')
    print("Node count:       {0}".format(header['Metadata']['NodeCount']), file=sys.stderr)
    print("Data value count: {0}".format(header['Metadata']['DatavalueCount']), file=sys.stderr)
    print("Resolution:       {0} ({1} arc seconds)".format(header['Metadata']['Resolution'], _resolution[header['Metadata']['Resolution']]), file=sys.stderr)
    offset_map = get_offset_map(header['Metadata']['NodeCount'], header['NodeOffsets'])

    if bip:
        convert_climate_file_to_bip(climate_file, header, offset_map)

    if dump:
        keys = offset_map.keys()
        keys.sort()
        for node_id in keys:
            print(node_id)

    if find_node:
        print('Finding node(s) {0}'.format(find_node))
        for node_ids in find_node:
            for node_id in node_ids.split(','):
                decimal_id = int(node_id) if not re.search('[a-fx]', node_id.lower()) else int(node_id, 16)
                if (decimal_id in offset_map):
                    print('Node id {0} ({1}) found in climate file.'.format(decimal_id, hex(decimal_id)))
                else:
                    print('Node id {0} ({1}) NOT found in climate file.'.format(decimal_id, hex(decimal_id)))

    if calc_hash:
        hash = calculate_md5_hash(climate_file)
        print('MD5 hash/checksum: {0}'.format(hash))

    if demographics:
        print("Validating against '{0}'".format(demographics), file=sys.stderr)
        demographics_data = load_json(demographics)
        demographic_nodes = extract_node_ids_from_demographics(demographics_data)
        climate_only_count, demographics_only_count = compare_climate_and_demographic_nodes(Set(offset_map.keys()), demographic_nodes)
        print('Count of nodes found only in climate file:      {0}'.format(climate_only_count), file=sys.stderr)
        print('Count of nodes found only in demographics file: {0}'.format(demographics_only_count), file=sys.stderr)

    pass


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('climate_file', help='Binary climate file name (not .json header)')
    parser.add_argument('-b', '--bip', default=False, action='store_true', help='Convert IDM DTK climate file to ESRI BIP (Band Interleaved by Pixel) format')
    parser.add_argument('-d', '--dump', default=False, action='store_true', help='Dump node ids to stdout')
    parser.add_argument('-f', '--find', action='append', metavar='node_id', help='Find node id(s) in climate file (--find id1[,id2[,id3]]) ids may be decimal or hex (prefix ambiguous hex values with 0x)')
    parser.add_argument('-m', '--md5', default=False, action='store_true', help='Calculate MD5 hash of binary data')
    parser.add_argument('-v', '--validate', metavar='demographics_file', help='Validate climate file node ids against given demographics file')
    args = parser.parse_args()
    if args.climate_file:
        main(args.climate_file, args.bip, args.dump, args.find, args.md5, args.validate)
    else:
        parser.print_usage(sys.stderr)
