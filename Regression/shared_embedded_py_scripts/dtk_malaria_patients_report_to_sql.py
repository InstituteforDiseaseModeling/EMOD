'''
Parses MalariaPatientReport.json and saves it in sqlite format.
The path to MalariaPatientReport.json must be passed as parameter. Usually it is the
DTK output folder which is defined with -O (e.g. DTK_PATH\Regression\Malaria\27_Malaria_OneIndividual_MultipleAntigens\testing )
'''

import json
import logging
import math
import sqlite3

import numpy as np
import pandas as pd


log = logging.getLogger(__name__)


def _parse(j, _dtype, channels=[]):

    ntsteps = j['ntsteps']
    patients = j['patient_array']
    npatients = len(patients)
    log.info('Parsing channels %s for %d patients and %d time steps',
             channels, npatients, ntsteps)

    ids = ["id_" + str(p['id']) for p in patients]
    birthdays = pd.Series(index=ids, data=[p['birthday'] for p in patients])

    channel_df_dict = {}
    for channel in channels:
        channel_df = pd.DataFrame(index=range(ntsteps), columns=ids, dtype=_dtype)
        channel_df.index.name = 'time'

        for i, p in enumerate(patients):
            first_data = max(birthdays.iloc[i], 0)
            try:
                channel_df.iloc[first_data:, i] = p[channel][0]
            except:
                 print( "not adding element: ", channel )

        channel_df_dict[channel] = channel_df

    # channel (items) x time (major) x patient (minor)
    return pd.Panel(data=channel_df_dict)

def _parse_unicode(j, _dtype, channels=[]):

    ntsteps = j['ntsteps']
    patients = j['patient_array']
    npatients = len(patients)
    log.info('Parsing channels %s for %d patients and %d time steps',
             channels, npatients, ntsteps)

    ids = ["id_" + str(p['id']) for p in patients]
    birthdays = pd.Series(index=ids, data=[p['birthday'] for p in patients])

    channel_df_dict = {}
    for channel in channels:
        channel_df = pd.DataFrame(index=range(ntsteps), columns=ids, dtype=_dtype)
        channel_df.index.name = 'time'

        for i, p in enumerate(patients):
            first_data = max(birthdays.iloc[i], 0)
            try:
                channel_df.iloc[first_data:, i] = len(p[channel][0])*["empty"]
            except:
                 print( "not adding element: ", channel )

        channel_df_dict[channel] = channel_df

    # channel (items) x time (major) x patient (minor)
    return pd.Panel(data=channel_df_dict)


def application(application_output_path):
    #report_path = "C:\\Users\\tfischle\\Documents\\GitHub\\dtkTrunk_master\\Regression\\Malaria\\27_Malaria_OneIndividual_MultipleAntigens\\testing\\"
    report_path = application_output_path + "\\"
    with open(report_path + 'MalariaPatientReport.json', 'r') as f:
        j = json.load(f)

    channels_numeric = ['true_gametocytes', 'gametocyte_positive_fields', 'hemoglobin', 'infected_mosquito_fraction',
                        'true_asexual_parasites', 'temps', 'gametocytes', 'asexual_positive_fields',
                        'asexual_parasites']
    channels_string = ['treatment']
    patient_panel = _parse(j, 'float', channels_numeric)
    patient_panel_str = _parse_unicode(j, 'unicode', channels_string)

    db = sqlite3.connect(report_path + 'MalariaPatientReport_sql.db')

    for c in channels_numeric:
        print( "hdf5 store:", c )
        patient_panel[c].to_sql(c, db, if_exists='append')

    for c in channels_string:
        print( "hdf5 store str:", c )
        patient_panel_str[c].to_sql(c, db, if_exists='append')

    db.commit()
    print( "sql file written" )
