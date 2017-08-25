#!/usr/bin/python

import lz4


# noinspection PyCamelCase
class Uncompressed(object):

    @classmethod
    def compress(cls, data):
        return data

    @classmethod
    def uncompress(cls, data):
        return data


class EllZeeFour(object):

    @classmethod
    def compress(cls, data):
        return lz4.block.compress(data)

    @classmethod
    def uncompress(cls, data):
        return lz4.block.decompress(data)


class SerialObject(dict):
    # noinspection PyDefaultArgument
    def __init__(self, dictionary={}):
        super(SerialObject, self).__init__(dictionary)
        self.__dict__ = self
        return
