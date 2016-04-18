import sys, os, json, collections

MigrationType = { "home" : 0, "local" : 1, "air" : 2, "regional" : 3, "sea" : 4, "family" : 5, "away" : 6 } # sea = work, family = sea
MigrationTypeKeys = []
MigrationTypeKeys.append( "home" )
MigrationTypeKeys.append( "local" )
MigrationTypeKeys.append( "air" )
MigrationTypeKeys.append( "regional" )
MigrationTypeKeys.append( "sea" )
MigrationTypeKeys.append( "family" )
MigrationTypeKeys.append( "away" )


class InputData:
    def __init__(self):
        self.time      = -1
        self.human_id  = -1
        self.age_years = -1
        self.gender    = ""
        self.is_adult  = False
        self.from_node = -1
        self.to_node   = -1
        self.mig_type  = 0
        self.mig_type_str = "UNKNOWN"


def ReadData( input_fn ):
    input_file = open(input_fn,"r")

    header = input_file.readline()

    data_map = collections.OrderedDict([])

    for line in input_file:
        line = line.replace('\n','')
        line_array = line.split( ',' )

        isAdult = False
        if( line_array[4] == "T" ):
            isAdult = True

        data = InputData()
        data.time      = float( line_array[0] )
        data.human_id  = int(   line_array[1] )
        data.age_years = float( line_array[2] )
        data.gender    =        line_array[3]
        data.is_adult  = isAdult
        data.home_node = int(   line_array[5] )
        data.from_node = int(   line_array[6] )
        data.to_node   = int(   line_array[7] )
        data.mig_type_str  =    line_array[8] 
        data.event     =        line_array[9]
        data.mig_type  = MigrationType[ data.mig_type_str ]

        if( not data_map.has_key( data.human_id ) ):
            data_map[ data.human_id ] = []

        data_map[ data.human_id ].append( data )

    input_file.close()

    return data_map

