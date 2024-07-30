#!/usr/bin/python

import struct

class Spatial_Report_Channels:
    Population = "Population"
    Births = "Births"
    Campaign_Cost = "Campaign_Cost"


class SpatialReportData(object):
    """
    Read a spatial report file into memory. Node ids will be a list in node_ids, node_data will be a multi-level
    dictionary of node ids -> { timesteps -> channel data } suitable for initializing a pandas dataframe from.

    Data is read on object initialization.
    """
    def __init__(self, report_file, channel='', is_filtered=False, debug=False):
        self.report_file = report_file
        # potentially we could automatically extract the channel name from the file name?
        self.channel = channel
        self.is_filtered = is_filtered
        self.debug = debug

        self.fh = None
        self.node_count = 0
        self.timesteps = 0

        self.start_time = 0.0
        self.interval_time = 1.0

        self.node_ids = []
        self.node_data = {}

        self.read_report()

    def debug_log(self, msg):
        if self.debug:
            print(msg)

    def read_report(self):
        """
        Open binary report file, read and translate data, add to node_data.
        """
        self.debug_log(f'Reading {self.report_file} ({self.channel} channel)')
        with open(self.report_file, 'rb') as fh:
            self.fh = fh
            self.node_count = self.read_int()
            self.debug_log(f'{self.node_count} nodes')

            self.timesteps = self.read_int()
            self.debug_log(f'{self.timesteps} timesteps')

            # filtered reports contain extra fields in the header
            if (self.is_filtered):
                self.start_time = self.read_float()
                self.interval_time = self.read_float()

            for n in range(self.node_count):
                node_id = self.read_int()
                self.debug_log(f'Adding node {node_id}')
                self.node_ids.append(node_id)
                self.node_data[node_id] = {}

            for ts in range(self.timesteps):
                for node_id in self.node_ids:
                    data = self.read_float()
                    self.node_data[node_id][ts] = data
                    self.debug_log(f'Adding {self.channel} data: {data} for node {node_id} at time {ts}')
        self.fh = None

    def read_int(self):
        """
        Read an integer from the fh file stream

        :return: integer data
        """
        int_data, = struct.unpack("i", self.fh.read(4))
        return int_data

    def read_float(self):
        """
        Read a float from the fh file stream

        :return: float data
        """
        float_data, = struct.unpack("f", self.fh.read(4))
        return float_data
