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
    sys.path.append( str(Path('../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_STI_Support as sti_support
import dtk_test.dtk_RelationshipTargeting_Support as rt_support
import dtk_test.dtk_InsetChart as InsetChart
from dtk_test.dtk_OutputFile import ReportEventRecorder
import dtk_test.dtk_STI_TestData as TestData 


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

def print_events_cycle_by_female(ds, events_df):
    # FOR DEBUGGING purposes only:
    list_of_widows = ds.female_widows['female_ID']
    for female in list_of_widows:
        events = events_df[(events_df['Individual_ID']==female) & (events_df['Event_Name']!="add here if you want to remove a specific event")]
        print(events)
        print("-" *50)

def plot_wifeinheritance_stages(ds):
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