#!/usr/bin/python

"""
This module converts a campaign.json file from HIV-Ongoing (1/1/2022) to a version
that Malaria-Ongoing can read (3/30/2022).  It might be able to handle older
versions of HIV-Ongoing but I'm not sure how much older.

The script will also check all of the event triggers and update the
Custom_Individual_Events list in the config.  It will give warnings about
user events that are broadcasted but not listened to or listened to but
not broadcasted.
"""

import os, sys, json, argparse

#The list of events that are broadcasted internally from EMOD
#Really should get this from the schema.
built_in_events = [
    "Births",
    "EveryUpdate",
    "NewInfectionEvent",
    "TBActivation",
    "NewClinicalCase",
    "NewSevereCase",
    "DiseaseDeaths",
    "OpportunisticInfectionDeath",
    "NonDiseaseDeaths",
    "TBActivationSmearPos",
    "TBActivationSmearNeg",
    "TBActivationExtrapulm",
    "TBActivationPostRelapse",
    "TBPendingRelapse",
    "TBActivationPresymptomatic",
    "TestPositiveOnSmear",
    "ProviderOrdersTBTest",
    "TBTestPositive",
    "TBTestNegative",
    "TBTestDefault",
    "TBRestartHSB",
    "TBMDRTestPositive",
    "TBMDRTestNegative",
    "TBMDRTestDefault",
    "TBFailedDrugRegimen",
    "TBRelapseAfterDrugRegimen",
    "TBStartDrugRegimen",
    "TBStopDrugRegimen",
    "PropertyChange",
    "STIDebut",
    "StartedART",
    "StoppedART",
    "InterventionDisqualified",
    "HIVNewlyDiagnosed",
    "GaveBirth",
    "Pregnant",
    "Emigrating",
    "Immigrating",
    "HIVTestedNegative",
    "HIVTestedPositive",
    "NewlySymptomatic",
    "SymptomaticCleared",
    "TwelveWeeksPregnant",
    "FourteenWeeksPregnant",
    "SixWeeksOld",
    "EighteenMonthsOld",
    "STIPreEmigrating",
    "STIPostImmigrating",
    "STINewInfection",
    "STIExposed",
    "NewExternalHIVInfection",
    "NodePropertyChange",
    "HappyBirthday",
    "EnteredRelationship",
    "ExitedRelationship",
    "FirstCoitalAct",
    "ExposureComplete",
    "VectorToHumanTransmission",
    "HumanToVectorTransmission",
    "ReceivedInfectiousBites",
    "InfectionCleared",
    "WouldHaveDied",
    "WouldHaveHadAIDS",
    "WouldHaveEnteredLatentStage",
    "HIVInfectionStageEnteredLatent",
    "HIVInfectionStageEnteredAIDS",
    "HIVInfectionStageEnteredOnART"
]

#events that were renamed when going from 2.18 to 2.20
renamed_events = {
    "HIVSymptomatic":"NewlySymptomatic"
}

#events removed in Malaria-Ongoing
removed_events = [
    "EveryTimeStep"
]

#parameter names in the campaign file that define events
#that will be listened for
#(Would be nice if we could extract this from the schema)
event_listener_params = [
    "Trigger_Condition_List"
]

#parameter names of events that are broadcasted
#(Would be nice if we could extract this from the schema)
events_broadcasted_params = [
    "Negative_Diagnosis_Event",
    "Positive_Diagnosis_Event",
    "Broadcast_Event",
    "Distributed_Event_Trigger",
    "Expired_Event_Trigger",
    "Actual_IndividualIntervention_Event",
    "Blackout_Event_Trigger",
    "Event_Trigger",
    "Broadcast_On_Expiration_Event",
    "Stop_ART_Event"
]

#Choice_Names is another parameter that has events but it is a list of
#events and not a single value.  It will be handled specially.

#keep track of events that are broadcasted and listened for
#so we can check for things not be connected as desired.
event_names_found_to_be_broadcasted = []
event_names_found_to_be_listened_for = []


def CheckListenedForEvents( event_list ):
    '''
    Take a list of events that are to be listened for
    and return a new list with any renamed events.
    '''
    new_list = []
    for event_value in event_list:
        if event_value in renamed_events.keys():
            event_value = renamed_events[ event_value ]
        elif event_value in removed_events:
            raise "Event not longer broadcasted: " + event_value
        new_list.append( event_value )
        if event_value not in event_names_found_to_be_listened_for:
            event_names_found_to_be_listened_for.append( event_value )
    return new_list


def UpdateEvents( intervention_config ):
    '''
    Look through the parameters in the intervention
    and update the event names.
    '''
    class_name = intervention_config["class"]
    if class_name == "HIVRandomChoice":
        choice_names = intervention_config["Choice_Names"]
        new_list = []
        for event_value in choice_names:        
            if event_value in renamed_events.keys():
                event_value = renamed_events[ event_value ]
            elif event_value in removed_events:
                raise "Event not longer broadcasted: " + event_value
            if event_value not in event_names_found_to_be_broadcasted:
                event_names_found_to_be_broadcasted.append( event_value )
            new_list.append( event_value )
        intervention_config["Choice_Names"] = new_list
    else:
        for broadcast_param in events_broadcasted_params:
            if broadcast_param in intervention_config.keys():
                event_value = intervention_config[ broadcast_param ]
                if event_value in renamed_events.keys():
                    intervention_config[ broadcast_param ] = renamed_events[ event_value ]
                    event_value = renamed_events[ event_value ]
                elif event_value in removed_events:
                    raise "Event not longer broadcasted: " + event_value
                if event_value not in event_names_found_to_be_broadcasted:
                    event_names_found_to_be_broadcasted.append( event_value )
        
        for listener_param in event_listener_params:
            if listener_param in intervention_config.keys():
                new_list = CheckListenedForEvents( intervention_config[ listener_param ] )
                intervention_config[ listener_param ] = new_list
    return
    
    
def CheckConfigEvents( config_json ):
    '''
    Check the parameters in config.json that specify events to listen for
    and validate/update them.
    '''
    cp = config_json["parameters"]
    
    new_list = CheckListenedForEvents( cp["Report_HIV_Event_Channels_List"] )
    cp["Report_HIV_Event_Channels_List"] = new_list
    
    if cp["Report_Event_Recorder"] == 1:
        new_list = CheckListenedForEvents( cp["Report_Event_Recorder_Events"] )
        cp["Report_Event_Recorder_Events"] = new_list
        
    if config_json["parameters"]["Report_HIV_ByAgeAndGender"] == 1:
        new_list = CheckListenedForEvents( cp["Report_HIV_ByAgeAndGender_Event_Counter_List"] )
        cp["Report_HIV_ByAgeAndGender_Event_Counter_List"] = new_list
            
    return


def UpdateAndValidateCustomIndividualEvents( config_json ):
    '''
    This is where we compare the total list of events being broadcasted
    versus those that are being listened for.  It gives appropriate warnings
    and updates Custom_Individual_Events with the set of used custom events.
    '''
    file_custom_events = config_json["parameters"]["Custom_Individual_Events"]
    new_custom_events = []
    
    for name in event_names_found_to_be_broadcasted:
        if name not in file_custom_events:
            print("WARNING: Event is missing from 'Custom_Individual_Events': "+name)
        if name not in event_names_found_to_be_listened_for:
            print("WARNING: Event is being broadcasted but not listened for: "+name)
        if name in built_in_events:
            print("WARNING: Built-in event being broadcasted by non-internal means: "+name)
        elif name not in new_custom_events:
            new_custom_events.append( name )
    
    for name in event_names_found_to_be_listened_for:
        if name not in built_in_events:
            if name not in file_custom_events:
                print("WARNING: Event is missing from 'Custom_Individual_Events': "+name)
            if name not in event_names_found_to_be_broadcasted:
                print("WARNING: Event is being listend for but is not being broadcasted: "+name)
            if name not in new_custom_events:
                new_custom_events.append( name )
    
    new_custom_events.sort()
    config_json["parameters"]["Custom_Individual_Events"] = new_custom_events
    return
    

#Conversion from duration to distribution
duration_to_distribution = {
    "FIXED_DURATION"          : "CONSTANT_DISTRIBUTION",
    "UNIFORM_DURATION"        : "UNIFORM_DISTRIBUTION",
    "GAUSSIAN_DURATION"       : "GAUSSIAN_DISTRIBUTION",
    "EXPONENTIAL_DURATION"    : "EXPONENTIAL_DISTRIBUTION",
    "POISSON_DURATION"        : "POISSON_DISTRIBUTION",
    "LOG_NORMAL_DURATION"     : "LOG_NORMAL_DISTRIBUTION",
    "WEIBULL_DURATION"        : "WEIBULL_DISTRIBUTION",
    "BIMODAL_DURATION"        : "DUAL_CONSTANT_DISTRIBUTION",
    "DUAL_TIMESCALE_DURATION" : "DUAL_EXPONENTIAL_DISTRIBUTION"
}

#A dictionary of campaign objects that use distributions.
#This dictionary is used to update/rename the parameters
#associated with the distribution.
distributions = {
    "CommunityHealthWorkerEventCoordinator" : {
        "Old_Distribution_Name" : "Initial_Amount_Distribution_Type",
        "New_Distribution_Name" : "Initial_Amount_Distribution",
        "FIXED_DURATION": {
            "Initial_Amount" : "Initial_Amount_Constant"
        },
        "EXPONENTIAL_DURATION": {
            "Initial_Amount" : "Initial_Amount_Exponential"
        },
        "UNIFORM_DURATION" : {
            "Initial_Amount_Min" : "Initial_Amount_Min",
            "Initial_Amount_Max" : "Initial_Amount_Max"
        },
        "GAUSSIAN_DURATION" : {
            "Initial_Amount_Mean"    : "Initial_Amount_Gaussian_Mean",
            "Initial_Amount_Std_Dev" : "Initial_Amount_Gaussian_Std_Dev"
        }
    },
    "DelayEventCoordinator" : {
        "Old_Distribution_Name" : "Delay_Distribution",
        "New_Distribution_Name" : "Delay_Period_Distribution",
        "FIXED_DURATION": {
            "Delay_Period" : "Delay_Period_Constant"
        },
        "EXPONENTIAL_DURATION": {
            "Delay_Period" : "Delay_Period_Exponential"
        },
        "UNIFORM_DURATION" : {
            "Delay_Period_Min" : "Delay_Period_Min",
            "Delay_Period_Max" : "Delay_Period_Max"
        },
        "GAUSSIAN_DURATION" : {
            "Delay_Period_Mean"    : "Delay_Period_Gaussian_Mean",
            "Delay_Period_Std_Dev" : "Delay_Period_Gaussian_Std_Dev"
        },
        "WEIBULL_DURATION" : {
            "Delay_Period_Scale" : "Delay_Period_Lambda",
            "Delay_Period_Shape" : "Delay_Period_Kappa"
        }
    },
    "DelayedIntervention" : {
        "Old_Distribution_Name" : "Delay_Distribution",
        "New_Distribution_Name" : "Delay_Period_Distribution",
        "FIXED_DURATION": {
            "Delay_Period" : "Delay_Period_Constant"
        },
        "EXPONENTIAL_DURATION": {
            "Delay_Period" : "Delay_Period_Exponential"
        },
        "UNIFORM_DURATION" : {
            "Delay_Period_Min" : "Delay_Period_Min",
            "Delay_Period_Max" : "Delay_Period_Max"
        },
        "GAUSSIAN_DURATION" : {
            "Delay_Period_Mean"    : "Delay_Period_Gaussian_Mean",
            "Delay_Period_Std_Dev" : "Delay_Period_Gaussian_Std_Dev"
        },
        "WEIBULL_DURATION" : {
            "Delay_Period_Scale" : "Delay_Period_Lambda",
            "Delay_Period_Shape" : "Delay_Period_Kappa"
        }
    },
    "HIVDelayedIntervention" : {
        "Old_Distribution_Name" : "Delay_Distribution",
        "New_Distribution_Name" : "Delay_Period_Distribution",
        "FIXED_DURATION": {
            "Delay_Period" : "Delay_Period_Constant"
        },
        "EXPONENTIAL_DURATION": {
            "Delay_Period" : "Delay_Period_Exponential"
        },
        "UNIFORM_DURATION" : {
            "Delay_Period_Min" : "Delay_Period_Min",
            "Delay_Period_Max" : "Delay_Period_Max"
        },
        "GAUSSIAN_DURATION" : {
            "Delay_Period_Mean"    : "Delay_Period_Gaussian_Mean",
            "Delay_Period_Std_Dev" : "Delay_Period_Gaussian_Std_Dev"
        },
        "WEIBULL_DURATION" : {
            "Delay_Period_Scale" : "Delay_Period_Lambda",
            "Delay_Period_Shape" : "Delay_Period_Kappa"
        }
    },
    "HIVMuxer" : {
        "Old_Distribution_Name" : "Delay_Distribution",
        "New_Distribution_Name" : "Delay_Period_Distribution",
        "FIXED_DURATION": {
            "Delay_Period" : "Delay_Period_Constant"
        },
        "EXPONENTIAL_DURATION": {
            "Delay_Period" : "Delay_Period_Exponential"
        },
        "UNIFORM_DURATION" : {
            "Delay_Period_Min" : "Delay_Period_Min",
            "Delay_Period_Max" : "Delay_Period_Max"
        },
        "GAUSSIAN_DURATION" : {
            "Delay_Period_Mean"    : "Delay_Period_Gaussian_Mean",
            "Delay_Period_Std_Dev" : "Delay_Period_Gaussian_Std_Dev"
        },
        "WEIBULL_DURATION" : {
            "Delay_Period_Scale" : "Delay_Period_Lambda",
            "Delay_Period_Shape" : "Delay_Period_Kappa"
        }
    },
    "AntiretroviralTherapyFull" : {
        "Old_Distribution_Name" : "Time_On_ART_Distribution",
        "New_Distribution_Name" : "Time_On_ART_Distribution",
        "FIXED_DURATION": {
            "Time_On_ART_Period" : "Time_On_ART_Constant"
        },
        "EXPONENTIAL_DURATION": {
            "Time_On_ART_Period" : "Time_On_ART_Exponential"
        },
        "UNIFORM_DURATION" : {
            "Time_On_ART_Min" : "Time_On_ART_Min",
            "Time_On_ART_Max" : "Time_On_ART_Max"
        },
        "GAUSSIAN_DURATION" : {
            "Time_On_ART_Mean"    : "Time_On_ART_Gaussian_Mean",
            "Time_On_ART_Std_Dev" : "Time_On_ART_Gaussian_Std_Dev"
        },
        "WEIBULL_DURATION" : {
            "Time_On_ART_Scale" : "Time_On_ART_Lambda",
            "Time_On_ART_Shape" : "Time_On_ART_Kappa"
        }
    },
    "CoitalActRiskFactors" : {
        "Old_Distribution_Name" : "Expiration_Distribution_Type",
        "New_Distribution_Name" : "Expiration_Distribution",
        "FIXED_DURATION": {
            "Expiration_Period" : "Expiration_Constant"
        },
        "EXPONENTIAL_DURATION": {
            "Expiration_Period" : "Expiration_Exponential"
        },
        "UNIFORM_DURATION" : {
            "Expiration_Period_Min" : "Expiration_Min",
            "Expiration_Period_Max" : "Expiration_Max"
        },
        "GAUSSIAN_DURATION" : {
            "Expiration_Period_Mean"    : "Expiration_Gaussian_Mean",
            "Expiration_Period_Std_Dev" : "Expiration_Gaussian_Std_Dev"
        }
    },
    "MigrateIndividuals" : {
        "Old_Distribution_Name" : "Duration_Before_Leaving_Distribution_Type",
        "New_Distribution_Name" : "Duration_Before_Leaving_Distribution",
        "FIXED_DURATION": {
            "Duration_Before_Leaving_Fixed" : "Duration_Before_Leaving_Constant"
        },
        "EXPONENTIAL_DURATION": {
            "Duration_Before_Leaving_Exponential_Period" : "Duration_Before_Leaving_Exponential"
        },
        "UNIFORM_DURATION" : {
            "Duration_Before_Leaving_Uniform_Min" : "Duration_Before_Leaving_Min",
            "Duration_Before_Leaving_Uniform_Max" : "Duration_Before_Leaving_Max"
        },
        "GAUSSIAN_DURATION" : {
            "Duration_Before_Leaving_Gaussian_Mean"   : "Duration_Before_Leaving_Gaussian_Mean",
            "Duration_Before_Leaving_Gaussian_StdDev" : "Duration_Before_Leaving_Gaussian_Std_Dev"
        },
        "POISSON_DURATION" : {
            "Duration_Before_Leaving_Poisson_Mean" : "Duration_Before_Leaving_Poisson_Mean"
        }
    },
    "MigrateIndividuals2" : {
        "Old_Distribution_Name" : "Duration_At_Node_Distribution_Type",
        "New_Distribution_Name" : "Duration_At_Node_Distribution",
        "FIXED_DURATION": {
            "Duration_At_Node_Fixed" : "Duration_At_Node_Constant"
        },
        "EXPONENTIAL_DURATION": {
            "Duration_At_Node_Exponential_Period" : "Duration_At_Node_Exponential"
        },
        "UNIFORM_DURATION" : {
            "Duration_At_Node_Uniform_Min" : "Duration_At_Node_Min",
            "Duration_At_Node_Uniform_Max" : "Duration_At_Node_Max"
        },
        "GAUSSIAN_DURATION" : {
            "Duration_At_Node_Gaussian_Mean"   : "Duration_At_Node_Gaussian_Mean",
            "Duration_At_Node_Gaussian_StdDev" : "Duration_At_Node_Gaussian_Std_Dev"
        },
        "POISSON_DURATION" : {
            "Duration_At_Node_Poisson_Mean" : "Duration_At_Node_Poisson_Mean"
        }
    },
    "MigrateFamily" : {
        "Old_Distribution_Name" : "Duration_Before_Leaving_Distribution_Type",
        "New_Distribution_Name" : "Duration_Before_Leaving_Distribution",
        "FIXED_DURATION": {
            "Duration_Before_Leaving_Fixed" : "Duration_Before_Leaving_Constant"
        },
        "EXPONENTIAL_DURATION": {
            "Duration_Before_Leaving_Exponential_Period" : "Duration_Before_Leaving_Exponential"
        },
        "UNIFORM_DURATION" : {
            "Duration_Before_Leaving_Uniform_Min" : "Duration_Before_Leaving_Min",
            "Duration_Before_Leaving_Uniform_Max" : "Duration_Before_Leaving_Max"
        },
        "GAUSSIAN_DURATION" : {
            "Duration_Before_Leaving_Gaussian_Mean"   : "Duration_Before_Leaving_Gaussian_Mean",
            "Duration_Before_Leaving_Gaussian_StdDev" : "Duration_Before_Leaving_Gaussian_Std_Dev"
        },
        "POISSON_DURATION" : {
            "Duration_Before_Leaving_Poisson_Mean" : "Duration_Before_Leaving_Poisson_Mean"
        }
    },
    "MigrateFamily2" : {
        "Old_Distribution_Name" : "Duration_At_Node_Distribution_Type",
        "New_Distribution_Name" : "Duration_At_Node_Distribution",
        "FIXED_DURATION": {
            "Duration_At_Node_Fixed" : "Duration_At_Node_Constant"
        },
        "EXPONENTIAL_DURATION": {
            "Duration_At_Node_Exponential_Period" : "Duration_At_Node_Exponential"
        },
        "UNIFORM_DURATION" : {
            "Duration_At_Node_Uniform_Min" : "Duration_At_Node_Min",
            "Duration_At_Node_Uniform_Max" : "Duration_At_Node_Max"
        },
        "GAUSSIAN_DURATION" : {
            "Duration_At_Node_Gaussian_Mean"   : "Duration_At_Node_Gaussian_Mean",
            "Duration_At_Node_Gaussian_StdDev" : "Duration_At_Node_Gaussian_Std_Dev"
        },
        "POISSON_DURATION" : {
            "Duration_At_Node_Poisson_Mean" : "Duration_At_Node_Poisson_Mean"
        }
    }
}


def ConvertDistribution( object_config ):
    '''
    This uses the 'distributions' dictionary defined above to update
    the paraemters of the given campaign object.
    '''
    class_name = object_config[ "class" ]
    if class_name in distributions.keys():
        dist_data = distributions[ class_name ]
        
        old_dist_value = object_config[ dist_data[ "Old_Distribution_Name" ] ]
        new_dist_value = duration_to_distribution[ old_dist_value ]
        object_config[ dist_data[ "New_Distribution_Name" ] ] = new_dist_value
        if dist_data[ "Old_Distribution_Name" ] != dist_data[ "New_Distribution_Name" ]:
            del object_config[ dist_data[ "Old_Distribution_Name" ] ]
        
        for old_key, new_key in dist_data[ old_dist_value ].items():
            if old_key in object_config.keys() and old_key != new_key:
                object_config[ new_key ] = object_config[ old_key ]
                del object_config[ old_key ]
    return    
    
    
#new formats
#CoverageByNodeEventCoordinator::CoverageByNode
#HIVRandomChoice::Choices -> Choice_Names, Choice_Probabilities

def ConvertCoverageByNode( object_config ):
    '''
    The CoveragebyNodeEventCoordinator had a parameter called CoverageByNode.
    In 2.18, this parameter was a two dimensional array.  In Malaria-Ongoing,
    it was converted to be an array of objects where each object had a
    'Node_Id' and 'Coverage' parameters.
    '''
    old_cbn = object_config["Coverage_By_Node"]
    new_cbn = []
    for inner_array in old_cbn:
        node_coverage = {}
        node_coverage["Node_Id" ] = inner_array[0]
        node_coverage["Coverage"] = inner_array[1]
        new_cbn.append( node_coverage )
    object_config["Coverage_By_Node"] = new_cbn
    return


def ConvertChoices( object_config ):
    '''
    In 2.18, HIVRandonChoice had a 'Choices' parameter that was a dictionary
    of event names to probability of selecting the event.  In 2.20, this was
    changed to be two parallel arrays - one for the event names and one for
    the probabilities.
    '''
    old_choices = object_config["Choices"]
    choice_names = []
    choice_probs = []
    for event_name, prob in old_choices.items():
        choice_names.append( event_name )
        choice_probs.append( prob )
    object_config["Choice_Names"] = choice_names
    object_config["Choice_Probabilities"] = choice_probs
    del object_config["Choices"]
    return


def ConvertFormats( object_config ):
    '''
    Update the campaign objects that have a format change
    '''
    class_name = object_config["class"]
    if class_name == "CoverageByNodeEventCoordinator":
        ConvertCoverageByNode( object_config )
    elif class_name == "HIVRandomChoice":
        ConvertChoices( object_config )
    return


#campaign parameters that are lists/arrays of interventions
#(Would be really nice to get this out of the schema)
intervention_array_names = [
    "Actual_IndividualIntervention_Configs",
    "Not_Covered_IndividualIntervention_Configs",
    "Intervention_List"
]

#campaign parameters that contain a single intervention
#(Would be really nice to get this out of the schema)
intervention_container_names = [
    "Intervention_Config",
    "Actual_IndividualIntervention_Config",
    "Positive_Diagnosis_Config",
    "Negative_Diagnosis_Config",
    "Defaulters_Config"
]

#Keep track of what interventions are used by the user
interventions_found = []

def RemoveEventOrConfig( intervention_config ):
    '''
    In Malaria-Ongoing, 'Event_Or_Config' no longer does anything for these
    interventions.  It will give an exception if it is defined.
    '''
    class_name = intervention_config["class"]
    if class_name == "HIVPiecewiseByYearAndSexDiagnostic" or\
       class_name == "HIVSigmoidByYearAndSexDiagnostic" or\
       class_name == "HIVARTStagingByCD4Diagnostic" or\
       class_name == "HIVARTStagingCD4AgnosticDiagnostic":
        if "Event_Or_Config" in intervention_config.keys():
            del intervention_config["Event_Or_Config"]


def RemoveEmptyInterventionName( intervention_config ):
    '''
    In Malaria-Ongoing, an empty string 'Intervention_Name' parameter
    is invalid so remove any occurances of this.
    '''
    if "Intervention_Name" in intervention_config.keys():
        name = intervention_config["Intervention_Name"]
        if len(name) == 0:
            del intervention_config["Intervention_Name"]


def UpdateIntervention( intervention_config ):
    '''
    Update the events, distributions, and any other parameter changes..
    It will recursively update any interventions that this
    intervention might distribute.
    '''
    for array_name in intervention_array_names:
        if array_name in intervention_config.keys():
            for object_config in intervention_config[ array_name ]:
                UpdateIntervention( object_config )

    for container_name in intervention_container_names:
        if container_name in intervention_config.keys():
            UpdateIntervention( intervention_config[ container_name ] )

    if intervention_config["class"] not in interventions_found:
        interventions_found.append( intervention_config["class"] )
        
    ConvertFormats( intervention_config )
    ConvertDistribution( intervention_config )
    UpdateEvents( intervention_config )
    RemoveEventOrConfig( intervention_config )
    RemoveEmptyInterventionName( intervention_config )
    
    return
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('config',              default=None, nargs='?', help='config.json to be used with campaign')
    parser.add_argument('new_config',          default=None, nargs='?', help='new config.json due to changes from campaign')
    parser.add_argument('campaign_to_convert', default=None, nargs='?', help='name of campaign file to convert')
    parser.add_argument('new_campaign',        default=None, nargs='?', help='name of the new converted campaign file')
    
    args = parser.parse_args()
    
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit()

    # open the config and campaign files
    config_json = {}
    with open( args.config ) as config_file:
        config_json = json.loads( config_file.read() )

    old_campaign_json = {}    
    with open( args.campaign_to_convert ) as old_campaign_file:
        old_campaign_json = json.loads( old_campaign_file.read() )

    #loop through each campaign event and update the JSON object in place.
    for campaign_event in old_campaign_json[ "Events" ]:
        event_coord = campaign_event["Event_Coordinator_Config"]
        ConvertFormats( event_coord )
        ConvertDistribution( event_coord )        
        UpdateIntervention( event_coord["Intervention_Config"] )
    
    #Check on the events in the config and update Custom_Individual_Events
    CheckConfigEvents( config_json )
    UpdateAndValidateCustomIndividualEvents( config_json )
    
    config_json["parameters"]["Campaign_Filename"] = args.new_campaign

    #save new config
    with open( args.new_config, "w" ) as handle:
        handle.write( json.dumps( config_json, indent=4, sort_keys=True ) )

    #save new campaign
    with open( args.new_campaign, "w" ) as handle:
        handle.write( json.dumps( old_campaign_json, indent=4, sort_keys=True ) )

    print("Done converting")