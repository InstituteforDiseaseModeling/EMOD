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
    used_columns.append( "STATE_INFECTIOUS" )
    used_columns.append( "STATE_INFECTED"   )
    used_columns.append( "STATE_ADULT"      )
    used_columns.append( "STATE_MALE"       )
    used_columns.append( "STATE_IMMATURE"   )
    used_columns.append( "STATE_LARVA"      )
    used_columns.append( "STATE_EGG"        )

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
def ReadData( filename, node_id_list ):
    node_genome_map = []
    for node_id in node_id_list:
        node_genome_map.append( {} )
        
    prev_time = -1.0
    with open( filename, "r" ) as rs:
        header = rs.readline() # chew up header
        col_indexes_map = GetColumnIndexesFromHeader( header )
        for line in rs:
            line = line.replace('\n','')
            line_array = line.split( ',' )

            if len(line_array) == 11:
                time    = float( line_array[ col_indexes_map["Time"]             ])
                node_id = int(   line_array[ col_indexes_map["NodeID"]           ])
                genome  =        line_array[ col_indexes_map["Genome"]           ]
                data    = float( line_array[ col_indexes_map["VectorPopulation"] ])

                if time == 0.0:
                    if genome not in node_genome_map[ 0 ].keys():
                        for genome_map in node_genome_map:
                            genome_map[ genome ] = []
                            genome_map[ genome ].append(0)
                elif prev_time < time:
                    for genome_map in node_genome_map:
                        for g in genome_map.keys():
                            genome_map[g].append(0)
                    prev_time = time
                
                if node_id in node_id_list:
                    node_index = node_id_list.index( node_id )
                    node_genome_map[ node_index ][ genome ][-1] += data
                    
                node_genome_map[ 0 ][ genome ][-1] += data                    

    return node_genome_map

def CreateColumnName( node_id, genome ):
    node_str = "Total"
    if node_id != 0:
        node_str = str(node_id)
    column_name = node_str + "=" + genome
    return column_name
    
def CalculatePercentagesGenomes( node_id_list, filter_genome_list, node_genome_map ):

    all_genome_list = list( node_genome_map[ 0 ] )
    num_pts = len( node_genome_map[ 0 ][ all_genome_list[0] ] )
    
    genome_to_percentage_map = {}
    for node_id in node_id_list:
        node_index = node_id_list.index( node_id )
 
        for genome in filter_genome_list:
            column_name = CreateColumnName( node_id, genome )
            genome_to_percentage_map[ column_name ] = []     
        column_name = CreateColumnName( node_id, "Other" )
        genome_to_percentage_map[ column_name ] = []
    
        for data_index in range(num_pts):
            for genome in filter_genome_list:
                column_name = CreateColumnName( node_id, genome )
                genome_to_percentage_map[ column_name ].append(0)
            column_name = CreateColumnName( node_id, "Other" )
            genome_to_percentage_map[ column_name ].append(0)
        
            total = 0
            for genome in all_genome_list:
                value = node_genome_map[ node_index ][ genome ][ data_index ]
                total += value
                if genome in filter_genome_list:
                    column_name = CreateColumnName( node_id, genome )
                else:
                    column_name = CreateColumnName( node_id, "Other" )
                genome_to_percentage_map[ column_name ][-1] += value                    
                
            for genome in filter_genome_list:
                column_name = CreateColumnName( node_id, genome )
                if total > 0.0:
                    genome_to_percentage_map[ column_name ][-1] /= total
            column_name = CreateColumnName( node_id, "Other" )
            if total > 0.0:
                genome_to_percentage_map[ column_name ][-1] /= total
                
                
    return genome_to_percentage_map
    
def CalculatePercentagesAlleles( node_id_list, allele_list, node_genome_map ):
    genome_list = list( node_genome_map[0] )
    num_pts = len( node_genome_map[0][ genome_list[0] ] )
 
    allele_to_percentage_map = {}
    for node_id in node_id_list:
        node_index = node_id_list.index(node_id)
        for allele in allele_list:
            column_name = CreateColumnName( node_id, allele )
            allele_to_percentage_map[ column_name ] = []
            
        for data_index in range(num_pts):
            total = 0
            for genome in genome_list:
                total += node_genome_map[ node_index ][ genome ][ data_index ]
            for allele in allele_list:
                total_with_allele = 0
                for genome in genome_list:
                    if allele in genome:
                        total_with_allele += node_genome_map[ node_index ][ genome ][ data_index ]
                column_name = CreateColumnName( node_id, allele )
                if total > 0.0:
                    allele_to_percentage_map[ column_name ].append( total_with_allele/total )
                else:
                    allele_to_percentage_map[ column_name ].append( 0 )
                
    return allele_to_percentage_map
    
def CalculateAlleleFrequency( node_id_list, allele_list, node_genome_map ):
    genome_list = list( node_genome_map[0] )
    num_pts = len( node_genome_map[0][ genome_list[0] ] )
 
    allele_to_frequency_map = {}
    for node_id in node_id_list:
        node_index = node_id_list.index(node_id)
        for allele in allele_list:
            column_name = CreateColumnName( node_id, allele )
            allele_to_frequency_map[ column_name ] = []
    
        for data_index in range(num_pts):
            total = 0
            for genome in genome_list:
                total += node_genome_map[ node_index ][ genome ][ data_index ]
            for allele in allele_list:
                total_with_allele = 0
                for genome in genome_list:
                    gamete_array = genome.split( ':' )
                    if allele in gamete_array[0]:
                        total_with_allele += node_genome_map[ node_index ][ genome ][ data_index ]
                    if allele in gamete_array[1]:
                        total_with_allele += node_genome_map[ node_index ][ genome ][ data_index ]
                        
                column_name = CreateColumnName( node_id, allele )
                if total > 0.0:
                    allele_to_frequency_map[ column_name ].append( total_with_allele/(2*total) )
                else:
                    allele_to_frequency_map[ column_name ].append( 0 )
                
    return allele_to_frequency_map
    

def PlotData( title, name_to_data_map ):
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
    parser.add_argument('--imperial', help='Use Imperial College allele names')
    parser.add_argument('data_file', help='Name of one file with data')
    args = parser.parse_args()

    a_allele_list = []
    if args.imperial:
        print("Using Imperial Allele Names")
        a_allele_list.append( "Aw" )
        a_allele_list.append( "Ad" )
    else:
        print("Using Test Allele Names")
        a_allele_list.append( "a0" )
        a_allele_list.append( "a1" )
        a_allele_list.append( "a2" )
        a_allele_list.append( "a3" )
        a_allele_list.append( "a4" )
        a_allele_list.append( "a5" )
    
    b_allele_list = []
    if args.imperial:
        b_allele_list.append( "Bw" )
        b_allele_list.append( "Bd" )
        b_allele_list.append( "BmL" )
        b_allele_list.append( "BmH" )
    else:
        b_allele_list.append( "b0" )
        b_allele_list.append( "b1" )
        b_allele_list.append( "b2" )
    
    filter_genome_list = []
    if args.imperial:
        filter_genome_list.append("X-Aw-Bw:X-Aw-Bw")
        filter_genome_list.append("X-Aw-Bd:X-Aw-Bd")
        filter_genome_list.append("X-Ad-Bw:X-Ad-Bw")
        filter_genome_list.append("X-Ad-Bd:X-Ad-Bd")
    else:
        #filter_genome_list.append("X-a0:X-a0")
        #filter_genome_list.append("X-a0:X-a1")
        #filter_genome_list.append("X-a1:X-a0")
        #filter_genome_list.append("X-a1:X-a1")
        filter_genome_list.append("X-a0-b0:X-a0-b0")
        filter_genome_list.append("X-a0-b1:X-a0-b1")
        filter_genome_list.append("X-a1-b0:X-a1-b0")
        filter_genome_list.append("X-a1-b1:X-a1-b1")
    
    # use zero for total
    node_id_list = [ 0, 1, 13, 25 ]
    
    genome_data_map = ReadData( args.data_file, node_id_list )
    
    genome_to_percentage_map = CalculatePercentagesGenomes( node_id_list, filter_genome_list, genome_data_map )
    a_allele_to_percentage_map = CalculatePercentagesAlleles( node_id_list, a_allele_list, genome_data_map )
    b_allele_to_percentage_map = CalculatePercentagesAlleles( node_id_list, b_allele_list, genome_data_map )
    a_allele_to_frequency_map = CalculateAlleleFrequency( node_id_list, a_allele_list, genome_data_map )
    b_allele_to_frequency_map = CalculateAlleleFrequency( node_id_list, b_allele_list, genome_data_map )
    
    WriteData( "PercentageByGenome.csv",   genome_to_percentage_map )
    WriteData( "PercentageByAllele_A.csv", a_allele_to_percentage_map )
    WriteData( "PercentageByAllele_B.csv", b_allele_to_percentage_map )
    WriteData( "AlleleFrequency_A.csv",    a_allele_to_frequency_map )
    WriteData( "AlleleFrequency_B.csv",    b_allele_to_frequency_map )
    
    #PlotData( args.data_file, genome_to_percentage_map )
    #PlotData( args.data_file, a_allele_to_percentage_map )
    #PlotData( args.data_file, b_allele_to_percentage_map )
    #PlotData( args.data_file, a_allele_to_frequency_map )
    #PlotData( args.data_file, b_allele_to_frequency_map )
    #PlotData( args.data_file, c_allele_to_frequency_map )


