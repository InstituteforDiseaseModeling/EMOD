import json, sys, os
import dtk_vector_stats_support as dtk_vss
    
def application( output_path ):

    dtk_vss.ConvertReportVectorGeneticsByGenome( "ReportVectorGenetics_arabiensis_GENOME.csv", output_path )
    
    print("Done converting")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("need output path")
        exit(0)

    output_path = sys.argv[1]

    application( output_path )
