#!/usr/bin/python
import os
from sre_constants import SUCCESS
from tracemalloc import start
if __name__ == '__main__':
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_STI_Support as sti_support
import dtk_test.dtk_RelationshipTargeting_Support as rt_support
import dtk_test.dtk_InsetChart as InsetChart
from dtk_test.dtk_OutputFile import ReportEventRecorder
import json
import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
from itertools import cycle
import argparse

"""
Output: scientific_feature_report.txt
   Validate:
    1.1:  "HasIP_StartNewRels:CREATED" channel and "HasIP_StartNewRels:YAY_SINGLE" are directly related, 
      this is, the sum of both values at any point of time should be equals to 1 with a buffer 
      for failure determined by error_index - TODO: Check with Dan.

    1.2 Test reports - Is_rel_outside_PFA column must have been added to the next reports:
        Test data: RelationshipStart.csv
        ReportEventRecorder.csv

    1.3 Validate number of relationships before and after the Start Day for StartNewRelationship interventions. 
        i.e.  at day 99 then at day 100 the number should have changed.
    1.4 Plot Related channels and save figures. 

    Tot scaled down pop: 1739*5 = 8695 

"""

start_day = 100
error_index = .00000015
failures = []

def create_report_file( start_df, end_df, event_df, 
                        end_report_name, start_report_name, event_report_name,
                        config_filename, dtk_post_process_report_name, debug=False):
    succeed = True
   
    with open(dtk_post_process_report_name, 'w') as pp_log:
        print_log(pp_log,f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")
        print_log(pp_log,f"1. Collecting the existing relationship pairs when intervention happens from "
                                 f"{start_report_name} and {end_report_name}.\n")
        print_log(pp_log,f"2. Validating {event_report_name}")

        # ------ REPORT VALIDATIONS --------- :

        # 1)  RelationshipStart.csv - Filter by RelationshipType
        start_df_marital = start_df[(start_df[rt_support.Constant.rel_type] == rt_support.Constant.marital_type) \
            & (start_df[rt_support.Constant.is_rel_outside_pfa] == "T" )]
        start_df_informal = start_df[(start_df[rt_support.Constant.rel_type] == rt_support.Constant.informal_type) \
            & (start_df[rt_support.Constant.is_rel_outside_pfa] == "T" )]
        start_df_commercial = start_df[(start_df[rt_support.Constant.rel_type] == rt_support.Constant.commercial_type) \
            & (start_df[rt_support.Constant.is_rel_outside_pfa] == "T" )]
        start_df_transitory = start_df[(start_df[rt_support.Constant.rel_type] == rt_support.Constant.transitory_type) \
            & (start_df[rt_support.Constant.is_rel_outside_pfa] == "T" )]

        start_df_AllOutsidePFA = start_df[start_df[rt_support.Constant.is_rel_outside_pfa] == "T" ]    # All rels inside PFA
        # expec_num_rels = len(start_df_AllOutsidePFA)            # NonPFA rels Started Rels minus PFA ones.
        
        # # TODO: Determine if we need to evaluate the total number of relationships being created.


        # 2) RelationshipEnd.Csv - column updates 
        end_df_AllOutPFA = end_df[end_df[rt_support.Constant.is_rel_outside_pfa] == "T" ]
        if len(end_df_AllOutPFA) < 1: 
            failures_append(pp_log, "Error on RelationshipEnd.Csv data, no values were found")  

        # 3) Relationship_Created  in ReportEventRecorded.csv (event_df) #TODO: Ask Dan if this is a correct validation.
        event_df_SNR = event_df[event_df[rt_support.Constant.event_name] == "Relationship_Created" ]
        print_log(pp_log, f"Validating ReportEventRecorded.csv - StartNewRelationships related events: {len(event_df_SNR)} - "
                        f"VERSUS Non-PFA started relationships entries in RelationshipStart.csv:  {len(start_df_AllOutsidePFA)} ")
        if len(event_df_SNR)!= len(start_df_AllOutsidePFA):
            errtxt =  f"Error on RelationshipEnd.Csv data, no values were found"
            failures_append(pp_log, errtxt)  
            print(errtxt)


        # --------- CHANNELS VALIDATIONS# --------- :

        InsetChart_path = os.path.join(os.getcwd(), 'output', 'InsetChart.json')
        print_log(pp_log, f"File path: {InsetChart_path}")
        results = InsetChart.InsetChart(file=InsetChart_path)

        # Validation of StartNewRelationships creation AT expected times.
        test_cases =  [["Active MARITAL Relationships", 100, "F", "MARITAL"],
                        ["Active COMMERCIAL Relationships", 200, "M", "COMMERCIAL"],
                        ["Active INFORMAL Relationships", 300, "M", "INFORMAL"],
                        ["Active TRANSITORY Relationships", 400, "F", "TRANSITORY"]]

        
        print_log(pp_log, "**** Evaluating InsertChart file data:")
        for case in test_cases:
            channel_name = case[0]
            start_day = case[1]
            test_channel = np.array(results.load_channel_data(channel_name))
            print_log(pp_log, f"Evaluating data for {channel_name} channel: At day {start_day-1} ({test_channel[start_day - 1]} relationships) "
                            f"- and - at day {start_day}  ({test_channel[start_day]} relationships)... ")
            if test_channel[start_day - 1]>= test_channel[start_day]: 
                failures_append(pp_log, f"Error generating {case[2]} StartNewRelationships outise PFA")
        
         # Validation 1.1 HasIP_StartNewRels:CREATED and HasIP_StartNewRels:YAY_SINGLE Correlation.
        c = np.array(results.load_channel_data("HasIP_StartNewRels:CREATED")).mean() \
            + np.array(results.load_channel_data("HasIP_StartNewRels:YAY_SINGLE")).mean() \
            + np.array(results.load_channel_data("HasIP_StartNewRels:IS_COMPLICATED")).mean()
                            
        print_log(pp_log, f"VALIDATE: The sum of the values at any time of StartNewRels should always be close to 100% (CREATED, "
                f"YAY_SINGLE and IS_COMPLICATED) Between {1 - error_index} and {1 + error_index}, \n\tCurrent value is: {c}")     
        if ((c < (1-error_index)) or (c > (1 + error_index))):
            failures_append(pp_log,  f"ERROR: Correlation between HasIP_StartNewRels:CREATED and HasIP_StartNewRels:YAY_SINGLE "
                                    f"is larger than tha allowed value, \nEXPECTED less than: {1-error_index}, ACTUAL {c}")

        ev_Marital = get_Relationship_Created(event_df, 100,'F')
        ev_Comm = get_Relationship_Created(event_df, 200,'M')
        ev_Inf = get_Relationship_Created(event_df, 300,'M')
        ev_Tran = get_Relationship_Created(event_df, 400,'F')
        
        # Created New Relationships - targets validation
        val1 = "ReportEventRecorder.csv Relationship_Created"
        val2 = "\n\t\tStartNewRelationship.csv relationship created"

        print_log(pp_log,f"\nMARITAL Female {val1}: {len(ev_Marital)} .VS. {val2} {len(start_df_marital)}  at step 100")  
        if len(ev_Marital)!=len(start_df_marital): failures_append(pp_log, f"MARITAL Events and Started Relationships don't match")
        
        print_log(pp_log,f"\nCOMMERCIAL Male {val1}: {len(ev_Comm)} .VS. {val2} {len(start_df_commercial)} at step 200")
        if len(ev_Comm)!=len(start_df_commercial): failures_append(pp_log, f"COMMERCIAL Events and Started Relationships don't match")
        
        print_log(pp_log,f"\nINFORMAL Male {val1}: {len(ev_Inf)} .VS. {val2} {len(start_df_informal)} at step 300")
        if len(ev_Inf)!=len(start_df_informal): failures_append(pp_log, f"INFORMAL Events and Started Relationships don't match")
        
        print_log(pp_log,f"\nTRANSITORY Female {val1}: {len(ev_Tran)} .VS. {val2} {len(start_df_transitory)} at step 400")
        if len(ev_Tran)!=len(start_df_transitory): failures_append(pp_log, f"TRANSITORY Events and Started Relationships don't match")

        
        # Evaluating StartNewRelationship Originator and Target are placed in the correct columns (A_ID for Males, B_ID for Females depending on the intervention):
        SNR_MARITAL = pd.merge( ev_Marital, start_df_marital, how="inner",  left_on="Individual_ID", right_on="B_ID")
        SNR_COMMERCIAL = pd.merge( ev_Comm, start_df_commercial, how="inner",  left_on="Individual_ID", right_on="A_ID")
        SNR_INFORMAL = pd.merge( ev_Inf, start_df_informal, how="inner",  left_on="Individual_ID", right_on="A_ID")
        SNR_TRANSITORY = pd.merge( ev_Tran, start_df_transitory, how="inner",  left_on="Individual_ID", right_on="B_ID")

        if len(SNR_MARITAL)!= len(ev_Marital):
            failures_append(pp_log, f"Wrong creation of new MARITAL Relationships, please verify the Relationship Originator ID is Female (B_ID) and the Target is the Male (A_ID")
        if len(SNR_COMMERCIAL)!= len(ev_Comm):
            failures_append(pp_log, f"Wrong creation of new COMMERCIAL Relationships, please verify the Relationship Originator ID is Male (A_ID) and the Target is the Female (B_ID")
        if len(SNR_INFORMAL)!= len(ev_Inf):
            failures_append(pp_log, f"Wrong creation of new INFORMAL Relationships, please verify the Relationship Originator ID is Male (A_ID) and the Target is the Female (B_ID")
        if len(SNR_TRANSITORY)!= len(ev_Tran):
            failures_append(pp_log, f"Wrong creation of new TRANSITORY Relationships, please verify the Relationship Originator ID is Female (B_ID) and the Target is the Male (A_ID")
        # Channels to be evaluated:
        channel_sets = {             
                    "Relationships_Plots": [
                        "Active MARITAL Relationships",
                        "Active COMMERCIAL Relationships",
                        "Active INFORMAL Relationships",
                        "Active TRANSITORY Relationships",
                        "Num Rels Outside PFA",
                        "Paired People",
                        "Number of Individuals Ever in a Relationship"
                        ]
                    ,
                    "HasIP_StartNewRels": [
                        "HasIP_StartNewRels:YAY_SINGLE",
                        "HasIP_StartNewRels:CREATED",
                        "HasIP_StartNewRels:IS_COMPLICATED",
                        ]
                    ,
                    "Fraction_of_New_Infections": [
                        "Fraction of New Infections From Rels Outside PFA"
                        ]
                }
        # # ------- Plotting -------
        for set in channel_sets:
            channels_plot(channel_sets[set], set, results)        
        
        # --------- Plot all channels -------
        plot_all(results.channels)

        # ------- Dump data to test file -------
        for set in channel_sets:
            channels = channel_sets[set]         
            for validate_channel in channels:
                print(validate_channel)
                print(results.load_channel_data(validate_channel))
        
    
        # --------- PRINT FAILURES ---------------
        for err in failures:
            print_log(pp_log, "====== ERRORS SUMMARY =======")
            print_log(pp_log, f"({err})")

        if len(failures) > 0: 
            succeed = False   
        
        pp_log.write(f"\nSUMMARY: Success={succeed}") # Do not delete this line
    
    return succeed
def print_log(file, log):
    log = f"\n{log}"
    file.write(log)
    print(log)

def failures_append(file, err):
    failures.append(f"{err}")
    err = f"ERROR!:: {err}"
    print_log(file, err)
    

# --------- Verify created relationships values end up on the right column
def get_Relationship_Created(events_df, on_step=100, gender='F'):
    # Marital: Validate for Relationship_Created Events there is 
    created_rels = pd.DataFrame(events_df[
                                            (events_df['Event_Name']== 'Relationship_Created') & 
                                            (events_df['Gender'] == gender) &
                                            (events_df['Time'] == on_step) 
                                            ], 
                                                columns=['Individual_ID'])
    
    return created_rels

def channels_plot(channel_names, description, results):
        
    df1 = results.load_df(channel_names)
    fig = plt.figure()
    fig.set_size_inches(10,4)
    ax = fig.add_axes([0.12, 0.15, 0.76, 0.76])
    # plot selected channels
    df1[channel_names].plot(ax=ax, title=f"{description}")
    plt.savefig(f"{description}.png")
    if dtk_sft.check_for_plotting(): plt.show()
    plt.close(fig)

def plot_all(df):

    channel_names = list(df)
    channels_data = df
    j = 1
    color = cycle(["blue","darkgreen", "darkmagenta",  "darkblue", "indigo", "orange", "red"])
    fig = plt.figure().set_size_inches(15,10)
    plt.subplots_adjust(left=0.1,bottom=0.1,right=0.9, top=.92, wspace=0.4, hspace=0.8)
    plt.style.use('ggplot')
    plt.rc('xtick', labelsize=7)   
    plt.rc('ytick', labelsize=7)   
    for channel in channel_names:
        y = channels_data[channel]['Data']
        x = range(0, len(y))
        plt.subplot(5, 7, j)
        plt.plot( x, y, color = "grey" , linewidth=.3)
        plt.scatter(x, y, marker = 'o', color =next(color) , s = 1)
        plt.title(channel.replace(",", ",\n").replace(":",":\n"), fontsize=7)
        j+=1
    plt.savefig("All_Channels.png")
    if dtk_sft.check_for_plotting(): plt.show()
    plt.close(fig)

def application( output_folder="output", stdout_filename="test.txt",
                 end_report_name="RelationshipEnd.csv",
                 start_report_name="RelationshipStart.csv",
                 event_report_name="ReportEventRecorder.csv",
                 config_filename="config.json",
                 report_name=dtk_sft.sft_output_filename,
                 debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("end_report_name: " + end_report_name + "\n")
        print("start_report_name: " + start_report_name + "\n")
        print("event_report_name: " + event_report_name + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")
    
    dtk_sft.wait_for_done(stdout_filename)

    end_df = sti_support.parse_relationship_end_report(report_path=output_folder, report_filename=end_report_name)
    start_df = sti_support.parse_relationship_start_report(report_path=output_folder, report_filename=start_report_name)
    event_df = ReportEventRecorder(os.path.join(output_folder, event_report_name)).df

    create_report_file(start_df, end_df, event_df, end_report_name, start_report_name, event_report_name, config_filename, "scientific_feature_report.txt")

    

if __name__ == "__main__":
    # execute only if run as a script
    
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-E', '--end_report', default="RelationshipEnd.csv", help="Relationship end report to parse (RelationshipEnd.csv)")
    parser.add_argument('-S', '--start_report', default="RelationshipStart.csv", help="Relationship start report to parse (RelationshipStart.csv)")
    parser.add_argument('-e', '--event_report', default="ReportEventRecorder.csv", help="Event Recorder report to parse (ReportEventRecorder.csv)") 
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", default=True, action='store_true')
    parser.add_argument('-wd', '--workingdirectory', default=".", help="Path to simulations local results folder")
    args = parser.parse_args()
    # Changing dir to make sure it finds all the simulation files. 
    # args.workingdirectory = "C:\\EMOD\\simulations_local\\2022_12_06_22_43_00_140_0001"
    os.chdir(args.workingdirectory)
    application(output_folder=args.output, stdout_filename=args.stdout,
                end_report_name=args.end_report,
                start_report_name=args.start_report,
                event_report_name=args.event_report,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

