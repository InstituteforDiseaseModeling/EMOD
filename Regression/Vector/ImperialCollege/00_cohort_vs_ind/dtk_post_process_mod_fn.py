import json, sys, os, shutil


def application( output_path ):

    model = "ind"
    mortality = 0
    dbf = 1
    
    with open( "config.json", "r" ) as config_file:
        config_json = json.loads( config_file.read() )["parameters"]
        
        if config_json["Vector_Sampling_Type"] == "VECTOR_COMPARTMENTS_NUMBER":
            model = "cohort"
        
        mortality = config_json["Enable_Vector_Mortality"]
        
        dbf = config_json["Vector_Species_Params"][0]["Days_Between_Feeds"]

        
    people = 0
    with open( "Demographics.json", "r" ) as demog_file:
        demog_json = json.loads( demog_file.read() )
        people = demog_json["Nodes"][0]["NodeAttributes"]["InitialPopulation"]
        
    new_fn = "ReportVectorStats"
    new_fn += "_" + model
    new_fn += "_" + "people-" + str(people)
    new_fn += "_" + "dbf-" + str(dbf)
    new_fn += "_" + "Mort-" + str(mortality)
    new_fn += ".csv"
    
    print(new_fn)
    copy_to_dir = "../../cohort_vs_ind/sugar_every_feed"
    
    new_fn = os.path.join( copy_to_dir, new_fn )
    base_fn = os.path.join( output_path, "ReportVectorStats.csv" )
    
    print(new_fn)
    print(base_fn)
    shutil.copyfile( base_fn, new_fn )
    
    print("Done copying ReportVectorStats")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("need output path")
        exit(0)

    output_path = sys.argv[1]

    application( output_path )