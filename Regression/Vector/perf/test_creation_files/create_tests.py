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
        dir_name += "_" + param_names[ index ] + "-" + param_values[ index ]
    
    return dir_name

    
def CreateParamOverrides( dir_name, param_names, param_values ):
    po_json = {}
    po_json["Default_Config_Path"] = "Vector/perf/default_config.json"
    po_json["parameters"] = {}
    po_json["parameters"]["Config_Name"] = dir_name
    po_json["parameters"]["Python_Script_Path"] = "LOCAL"
    
    num_nodes = 1
    campaign_fn = "campaign"
    for index in range(len(param_names)):
        param_abr = param_names[ index ]
        if( param_abr == "Age" ):
            po_json["parameters"]["Enable_Vector_Aging"] = int(param_values[index])
        elif( param_abr == "Nodes" ):
            po_json["parameters"]["Demographics_Filenames"] = []
            if( param_values[index] == "1" ):
                num_nodes = 1
                po_json["parameters"]["Demographics_Filenames"].append( "../Demographics_1.json" )
            elif( param_values[index] == "2" ):
                num_nodes = 2
                po_json["parameters"]["Demographics_Filenames"].append( "../Demographics_2.json" )
            elif( param_values[index] == "25" ):
                num_nodes = 25
                po_json["parameters"]["Demographics_Filenames"].append( "../Demographics_25.json" )
            else:
                num_nodes = 100
                po_json["parameters"]["Demographics_Filenames"].append( "../Demographics_100.json" )
        elif( param_abr == "Genes" ):
            if( param_values[index] == "2" ):
                gene_a = {}
                gene_a["Alleles"] = {}
                gene_a["Alleles"]["a0"] = 0.6
                gene_a["Alleles"]["a1"] = 0.4
                gene_a["Mutations"] = {}
                gene_b = {}
                gene_b["Alleles"] = {}
                gene_b["Alleles"]["b0"] = 0.3
                gene_b["Alleles"]["b1"] = 0.7
                gene_b["Mutations"] = {}
                po_json["parameters"]["Vector_Species_Params"] = {}
                po_json["parameters"]["Vector_Species_Params"]["arabiensis"] = {}
                po_json["parameters"]["Vector_Species_Params"]["arabiensis"]["Genes"] = []
                po_json["parameters"]["Vector_Species_Params"]["arabiensis"]["Genes"].append( gene_a )
                po_json["parameters"]["Vector_Species_Params"]["arabiensis"]["Genes"].append( gene_b )
        elif( param_abr == "Mig" ):
            if( param_values[index] == "1" ):
                po_json["parameters"]["Enable_Vector_Migration"] = 1
                po_json["parameters"]["Enable_Vector_Migration_Local"] = 1
                if( num_nodes == 2 ):
                    po_json["parameters"]["Vector_Migration_Filename_Local"] = "Local_Vector_Migration_2_Nodes.bin"
                elif( num_nodes == 25 ):
                    po_json["parameters"]["Vector_Migration_Filename_Local"] = "Local_Vector_Migration_25_Node_Torus.bin"
                elif( num_nodes == 100 ):
                    po_json["parameters"]["Vector_Migration_Filename_Local"] = "Local_Vector_Migration_100_Node_Torus.bin"
               
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
        test["path"] = "Vector/perf/" + dir_name
        test_suite_json["tests"].append( test )
        os.mkdir( dir_name )
        CopyFile( dir_name, "dtk_post_process.py" )
        CreateParamOverrides( dir_name, param_names, param_values )
        CreateOutputDir( dir_name )
        test_num += 1
        
    with open( "vector_perf.json", "w" ) as handle:
        handle.write( json.dumps( test_suite_json, indent=4, sort_keys=True ) )
        
    print("Done create test directories")
            
            
            