import sys, os, json, collections, struct, datetime
import argparse
import math
import matplotlib.pyplot as plt


def GetColumnIndexesFromHeader( header ):
    used_columns = []
    used_columns.append( "Time"             )
    used_columns.append( "NodeID"           )
    used_columns.append( "Genome"           )
    used_columns.append( "VectorPopulation" )
    #used_columns.append( "STATE_INFECTIOUS" )
    #used_columns.append( "STATE_INFECTED"   )
    #used_columns.append( "STATE_ADULT"      )
    #used_columns.append( "STATE_MALE"       )
    #used_columns.append( "STATE_IMMATURE"   )
    #used_columns.append( "STATE_LARVA"      )
    #used_columns.append( "STATE_EGG"        )

    header = header.replace('\n','')
    header_array = header.split( ',' )
    col_indexes_map = {}
    for index in range( len( header_array ) ):
        ch = header_array[ index ].strip()
        for used in used_columns:
            if ch.startswith( used ):
                col_indexes_map[ used ] = index
                break
    return col_indexes_map
    
# --------------------------------------------------------------
# Read the file produced by ReportVectorGenetics.cpp and return
# a list of GenomeData objects representing each line/genome
# --------------------------------------------------------------
def ReadData( filename ):
    data_map = {}
    prev_time = 0.0
    with open( filename, "r" ) as rs:
        header = rs.readline() # chew up header
        col_indexes_map = GetColumnIndexesFromHeader( header )
        for line in rs:
            line = line.replace('\n','')
            line_array = line.split( ',' )

            if len(line_array) == len(col_indexes_map):
                genome = line_array[ col_indexes_map["Genome"] ]
                data = float(   line_array[ col_indexes_map["VectorPopulation"] ])

                if genome not in data_map.keys():
                    data_map[ genome ] = []
                    
                data_map[ genome ].append( data )

    return data_map

def CalculatePercentagesGenomes( genome_to_data_map ):
    genome_list = list( genome_to_data_map )
    num_pts = len( genome_to_data_map[ genome_list[0] ] )
 
    genome_to_totals_map = {}
    genome_to_percentage_map = {}
    for genome in genome_list:
        genome_to_totals_map[ genome ] = 0.0
        genome_to_percentage_map[ genome ] = []
    
    for i in range(num_pts):
        total = 0
        for genome in genome_list:
            total += genome_to_data_map[ genome ][i]
        if total > 0.0:
            for genome in genome_list:
                value = genome_to_data_map[ genome ][i]/total
                genome_to_percentage_map[ genome ].append( value )
                genome_to_totals_map[ genome ] += value
                
    for genome in genome_list:
        if genome_to_totals_map[ genome ] < 0.1:
            del genome_to_percentage_map[ genome ]
                
    return genome_to_percentage_map
    
def CalculatePercentagesAlleles( allele_list, genome_to_data_map ):
    genome_list = list( genome_to_data_map )
    num_pts = len( genome_to_data_map[ genome_list[0] ] )
 
    allele_to_percentage_map = {}
    for allele in allele_list:
        allele_to_percentage_map[ allele ] = []
    
    for i in range(num_pts):
        total = 0
        for genome in genome_list:
            total += genome_to_data_map[ genome ][i]
        for allele in allele_list:
            total_with_allele = 0
            for genome in genome_list:
                if allele in genome:
                    total_with_allele += genome_to_data_map[ genome ][ i ]
            if total > 0.0:
                allele_to_percentage_map[ allele ].append( total_with_allele/total )
            else:
                allele_to_percentage_map[ allele ].append( 0 )
                
    return allele_to_percentage_map
    
def CalculateAlleleFrequency( allele_list, genome_to_data_map ):
    genome_list = list( genome_to_data_map )
    num_pts = len( genome_to_data_map[ genome_list[0] ] )
 
    allele_to_frequency_map = {}
    for allele in allele_list:
        allele_to_frequency_map[ allele ] = []
    
    for i in range(num_pts):
        total = 0
        for genome in genome_list:
            total += genome_to_data_map[ genome ][i]
        for allele in allele_list:
            total_with_allele = 0
            for genome in genome_list:
                gamete_array = genome.split( ':' )
                if allele in gamete_array[0]:
                    total_with_allele += genome_to_data_map[ genome ][ i ]
                if allele in gamete_array[1]:
                    total_with_allele += genome_to_data_map[ genome ][ i ]
            if total > 0.0:
                allele_to_frequency_map[ allele ].append( total_with_allele/(2*total) )
            else:
                allele_to_frequency_map[ allele ].append( 0 )
                
    return allele_to_frequency_map
    

def PlotData( title, name_to_data_map ):
    if len(name_to_data_map) == 0:
        return;
        
    num_names = len( name_to_data_map )
    square_root = math.ceil(math.sqrt(num_names))

    n_figures_x = square_root
    n_figures_y = math.ceil(float(num_names)/float(square_root)) #Explicitly perform a float division here as integer division floors in Python 2.x

    idx = 1
    for name in name_to_data_map:
        try:
            subplot = plt.subplot( n_figures_x, n_figures_y, idx ) 
            subplot.plot( name_to_data_map[ name ], 'r-' )
            plt.setp( subplot.get_xticklabels(), fontsize='6' )
            plt.title( name, fontsize='10' )
            idx += 1
        except Exception as ex:
            print( str(ex) )

    plt.suptitle( title )
    plt.subplots_adjust( bottom=0.05, hspace=0.4 )
    plt.show()

def WriteData( filename, name_to_data_map ):
    if len(name_to_data_map) == 0:
        return;
        
    name_list = list( name_to_data_map.keys() )
    num_pts = len( name_to_data_map[ name_list[0] ] )
    
    with open( filename, "w" ) as output:
        # write header
        output.write("Time")
        for name in name_list:
            output.write(","+name)
        output.write("\n")
        
        for i in range(num_pts):
            output.write(str(i))
            for name in name_list:
                output.write(","+str( name_to_data_map[ name ][i]) )
            output.write("\n")


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('data_file', help='Name of one file with data')
    args = parser.parse_args()

    a_allele_list = []
    #a_allele_list.append( "Aw" )
    #a_allele_list.append( "Ad" )
    #a_allele_list.append( "Ar" )
    a_allele_list.append( "A1" )
    a_allele_list.append( "A2" )
    a_allele_list.append( "A3" )
    a_allele_list.append( "A4" )
    a_allele_list.append( "A5" )
    a_allele_list.append( "A6" )
    a_allele_list.append( "A7" )
    a_allele_list.append( "A8" )
    
    b_allele_list = []
    #b_allele_list.append( "Bw" )
    #b_allele_list.append( "Be" )
    b_allele_list.append( "B1" )
    b_allele_list.append( "B2" )
    b_allele_list.append( "B3" )
    b_allele_list.append( "B4" )
    b_allele_list.append( "B5" )
    b_allele_list.append( "B6" )
    b_allele_list.append( "B7" )
    b_allele_list.append( "B8" )
    
    #c_allele_list = []
    #c_allele_list.append( "Cw" )
    #c_allele_list.append( "Ce" )
    #c_allele_list.append( "c0" )
    #c_allele_list.append( "c1" )
    
    #d_allele_list = []
    #d_allele_list.append( "Dw" )
    #d_allele_list.append( "De" )
    #d_allele_list.append( "d0" )
    #d_allele_list.append( "d1" )
    
    genome_data_map = ReadData( args.data_file )
    genome_to_percentage_map = CalculatePercentagesGenomes( genome_data_map )
    a_allele_to_percentage_map = CalculatePercentagesAlleles( a_allele_list, genome_data_map )
    b_allele_to_percentage_map = CalculatePercentagesAlleles( b_allele_list, genome_data_map )
    #c_allele_to_percentage_map = CalculatePercentagesAlleles( c_allele_list, genome_data_map )
    #d_allele_to_percentage_map = CalculatePercentagesAlleles( d_allele_list, genome_data_map )
    a_allele_to_frequency_map = CalculateAlleleFrequency( a_allele_list, genome_data_map )
    b_allele_to_frequency_map = CalculateAlleleFrequency( b_allele_list, genome_data_map )
    #c_allele_to_frequency_map = CalculateAlleleFrequency( c_allele_list, genome_data_map )
    #d_allele_to_frequency_map = CalculateAlleleFrequency( d_allele_list, genome_data_map )
    
    WriteData( "PercentageByGenome.csv", genome_to_percentage_map )
    WriteData( "PercentageByAllele_A.csv", a_allele_to_percentage_map )
    WriteData( "PercentageByAllele_B.csv", b_allele_to_percentage_map )
    #WriteData( "PercentageByAllele_C.csv", c_allele_to_percentage_map )
    #WriteData( "PercentageByAllele_D.csv", d_allele_to_percentage_map )
    WriteData( "AlleleFrequency_A.csv", a_allele_to_frequency_map )
    WriteData( "AlleleFrequency_B.csv", b_allele_to_frequency_map )
    #WriteData( "AlleleFrequency_C.csv", c_allele_to_frequency_map )
    #WriteData( "AlleleFrequency_D.csv", d_allele_to_frequency_map )
    
    #PlotData( args.data_file, genome_to_percentage_map )
    #PlotData( args.data_file, a_allele_to_percentage_map )
    #PlotData( args.data_file, b_allele_to_percentage_map )
    #PlotData( args.data_file, a_allele_to_frequency_map )
    #PlotData( args.data_file, b_allele_to_frequency_map )
    #PlotData( args.data_file, c_allele_to_frequency_map )


