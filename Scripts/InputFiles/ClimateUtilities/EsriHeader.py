from __future__ import print_function

class EsriHeader(object):
    """ESRI BIP file header"""
    def __init__(self):
        self.number_of_rows = None      # number of rows
        self.number_of_columns = None   # number of columns
        self.number_of_bands = 1        # number of bands
        self.number_of_bits = 8         # bits per pixel band value (not total for pixel)
        self.pixel_type = 'FLOAT'       # floating point (cf 'SIGNEDINT')
        self.byte_order = 'I'           # 'I' - Intel/big-endian, 'M' - Motorola/little-endian
        self.layout = 'BIP'             # 'BIP' - Band Interleaved by Pixel
        self.skip_bytes = 0             # Bytes to skip to reach pixel data, allows embedded header
        self.upper_left_x = 0           # X-axis map coordinate of the center of the upper left pixel
        self.upper_left_y = 0           # Y-axis map coordinate of the center of the upper left pixel
        self.x_dimension = 1            # X-dimension of a pixel in map units
        self.y_dimension = 1            # Y-dimension of a pixel in map units
        self.band_row_bytes = 0         # Bytes per band per row. Only with BIL files.
        self.total_row_bytes = 0        # = (ncols x nbands x nbits) / 8 rounded up
        self.band_gap_bytes = 0         # Bytes between bands in a BSQ format image
        self.no_data = -40.0            # transparent/no-data value
        pass

    @property
    def number_of_rows(self):
        return self._number_of_rows

    @number_of_rows.setter
    def number_of_rows(self, value):
        self._number_of_rows = value
        pass

    @property
    def number_of_columns(self):
        return self._number_of_columns

    @number_of_columns.setter
    def number_of_columns(self, value):
        self._number_of_columns = value
        pass

    @property
    def number_of_bands(self):
        return self._number_of_bands

    @number_of_bands.setter
    def number_of_bands(self, value):
        self._number_of_bands = value
        pass

    @property
    def number_of_bits(self):
        return self._number_of_bits

    @number_of_bits.setter
    def number_of_bits(self, value):
        self._number_of_bits = value
        pass

    @property
    def pixel_type(self):
        return self._pixel_type

    @pixel_type.setter
    def pixel_type(self, value):
        self._pixel_type = value
        pass

    @property
    def upper_left_x(self):
        return self._upper_left_x

    @upper_left_x.setter
    def upper_left_x(self, value):
        self._upper_left_x = value
        pass

    @property
    def upper_left_y(self):
        return self._upper_left_y

    @upper_left_y.setter
    def upper_left_y(self, value):
        self._upper_left_y = value
        pass

    @property
    def x_dimension(self):
        return self._x_dimension

    @x_dimension.setter
    def x_dimension(self, value):
        self._x_dimension = value
        pass

    @property
    def y_dimension(self):
        return self._y_dimension

    @y_dimension.setter
    def y_dimension(self, value):
        self._y_dimension = value
        pass

    @property
    def no_data(self):
        return self._no_data

    @no_data.setter
    def no_data(self, value):
        self._no_data = value
        pass

    def print(self, filename):
        with open(filename, 'w') as handle:
            print('NROWS         {0}'.format(self.number_of_rows), file=handle)
            print('NCOLS         {0}'.format(self.number_of_columns), file=handle)
            print('NBANDS        {0}'.format(self.number_of_bands), file=handle)
            print('NBITS         {0}'.format(self.number_of_bits), file=handle)
            print('PIXELTYPE     {0}'.format(self.pixel_type), file=handle)
            print('BYTEORDER     {0}'.format(self.byte_order), file=handle)
            print('LAYOUT        {0}'.format(self.layout), file=handle)
            print('SKIPBYTES     {0}'.format(self.skip_bytes), file=handle)
            print('ULXMAP        {0}'.format(self.upper_left_x), file=handle)
            print('ULYMAP        {0}'.format(self.upper_left_y), file=handle)
            print('XDIM          {0}'.format(self.x_dimension), file=handle)
            print('YDIM          {0}'.format(self.y_dimension), file=handle)
            print('BANDROWBYTES  {0}'.format(self.band_row_bytes), file=handle)
            print('TOTALROWBYTES {0}'.format(self.total_row_bytes), file=handle)
            print('BANDGAPBYTES  {0}'.format(self.band_gap_bytes), file=handle)
            print('NODATA        {0}'.format(self.no_data), file=handle)
