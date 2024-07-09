import json, sys, os, shutil

def CopyFile( dir_name, file_name ):
    new_fn = os.path.join( dir_name, file_name )
    shutil.copyfile( file_name, new_fn )


def CreateOutputDir( dir_name ):    
    output_dir = os.path.join( dir_name, "output" )
    os.mkdir( output_dir )
    CopyFile( output_dir, "InsetChart.json" )
    CopyFile( output_dir, "ReportVectorStats.json" )

    
def CreateDirectoryName( test_num, param_names, param_values ):
    dir_name = str(test_num)
    for index in range(len(param_names)):
        if( param_names[index] != "SimDur" ):
            dir_name += "_" + param_names[ index ] + "-" + param_values[ index ]
    
    return dir_name

    
def CreateParamOverrides( dir_name, param_names, param_values ):
    po_json = {}
    po_json["Default_Config_Path"] = "Vector/cohort_vs_ind/default_config.json"
    po_json["parameters"] = {}
    po_json["parameters"]["Config_Name"] = dir_name
    po_json["parameters"]["Python_Script_Path"] = "LOCAL"
    po_json["parameters"]["Vector_Species_Params"] = {}
    po_json["parameters"]["Vector_Species_Params"]["arabiensis"] = {}
    
    campaign_fn = "campaign"
    for index in range(len(param_names)):
        param_abr = param_names[ index ]
        if( param_abr == "SimDur" ):
            po_json["parameters"]["Simulation_Duration"] = float(param_values[index])
        elif( param_abr == "Mort" ):
            po_json["parameters"]["Enable_Vector_Mortality"] = int(param_values[index])
        elif( param_abr == "Age" ):
            po_json["parameters"]["Enable_Vector_Aging"] = int(param_values[index])
        elif( param_abr == "Peop" ):
            if( param_values[index] == "1000" ):
                po_json["parameters"]["Demographics_Filenames"] = []
                po_json["parameters"]["Demographics_Filenames"].append( "../Demographics.json" )
                po_json["parameters"]["Demographics_Filenames"].append( "../Overlay_1000.json" )        
        elif( param_abr == "DBF" ):
            po_json["parameters"]["Vector_Species_Params"]["arabiensis"]["Days_Between_Feeds"] = float(param_values[index])
        elif( param_abr == "Anth" ):
            po_json["parameters"]["Vector_Species_Params"]["arabiensis"]["Anthropophily"] = float(param_values[index])
        elif( param_abr == "IndoorFF" ):
            po_json["parameters"]["Vector_Species_Params"]["arabiensis"]["Indoor_Feeding_Fraction"] = float(param_values[index])
        elif( param_abr == "SugarFF" ):
            if( param_values[index] == "EveryDay" ):
                po_json["parameters"]["Vector_Species_Params"]["arabiensis"]["Vector_Sugar_Feeding_Frequency"] = "VECTOR_SUGAR_FEEDING_EVERY_DAY"
            elif( param_values[index] == "EveryFeed" ):
                po_json["parameters"]["Vector_Species_Params"]["arabiensis"]["Vector_Sugar_Feeding_Frequency"] = "VECTOR_SUGAR_FEEDING_EVERY_FEED"
            elif( param_values[index] == "OnEmerg" ):
                po_json["parameters"]["Vector_Species_Params"]["arabiensis"]["Vector_Sugar_Feeding_Frequency"] = "VECTOR_SUGAR_FEEDING_ON_EMERGENCE_ONLY"
        elif( param_abr == "Infection" ):
            campaign_fn += "_Infect-" + param_values[index]
        elif( param_abr == "Interven" ):
            campaign_fn += "_Interven-" + param_values[index]
            
    campaign_fn += ".json"
    new_campaign_fn = os.path.join( dir_name, "campaign.json" )
    shutil.copyfile( campaign_fn, new_campaign_fn )
        
    po_fn = os.path.join( dir_name, "param_overrides.json" )
    
    with open( po_fn, "w" ) as handle:
        handle.write( json.dumps( po_json, indent=4, sort_keys=True ) )

if __name__ == "__main__":

    param_names = []
    tests = []
    with open( "TestMatrix.csv", "r" ) as test_matrix_file:
        header = test_matrix_file.readline()
        header = header.rstrip()
        param_names = header.split(",")
        for line in test_matrix_file:
            line = line.rstrip()
            param_values = line.split(",")
            tests.append( param_values )

    test_suite_json = {}
    test_suite_json["tests"] = []
    test_num = 1
    for param_values in tests:
        dir_name = CreateDirectoryName( test_num, param_names, param_values )
        print(dir_name)
        test = {}
        test["path"] = "Vector/cohort_vs_ind/" + dir_name
        test_suite_json["tests"].append( test )
        os.mkdir( dir_name )
        CopyFile( dir_name, "dtk_post_process.py" )
        CopyFile( dir_name, "custom_reports.json" )
        CreateParamOverrides( dir_name, param_names, param_values )
        CreateOutputDir( dir_name )
        test_num += 1
        
    with open( "cohort_vs_ind.json", "w" ) as handle:
        handle.write( json.dumps( test_suite_json, indent=4, sort_keys=True ) )
        
    print("Done create test directories")
            
            
            