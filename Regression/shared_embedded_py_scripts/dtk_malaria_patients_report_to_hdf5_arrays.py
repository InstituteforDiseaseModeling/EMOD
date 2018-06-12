'''
Parses MalariaPatientReport.json and saves it in hdf5 format.
The path to MalariaPatientReport.json must be passed as parameter. Usually it is the
DTK output folder which is defined with -O (e.g. DTK_PATH\Regression\Malaria\27_Malaria_OneIndividual_MultipleAntigens\testing )
'''
import json
import logging
import math
import h5py
import numpy as np
import pandas as pd

log = logging.getLogger(__name__)


def _parse(j, store, channels=[]):

    ntsteps = j['ntsteps']
    patients = j['patient_array']
    npatients = len(patients)
    log.info('Parsing channels %s for %d patients and %d time steps',
             channels, npatients, ntsteps)

    temp = np.empty((ntsteps, npatients), dtype='float')
    birthdays = [p['birthday'] for p in patients]

    for channel in channels:
        for i, p in enumerate(patients):
            pos_first_data = max(birthdays[i], 0)
            temp[pos_first_data:,i] = p[channel][0]
        store.create_dataset(str(channel), data=temp, compression="gzip",compression_opts=9)

def _parse_unicode(j, store, channels=[]):

    ntsteps = j['ntsteps']
    patients = j['patient_array']
    npatients = len(patients)
    log.info('Parsing channels %s for %d patients and %d time steps',
             channels, npatients, ntsteps)

    # dtype='S5' means a string with 5 characters
    temp = np.full((ntsteps, npatients), "empty", dtype='S5') #see http://docs.h5py.org/en/latest/strings.html
    birthdays = [p['birthday'] for p in patients]
    for channel in channels:
        for i, p in enumerate(patients):
            pos_first_data = max(birthdays[i], 0)
            data = [n.encode('latin-1') for n in p[channel][0]]
            temp[pos_first_data:,i] = data
        store.create_dataset(str(channel), data=temp, compression="gzip", compression_opts=9)

def application(application_output_path):
    #report_path = "C:\\Users\\tfischle\\Documents\\GitHub\\dtkTrunk_master\\Regression\\Malaria\\27_Malaria_OneIndividual_MultipleAntigens\\testing\\"
    report_path = application_output_path + "\\"
    print( "Reading from path: ", report_path )
    with open(report_path + 'MalariaPatientReport.json', 'r') as f:
        j = json.load(f)

    #print "All channels: ", [elem for elem in j['patient_array'][0]]

    channels_numeric = ['true_gametocytes', 'gametocyte_positive_fields', 'hemoglobin', 'infected_mosquito_fraction', 'true_asexual_parasites', 'temps', 'gametocytes', 'asexual_positive_fields', 'asexual_parasites']
    channels_string = ['treatment']
    h5f = h5py.File(report_path + 'MalariaPatientReport_hdf5_array.h5', 'w')

    h5f.create_dataset('ntsteps', data=j['ntsteps'])

    _parse(j, h5f, channels_numeric)
    _parse_unicode(j, h5f, channels_string)
    h5f.close()
    print( "hdf5 file written" )

