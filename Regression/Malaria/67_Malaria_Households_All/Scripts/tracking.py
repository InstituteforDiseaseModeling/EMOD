import sys, os, json, collections, operator
from human_tracking_common import *


MigrationDemographicsName = [ "Adult_Male", "Adult_Female", "Child_Male", "Child_Female" ]


class MigrationStats:
    def __init__(self):
        self.max_time  = -1
        self.avg_time  = 0
        self.num_trips = 0
        self.counter   = 0

    def Update( self, delta_time ):
        if( delta_time > 0.0 ):
            self.num_trips += 1
            self.avg_time  += delta_time
            if( self.max_time < delta_time ):
                self.max_time = delta_time

    def UpdateAverages( self ):
        if( self.num_trips == 0 ):
            self.avg_time = 0
        else:
            self.avg_time = self.avg_time / self.num_trips
    
    def UpdateWithStats( self, ind_mig_stats ):
        if( ind_mig_stats.max_time > 0 ):
            self.num_trips += ind_mig_stats.num_trips
            self.avg_time += ind_mig_stats.avg_time
            self.counter += 1
            if( self.max_time < ind_mig_stats.max_time ):
                self.max_time = ind_mig_stats.max_time

class IndividualStats:
    def __init__(self):
        self.type_stats = []
        self.age_years = -1
        self.gender    = ""
        self.is_adult  = False

def GetMigrationDemographicsIndex( gender, is_adult ):
    index = 0
    if( (gender == "M") and is_adult ):
        index = 0
    elif( (gender == "F") and is_adult ):
        index = 1
    elif( (gender == "M") and not is_adult ):
        index = 2
    elif( (gender == "F") and not is_adult ):
        index = 3
    return index

def GetStats( input_data_list, total_stats, demog_stats ):
    
    home_node = input_data_list[0].from_node

    ind_stats = IndividualStats()
    for mig_type in MigrationTypeKeys:
        ind_stats.type_stats.append( MigrationStats() )

    prev_time = 0

    for data in input_data_list:
        delta_time = data.time - prev_time

        if( data.age_years > 0 ):
            ind_stats.age_years = data.age_years
            ind_stats.gender    = data.gender
            ind_stats.is_adult  = data.is_adult

        mig_type = 0
        if data.from_node == home_node:
            mig_type = MigrationType["home"]
        else:
            mig_type = data.mig_type

        ind_stats.type_stats[ mig_type ].Update( delta_time )

        total_stats.type_stats[ mig_type ].UpdateWithStats( ind_stats.type_stats[ mig_type ] )

        demog_index = GetMigrationDemographicsIndex( data.gender, data.is_adult )
        demog_stats[ demog_index ].type_stats[ mig_type ].UpdateWithStats( ind_stats.type_stats[ mig_type ] )

        prev_time = data.time

    for ts in ind_stats.type_stats:
        ts.UpdateAverages()

    return ind_stats

def WriteStats( human_id_list, stats_map, total_stats, demog_stats ):

    output_fn = "output.csv"

    output_file = open( output_fn, "w" )

    header = "HumanId,Gender,IsAdult"
    for mig_type in MigrationTypeKeys:
        header += "," + mig_type + "_MaxTime"
        header += "," + mig_type + "_AvgTime"
        header += "," + mig_type + "_NumTrips"
    header += "\n"
    output_file.write( header )

    for human_id in human_id_list:
        ind_stats = stats_map[ human_id ]

        line  = str(human_id)
        line += "," + ind_stats.gender
        line += "," + str(ind_stats.is_adult)

        for mig_type in MigrationTypeKeys:
            ind_mig_stats = ind_stats.type_stats[ MigrationType[ mig_type ] ]

            line += ", " + str(ind_mig_stats.max_time )
            line += ", " + str(ind_mig_stats.avg_time )
            line += ", " + str(ind_mig_stats.num_trips)

        line += "\n"
        output_file.write( line )
    
    line  = "Total,,"
    for mig_type in MigrationTypeKeys:
        ts_mig_stats = total_stats.type_stats[ MigrationType[ mig_type ] ]
        if( ts_mig_stats.counter > 0 ):
            ts_mig_stats.avg_time  = ts_mig_stats.avg_time  / float(ts_mig_stats.counter)
            ts_mig_stats.num_trips = ts_mig_stats.num_trips / float(ts_mig_stats.counter)

        line += ", " + str( ts_mig_stats.max_time )
        line += ", " + str( ts_mig_stats.avg_time )
        line += ", " + str( ts_mig_stats.num_trips )
    line += "\n"
    output_file.write( line )

    for i in range(4):
        demog_ind_stats = demog_stats[i]
        line = MigrationDemographicsName[i] + ",,"

        for mig_type in MigrationTypeKeys:
            demog_mig_stats = demog_ind_stats.type_stats[ MigrationType[ mig_type ] ]
            if( demog_mig_stats.counter > 0 ):
                demog_mig_stats.avg_time  = demog_mig_stats.avg_time  / float(demog_mig_stats.counter)
                demog_mig_stats.num_trips = demog_mig_stats.num_trips / float(demog_mig_stats.counter)

            line += ", " + str( demog_mig_stats.max_time )
            line += ", " + str( demog_mig_stats.avg_time )
            line += ", " + str( demog_mig_stats.num_trips )
        line += "\n"
        output_file.write( line )

    output_file.write( "\n" )
    
    header = "Type"
    for mig_type in MigrationTypeKeys:
        header += "," + mig_type + "_Counter"
    header += "\n"
    output_file.write( header )

    line = "Total"
    for mig_type in MigrationTypeKeys:
        ts_mig_stats = total_stats.type_stats[ MigrationType[ mig_type ] ]
        line += ", " + str( ts_mig_stats.counter )
    line += "\n"
    output_file.write( line )

    for i in range(4):
        demog_ind_stats = demog_stats[i]
        line = MigrationDemographicsName[i]

        for mig_type in MigrationTypeKeys:
            demog_mig_stats = demog_ind_stats.type_stats[ MigrationType[ mig_type ] ]
            line += ", " + str( demog_mig_stats.counter )
        line += "\n"
        output_file.write( line )

    output_file.close()


if __name__ == "__main__":

    input_fn = "ReportHumanMigrationTracking.csv"

    data_map = ReadData( input_fn )

    human_id_list = data_map.keys()
    human_id_list.sort()


    demog_stats = []
    for i in range(4):
        ind_stats = IndividualStats()
        if( i == 0 ):
            ind_stats.gender = "M"
            ind_stats.is_adult = True
        elif( i == 1 ):
            ind_stats.gender = "F"
            ind_stats.is_adult = True
        elif( i == 2 ):
            ind_stats.gender = "M"
            ind_stats.is_adult = False
        elif( i == 3 ):
            ind_stats.gender = "F"
            ind_stats.is_adult = False

        for mig_type in MigrationTypeKeys:
            ind_stats.type_stats.append( MigrationStats() )
        demog_stats.append( ind_stats )

    total_stats = IndividualStats()
    for mig_type in MigrationTypeKeys:
        total_stats.type_stats.append( MigrationStats() )

    stats_map = collections.OrderedDict([])
    for human_id in human_id_list:
        stats_map[ human_id ] = GetStats( data_map[ human_id ], total_stats, demog_stats )

    WriteStats( human_id_list, stats_map, total_stats, demog_stats )


