import sys, os, json, collections
import numpy as np
import matplotlib.pyplot as plt
human_villages = [[1,2,3,4,11,12,13,14,21,22,23,24,31,32,33,34],[7,8,9,10,17,18,19,20,27,28,29,30,37,38,39,40,47,48,49,50,57,58,59,60],[61,62,63,64,65,71,72,73,74,75,76,81,82,83,84,85,86,91,92,93,94,95,96],[100]]
village_labels = ["Vulcan", "Kronos", "Andoria", "Earth" ]

def ShowUsage():
    print ('\nUsage: %s [ReportHumanInfection.csv]' % os.path.basename(sys.argv[0]))

def GetVillageIndex( node_id ):
    village_index = 0
    for village in human_villages:
        for id in village:
            if( id == node_id):
                return village_index
        village_index += 1

class ReportHumanInfection_Data:
    def __init__(self):
        self.time            = -1
        self.human_id        = -1
        self.age_years       = -1
        self.is_adult        = False
        self.gender          = ""
        self.home_node_id    = -1
        self.current_node_id = -1
        self.is_infected     = False

class VillageStats:
    def __init__(self):
        self.population = 0
        self.num_infected = 0

def ReportHumanInfection_Read( input_fn, time_data, percent_data  ):
    input_file = open(input_fn,"r")

    header = input_file.readline()

    for village in human_villages:
        percent_infected = []
        percent_data.append( percent_infected )
        time_array = []
        time_data.append( time_array )

    village_stats = collections.OrderedDict([])

    prev_time = -1
    for line in input_file:
        line = line.replace('\n','')
        line_array = line.split( ',' )

        isAdult = False
        if( line_array[3] == "T" ):
            isAdult = True

        isInfected = False
        if( int( line_array[7] ) == 1 ):
            isInfected = True

        data = ReportHumanInfection_Data()
        data.time            = float( line_array[0] )
        data.human_id        = int(   line_array[1] )
        data.age_years       = float( line_array[2] )
        data.is_adult        = isAdult
        data.gender          =        line_array[4]
        data.home_node_id    = int(   line_array[5] )
        data.current_node_id = int(   line_array[6] )
        data.is_infected     = isInfected


        if( data.time != prev_time  ):
            if( prev_time != -1 ):

                for vi in range(len(village_labels)):
                    #print vi, prev_time, village_stats[ vi ].num_infected, village_stats[ vi ].population
                    time_data[ vi ].append( prev_time )
                    percent_data[ vi ].append( float(village_stats[ vi ].num_infected) / float(village_stats[ vi ].population) )

            prev_time = data.time

            village_stats = collections.OrderedDict([])
            for vi in range(len(village_labels)):
                village_stats[ vi ] = VillageStats()

        village_index = GetVillageIndex( data.current_node_id )

        village_stats[ village_index ].population += 1
        if( data.is_infected ):
            village_stats[ village_index ].num_infected += 1


    for vi in range(len(village_labels)):
        time_data[ vi ].append( prev_time )
        percent_data[ vi ].append( village_stats[ vi ].num_infected / village_stats[ vi ].population )

    input_file.close()


def PlotData( time_data, percent_data, legend_data ):
    print len(time_data), len(percent_data)
    for i in range(len(time_data)):
        plt.plot( time_data[i], percent_data[i])
    plt.title( "Percentage of Infected People Per Village By Time" )
    plt.legend( legend_data )
    plt.show()



if __name__ == "__main__":
    if len(sys.argv) != 2:
        ShowUsage()
        exit(0)

    report_human_infection_fn = sys.argv[1]

    time_data = []
    percent_data = []

    ReportHumanInfection_Read( report_human_infection_fn, time_data, percent_data )

    PlotData( time_data, percent_data, village_labels )

