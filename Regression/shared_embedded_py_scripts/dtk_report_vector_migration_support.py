import json, sys, os, shutil
import pandas as pd
import numpy as np


def CreateHeader(start_time, num_timesteps, timestep, num_channels):
    inset_json = {}

    inset_json["DateTime"] = "Unknown"
    inset_json["DTK_Version"] = "Unknown"
    inset_json["Report_Type"] = "InsetChart"
    inset_json["Report_Version"] = "1.0"
    inset_json["Start_Time"] = start_time
    inset_json["Simulation_Timestep"] = timestep
    inset_json["Timesteps"] = num_timesteps
    inset_json["Channels"] = num_channels

    return inset_json


def ConvertReportVectorMigration(output_path, filename="ReportVectorMigration.csv"):
    csv_fn = os.path.join(output_path, filename)

    json_fn = os.path.join(output_path, "InsetChart_" + os.path.splitext(filename)[0] + ".json")

    df = pd.read_csv(csv_fn)
    if len(df.Time) < 1:  # empty report nothing to be done
        os.remove(csv_fn)
        return
    if len(set(df.Species)) > 1:
        raise ValueError(f"Please limit the {filename} report to one species.\n")
    if len(set(df.State)) > 3:
        raise ValueError(f"Please limit the {filename} report to either all male or all female vectors.\n")

    # retrieve and aggregate data
    from_nodes_data = {int(from_node): [] for from_node in set(df.FromNodeID.tolist())}
    to_nodes_data = {int(to_node): [] for to_node in set(df.ToNodeID.tolist())}
    times = list(set(df.Time.tolist()))
    start_time = times[0]
    end_time = times[-1]

    for time in range(start_time, end_time):
        on_this_day = df.loc[df.Time == time]
        for to_node, data in to_nodes_data.items():
            # number of vectors travelling to that node on that day
            data.append(int(on_this_day[on_this_day.ToNodeID == to_node]["Population"].sum()))
        for from_node, data in from_nodes_data.items():
            # number of vectors travelling from that node on that day
            data.append(int(on_this_day[on_this_day.FromNodeID == from_node]["Population"].sum()))

    # add data to the json
    json_data = {}
    json_data["Channels"] = {}

    for to_node, data in to_nodes_data.items():
        channel_name = f"Migration TO Node {to_node}"
        json_data["Channels"][channel_name] = {}
        json_data["Channels"][channel_name]["Units"] = "Vectors"
        json_data["Channels"][channel_name]["Data"] = data

    for from_node, data in from_nodes_data.items():
        channel_name = f"Migration FROM Node {from_node}"
        json_data["Channels"][channel_name] = {}
        json_data["Channels"][channel_name]["Units"] = "Vectors"
        json_data["Channels"][channel_name]["Data"] = data


    # data for header
    num_timesteps = end_time - start_time
    timestep = 1  # assume vectors so assume 1
    num_channels = len(set(df.FromNodeID)) + len(set(df.ToNodeID))
    inset_json = CreateHeader(start_time, num_timesteps, timestep, num_channels)

    json_data["Header"] = inset_json

    with open(json_fn, "w") as handle:
        handle.write(json.dumps(json_data, indent=4, sort_keys=False))

    # os.remove(csv_fn)
    print(f"Done creating {filename}")

    return
