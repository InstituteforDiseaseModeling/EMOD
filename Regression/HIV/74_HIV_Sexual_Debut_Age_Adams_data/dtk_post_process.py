import os
import sys
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

fig, axes = plt.subplots(nrows=5, ncols=1, figsize=(20,22))

def get_target(row):
    # Values from campaign.json
    times = [1960.5, 1999, 2000, 2001, 2003, 2004, 2005, 2007, 2009, 2010, 2012, 2014, 2016, 2017]
    data = {
        "Male":
            {
                15:
                {
                    # "Times": [1960.5, 1999, 2000, 2001, 2003, 2004, 2005, 2007, 2009, 2010, 2012, 2014, 2016, 2017],
                    "Values": [0.665211063, 0.665211063, 0.677925211, 0.691513761, 0.664133739, 0.654263566, 0.623529412,
                              0.549700599, 0.488764045, 0.44140625, 0.440336134, 0.428791378, 0.429424944, 0.361735089]
                },
                20:
                {
                    # "Times": [1960.5, 1999, 2000, 2001, 2003, 2004, 2005, 2007, 2009, 2010, 2012, 2014, 2016, 2017],
                    "Values": [0.955583756, 0.955583756, 0.958695652, 0.959016393, 0.954059829, 0.960912052, 0.954545455,
                              0.942216981, 0.927573062, 0.926143025, 0.929240375, 0.914367269, 0.911380597, 0.897196262]
                },
                25:
                {
                    # "Times": [1960.5, 1999, 2000, 2001, 2003, 2004, 2005, 2007, 2009, 2010, 2012, 2014, 2016, 2017],
                    "Values": [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
                }
            },
        "Female":
            {
                15:
                {
                    # "Times": [1960.5, 1999, 2000, 2001, 2003, 2004, 2005, 2007, 2009, 2010, 2012, 2014, 2016, 2017],
                    "Values": [0.768916155, 0.768916155, 0.774891775, 0.799460432, 0.801633606, 0.805107527, 0.68401487,
                            0.663687151, 0.581445523, 0.545454545, 0.571059432, 0.499619772, 0.483751846, 0.397669337]
                },
                20:
                {
                    # "Times": [1960.5, 1999, 2000, 2001, 2003, 2004, 2005, 2007, 2009, 2010, 2012, 2014, 2016, 2017],
                    "Values": [0.988976378, 0.988976378, 0.989751098, 0.986442866, 0.985086342, 0.994043187, 0.980769231,
                           0.982101167, 0.983130272, 0.978995434, 0.98066512, 0.965591398, 0.968325792, 0.957627119]
                },
                25:
                {
                    # "Times": [1960.5, 1999, 2000, 2001, 2003, 2004, 2005, 2007, 2009, 2010, 2012, 2014, 2016, 2017],
                    "Values": [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
                }
            }
        }
    if row["Year"] > times[-1]:
        index = len(times) - 1
    elif row["Year"] < times[0]:
        index = 0
    else:
        for n, y in enumerate(times):
            if y == row["Year"]:
                index = n
                break
            if y > row["Year"]:
                index = n - 1
                break
                
    value = -1
    if row[" Gender"] == 0:  # Male, Name of column is "space + Gender"
        for age in data["Male"].keys():
            if row[" Age"] >= age:
                value = data["Male"][age]["Values"][index]
            else:
                break
    else:
        for age in data["Female"].keys():
            if row[" Age"] >= age:
                value = data["Female"][age]["Values"][index]
            else:
                break

    return value


def application( out_path ):
    add_ratio_and_target_to_report(out_path)
    graph_pfa_queues(out_path, [0,1], "INFORMAL")
    graph_pfa_queues(out_path, [2,3], "TRANSITORY")
    graph_pivot(out_path, 4)
    fig.tight_layout()
    plt.savefig("graph_pfa.png")

    
def add_ratio_and_target_to_report(out_path):
    path = os.path.join(out_path, "ReportHIVByAgeAndGender.csv")
    print("Reading: ", path)
    out_path_file = os.path.join(out_path, "ReportHIVByAgeAndGender_ratio_and_target.csv")
    df = pd.read_csv(path)
    df["Ratio - Has Debuted"] = df["Has Debuted"]/df[" Population"]
    df["Ratio - First Coital Act"] = df["Had First Coital Act"]/df[" Population"]
    df["Target"] = df.apply(get_target, axis=1)
    df["Ratio - Considering (TRANSITORY)"] = df["Considering (TRANSITORY)"]/df[" Population"]
    df["Ratio - Available for Relationship (TRANSITORY)"] = df["Available for Relationship (TRANSITORY)"]/df[" Population"]
    df["In queue no partner"] = df["Available for Relationship (TRANSITORY)"] - df["Considering (TRANSITORY)"]
    df.to_csv(out_path_file, sep=',')

 
def graph_pivot(out_path, subplot_id):
    path = os.path.join(out_path, "ReportHIVByAgeAndGender_ratio_and_target.csv")
    print("Reading: ", path)
    df = pd.read_csv(path)
    table = pd.pivot_table(df, values=["Ratio - Has Debuted", "Ratio - First Coital Act", "Target"], index=["Year"], columns=[" Gender", " Age"], aggfunc=np.sum)
    # print(table)
    axes[subplot_id].plot(table.index, table["Ratio - Has Debuted"][0][15], label="Ratio - Has Debuted_15-20")
    axes[subplot_id].plot(table.index, table["Ratio - Has Debuted"][1][15], label="Ratio - Has Debuted Female_15-20")
    axes[subplot_id].plot(table.index, table["Ratio - First Coital Act"][0][15], label="Ratio - First Coital Act_Male_15-20")
    axes[subplot_id].plot(table.index, table["Ratio - First Coital Act"][1][15], label="Ratio - First Coital Act Female_15-20")
    axes[subplot_id].plot(table.index, table["Target"][0][15], label="Ratio_Target_Male")
    axes[subplot_id].plot(table.index, table["Target"][1][15], label="Ratio_Target_Female")
    axes[subplot_id].legend()
 
 
def graph_pfa_queues(out_path, subplot_ids, trans_type: str): 
    path = os.path.join(out_path, "ReportPfaQueues.csv")
    print("Reading: ", path)
    
    df_pfa = df = pd.read_csv(path)    
    axes[subplot_ids[0]].plot(df_pfa["Time"], df_pfa[trans_type + "_F_Before_17.5"], label=trans_type + "_F_Before_17.5")
    axes[subplot_ids[0]].plot(df_pfa["Time"], df_pfa[trans_type + "_M_Before_17.5"], label=trans_type + "_M_Before_17.5")
    axes[subplot_ids[0]].plot(df_pfa["Time"], df_pfa[trans_type + "_F_After_17.5"], label=trans_type + "_F_After_17.5")
    axes[subplot_ids[0]].plot(df_pfa["Time"], df_pfa[trans_type + "_M_After_17.5"], label=trans_type + "_M_After_17.5")
    axes[subplot_ids[0]].legend()
    
    axes[subplot_ids[1]].plot(df_pfa["Time"], df_pfa[trans_type + "_LOW_F_17.5"], label="rate_" + trans_type + "_LOW_F_17.5")
    axes[subplot_ids[1]].plot(df_pfa["Time"], df_pfa[trans_type + "_HIGH_F_17.5"], label = "rate_" + trans_type + "_HIGH_F_17.5")
    axes[subplot_ids[1]].plot(df_pfa["Time"], df_pfa[trans_type + "_LOW_M_17.5"], label="rate_" + trans_type + "_LOW_M_17.5")
    axes[subplot_ids[1]].plot(df_pfa["Time"], df_pfa[trans_type + "_HIGH_M_17.5"], label = "rate_" + trans_type + "_HIGH_M_17.5")
    axes[subplot_ids[1]].legend(loc=9)      
    
    
def graph_pfa_desired(out_path, subplot_id): 
    path = os.path.join(out_path, "ReportPfaQueues.csv")
    print("Reading: ", path)
    df_pfa = df = pd.read_csv(path)                      
    axes[subplot_id].plot(df_pfa["Time"], df_pfa["desired_flow_TRANSITORY_M_17.5"], label="desired_flow_TRANSITORY_M_17.5")
    axes[subplot_id].plot(df_pfa["Time"], df_pfa["desired_flow_TRANSITORY_F_17.5"], label="desired_flow_TRANSITORY_F_17.5")
    axes[subplot_id].legend()
    

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: dtk_post_process.py output_dir")
        exit(0)

    out_path = sys.argv[1]
    application( out_path )
    