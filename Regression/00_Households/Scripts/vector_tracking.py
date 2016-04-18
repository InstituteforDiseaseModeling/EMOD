import sys, os, json, collections


class InputData:
    def __init__(self):
        self.time       = -1
        self.vec_id     = -1
        self.to_node_id = -1
        self.mig_type   = "UNKNOWN"
        self.species    = "UNKNOWN"
        self.age_days   = -1

class CountData:
    def __init__(self):
        self.local = 0
        self.regional = 0

def ReadData( input_fn, start_time, count_data ):
    input_file = open(input_fn,"r")

    header = input_file.readline()

    prev_time = -1

    for line in input_file:
        line = line.replace('\n','')
        line_array = line.split( ',' )

        time       = float( line_array[0] )
        vec_id     = int(   line_array[1] )
        to_node_id = int(   line_array[2] )
        mig_type   =        line_array[3]
        species    =        line_array[4]
        age_days   = float( line_array[5] )

        if( time >= start_time ):
            if( mig_type == "local" ):
                count_data.local += 1
            else:
                count_data.regional += 1

        if( prev_time != time ):
            print time
            prev_time = time

    input_file.close()



if __name__ == "__main__":
    if( len(sys.argv) != 2 ):
        print ('\nUsage: %s [start_time]' % os.path.basename(sys.argv[0]))        
        exit(0)


    input_fn  = "ReportVectorMigration.csv"
    start_time = float(sys.argv[1])

    count_data = CountData()

    data_map = ReadData( input_fn, start_time, count_data )

    total = count_data.local + count_data.regional
    percent_local = 0.0
    percent_regional = 0.0
    if( total > 0 ):
        percent_local    = float(count_data.local)    / float(total)
        percent_regional = float(count_data.regional) / float(total)
    print count_data.local, count_data.regional, total, percent_local, percent_regional

    #ExtractNodeVectorData( node_map )


