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
import numpy as np

"""
Wife Inheritance Post Validations
For full description, see Readme_WifeInheritance.txt
"""

start_day = 100
error_index = .0015
failures = []
class ds:
    # datasets
    female_widows = None
    females_on_purification = None
    females_inherited = None
    config = None
    expected_infection_rate = .90
    purification_rate = .90
def create_report_file( start_df, end_df, event_df, 
                        end_report_name, start_report_name, event_report_name,
                        config_filename, dtk_post_process_report_name, debug=False):
    succeed = True

    # filling up datasets
    ds.female_widows = get_female_widows(end_df, event_df)
    ds.females_on_purification = get_females_on_purification(event_df)
    ds.females_inherited = inherited_females(event_df)
    ds.config = get_json_file('config.json')
    

    with open(dtk_post_process_report_name, 'w') as pp_log:
        print_log(pp_log, f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")
        print_log(pp_log, f"1. Collecting the existing relationship pairs when intervention happens from "
                                 f"{start_report_name} and {end_report_name}.\n")
        print_log(pp_log, f"2. Validating {event_report_name}")
        print_log(pp_log, f"3. Validating RelationshipConsummated report")
        print_log(pp_log, f"4. Verifying special commercial settings for cleansing purpose")
        print_log(pp_log, f"5. Validating WifeInherithance stages")
        
        print_log(pp_log, f"\nSimulation_Duration: {ds.config['parameters']['Simulation_Duration']}")
        print_log(pp_log, f"Simulation_Timestep: {ds.config['parameters']['Simulation_Timestep']}")

        # ------------------- COITAL ACT REPORTS VALIDATIONS:
        
        # Gets the df with PFA - RECORDED - relationships consummated  with duplicates and any use of condom.
        reported_coital_acts_for_purification = get_successful_purification_encounters()   
        print_log(pp_log, f"Total Records found in Relationship Consummated report:"
                        f" {len(reported_coital_acts_for_purification)} (this may include other type of relationships)")

        # Gets df with Females who have BROADCASTED the purification event, it includes duplicates, 
        # this is a female could have undergone multiple times through the WI cycle. 
        widows_on_purification_stage = get_females_on_purification(event_df)  #  All - including duplicates.
        print_log(pp_log, f"Widows on purification stage: {len(widows_on_purification_stage)} ")
        
        # Gets the MERGE between the females EXPECTED to be purified, and the individuals that actually 
        # reached a succesful purification event (at least ONCE - hence removing duplicates of the Female's id)
        widows_who_got_purified = pd.merge( widows_on_purification_stage, reported_coital_acts_for_purification.drop_duplicates(subset="B_ID"),  how="inner",  left_on="Individual_ID", right_on="B_ID")
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

            # # Coital events
            # # BUG 4808
            # print_log(pp_log, "Evaluating generation of coital events - relationship consummated report")
            # results = f"Expected:  {len(widows_on_purification_stage)}, \nActual: {len(widows_who_got_purified)}" 
            # print_log(pp_log, results )
            # if len(widows_who_got_purified) != len(widows_on_purification_stage):
            #     report_failure(pp_log, f"Failure consummating purifications (RelationshipConsummated.csv), \n {results}")

        # ------------------ FEMALE STAGES VALIDATIONS:
        analyze_wifeinheritance_stages(event_df, pp_log)
        plot_wifeinheritance_stages()

        # ------------------ INSET CHART CHANNELS VALIDATIONS:
        InsetChart_path = os.path.join(os.getcwd(), 'output', 'InsetChart.json')
        print_log(pp_log, f"File path: {InsetChart_path}")
        results = InsetChart.InsetChart(file=InsetChart_path)
        expected_channels = ['New Pregnancies', 'Currently Pregnant']
        if not all(x in list(results.channels) for x in expected_channels): 
            report_failure(pp_log, f"Failed to find all expected columns {expected_channels} in inset chart")
       
        #plot_all(results.channels)
       
        # Channel Sets to be evaluated (from dtk_test.dtk_STI_TestData):
        cs = TestData.inset_chart_channel_sets

        # Generate plots:
        channels_plot(cs["s1"]["channels"], cs["s1"]["desc"], results.load_df(cs["s1"]["channels"]))       
        channels_plot(cs["s2"]["channels"], cs["s2"]["desc"], results.load_df(cs["s2"]["channels"]))       
        channels_plot(cs["s3"]["channels"], cs["s3"]["desc"], results.load_df(cs["s3"]["channels"]))     
        channels_plot(cs["s4"]["channels"], cs["s4"]["desc"], results.load_df(cs["s4"]["channels"]))  
        channels_plot(cs["s5"]["channels"], cs["s5"]["desc"], results.load_df(cs["s5"]["channels"]), combined_plots=True )  
        channels_plot(cs["s6"]["channels"], cs["s6"]["desc"], results.load_df(cs["s6"]["channels"]))          
        
        # ---------------------------- PROPERTY REPORTS CHANNELS PLOTTING:
        #plot_property_report()



        # fw = identify_widows(end_df, event_df, 0 )
        # mw = mourning_widows(event_df, 0)
            
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

def get_female_widows(rend_df, events_df):
    # List of diseased MALES
    diseased_males = pd.DataFrame(events_df[
                                            ((events_df['Event_Name']== 'NonDiseaseDeaths') |
                                            (events_df['Event_Name']== 'DiseaseDeaths')) &
                                            (events_df['Gender']== 'M') 
                                            ], 
                                        columns=['Individual_ID'])

    # Terminated Relationship between MARRIED couple due to PARTNER_DIED
    all_widows = pd.DataFrame(rend_df[
                                        (rend_df['Termination_Reason']=='PARTNER_DIED') & 
                                        #(rend_df['Is_rel_outside_PFA']=='T') & 
                                        (rend_df[rt_support.Constant.rel_type_long]==rt_support.Constant.marital_type)]).rename(columns={"Rel_actual_end_time": "Time"})
                                    #     ], 
                                    # columns=['Rel_actual_end_time', 'male_ID', 'female_ID']).rename(columns={"Rel_actual_end_time": "Time"})
    
    # Find the spouses of the diseased males 
    female_widows = pd.merge( diseased_males, all_widows, how="inner",  left_on="Individual_ID", right_on="male_ID")
    return female_widows

def get_females_on_purification(events_df):
    # aid to identify females on purification stage.
    wives_on_purification = pd.DataFrame(events_df[
                                            ((events_df['Event_Name']== 'Started_Purification')) &
                                            (events_df['Gender']== 'F') 
                                            ], 
                                        columns=['Time', 'Individual_ID'])
    return wives_on_purification

def get_successful_purification_encounters():
    # aid to validate if all females who tried to be purified and really completed the process.
    # it includes duplicates - this is, a female could have had multiple commercial encounters.

    relcon_df = parse_csv_report(report_filename="RelationshipConsummated.csv")
    successful_female_purifications = pd.DataFrame(relcon_df[(relcon_df['Is_rel_outside_PFA']=='T') &
                                                             # (relcon_df['Did_Use_Condom']==0) &
                                                             (relcon_df[rt_support.Constant.rel_type_long]==rt_support.Constant.commercial_type)]) 
                                                             # .drop_duplicates(subset=['B_ID' ])
    return successful_female_purifications

def inherited_females(events_df):
    # aid to validate inherited wives
    inherited_females = pd.DataFrame(events_df[
                                            ((events_df['Event_Name']== 'Started_Inherited_Marriage')) &
                                            (events_df['Gender']== 'F') 
                                            ], 
                                        columns=['Time', 'Individual_ID'])
    return inherited_females

def mourning_widows(events_df, on_step=0):
    # aid to validate waiting period
    max_waiting_period = on_step + 60
    mourning_widows = pd.DataFrame(events_df[
                                            (events_df['Event_Name']== 'Started_Purification') & 
                                            (events_df['Time']<= max_waiting_period+1) 
                                            ],
                                        columns=['Individual_ID'])



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

    didnt_pass_infection = len(exposed_to_infection[exposed_to_infection['Infected']==0])  # Since femailes are grouped by ID and the infected is being added up, having 0 is that never got an infected record.
    exposed_to_infection = len(exposed_to_infection)

    print_log(pp_log, f"Analyzing the infection rate among women who got purified, therefore exposed:\n\
                        Didn't pass the infection:    {didnt_pass_infection},\n\
                        Passed the infection:         {exposed_to_infection - didnt_pass_infection},\n\
                        Infection rate: {1 - (didnt_pass_infection/exposed_to_infection)} ")

    if didnt_pass_infection/exposed_to_infection> 1-ds.expected_infection_rate:
        report_failure(pp_log, f"Percentage of infection transmitted due to Purification Practice is below expected"
                        f"\tEXPECTED: greater than {ds.expected_infection_rate}  \tACTUAL: {1 - didnt_pass_infection/exposed_to_infection}")

def print_events_cycle_by_female(events_df):
    # FOR DEBUGGING purposes only:
    list_of_widows = ds.female_widows['female_ID']
    for female in list_of_widows:
        events = events_df[(events_df['Individual_ID']==female) & (events_df['Event_Name']!="add here if you want to remove a specific event")]
        print(events)
        print("-" *50)

def plot_wifeinheritance_stages():
    # Visual aid to evaluate wife inheritance
        
    summary = pd.DataFrame(ds.female_widows).groupby('Time').count()
    summary2 = pd.DataFrame(ds.females_on_purification).groupby('Time').count()
    summary3 = pd.DataFrame(ds.females_inherited).groupby('Time').count()
    
    fig = plt.figure()
    fig.set_size_inches(10, 6)
    ax = fig.add_subplot(111)
    summary['Individual_ID'].plot(ax=ax, label="Female Widows", color="indigo")
    summary2['Individual_ID'].plot(ax=ax, label="Females On Purification", color= "lightgreen")
    summary3['Individual_ID'].plot(ax=ax, label="Inherited Females", color="gray")

    plt.legend()
    plt.savefig(f"Wives_Analysis.png")
    if dtk_sft.check_for_plotting(): plt.show()
    plt.close(fig)

def channels_plot(channel_names, description, results, combined_plots=False):
    from matplotlib import pyplot as plt
    
    df1 = results

    # plot selected channels
    if combined_plots:
        for channel in channel_names:
            fig0 = plt.figure()
            fig0.set_size_inches(12,8)
            y = df1[channel]
            x = range(0, len(y))
            ax = fig0.add_subplot(111)
            ax.plot(x, y, color = 'lightblue', linewidth=.9)
            ax.scatter(x, y, color= 'Green', s = 3)
            plt.title( f"{channel}")
            plt.savefig(f"{valid_filename(channel)}.png")
            if dtk_sft.check_for_plotting(): plt.show()
            plt.close(fig0)
            
    else:
        fig = plt.figure()
        fig.set_size_inches(10,4)
        ax = fig.add_axes([0.12, 0.15, 0.76, 0.76])
        df1[channel_names].plot(ax=ax, title=f"{description}")
        plt.savefig(f"{valid_filename(description)}.png")
        if dtk_sft.check_for_plotting(): plt.show()
        plt.close(fig)

    for channel in channel_names:
        print(channel)
        print(list(df1[channel]))
    print("-"*200)

def valid_filename(s):
    import string
    valid_chars = "-_.() %s%s" % (string.ascii_letters, string.digits)
    filename = ''.join(c for c in s if c in valid_chars)
    filename = filename.replace(' ','_')
    return filename

def parse_csv_report(report_path="output", report_filename=""):
    import pandas as pd
    import os
    report_file = os.path.join(report_path, report_filename)
    report_df = pd.read_csv(report_file)
    return report_df

def get_json_file(json_file_name=""):
    import json
    with open(json_file_name, 'r') as file:
        json_dict = json.load(file)
    return json_dict

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
    args.workingdirectory = "C:\\EMOD\\simulations_local\\2022_12_08_13_26_13_928_0001"
    os.chdir(args.workingdirectory)
    application(output_folder=args.output, stdout_filename=args.stdout,
                end_report_name=args.end_report,
                start_report_name=args.start_report,
                event_report_name=args.event_report,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

