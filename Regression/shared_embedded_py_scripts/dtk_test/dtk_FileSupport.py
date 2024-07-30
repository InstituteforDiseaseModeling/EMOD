#!/usr/bin/python

import lz4.block

try:
    import snappy
    SNAPPY_SUPPORT = True
except:
    SNAPPY_SUPPORT = False


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
        return lz4.block.compress(data if type(data) is bytes else data.encode())

    @classmethod
    def uncompress(cls, data):
        return lz4.block.decompress(data)


class Snappy(object):

    @classmethod
    def compress(cls, data):
        if SNAPPY_SUPPORT:
            return snappy.compress(data)
        else:
            raise UserWarning("Snappy [de]compression not available.")

    @classmethod
    def uncompress(cls, data):
        if SNAPPY_SUPPORT:
            return snappy.uncompress(data)
        else:
            raise UserWarning("Snappy [de]compression not available.")


class SerialObject(dict):
    # noinspection PyDefaultArgument
    def __init__(self, dictionary={}):
        super(SerialObject, self).__init__(dictionary)
        self.__dict__ = self
        return


class NullPtr(SerialObject):
    def __init__(self):
        nullptr = {'__class__': 'nullptr'}
        super(NullPtr, self).__init__(nullptr)

