#!/usr/bin/python

from __future__ import print_function
import argparse
import dtkFileTools as dft
import random


def main(arguments):

    if arguments.make_bad_chunk_lz4:
        make_bad_chunk_lz4()

    if arguments.make_bad_chunk_snappy:
        make_bad_chunk_snappy()

    if arguments.make_bad_sim_lz4:
        make_bad_sim_lz4()

    if arguments.make_bad_sim_snappy:
        make_bad_sim_snappy()

    if arguments.make_lz4_none:
        make_lz4_none()

    if arguments.make_lz4_snappy:
        make_lz4_snappy()

    if arguments.make_none_lz4:
        make_none_lz4()

    if arguments.make_none_snappy:
        make_none_snappy()

    if arguments.make_snappy_lz4:
        make_snappy_lz4()

    if arguments.make_snappy_none:
        make_snappy_none()

    if arguments.make_uncompressed:
        make_uncompressed()

    if arguments.make_snappy:
        make_snappy()

    return


def make_bad_chunk_lz4():

    dtk = dft.read('test-data/baseline.dtk')
    # get the chunk, first node, not simulation
    chunk = dtk.chunks[1]
    # choose an index
    index = random.randint(0, len(chunk))

    # convert string to array of chars
    chunk = [c for c in chunk]
    # perturb the bits
    old = chunk[index]
    new = chr(~ord(old) % 256)
    chunk[index] = new
    # convert array of chars back to string
    chunk = ''.join(chunk)

    dtk.chunks[1] = chunk

    print('Flipped bits of chunk #1 byte {0} ({1} -> {2})'.format(index, ord(old), ord(new)))

    dft.write(dtk, 'test-data/bad-chunk-lz4.dtk')

    return


def make_bad_chunk_snappy():

    dtk = dft.read('test-data/snappy.dtk')
    # get the chunk, first node, not simulation
    chunk = dtk.chunks[1]
    # choose an index
    index = random.randint(0, len(chunk))

    # convert string to array of chars
    chunk = [c for c in chunk]
    # perturb the bits
    old = chunk[index]
    new = chr(~ord(old) % 256)
    chunk[index] = new
    # convert array of chars back to string
    chunk = ''.join(chunk)

    dtk.chunks[1] = chunk

    print('Flipped bits of chunk #1 byte {0} ({1} -> {2})'.format(index, ord(old), ord(new)))

    dft.write(dtk, 'test-data/bad-chunk-snappy.dtk')

    return


def make_bad_sim_lz4():

    dtk = dft.read('test-data/baseline.dtk')
    sim_text = dtk.contents[0]
    sim_text = sim_text.replace('"__class__":"Simulation"', '"__class__"*"Simulation"', 1)
    dtk.contents[0] = sim_text
    dft.write(dtk, 'test-data/bad-sim-lz4.dtk')

    return


def make_bad_sim_snappy():

    dtk = dft.read('test-data/snappy.dtk')
    sim_text = dtk.contents[0]
    sim_text = sim_text.replace('"__class__":"Simulation"', '"__class__"*"Simulation"', 1)
    dtk.contents[0] = sim_text
    dft.write(dtk, 'test-data/bad-sim-snappy.dtk')

    return


def make_lz4_none():

    dtk = dft.read('test-data/uncompressed.dtk')
    dtk.header['compressed'] = True
    dtk.header['engine'] = dft.LZ4
    dft.write(dtk, 'test-data/lz4-none.dtk')

    return


def make_lz4_snappy():

    dtk = dft.read('test-data/snappy.dtk')
    dtk.header['compressed'] = True
    dtk.header['engine'] = dft.LZ4
    dft.write(dtk, 'test-data/lz4-snappy.dtk')

    return


def make_none_lz4():

    dtk = dft.read('test-data/baseline.dtk')
    dtk.header['compressed'] = False
    dtk.header['engine'] = dft.NONE
    dft.write(dtk, 'test-data/none-lz4.dtk')

    return


def make_none_snappy():

    dtk = dft.read('test-data/snappy.dtk')
    dtk.header['compressed'] = False
    dtk.header['engine'] = dft.NONE
    dft.write(dtk, 'test-data/none-snappy.dtk')

    return


def make_snappy_lz4():

    dtk = dft.read('test-data/baseline.dtk')
    dtk.header['compressed'] = True
    dtk.header['engine'] = dft.SNAPPY
    dft.write(dtk, 'test-data/snappy-lz4.dtk')

    return


def make_snappy_none():

    dtk = dft.read('test-data/uncompressed.dtk')
    dtk.header['compressed'] = True
    dtk.header['engine'] = dft.SNAPPY
    dft.write(dtk, 'test-data/snappy-none.dtk')

    return


def make_uncompressed():

    dtk = dft.read('test-data/baseline.dtk')
    dtk.compression = dft.NONE
    dft.write(dtk, 'test-data/uncompressed.dtk')

    return


def make_snappy():

    dtk = dft.read('test-data/baseline.dtk')
    dtk.compression = dft.SNAPPY
    dft.write(dtk, 'test-data/snappy.dtk')

    return


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--make-bad-chunk-lz4', default=False, action='store_true')
    parser.add_argument('--make-bad-chunk-snappy', default=False, action='store_true')
    parser.add_argument('--make-bad-sim-lz4', default=False, action='store_true')
    parser.add_argument('--make-bad-sim-snappy', default=False, action='store_true')
    parser.add_argument('--make-lz4-none', default=False, action='store_true')
    parser.add_argument('--make-lz4-snappy', default=False, action='store_true')
    parser.add_argument('--make-none-lz4', default=False, action='store_true')
    parser.add_argument('--make-none-snappy', default=False, action='store_true')
    parser.add_argument('--make-snappy-lz4', default=False, action='store_true')
    parser.add_argument('--make-snappy-none', default=False, action='store_true')
    parser.add_argument('--make-uncompressed', default=False, action='store_true')
    parser.add_argument('--make-snappy', default=False, action='store_true')

    args = parser.parse_args()

    main(args)
