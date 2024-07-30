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


def CreateCampaignEvent( startDay, eventCoord ):
    ce = {}
    ce["class"] = "CampaignEvent"
    ce["Start_Day"] = startDay
    ce["Nodeset_Config"] = {}
    ce["Nodeset_Config"]["class"] = "NodeSetAll"
    ce["Event_Coordinator_Config"] = eventCoord
    return ce

def CreateNChooser( startDay, numTgt, interven ):
    age_range = {}
    age_range["Min"] = 0
    age_range["Max"] = 125
    
    dist = {}
    dist["Start_Day"] = startDay
    dist["End_Day"] = startDay+1
    dist["Property_Restrictions_Within_Node"] = []
    dist["Age_Ranges_Years"] = []
    dist["Age_Ranges_Years"].append( age_range )
    dist["Num_Targeted"] = []
    dist["Num_Targeted"].append( numTgt )

    nchooser = {}
    nchooser["class"] = "NChooserEventCoordinator"
    nchooser["Distributions"] = []
    nchooser["Distributions"].append( dist )    
    nchooser["Intervention_Config"] = interven
    return nchooser
    

def CreateOutbreak():
    outbreak = {}
    outbreak["class"] = "OutbreakIndividual"
    outbreak["Cost_To_Consumer"] = 0
    outbreak["Antigen"] = 0
    outbreak["Genome"] = 0
    return outbreak
    
    
def CreateWaningEffectConstant( effect ):
    we = {}
    we["class"] = "WaningEffectConstant"
    we["Initial_Effect"] = effect
    return we


def CreateBednet( repel, block, kill ):
    bednet = {}
    bednet["class"] = "SimpleBednet"
    bednet["Cost_To_Consumer"] = 1
    bednet["Usage_Config"    ] = CreateWaningEffectConstant( 1.0 )
    bednet["Repelling_Config"] = CreateWaningEffectConstant( repel )
    bednet["Blocking_Config" ] = CreateWaningEffectConstant( block )
    bednet["Killing_Config"  ] = CreateWaningEffectConstant( kill )
    return bednet


def CreateIRS( block, kill ):
    irs = {}
    irs["class"] = "IRSHousingModification"
    irs["Cost_To_Consumer"] = 1
    irs["Blocking_Config" ] = CreateWaningEffectConstant( block )
    irs["Killing_Config"  ] = CreateWaningEffectConstant( kill )
    return irs


def CreateRepellent( repel ):
    repellent = {}
    repellent["class"] = "SimpleIndividualRepellent"
    repellent["Cost_To_Consumer"] = 1
    repellent["Repelling_Config"] = CreateWaningEffectConstant( repel )
    return repellent
    

def CreateDrug():
    drug = {}
    drug["class"] = "AntimalarialDrug"
    drug["Cost_To_Consumer"] = 1
    drug["Drug_Type"] = "Chloroquine"
    return drug
    

def CreateEvent( startDay, numTgt, interven ):
    nchooser = CreateNChooser( startDay, numTgt, interven )
    ce = CreateCampaignEvent( startDay, nchooser )
    return ce

    
def CreateCampaignFile( dir_name, numTgt, interven_abr, interven ):
    campaign = {}
    campaign["Use_Defaults"] = 1
    campaign["Events"] = []
    campaign["Events"].append( CreateEvent( 100, 100, CreateOutbreak() ) )
    if (interven_abr == "Drug") and (numTgt > 0):
        campaign["Events"].append( CreateEvent( 400, numTgt, interven ) )
        campaign["Events"].append( CreateEvent( 500, numTgt, interven ) )
        campaign["Events"].append( CreateEvent( 600, numTgt, interven ) )
    elif (interven != None) and (numTgt > 0):
        campaign["Events"].append( CreateEvent( 400, numTgt, interven ) )
        
    campaign_fn = os.path.join( dir_name, "campaign.json" )
    with open( campaign_fn, "w" ) as handle:
        handle.write( json.dumps( campaign, indent=4, sort_keys=True ) )
    
    return
    

def CreateParamOverrides( dir_name, param_names, param_values ):
    interven_abr = ""
    num_target = 0
    repel = 0.0
    block = 0.0
    kill = 0.0
    infectious_mortality = 1.0
    dose_interval = 0.0
    decay_T1 = 0.0
    for index in range(len(param_names)):
        param_abr = param_names[ index ]
        if( param_abr == "Mort" ):
            infectious_mortality = float(param_values[index])
        elif( param_abr == "Int" ):
            interven_abr = param_values[index]
        elif( param_abr == "Tgt" ):
            num_target = int(param_values[index])
        elif( param_abr == "R" ):
            repel = float(param_values[index])
        elif( param_abr == "B" ):
            block = float(param_values[index])
        elif( param_abr == "K" ):
            kill = float(param_values[index])
        elif( param_abr == "DoseInt" ):
            dose_interval = float(param_values[index])
        elif( param_abr == "T1" ):
            decay_T1 = float(param_values[index])
            
    po_json = {}
    po_json["Default_Config_Path"] = "Malaria/fpg_vs_base/default_config.json"
    po_json["parameters"] = {}
    po_json["parameters"]["Config_Name"] = dir_name    
    po_json["parameters"]["Python_Script_Path"] = "SHARED"
    
    if( interven_abr == "Drug" ):
        po_json["parameters"]["Malaria_Drug_Params"] = {}
        po_json["parameters"]["Malaria_Drug_Params"]["Chloroquine"] = {}
        po_json["parameters"]["Malaria_Drug_Params"]["Chloroquine"]["Drug_Dose_Interval"] = dose_interval
        po_json["parameters"]["Malaria_Drug_Params"]["Chloroquine"]["Drug_Decay_T1"] = decay_T1
    else:
        vsp_silly_skeeter = {}
        vsp_silly_skeeter["Name"] = "SillySkeeter"
        vsp_silly_skeeter["Infectious_Human_Feed_Mortality_Factor"] = infectious_mortality
                
        po_json["parameters"]["Vector_Species_Params"] = []
        po_json["parameters"]["Vector_Species_Params"].append( vsp_silly_skeeter )
    
    
    po_fn = os.path.join( dir_name, "param_overrides.json" )
    
    with open( po_fn, "w" ) as handle:
        handle.write( json.dumps( po_json, indent=4, sort_keys=True ) )

    interven = None
    if interven_abr == "Bednet":
        interven = CreateBednet( repel, block, kill )
    elif interven_abr == "IRS":
        interven = CreateIRS( block, kill )
    elif interven_abr == "Repellent":
        interven = CreateRepellent( repel )
    elif interven_abr == "Drug":
        interven = CreateDrug()
    
    CreateCampaignFile( dir_name, num_target, interven_abr, interven )
        
        
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
    test_num = 65
    for param_values in tests:
        dir_name = CreateDirectoryName( test_num, param_names, param_values )
        print(dir_name)
        test = {}
        test["path"] = "Malaria/fpg_vs_base/" + dir_name
        test_suite_json["tests"].append( test )
        os.mkdir( dir_name )
        CreateParamOverrides( dir_name, param_names, param_values )
        CreateOutputDir( dir_name )
        CopyFile( dir_name, "dtk_post_process.py" )
        test_num += 1
        
    with open( "fpg.json", "w" ) as handle:
        handle.write( json.dumps( test_suite_json, indent=4, sort_keys=True ) )
        
    print("Done create test directories")

