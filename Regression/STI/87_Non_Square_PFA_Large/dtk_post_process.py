#!/usr/bin/python

"""
Purpose:
This script is a DTK post-processing validation test for STI_SIM and its 
derivatives.  It is intended to verify that the relationship forming worked
as expected.

Assumptions/Inputs:
It assumes the existence of the RelationshipStart.csv is in the "output" directory.

Outputs:
It produces an outupt file, PFA_Resuls.csv, that shows how well the pair forming algorithm did
against the expected result.

How-To Use:
This script can be invoked directly from the DTK if it is renamed to 
dtk_post_process.py, or can be imported from a script with that name and 
invoked by calling this application method.
"""

import os
import json
from jsonmerge import merge
import argparse
#import matplotlib.pyplot as plt
#import numpy as np

# ----------
# Constants
# ----------
input_path                        = "//bayesianfil01/IDM/home/dbridenbecker/input/"
config_filename                   = "config.json"
input_filename                    = "RelationshipStart.csv"
pfa_results_filename              = "PFA_Results.csv"
base_assortivity_summary_filename = "Assortivity_Summary"
base_assortivity_details_filename = "Assortivity_Details"

# -------------------------------------------------------------------------
# The RelationshipType code here is intended to mimic the RelationshipType
# enum found in Relationship.h
# -------------------------------------------------------------------------
NumRelationshipTypes = 4

def RelationshipType_LookUpName( rel_type ):
    if rel_type == 0:
        return "TRANSITORY"
    elif rel_type == 1:
        return "INFORMAL"
    elif rel_type == 2:
        return "MARITAL"
    elif rel_type == 3:
        return "COMMERCIAL"
    else:
        raise exception( "Unknown relationshp type = " + str(rel_type) )

# ----------------------------------------------------------------------------------
# The ReceivedTestResultsType code here is intended to mimic ReceivedTestResultsType
# enum found in HIVEnums.h
# ----------------------------------------------------------------------------------
ReceivedTestResultsType = 3

def ReceivedTestResultsType_LookUpName( rr_type ):
    if rr_type == 0:
        return "UNKNOWN"
    elif rr_type == 1:
        return "POSITIVE"
    elif rr_type == 2:
        return "NEGATIVE"
    else:
        raise exception( "Unknown received test result type = " + str(rr_type) )

# ----------------------------------------------------------------------------------
# The Parameters class is intended to somewhat mimic the PairFormationParametersImpl
# class.  The one difference is that it also includes Assortivity information.
# One should note that this class is responsible for reading the data from JSON.
# ----------------------------------------------------------------------------------
class Parameters:

    def __init__(self, json_data):
        self.num_bins_m = 0
        self.num_bins_f = 0
        self.initial_age_years_m   = 0.0
        self.initial_age_years_f   = 0.0
        self.age_increment_years_m = 0.0
        self.age_increment_years_f = 0.0
        self.age_bins_m = []
        self.age_bins_f = []
        self.joint_probs = []
        self.group = ""
        self.property_name = ""
        self.axes_names = []

        self.num_bins_m            = json_data[ "Number_Age_Bins_Male"           ]
        self.num_bins_f            = json_data[ "Number_Age_Bins_Female"         ]
        self.initial_age_years_m   = json_data[ "Age_of_First_Bin_Edge_Male"     ]
        self.initial_age_years_f   = json_data[ "Age_of_First_Bin_Edge_Female"   ]
        self.age_increment_years_m = json_data[ "Years_Between_Bin_Edges_Male"   ]
        self.age_increment_years_f = json_data[ "Years_Between_Bin_Edges_Female" ]
        self.joint_probs           = json_data[ "Joint_Probabilities"  ]
        self.InitializeAgeBins(self.num_bins_m,self.initial_age_years_m,self.age_increment_years_m,self.age_bins_m)
        self.InitializeAgeBins(self.num_bins_f,self.initial_age_years_f,self.age_increment_years_f,self.age_bins_f)
        self.InitializeCumulativeProbabilities()

        self.group = json_data[ "Assortivity" ][ "Group" ]
        if self.group != "NO_GROUP":
            self.axes_names = json_data[ "Assortivity" ][ "Axes" ]
        if self.group == "INDIVIDUAL_PROPERTY" :
            self.property_name = json_data[ "Assortivity" ][ "Property_Name" ]

    def InitializeAgeBins( self, num_bins, initial_age_years, age_increment_years, age_bins ):
        for bin_index in range( num_bins ):
            age_bins.append( initial_age_years + bin_index * age_increment_years )

    def InitializeCumulativeProbabilities( self ):
        if len(self.joint_probs) != self.num_bins_m:
            raise exception( "# marg_probs rows(" +str(len(self.joint_probs))+") != num_bins_m(="+str(self.num_bins_m)+")")
        if len(self.joint_probs[0]) != self.num_bins_f:
            raise exception( "# marg_probs colums(" +str(len(self.joint_probs[0]))+") != num_bins_f(="+str(self.num_bins_f)+")")

        for row in range( self.num_bins_m ):
            cumulative = 0.0
            for col in range( self.num_bins_f ):
                cumulative += self.joint_probs[row][col]

            for col in range( self.num_bins_f ):
                if cumulative > 0.0:
                    self.joint_probs[row][col] = self.joint_probs[row][col] / cumulative;


    def BinIndexForAge( self, age_bins, age_years ):
        for bin_index in range( len( age_bins ) ):
            if age_years < age_bins[ bin_index ]:
                return bin_index
        return len( age_bins ) -1

    def GetBinIndexForAgeMale( self, age_years ):
        return self.BinIndexForAge( self.age_bins_m, age_years )

    def GetBinIndexForAgeFemale( self, age_years ):
        return self.BinIndexForAge( self.age_bins_f, age_years )

    def GetAgeFromIndexMale( self, index ):
        age_m = self.initial_age_years_m + float(index) * self.age_increment_years_m
        return age_m

    def GetAgeFromIndexFemale( self, index ):
        age_f = self.initial_age_years_f + float(index) * self.age_increment_years_f
        return age_f

    def GetAssortivityNumAxes( self ):
        return len(self.axes_names)

    def GetAssortivityAxesNames( self ):
        return self.axes_names

    def IsNoGroup( self ):
        return self.group == "NO_GROUP"

    # PropIndex is the index into the array of possible Assortivity property matches.
    # That is, the property data is expected to be stored in an array and this index
    # is used to access it.  One can imagine the array as a matrix that is num_axes by num_axes
    # where the rows are the male attribute and the columns are the female attribute.
    def GetPropIndex( self, rel_data ):
        prop_index = 0
        if self.group == "STI_INFECTION_STATUS" or self.group == "HIV_INFECTION_STATUS":
            if rel_data.infected_A and not(rel_data.infected_B) :
                prop_index = 1
            elif not(rel_data.infected_A) and rel_data.infected_B :
                prop_index = 2
            elif not(rel_data.infected_A) and not(rel_data.infected_B) :
                prop_index = 3
        elif self.group == "INDIVIDUAL_PROPERTY":
            val_A      = self.GetPropValue( rel_data.props_A )
            val_B      = self.GetPropValue( rel_data.props_B )
            index_A    = self.GetAxisIndex( val_A )
            index_B    = self.GetAxisIndex( val_B )
            num_axes   = self.GetAssortivityNumAxes()
            prop_index = index_A*num_axes + index_B
        elif self.group == "STI_COINFECTION_STATUS":
            if rel_data.STI_CoInfected_A and not(rel_data.STI_CoInfected_B) :
                prop_index = 1
            elif not(rel_data.STI_CoInfected_A) and rel_data.STI_CoInfected_B :
                prop_index = 2
            elif not(rel_data.STI_CoInfected_A) and not(rel_data.STI_CoInfected_B) :
                prop_index = 3
        elif self.group == "HIV_TESTED_POSITIVE_STATUS":
            if rel_data.HIV_Tested_Positive_A and not(rel_data.HIV_Tested_Positive_B) :
                prop_index = 1
            elif not(rel_data.HIV_Tested_Positive_A) and rel_data.HIV_Tested_Positive_B :
                prop_index = 2
            elif not(rel_data.HIV_Tested_Positive_A) and not(rel_data.HIV_Tested_Positive_B) :
                prop_index = 3
        elif self.group == "HIV_RECEIVED_RESULTS_STATUS":
            index_A    = self.GetAxisIndex( rel_data.HIV_Received_Results_A )
            index_B    = self.GetAxisIndex( rel_data.HIV_Received_Results_B )
            num_axes   = self.GetAssortivityNumAxes()
            prop_index = index_A*num_axes + index_B
        return prop_index

    # Return the value of the property defined in the Assortivity data.
    # The props value is the value returned by the method PropertiesToStringCsvFriendly()
    # where (property,value) pairs are separated by dashes and the pairs are separated
    # by semi-colons.  For example, "Race-HUMAN;Relationship-00"
    def GetPropValue( self, props ):
        props_array = props.split(";")
        for prop_val in props_array:
            prop_val_array = prop_val.split("-")
            if self.property_name == prop_val_array[0]:
                return prop_val_array[1]
        raise exception("Could not find Property_Name="+self.property_name+" in props="+props)

    # Return the index in the axes_names array for the given axis/property value name        
    def GetAxisIndex( self, axis_name ):
        for index in range( len(self.axes_names) ):
            if axis_name == self.axes_names[ index ]:
                return index
        raise exception("Could not find axis_name="+axis_name+" from axes_names="+self.axes_names)

# ---------------------------------------------------------------------------------------
# Demographics is the class responsible for getting data out of the demographics file.
# It knows how to get the file names out of the config.json file and to parse the files
# themselves.  HOWEVER, it ASSUMES that the 'Society' element is within the 'Default'
# element.
# ---------------------------------------------------------------------------------------
class Demographics:
    input_dir = ""
    demo_fn_array = []
    geo_name = ""
    society_json = {}

    def __init__( self, inputPath ):
        self.input_dir = inputPath

    def ReadConfigFile( self, config_fn ):
        config_file = open( config_fn, "r" )
        config_json = json.loads( config_file.read() )
        self.demo_fn_array = config_json["parameters"]["Demographics_Filenames"]
        self.geo_name = config_json["parameters"]["Geography"]
        config_file.close()
    
    def ReadSocietyData( self ):
        #demo_fn_array = self.demo_fn_str.split(";")
        overlayed_json = {}
        for demo_fn in self.demo_fn_array:
            filename = demo_fn
            if os.path.isfile( filename ) == False :
                filename = os.path.join( os.path.join( self.input_dir, self.geo_name ), filename )
            #print( "filename = " + filename + "\n" )
            demo_file = open( filename, "r" )
            demo_json = json.loads( demo_file.read() )
            overlayed_json = merge( overlayed_json, demo_json )
            demo_file.close()

        if "Defaults" in overlayed_json.keys():
            if "Society" in overlayed_json["Defaults"].keys():
                self.society_json = overlayed_json["Defaults"]["Society"] # !!! GOT DATA !!!
        else:
            raise exception("Could not find Society element in demographics")

    def GetParameters( self ):
        param_list = []
        for rel_type in range(NumRelationshipTypes):
            rel_params_name = RelationshipType_LookUpName( rel_type )
            param = Parameters( self.society_json[ rel_params_name ]["Pair_Formation_Parameters"] )
            param_list.append( param )
        return param_list

# -------------------------------------------------------------
# RelData is the Relationship data extracted from the CSV file
# for a given relationship
# -------------------------------------------------------------
class RelData:
    start_time = -1.0
    end_time = -1.0
    id_A = -1
    id_B = -1
    gender_A = -1
    gender_B = -1
    age_A = -1.0
    age_B = -1.0
    infected_A = -1
    infected_B = -1
    prop_A = ""
    prop_B = ""
    STI_CoInfected_A = -1
    STI_CoInfected_B = -1
    HIV_tested_positive_A = -1
    HIV_Tested_Positive_B = -1
    HIV_received_results_A = ""
    HIV_received_results_B = ""
    rel_type = -1

def GetColumnIndexesFromHeader( header ):
    used_columns = []
    used_columns.append( "Rel_start_time"         )
    used_columns.append( "Rel_scheduled_end_time" )
    used_columns.append( "Rel_type"               )
    used_columns.append( "A_ID"                   )
    used_columns.append( "B_ID"                   )
    used_columns.append( "A_gender"               )
    used_columns.append( "B_gender"               )
    used_columns.append( "A_age"                  )
    used_columns.append( "B_age"                  )
    used_columns.append( "A_is_infected"          )
    used_columns.append( "B_is_infected"          )
    used_columns.append( "A_IndividualProperties" )
    used_columns.append( "B_IndividualProperties" )
    used_columns.append( "A_STI_CoInfection"      )
    used_columns.append( "B_STI_CoInfection"      )
    used_columns.append( "A_HIV_Tested_Positive"  )
    used_columns.append( "B_HIV_Tested_Positive"  )
    used_columns.append( "A_HIV_Received_Results" )
    used_columns.append( "B_HIV_Received_Results" )

    header = header.replace('\n','')
    header_array = header.split( ',' )
    col_indexes_map = {}
    for index in range( len( header_array ) ):
        ch = header_array[ index ]
        for used in used_columns:
            if ch.startswith( used ):
                col_indexes_map[ used ] = index
                break
    return col_indexes_map
    
# --------------------------------------------------------------
# Read the file produced by StiRelationshipStart.cpp and return
# a list of RelData objects representing each line/relationship
# --------------------------------------------------------------
def ReadData( filename ):
    rel_data_list = []
    with open( filename ) as rs:
        header = rs.readline() # chew up header
        col_indexes_map = GetColumnIndexesFromHeader( header )
        for line in rs:
            line = line.replace('\n','')
            line_array = line.split( ',' )

            data = RelData()
            data.start_time             = float(line_array[ col_indexes_map["Rel_start_time"        ] ])
            data.end_time               = float(line_array[ col_indexes_map["Rel_scheduled_end_time"] ])
            data.rel_type               = int(  line_array[ col_indexes_map["Rel_type"              ] ])
            data.id_A                   = int(  line_array[ col_indexes_map["A_ID"                  ] ])
            data.id_B                   = int(  line_array[ col_indexes_map["B_ID"                  ] ])
            data.gender_A               = int(  line_array[ col_indexes_map["A_gender"              ] ])
            data.gender_B               = int(  line_array[ col_indexes_map["B_gender"              ] ])
            data.age_A                  = float(line_array[ col_indexes_map["A_age"                 ] ])
            data.age_B                  = float(line_array[ col_indexes_map["B_age"                 ] ])
            data.infected_A             = int(  line_array[ col_indexes_map["A_is_infected"         ] ])
            data.infected_B             = int(  line_array[ col_indexes_map["B_is_infected"         ] ])
            data.props_A                =       line_array[ col_indexes_map["A_IndividualProperties"] ]
            data.props_B                =       line_array[ col_indexes_map["B_IndividualProperties"] ]

            if header.find( "A_STI_CoInfection" ) > 0:
                data.STI_CoInfected_A       = int(  line_array[ col_indexes_map["A_STI_CoInfection"     ] ] )
                data.STI_CoInfected_B       = int(  line_array[ col_indexes_map["B_STI_CoInfection"     ] ] )
                data.HIV_Tested_Positive_A  = int(  line_array[ col_indexes_map["A_HIV_Tested_Positive" ] ] )
                data.HIV_Tested_Positive_B  = int(  line_array[ col_indexes_map["B_HIV_Tested_Positive" ] ] )
                data.HIV_Received_Results_A =       line_array[ col_indexes_map["A_HIV_Received_Results"] ]
                data.HIV_Received_Results_B =       line_array[ col_indexes_map["B_HIV_Received_Results"] ]

            #print "time={0}, id_A={1}, id_B={2}, age_A={3}, age_B={4}".format( data.start_time, data.id_A, data.id_B, data.age_A, data.age_B )

            #if( data.start_time > 5000 ):
            rel_data_list.append( data )

    return rel_data_list


# --------------------------------------------------------------------------------------------
# --- "Koehler and Larntz suggest that if the total number of observations is at least 10, 
# --- the number categories is at least 3, and the square of the total number of observations 
# --- is at least 10 times the number of categories, then the chi-square approximation should
# --- be reasonable."
# ---
# --- "Care should be taken when cell categories are combined (collapsed together) to fix
# --- problems of small expected cell frequencies. Collapsing can destroy evidence of
# --- non-independence, so a failure to reject the null hypothesis for the collapsed table 
# --- does not rule out the possibility of non-independence in the original table."
# ---
# --- http://www.basic.northwestern.edu/statguidefiles/gf-dist_ass_viol.html
# --------------------------------------------------------------------------------------------
class ChiSquare:
    df = -1
    critical_value = -1.0
    statistic = -1.0

    def ReduceDataForChiSquareStatistic( self, minCategorySize, inExp, inAct ):
        out_exp = []
        out_act = []

        #print( inExp )

        exp_next = 0.0
        act_next = 0.0
        for i in range( len( inExp ) ):
            if (inExp[i] < minCategorySize) and ((i+1) == len(inExp)):
                if not((len(out_exp) == 0) and ((i+1) == len(inExp))):
                    if (inExp[i] + exp_next) > minCategorySize:
                        out_exp.append( inExp[i] + exp_next )
                        out_act.append( inAct[i] + act_next )
                    else:
                        out_exp[ len(out_exp)-1 ] += inExp[i] + exp_next
                        out_act[ len(out_act)-1 ] += inAct[i] + act_next

                    exp_next = 0.0 ;
                    act_next = 0.0 ;
            elif inExp[i] < minCategorySize:
                exp_next += inExp[i]
                act_next += inAct[i]
                if exp_next > minCategorySize:
                    out_exp.append( exp_next )
                    out_act.append( act_next )
                    exp_next = 0.0
                    act_next = 0.0 
            else:
                if exp_next > minCategorySize:
                    out_exp.append( exp_next )
                    out_act.append( act_next )
                else:
                    out_exp.append( inExp[i] + exp_next )
                    out_act.append( inAct[i] + act_next )

                exp_next = 0.0
                act_next = 0.0

        if exp_next > 0.0:
            out_exp.append( exp_next );
            out_act.append( act_next );

        #print( out_exp )
        #print("\n")

        return out_exp, out_act

    # --------------------------------------------------------------------------------------
    # --- http://www.itl.nist.gov/div898/handbook/eda/section3/eda3674.htm
    # --- Upper-tail critical values of chi-square distribution with ? degrees of freedom 
    # --- for v = 0.95, 0.975
    # --------------------------------------------------------------------------------------
    def GetChiSquareCriticalValue( self, dof ):
        if dof < 1:
            return -1.0 ;

        chi_sq_crit_val_095 =  [   3.841,  5.991,  7.815,  9.488, 11.070, 
                                  12.592, 14.067, 15.507, 16.919, 18.307,
                                  19.675, 21.026, 22.362, 23.685, 24.996,
                                  26.296, 27.587, 28.869, 30.144, 31.410,
                                  32.671, 33.924, 35.173, 36.415, 37.653 ]

        chi_sq_crit_val_0975 = [  5.042,  7.378,  9.348, 11.143, 12.833,
                                 14.449, 16.013, 17.535, 19.023, 20.483, 
                                 21.920, 23.337, 24.736, 26.119, 27.488,
                                 28.845, 30.191, 31.526, 32.852, 34.170,
                                 35.479, 36.781, 38.076, 39.364, 40.647 ]

        dof_index = dof - 1
        print dof_index
        return chi_sq_crit_val_0975[ dof_index ]


    def __init__( self, minCategorySize, values_exp, values_act ):
        if len( values_exp ) != len( values_act ):
            raise exception( "# exp("+str(len(values_exp))+") != # act("+str(len(values_exp))+")" )

        exp, act = values_exp, values_act
        exp, act = self.ReduceDataForChiSquareStatistic( minCategorySize, exp, act )

        self.df = len(exp) - 1
        self.statistic = 0.0
        for i in range( len(exp) ):
            if exp[i] > 0.0:
                self.statistic += (exp[i] - act[i]) * (exp[i] - act[i]) / exp[i]
        #print( "df="+str(self.df)+"  statistic="+str(self.statistic)+"\n")

        self.critical_value = self.GetChiSquareCriticalValue( self.df )

# -----------------------------------------------------------------------
# -----------------------------------------------------------------------

#def plotBunch( all_data, plot_name, baseline_data=None ):
#    num_chans = all_data[0]["Header"]["Channels"]
#    plt.suptitle( plot_name )
#    square_root = 4
#    if num_chans > 30:
#        square_root = 6
#    elif num_chans > 16:
#        square_root = 5
#    plots = []
#    labels = []
#
#    ncols = square_root
#    nrows = num_chans / square_root
#    if nrows < 1:
#        nrows = 1
#
#    idx = 0
#    for chan_title in sorted(all_data[0]["Channels"]):
#        idx_x = idx%square_root
#        idx_y = int(idx/square_root)
#
#        try:
#            subplot = plt.subplot2grid( (nrows,ncols), (idx_y,idx_x)  ) 
#            colors = [ 'b', 'g', 'c', 'm', 'y', 'k' ]
#
#            if baseline_data is not None:
#                tstep = 1
#                if( "Simulation_Timestep" in baseline_data["Header"] ):
#                    tstep = baseline_data["Header"]["Simulation_Timestep"]
#                x_len = len( baseline_data["Channels"][chan_title]["Data"] )
#                x_data = np.arange( 0, x_len*tstep, tstep )
#                plots.append( subplot.plot( x_data, baseline_data["Channels"][chan_title]["Data"], 'r-', linewidth=2 ) )
#
#            for sim_idx in range(0,len(all_data)):
#                labels.append(str(sim_idx))
#
#                x_len = len( all_data[sim_idx]["Channels"][chan_title]["Data"] )
#
#                tstep = 1
#                if( "Simulation_Timestep" in all_data[sim_idx]["Header"] ):
#                    tstep = all_data[sim_idx]["Header"]["Simulation_Timestep"]
#
#                x_data = np.arange( 0, x_len*tstep, tstep )
#
#                plots.append( subplot.plot( x_data, all_data[sim_idx]["Channels"][chan_title]["Data"], colors[sim_idx%len(colors)] + '-' ) )
#
#            plt.title( chan_title )
#        except Exception as ex:
#            print str(ex)
#        if idx == (square_root*square_root)-1:
#            break
#
#        idx += 1
#
#    #plt.legend( plots, labels )
#
#    #plt.set_size( 'xx-small' )
#    plt.subplots_adjust( left=0.04, right=0.99, bottom=0.05, top =0.91, wspace=0.3, hspace=0.3 )
#    #pylab.savefig( plot_name.replace( " ", "_" ) + ".png", bbox_inches='tight', orientation='landscape' )
#    plt.show()
#    # print( "Exiting from plotBunch.\n" )


# -----------------------------------------------------------------------
# Write the pair forming agent results to a file.  The results compare
# the expected distribution of relationships to the actual distribution.
# The method uses the Chi-Square statistic to compare the distributions.
# This comparison is based on the logic in BehaviorPfa.cpp.
# -----------------------------------------------------------------------
def WritePfaResults( filename, params_list, rel_count_fbin_mbin_type ):

    exp_fn = "output/PFA_expected.csv"
    exp_file = open( exp_fn, "w" )
    with open( filename, "w" ) as output_file:
        # -----------------------------------------------------
        # Write one matrix of counts and chi-square statistics 
        # for each relationship type
        # -----------------------------------------------------

        for rel_type in range(NumRelationshipTypes):

            act_data = []
            act_data.append( {} )
            act_data[0]["Header"] = {}
            act_data[0]["Header"]["Channels"] = params_list[rel_type].num_bins_m
            act_data[0]["Header"]["Simulation_Timestep"] = 1
            act_data[0]["Channels"] = {}

            exp_data = {}
            exp_data["Header"] = {}
            exp_data["Header"]["Channels"] = params_list[rel_type].num_bins_m
            exp_data["Header"]["Simulation_Timestep"] = 1
            exp_data["Channels"] = {}

            # ------------
            # Write Header
            # ------------
            rel_str = RelationshipType_LookUpName( rel_type )
            output_file.write( rel_str )
            exp_file.write( rel_str )
             
            for female_bin_index in range(params_list[rel_type].num_bins_f):
                age_f = params_list[rel_type].initial_age_years_f + float(female_bin_index)*params_list[rel_type].age_increment_years_f
                output_file.write( ",F=" + str(age_f) )
                exp_file.write( ",F=" + str(age_f) )

            output_file.write( ",df,Crit_Val,CS_Stat,Pass" )
            output_file.write( "\n" )
            exp_file.write( "\n" )

            # -------------------------------------------------------------
            # Write Matrix - male bins are rows and female bins are columns
            # -------------------------------------------------------------
            for male_bin_index in range(params_list[rel_type].num_bins_m):
                age_m = params_list[rel_type].GetAgeFromIndexMale( male_bin_index )
                age_m_str = "M=" + str(age_m)
                act_data[0]["Channels"][age_m_str] = {}
                act_data[0]["Channels"][age_m_str]["Data"] = []
                exp_data["Channels"][age_m_str] = {}
                exp_data["Channels"][age_m_str]["Data"] = []

                total_females_for_this_age_male = 0
                for female_bin_index in range(params_list[rel_type].num_bins_f):
                    act = rel_count_fbin_mbin_type[ rel_type ][ male_bin_index ][ female_bin_index ]
                    total_females_for_this_age_male += act

                ftotal_females_for_this_age_male = float(total_females_for_this_age_male)
                actual = []
                expected = []
                output_file.write( age_m_str )
                exp_file.write( age_m_str )

                for female_bin_index in range(params_list[rel_type].num_bins_f):
                    act = rel_count_fbin_mbin_type[ rel_type ][ male_bin_index ][ female_bin_index ]
                    output_file.write( "," + str( act ) )
                    exp_file.write( "," + str( act ) )
                    
                    # ------------------------------------
                    # Save data for Chi-Square calculation
                    # ------------------------------------
                    fact = float(act)
                    mp_val = params_list[ rel_type ].joint_probs[ male_bin_index ][ female_bin_index ]
                    fexp = ftotal_females_for_this_age_male * mp_val
                    #print( str(ftotal_females_for_this_age_male) + " * " + str(mp_val) + " = " + str(fexp) + "\n" )
                    actual.append( fact )
                    expected.append( fexp )

                    act_data[0]["Channels"][age_m_str]["Data"].append( fact )
                    exp_data["Channels"][age_m_str]["Data"].append( fexp )

                # -----------------------------------------------------
                # Calculate Chi-Square Statistic and determine
                # if the distribution of females for this male age bin
                # is consistent with our input/parameter values
                # -----------------------------------------------------

                cs = ChiSquare( 5.0, expected, actual )

                passed = cs.critical_value > cs.statistic

                output_file.write( "," + str(cs.df) )
                output_file.write( "," + str(cs.critical_value) )
                output_file.write( "," + str(cs.statistic) )
                output_file.write( "," + str(passed) )
                output_file.write( "\n" )
                exp_file.write( "\n" )

            output_file.write( "\n" )
            output_file.write( "\n" )

            exp_file.write( "\n" )
            exp_file.write( "\n" )

            #plot_title = RelationshipType_LookUpName( rel_type )
            #plotBunch( act_data, plot_title, exp_data )

    exp_file.close()

# -------------------------------------------------------------------------------
# Write a summary of the Assortivity results.  This is the break down of how
# relationships were formed based on a preference for a given attribute.
# That is, a person with attribute A may lean towards partners with attribute B.
# The summary just shows the data for each male age bin.
# -------------------------------------------------------------------------------
def WriteAssortivityResultsSummary( filename, params, rel_count_prop_fbin_mbin ):
    with open( filename, "w" ) as output_file:
        # --------------
        # Write Header
        # --------------
        header = "Male_Age_Bin,Total"
        axis_names = params.GetAssortivityAxesNames()
        for name1 in axis_names:
            for name2 in axis_names:
                header += ","+name1+"-"+name2
        for name1 in axis_names:
            for name2 in axis_names:
                header += ","+name1+"-"+name2+"%"

        output_file.write( header + "\n")

        num_axes = params.GetAssortivityNumAxes()
        num_props = num_axes * num_axes

        for male_bin_index in range(len(rel_count_prop_fbin_mbin)) :
            total_bin_m = 0
            total_prop_bin = [] ;
            for prop_index in range(num_props):
                total_prop_bin.append(0)

            for female_bin_index in range(len( rel_count_prop_fbin_mbin[ male_bin_index ])):
                total_bin_f = 0
                for prop_index in range(num_props):
                    val = rel_count_prop_fbin_mbin[ male_bin_index ][ female_bin_index ][ prop_index ]
                    total_bin_f += val
                    total_prop_bin[ prop_index ] += val ;
                total_bin_m += total_bin_f ;

            percent_bin = []
            for prop_index in range(num_props):
                percent = 0.0
                if total_bin_m > 0:
                    percent =  float(total_prop_bin[prop_index]) / float(total_bin_m)
                percent_bin.append( percent )

            male_age = params.GetAgeFromIndexMale( male_bin_index )
            output_file.write(       str(male_age) )
            output_file.write( "," + str(total_bin_m) )
            for prop_index in range(num_props):
                output_file.write( "," + str(total_prop_bin[ prop_index ]) )
            for prop_index in range(num_props):
                output_file.write( "," + str(percent_bin[ prop_index ]) )
            output_file.write("\n")

# -------------------------------------------------------------------------------
# Write the details of the assortivity results.  This will show the assortivity
# preferences for each male/female age bin.
# -------------------------------------------------------------------------------
def WriteAssortivityResultsDetails( filename, params, rel_count_prop_fbin_mbin ):
    with open( filename, "w" ) as output_file:
        # --------------
        # Write Header
        # --------------
        header = "Male_Age_Bin,Female_Age_Bin,Total"
        axis_names = params.GetAssortivityAxesNames()
        for name1 in axis_names:
            for name2 in axis_names:
                header += ","+name1+"-"+name2

        output_file.write( header + "\n")

        num_axes = params.GetAssortivityNumAxes()
        num_props = num_axes * num_axes

        for male_bin_index in range(len(rel_count_prop_fbin_mbin)) :
            age_m = params.GetAgeFromIndexMale( male_bin_index )

            for female_bin_index in range(len( rel_count_prop_fbin_mbin[ male_bin_index ])):
                age_f = params.GetAgeFromIndexFemale( female_bin_index )

                total_bin_f = 0
                total_prop_bin = [] ;
                for prop_index in range(num_props):
                    total_prop_bin.append(0)

                for prop_index in range(num_props):
                    val = rel_count_prop_fbin_mbin[ male_bin_index ][ female_bin_index ][ prop_index ]
                    total_bin_f += val
                    total_prop_bin[ prop_index ] += val ;

                output_file.write(       str(age_m) )
                output_file.write( "," + str(age_f) )
                output_file.write( "," + str(total_bin_f) )
                for prop_index in range(num_props):
                    output_file.write( "," + str(total_prop_bin[ prop_index ]) )
                output_file.write("\n")
            output_file.write("\n")

# -----------------------------------------------------------------------------
# The main application reads the config.json file to get the demographics data.
# It then reads the relationship data and produces PFA and Assortivity results.
# -----------------------------------------------------------------------------
def application():

    print( "You are here! " + os.getcwd() )

    #parser = argparse.ArgumentParser()
    #parser.add_argument('output_dir', help='Directory containing data.  One directory up should contain config.json.')
    #args = parser.parse_args()
    #
    #output_dir = args.output_dir
    output_dir = "output"

    input_fn                    = os.path.join( output_dir, input_filename                    )
    pfa_results_fn              = os.path.join( output_dir, pfa_results_filename              )
    base_assortivity_summary_fn = os.path.join( output_dir, base_assortivity_summary_filename )
    base_assortivity_details_fn = os.path.join( output_dir, base_assortivity_details_filename )
    
    if os.path.isfile( input_fn ) == False :
        print( "!!!! Can't open " + input_fn +"!!!!" )
        return

    # ------------------------------------------------------
    # Read the parameter information used in the simulation
    # ------------------------------------------------------
    print( "read demog" )
    demog = Demographics( input_path )

    demog.ReadConfigFile( config_filename )
    demog.ReadSocietyData()
    params_list = demog.GetParameters()

    print( "got demog" )
    # -----------------------------
    # Collect data from input file
    # -----------------------------
    rel_data_list = ReadData( input_fn )

    print( "read data") 
    # ---------------------------
    # Initialize PFA counting matrix
    # ---------------------------
    rel_count_fbin_mbin_type = []
    for rel_type in range(NumRelationshipTypes):
        rel_count_fbin_mbin = []
        for male_bin_index in range(params_list[rel_type].num_bins_m):
            rel_count_fbin = []
            for female_bin_index in range(params_list[rel_type].num_bins_f):
                rel_count_fbin.append(0)
            rel_count_fbin_mbin.append( rel_count_fbin )
        rel_count_fbin_mbin_type.append( rel_count_fbin_mbin )

    # --------------------------------------
    # Initialize Assortivity counting matrix
    # --------------------------------------
    rel_count_prop_fbin_mbin_type = []
    for rel_type in range(NumRelationshipTypes):
        rel_count_prop_fbin_mbin = []
        for male_bin_index in range(params_list[rel_type].num_bins_m):
            rel_count_prop_fbin = []
            for female_bin_index in range(params_list[rel_type].num_bins_f):
                num_axes = params_list[ rel_type ].GetAssortivityNumAxes()
                num_props = num_axes * num_axes
                rel_count_prop = []
                for i in range(num_props):
                    rel_count_prop.append(0)
                rel_count_prop_fbin.append(rel_count_prop)
            rel_count_prop_fbin_mbin.append( rel_count_prop_fbin )
        rel_count_prop_fbin_mbin_type.append( rel_count_prop_fbin_mbin )

    # ---------------------------------
    # Count relationships in each bin
    # ---------------------------------
    for rel_data in rel_data_list:
        rel_type         = rel_data.rel_type ;
        male_bin_index   = params_list[ rel_type ].GetBinIndexForAgeMale( rel_data.age_A )
        female_bin_index = params_list[ rel_type ].GetBinIndexForAgeFemale( rel_data.age_B )
        #print "rt="+str(rel_type)+"  mbi="+ str(male_bin_index) + "  fbi="+str(female_bin_index)
        rel_count_fbin_mbin_type[ rel_type ][ male_bin_index ][ female_bin_index ] += 1

        num_axes = params_list[ rel_type ].GetAssortivityNumAxes()
        if num_axes > 0:
            prop_index = params_list[ rel_type ].GetPropIndex( rel_data )
            rel_count_prop_fbin_mbin_type[ rel_type ][ male_bin_index ][ female_bin_index ][ prop_index ] += 1

    print("done counting")

    # -------------------------------
    # Write the PFA results to a file
    # -------------------------------
    WritePfaResults( pfa_results_fn, params_list, rel_count_fbin_mbin_type )

    print( "wrote pfa" )
    for rel_type in range(NumRelationshipTypes):
        if not params_list[ rel_type ].IsNoGroup():
            rel_name = RelationshipType_LookUpName( rel_type )
            sum_fn = base_assortivity_summary_fn + "_" + rel_name + ".csv"
            det_fn = base_assortivity_details_fn + "_" + rel_name + ".csv"
            WriteAssortivityResultsSummary( sum_fn, params_list[ rel_type ], rel_count_prop_fbin_mbin_type[ rel_type ] )
            WriteAssortivityResultsDetails( det_fn, params_list[ rel_type ], rel_count_prop_fbin_mbin_type[ rel_type ] )

    print ("post processing complete")

if __name__ == "__main__":
    application()
