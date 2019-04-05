#!/usr/bin/python

"""
Support for three formats of serialized population files:
1. "Original version": single payload chunk with simulation and all nodes, uncompressed or snappy or LZ4
2. "First chunked version": multiple payload chunks, one for simulation and one each for nodes
3. "Second chunked version": multiple payload chunks, simulation and node objects are "root" objects in each chunk
4. "Metadata update": compressed: true|false + engine: NONE|LZ4|SNAPPY replaced with compression: NONE|LZ4|SNAPPY
"""

from __future__ import print_function
import copy
import dtk_test.dtk_FileSupport as support
import json
import os
import snappy
import time

IDTK = 'IDTK'
NONE = 'NONE'
LZ4 = 'LZ4'
SNAPPY = 'SNAPPY'
MAX_VERSION = 4


__engines__ = {LZ4: support.EllZeeFour, SNAPPY: snappy, NONE: support.Uncompressed}


def uncompress(data, engine):
    if engine in __engines__:
        return __engines__[engine].uncompress(data)
    else:
        raise RuntimeError("Unknown compression scheme '{0}'".format(engine))


def compress(data, engine):
    if engine in __engines__:
        return __engines__[engine].compress(data)
    else:
        raise RuntimeError("Unknown compression scheme '{0}'".format(engine))


class DtkHeader(support.SerialObject):
    # noinspection PyDefaultArgument
    def __init__(self, dictionary={
            'author': 'unknown',
            'bytecount': 0,
            'chunkcount': 0,
            'chunksizes': [],
            'compressed': True,
            'date': time.strftime('%a %b %d %H:%M:%S %Y'),
            'engine': LZ4,
            'tool': os.path.basename(__file__),
            'version': 1}):
        super(DtkHeader, self).__init__(dictionary)
        return

    def __str__(self):
        text = json.dumps(self, separators=(',', ':'))
        return text

    def __len__(self):
        length = len(self.__str__())
        return length


class DtkFile(object):

    class Contents(object):
        def __init__(self, parent):
            self.__parent__ = parent
            return

        def __iter__(self):
            index = 0
            while index < len(self):
                yield self.__getitem__(index)
                index += 1

        def __getitem__(self, index):
            data = str(uncompress(self.__parent__.chunks[index], self.__parent__.compression), 'utf-8')
            return data

        def __setitem__(self, index, value):
            data = compress(value.encode(), self.__parent__.compression)
            self.__parent__.chunks[index] = data
            return

        def append(self, item):
            data = compress(item, self.__parent__.compression)
            self.__parent__.chunks.append(data)

        def __len__(self):
            length = len(self.__parent__.chunks)
            return length

    class Objects(object):
        def __init__(self, parent):
            self.__parent__ = parent
            return

        def __iter__(self):
            index = 0
            while index < len(self):
                yield self.__getitem__(index)
                index += 1

        def __getitem__(self, index):
            try:
                contents = self.__parent__.contents[index]
                item = json.loads(contents, object_hook=support.SerialObject)
            except:
                raise UserWarning("Could not parse JSON in chunk {0}".format(index))
            return item

        def __setitem__(self, index, value):
            contents = json.dumps(value, separators=(',', ':'))
            self.__parent__.contents[index] = contents
            return

        def append(self, item):
            contents = json.dumps(item, separators=(',', ':'))
            self.__parent__.contents.append(contents)
            return

        def __len__(self):
            length = len(self.__parent__.chunks)
            return length

    def __init__(self, header):
        self.__header__ = header
        self._chunks = [None for index in range(header.chunkcount)]
        self.contents = self.Contents(self)
        self.objects = self.Objects(self)
        return

    @property
    def header(self):
        return self.__header__

    # "Required" header entries
    @property
    def version(self):
        v = self.__header__.version
        return v

    @property
    def compressed(self):
        is_compressed = (self.__header__.engine.upper() != NONE)
        return is_compressed

    @property
    def compression(self):
        engine = self.__header__.engine.upper()
        return engine

    @compression.setter
    def compression(self, engine):
        self.__set_compression__(engine.upper())

    @property
    def byte_count(self):
        total = sum(self.chunk_sizes)
        return total

    @property
    def chunk_count(self):
        length = len(self.chunks)
        return length

    @property
    def chunk_sizes(self):
        sizes = [len(chunk) for chunk in self.chunks]
        return sizes

    # Optional header entries
    @property
    def author(self):
        return self.__header__.author if 'author' in self.__header__ else ''

    @author.setter
    def author(self, value):
        self.__header__['author'] = str(value)
        return

    @property
    def date(self):
        return self.__header__.date if 'date' in self.__header__ else ''

    @date.setter
    def date(self, value):
        self.__header__['date'] = str(value)

    @property
    def tool(self):
        return self.__header__.tool if 'tool' in self.__header__ else ''

    @tool.setter
    def tool(self, value):
        self.__header__['tool'] = str(value)
        return

    @property
    def version(self):
        return self.__header__.version

    @property
    def chunks(self):
        return self._chunks

    @property
    def nodes(self):
        return self._nodes

    def _sync_header(self):

        self.__header__.date = time.strftime('%a %b %d %H:%M:%S %Y')
        self.__header__.chunkcount = len(self.chunks)
        self.__header__.chunksizes = [len(chunk) for chunk in self.chunks]
        self.__header__.bytecount = sum(self.__header__.chunksizes)

        return

    def __set_compression__(self, engine):
        if engine != self.compression:
            for index in range(self.chunk_count):
                chunk = compress(self.contents[index], engine)
                self._chunks[index] = chunk
            self.__header__.engine = engine
            self.__header__['compressed'] = (engine != NONE)
        return


class DtkFileV1(DtkFile):

    def __init__(self, header=DtkHeader(), filename='', handle=None):
        header.version = 1
        super(DtkFileV1, self).__init__(header)
        if handle is not None:
            self.chunks[0] = handle.read(header.chunksizes[0])
            self._nodes = [entry.node for entry in self.simulation.nodes]
        return

    @property
    def simulation(self):
        return self.objects[0].simulation

    @simulation.setter
    def simulation(self, value):
        self.objects[0] = {'simulation': value}
        return


class DtkFileV2(DtkFile):

    class NodesV2(object):
        def __init__(self, parent):
            self.__parent__ = parent
            return

        def __iter__(self):
            index = 0
            while index < len(self):
                # Version 2 looks like this {'suid':{'id':id},'node':{...}}, dereference the node here for simplicity.
                yield self.__getitem__(index)
                index += 1

        def __getitem__(self, index):
            item = self.__parent__.objects[index+1]
            return item.node

        def __setitem__(self, index, value):
            # Version 2 actually saves the entry from simulation.nodes (C++) which is a map of suid to node.
            self.__parent__.objects[index+1] = {'suid': {'id': value.suid.id}, 'node': value}
            return

        def __len__(self):
            length = self.__parent__.chunk_count - 1
            return length

    def __init__(self, header=DtkHeader(), filename='', handle=None):
        header.version = 2
        super(DtkFileV2, self).__init__(header)
        for index, size in enumerate(header.chunksizes):
            self.chunks[index] = handle.read(size)
            if len(self.chunks[index]) != size:
                raise UserWarning(
                    "Only read {0} bytes of {1} for chunk {2} of file '{3}'".format(len(self.chunks[index]),
                                                                                    size, index, filename))
        # Version 2 looks like this: {'simulation':{...}} so we dereference the simulation here for simplicity.
        self._nodes = self.NodesV2(self)
        return

    @property
    def simulation(self):
        sim = self.objects[0]['simulation']
        del sim['nodes']
        return sim

    @simulation.setter
    def simulation(self, value):
        sim = copy.deepcopy(value)
        sim['nodes'] = []
        self.objects[0] = {'simulation': sim}
        return


class DtkFileV3(DtkFile):

    class NodesV3(object):
        def __init__(self, parent):
            self.__parent__ = parent
            return

        def __iter__(self):
            index = 0
            while index < len(self):
                yield self.__getitem__(index)
                index += 1

        def __getitem__(self, index):
            item = self.__parent__.objects[index+1]
            return item

        def __setitem__(self, index, value):
            self.__parent__.objects[index+1] = value
            return

        def __len__(self):
            length = self.__parent__.chunk_count - 1
            return length

    def __init__(self, header=DtkHeader(), filename='', handle=None):
        header.version = 3
        super(DtkFileV3, self).__init__(header)
        for index, size in enumerate(header.chunksizes):
            self.chunks[index] = handle.read(size)
            if len(self.chunks[index]) != size:
                raise UserWarning("Only read {0} bytes of {1} for chunk {2} of file '{3}'".format(len(self.chunks[index]), size, index, filename))
        self._nodes = self.NodesV3(self)
        return

    @property
    def simulation(self):
        sim = self.objects[0]
        del sim['nodes']
        return sim

    @simulation.setter
    def simulation(self, value):
        sim = copy.deepcopy(value)
        sim['nodes'] = []
        self.objects[0] = sim
        return


class DtkFileV4(DtkFileV3):

    def __init__(self, header=DtkHeader(), filename='', handle=None):
        super(DtkFileV4, self).__init__(header, filename, handle)
        header.version = 4
        return


def read(filename):

    new_file = None
    with open(filename, 'rb') as handle:
        __check_magic_number__(handle)
        header = __read_header__(handle)
        if header.version == 1:
            new_file = DtkFileV1(header, filename=filename, handle=handle)
        elif header.version == 2:
            new_file = DtkFileV2(header, filename=filename, handle=handle)
        elif header.version == 3:
            new_file = DtkFileV3(header, filename=filename, handle=handle)
        elif header.version == 4:
            new_file = DtkFileV4(header, filename=filename, handle=handle)
        else:
            raise UserWarning('Unknown serialized population file version: {0}'.format(header.version))

    return new_file


def __check_magic_number__(handle):
    magic = handle.read(4).decode()
    if magic != IDTK:
        raise UserWarning("File has incorrect magic 'number': '{0}'".format(magic))
    return


def __read_header__(handle):

    size_string = handle.read(12)
    header_size = int(size_string)
    __check_header_size__(header_size)
    header_text = handle.read(header_size)
    header = __try_parse_header_text__(header_text)

    if 'metadata' in header:
        header = header.metadata

    if 'version' not in header:
        header.version = 1

    if header.version < 2:
        header.engine = SNAPPY if header.compressed else NONE
        header.chunkcount = 1
        header.chunksizes = [header.bytecount]
    __check_version__(header.version)
    if header.version < 4:
        header.engine = header.engine.upper()
    else:
        header['engine'] = header.compression.upper()
    __check_chunk_sizes__(header.chunksizes)

    return header


def __check_header_size__(header_size):
    if header_size <= 0:
        raise UserWarning("Invalid header size: {0}".format(header_size))
    return


def __try_parse_header_text__(header_text):
    try:
        header = json.loads(header_text, object_hook=DtkHeader)
    except ValueError as err:
        raise UserWarning("Couldn't decode JSON header '{0}'".format(err))
    return header


def __check_version__(version):
    if version <= 0 or version > MAX_VERSION:
        raise UserWarning("Unknown version: {0}".format(version))
    return


def __check_chunk_sizes__(chunk_sizes):
    for size in chunk_sizes:
        if size <= 0:
            raise UserWarning("Invalid chunk size: {0}".format(size))
    return


def write(dtk_file, filename):

    # noinspection PyProtectedMember
    dtk_file._sync_header()

    with open(filename, 'wb') as handle:
        __write_magic_number__(handle)
        if dtk_file.version <= 3:
            header = json.dumps({'metadata': dtk_file.header}, separators=(',', ':'))
        else:
            header = json.dumps(dtk_file.header, separators=(',', ':')).replace('"engine"', '"compression"')

        __write_header_size__(len(header), handle)
        __write_header__(header, handle)
        __write_chunks__(dtk_file.chunks, handle)

    return


def __write_magic_number__(handle):
    handle.write('IDTK'.encode())
    return


def __write_header_size__(size, handle):
    size_string = '{:>12}'.format(size)     # decimal value right aligned in 12 character space
    handle.write(size_string.encode())
    return


def __write_header__(string, handle):
    handle.write(string.encode())
    return


def __write_chunks__(chunks, handle):
    for chunk in chunks:
        handle.write(chunk)
    return
