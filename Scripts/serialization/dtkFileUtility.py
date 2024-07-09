#!/usr/bin/python

from __future__ import print_function
import argparse
import dtkFileTools as dft
import json
import os
import sys


def __do_read__(args):

    if args.output is not None:
        prefix = args.output
    else:
        root, _ = os.path.splitext(args.filename)
        prefix = root

    if args.raw:
        extension = 'bin'
    else:
        extension = 'json'

    dtk_file = dft.read(args.filename)

    if args.header:
        with open(args.header, 'w') as handle:
            json.dump(dtk_file.header, handle, indent=2, separators=(',', ':'))

    print('File header: {0}'.format(dtk_file.header))

    for index in range(len(dtk_file.chunks)):
        if args.raw:
            # Write raw chunks to disk
            output = dtk_file.chunks[index]
        else:
            if args.unformatted:
                # Expand compressed contents, but don't serialize and format
                output = dtk_file.contents[index]
            else:
                # Expand compressed contents, serialize, write out formatted
                obj = dtk_file.objects[index]
                output = json.dumps(obj, indent=2, separators=(',', ':'))

        if index == 0:
            output_filename = '.'.join([prefix, 'simulation', extension])
        else:
            output_filename = '.'.join([prefix, 'node-{:0>5}'.format(index), extension])

        if args.raw:
            with open(output_filename, 'wb') as handle:
                handle.write(output)
        else:
            with open(output_filename, 'wt', encoding='utf-8') as handle:
                handle.write(output)

    return


def __do_write__(args):

    print("Writing file '{0}'".format(args.filename), file=sys.stderr)
    print("Reading simulation data from '{0}'".format(args.simulation), file=sys.stderr)
    print("Reading node data from {0}".format(args.nodes), file=sys.stderr)
    print("Author = {0}".format(args.author), file=sys.stderr)
    print("Tool = {0}".format(args.tool), file=sys.stderr)
    print("{0} contents".format("Compressing" if args.compress else "Not compressing"), file=sys.stderr)
    print("{0} contents".format("Verifying" if args.verify else "Not verifying"), file=sys.stderr)
    print("Using compression engine '{0}'".format(args.engine), file=sys.stderr)

    dtk_file = dft.DtkFileV3()
    dtk_file.author = args.author
    dtk_file.tool = args.tool
    dtk_file.compression = args.engine

    _prepare_simulation_data(args.simulation, dtk_file)
    _prepare_node_data(args.nodes, dtk_file)

    dft.write(dtk_file, args.filename)

    return


def _prepare_simulation_data(filename, dtk_file):

    with open(filename, 'rb') as handle:
        data = handle.read()
        # Do not use dtk_file.simulation property because this is text rather than a Python object
        dtk_file.contents.append(data)

    return


# noinspection SpellCheckingInspection
def _prepare_node_data(filenames, dtk_file):

    for filename in filenames:
        with open(filename, 'rb') as handle:
            data = handle.read()
            # Do not use dtk_file.nodes here because this is text rather than a Python object
            dtk_file.contents.append(data)

    return

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(help='add_subparsers help')

    read_parser = subparsers.add_parser('read', help='read help')
    read_parser.add_argument('filename')
    read_parser.add_argument('--header', default=None, help='Write header to file', metavar='<filename>')
    read_parser.add_argument('-r', '--raw', default=False, action='store_true',
                             help='Write raw contents of chunks to disk')
    # noinspection SpellCheckingInspection
    read_parser.add_argument('-u', '--unformatted', default=False, action='store_true',
                             help='Write unformatted (compact) JSON to disk')
    read_parser.add_argument('-o', '--output', default=None,
                             help='Output filename prefix, defaults to input filename with .json extension')
    read_parser.set_defaults(func=__do_read__)

    username = os.environ['USERNAME'] if 'USERNAME' in os.environ else os.environ['USER']
    tool_name = os.path.basename(__file__)

    write_parser = subparsers.add_parser('write', help='write help')
    write_parser.add_argument('filename', help='Output .dtk filename')
    write_parser.add_argument('simulation', help='Filename for simulation JSON')
    write_parser.add_argument('nodes', nargs='+', help='Filename(s) for node JSON')
    write_parser.add_argument('-a', '--author', default=username, help='Author name for header [{0}]'.format(username))
    write_parser.add_argument('-t', '--tool', default=tool_name, help='Tool name for header [{0}]'.format(tool_name))
    write_parser.add_argument('-u', '--uncompressed', default=True, action='store_false', dest='compress',
                              help='Do not compress contents of new .dtk file')
    write_parser.add_argument('-v', '--verify', default=False, action='store_true',
                              help='Verify JSON in simulation and nodes (could be slow).')
    write_parser.add_argument('-e', '--engine', default='LZ4', help='Compression engine {NONE|LZ4|SNAPPY} [LZ4]')
    write_parser.set_defaults(func=__do_write__)

    commandline_args = parser.parse_args()
    commandline_args.func(commandline_args)
