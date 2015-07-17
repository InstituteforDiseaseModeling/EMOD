import os           # mkdir, chdir, path, etc.
import json         # to read JSON output files
import numpy as np  # for reading spatial output data by node and timestep
import struct       # for binary file unpacking
import threading    # for multi-threaded job submission and monitoring
import csv

# A class to parse output files
class DTKOutputParser(threading.Thread):

    def __init__(self, sim_dir, sim_id, sim_data, analyzers):
        threading.Thread.__init__(self)
        self.sim_dir = sim_dir
        self.sim_id = sim_id
        self.sim_data = sim_data
        self.analyzers = analyzers
        self.output_data = {}
        self.emit_data = {}

    def run(self):

        # list of output files needed by any analysis
        filenames = set()
        for a in self.analyzers:
            filenames.update(a.filenames)
        filenames = list(filenames)

        # parse output files for analysis
        for filename in filenames:
            file_extension = os.path.splitext(filename)[1][1:]
            if file_extension == 'json':
                #print(filename + ' is a JSON file.  Loading JSON output data...\n')
                self.load_json_file(filename)
            elif file_extension == 'bin' and 'SpatialReport' in filename:
                #print(filename + ' is a binary spatial output file.  Loading BIN output data...\n')
                self.load_bin_file(filename)
            elif file_extension == 'csv':
                #print(filename + ' is a CSV output file.  Loading CSV data...\n')
                self.load_csv_file(filename)
            else:
                print(filename + ' is of an unknown type.  Skipping...')
                continue

        # do sim-specific part of analysis on parsed output data
        for analyzer in self.analyzers:
            # What if two analyzers use the same file and do different things to it?
            self.emit_data.update(analyzer.map(self.output_data))

    def load_csv_file(self, filename):
        with open( filename, 'rb') as f:
            reader = csv.reader(f)

            # For headers, take everything up to the first space
            headers = [s.split(' ')[0] for s in reader.next()]

            colMap = {}
            for (i,h) in enumerate(headers):
                colMap[h] = i

            rows = []
            for row in reader:
                rows.append(row)
            self.output_data[filename] = { 'colMap': colMap,
                                            'data': rows }


    def load_json_file(self, filename):
        with open(os.path.join(self.get_sim_dir(), filename)) as json_file:
            self.output_data[filename] = json.loads(json_file.read())

    def load_bin_file(self, filename):
        with open(os.path.join(self.get_sim_dir(), filename), 'rb') as bin_file:
            data = bin_file.read(8)
            n_nodes, = struct.unpack( 'i', data[0:4] )
            n_tstep, = struct.unpack( 'i', data[4:8] )
            #print( "There are %d nodes and %d time steps" % (n_nodes, n_tstep) )

            nodeids_dtype = np.dtype( [ ( 'ids', '<i4', (1, n_nodes ) ) ] )
            nodeids = np.fromfile( bin_file, dtype=nodeids_dtype, count=1 )
            nodeids = nodeids['ids'][:,:,:].ravel()
            #print( "node IDs: " + str(nodeids) )

            channel_dtype = np.dtype( [ ( 'data', '<f4', (1, n_nodes ) ) ] )
            channel_data = np.fromfile( bin_file, dtype=channel_dtype )
            channel_data = channel_data['data'].reshape(n_tstep, n_nodes)

        self.output_data[filename] = {'n_nodes': n_nodes,
                                      'n_tstep': n_tstep,
                                      'nodeids': nodeids,
                                      'data': channel_data}

    def get_sim_dir(self):
        return os.path.join(self.sim_dir, self.sim_id)

class CompsDTKOutputParser(DTKOutputParser):

    def __init__(self, sim_dir_map, sim_dir, sim_id, sim_data, analyzers):
        DTKOutputParser.__init__(self, sim_dir, sim_id, sim_data, analyzers)
        self.sim_dir_map = sim_dir_map

    @staticmethod
    def createSimDirectoryMap(exp_id):
        from COMPSJavaInterop import Experiment, QueryCriteria

        e = Experiment.GetById(exp_id)
        sims = e.GetSimulations(QueryCriteria().Select('Id').SelectChildren('HPCJobs')).toArray()
        sim_map = { sim.getId().toString() : sim.getHPCJobs().toArray()[-1].getWorkingDirectory() for sim in sims }
        print(sim_map)
        return sim_map

    def get_sim_dir(self):
        return self.sim_dir_map[self.sim_id]
