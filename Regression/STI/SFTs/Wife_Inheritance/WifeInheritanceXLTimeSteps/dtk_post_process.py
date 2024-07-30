#!/usr/bin/python
from fileinput import filename
from matplotlib import pyplot as plt
import pandas as pd
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
import dtk_test.dtk_STI_TestData as TestData 
import dtk_test.dtk_STI_WifeInheritance_Support as wi

"""
Wife Inheritance Post Validations with 30 days timesteps to make sure that the relationship doesn't expire before they can consummate
(which could result on coital act for purification purposes never happens - which with large time steps could be the case)
Reference: https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/4808#issuecomment-1340249311 
"""

start_day = 90
error_index = .0015
failures = []
class ds:
    # datasets
    female_widows = None
    females_on_purification = None
    females_inherited = None
    config = None
    expected_infection_rate = .85
    purification_rate = .85

def create_report_file( start_df, end_df, event_df, 
                        end_report_name, start_report_name, event_report_name,
                        config_filename, dtk_post_process_report_name, debug=False):
    succeed = True
    # filling up datasets
    ds.female_widows = wi.get_female_widows(end_df, event_df)
    ds.females_on_purification = wi.get_females_on_purification(event_df)
    ds.females_inherited = wi.inherited_females(event_df)
    ds.config = wi.get_json_file('config.json')
       
    with open(dtk_post_process_report_name, 'w') as pp_log:
        print_log(pp_log, f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")

        print_log(pp_log, f"\nSimulation_Duration: {ds.config['parameters']['Simulation_Duration']} \n\
                         Simulation_Timestep: {ds.config['parameters']['Simulation_Timestep']}")

        # ------------------- COITAL ACT REPORTS VALIDATIONS:
        
        # Gets the df with PFA - RECORDED - relationships consummated  with duplicates and any use of condom.
        reported_coital_acts_for_purification = wi.get_successful_purification_encounters()   
        print_log(pp_log, f"Total Records found in Relationship Consummated report:"
                        f" {len(reported_coital_acts_for_purification)} (this may include other type of relationships)")

        # Gets df with Females who have BROADCASTED the purification event, it includes duplicates, 
        # this is a female could have undergone multiple times through the WI cycle. 
        widows_on_purification_stage = wi.get_females_on_purification(event_df)  #  All - including duplicates. Returns Time and Individual_ID
        print_log(pp_log, f"Widows on purification stage: {len(widows_on_purification_stage)} ")
        
        # Gets the MERGE between the females EXPECTED to be purified, and the individuals that actually 
        # reached a succesful purification event (at least ONCE - hence removing duplicates of the Female's id)
        widows_who_got_purified = pd.merge( widows_on_purification_stage, \
                                            reported_coital_acts_for_purification.drop_duplicates(subset="B_ID"),  
                                            how="inner",  left_on="Individual_ID", right_on="B_ID")
                                            
        print_log(pp_log, f"Reported coital acts (from merge between widows looking for purification and successful purifications): {len(widows_who_got_purified)} \n")

        if len(reported_coital_acts_for_purification)==0: 
            report_failure(pp_log, "Total Relationship Consummated records: 0")
        else:
            # Report column updates validation:
            expected_cols = TestData.coital_act_report_expected_columns
            print_log(pp_log, f"Coital Act Report COLUMNS validation (RelationshipConsumated.csv) \n ")
            results = f"Expected: {expected_cols} \n Actual: {list(reported_coital_acts_for_purification)}"
            print_log(pp_log, results )
            if not all(x in list(reported_coital_acts_for_purification) for x in expected_cols): 
                report_failure(pp_log, f"Failed to find all columns in RelationshipConsummated.csv report,\n {results}")

            # Evaluating use of condom during purification process.
            if len(reported_coital_acts_for_purification[reported_coital_acts_for_purification['Did_Use_Condom']==1])!= 0:
                report_failure(pp_log, f"Purification events shouldn't have used condom. There are"
                                        f"{len(reported_coital_acts_for_purification[reported_coital_acts_for_purification['Did_Use_Condom']==1])} reported cases")

            # Coital events validations ***
            Unique_widows_on_purification_stage = widows_on_purification_stage.drop_duplicates(subset="Individual_ID")
            Unique_widows_who_got_purified = pd.DataFrame(widows_who_got_purified.drop_duplicates(subset="Individual_ID"), columns=["Individual_ID", "B_ID", "A_ID"])
            
            print_log(pp_log, "Evaluating generation of coital events - relationship consummated report")
            results = f"Expected:  {len(Unique_widows_on_purification_stage)}, \nActual: {len(Unique_widows_who_got_purified)}" 
            print_log(pp_log, results )

            # Validate:
            if len(Unique_widows_who_got_purified) != len(Unique_widows_on_purification_stage):
                report_failure(pp_log, f"Failure consummating purifications (RelationshipConsummated.csv), \n {results}")

            print(len(Unique_widows_on_purification_stage))
            print(len(Unique_widows_who_got_purified))

            # Are the women found on purification stage the same women reported on coital events? 
            are_they_the_same_women = pd.merge( Unique_widows_on_purification_stage, \
                                                Unique_widows_who_got_purified, \
                                                how="inner",  on="Individual_ID")

            # Validate:
            if len(are_they_the_same_women)!=len(Unique_widows_on_purification_stage):
                report_failure(pp_log, "Individuals reported on Relationship \
                                       consumated report are not the same as the ones who got purified")    
                                       
        # # ------------------ FEMALE STAGES VALIDATIONS:
        analyze_wifeinheritance_stages(event_df, pp_log)
        wi.plot_wifeinheritance_stages(ds)

           
        # # ------------------------- PRINT FAILURES:
        if len(failures)>0: 
            print("=========== FAILURES =========== ")
            for err in failures:
                print(err)
                succeed = False   

        print_log(pp_log, f"SUMMARY: Success={succeed}") # Do not delete this line
    
    return succeed

def print_log(file, log):
    log = f"\n{log}"
    file.write(log)
    print(log)

def report_failure(file, err):
    failures.append(f"{err}")
    err = f"ERROR!:: {err}"
    print_log(file, err)


def analyze_wifeinheritance_stages(events_df, pp_log):
    
    # Analyzing the purified VS inherithed
    total_purified = len(ds.females_on_purification.drop_duplicates(subset=['Individual_ID']))
    total_inherithed = len(ds.female_widows.drop_duplicates(subset=['female_ID']))

    print_log(pp_log, f"Verifying Purified events: {total_purified} versus inherithed events: {total_inherithed}")
    if total_purified/total_inherithed < ds.purification_rate:
        report_failure(pp_log, f"Percentage of purified woman versus inherihed ones below benchmark: \tEXPECTED: Greater than {ds.purification_rate} \tACTUAL:  {total_purified/total_inherithed} ")

    # Analyzing the infection rate among women who got purified - therefore exposed
    exposed_to_infection = pd.DataFrame( events_df[events_df['Individual_ID'].isin(list(ds.females_on_purification['Individual_ID']))], columns=['Individual_ID', 'Infected'])\
                                                .groupby('Individual_ID')\
                                                .sum()

    didnt_pass_infection = len(exposed_to_infection[exposed_to_infection['Infected']==0])  # Filter out females with 0 infections.
    exposed_to_infection = len(exposed_to_infection)

    print_log(pp_log, f"Analyzing the infection rate among women who got purified, therefore exposed:\n\
                        Didn't pass the infection:    {didnt_pass_infection},\n\
                        Passed the infection:         {exposed_to_infection - didnt_pass_infection},\n\
                        Infection rate: {1 - (didnt_pass_infection/exposed_to_infection)} ")

    if didnt_pass_infection/exposed_to_infection> 1-ds.expected_infection_rate:
        report_failure(pp_log, f"Percentage of infection transmitted due to Purification Practice is below expected"
                        f"\tEXPECTED: greater than {ds.expected_infection_rate}  \tACTUAL: {1 - didnt_pass_infection/exposed_to_infection}")


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
    import argparse

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
    args.workingdirectory = "C:\\EMOD\\simulations_local\\2023_01_12_10_18_25_975_0001"
    os.chdir(args.workingdirectory)
    application(output_folder=args.output, stdout_filename=args.stdout,
                end_report_name=args.end_report,
                start_report_name=args.start_report,
                event_report_name=args.event_report,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

