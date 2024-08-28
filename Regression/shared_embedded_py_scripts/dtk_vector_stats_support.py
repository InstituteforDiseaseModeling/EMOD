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


def ConvertReportVectorStats(output_path, filename="ReportVectorStats.csv"):
    csv_fn = os.path.join(output_path, filename)
    if filename != "ReportVectorStats.csv":
        json_fn = "InsetChart_" + filename
        json_fn = os.path.join(output_path, json_fn)
        json_fn = json_fn.replace("csv", "json")
    else:
        json_fn = csv_fn.replace("csv", "json")

    # data for header
    start_time = -1
    num_timesteps = 0
    timestep = 1  # assume vectors so assume 1
    num_channels = 0

    json_data = {}
    json_data["Channels"] = {}

    given_minus_received = []

    with open(csv_fn, "r") as rs:
        header = rs.readline()  # assume Time is first column
        header = header.rstrip()
        column_names = header.split(",")
        num_channels = len(column_names) - 2  # No channles for Time and NodeID
        for name in column_names:
            if ((name != "Time") and (name != "NodeID")):
                json_data["Channels"][name] = {}
                json_data["Channels"][name]["Units"] = ""
                json_data["Channels"][name]["Data"] = []
        prev_time = -1
        for line in rs:
            line = line.rstrip()
            values = line.split(",")
            num_timesteps += 1
            for index in range(len(values)):
                name = column_names[index]
                if (name == "Time"):
                    time = values[index]
                    if (start_time == -1):
                        start_time = time
                    if (time != prev_time):
                        for channel_name in column_names:
                            if ((channel_name != "Time") and (channel_name != "NodeID")):
                                json_data["Channels"][channel_name]["Data"].append(0.0)
                                given_minus_received.append(0.0)
                                prev_time = time
                elif (name != "NodeID"):
                    json_data["Channels"][name]["Data"][-1] += float(values[index])
                    if (name == "InfectiousBitesGivenMinusReceived"):
                        given_minus_received[-1] += float(values[index])

    inset_json = CreateHeader(start_time, num_timesteps, timestep, num_channels)

    json_data["Header"] = inset_json

    with open(json_fn, "w") as handle:
        handle.write(json.dumps(json_data, indent=4, sort_keys=False))

    print("Done converting ReportVectorStats")

    return given_minus_received


def ConvertReportVectorStatsSpeciesStratified(output_path):
    base_column_names = [
        "VectorPopulation",
        "STATE_INFECTIOUS",
        "STATE_INFECTED",
        "STATE_ADULT"
    ]

    csv_fn = os.path.join(output_path, "ReportVectorStats.csv")
    json_fn = csv_fn.replace("csv", "json")

    # data for header
    start_time = -1
    num_timesteps = 0
    timestep = 1  # assume vectors so assume 1
    num_channels = 0

    json_data = {}
    json_data["Channels"] = {}

    with open(csv_fn, "r") as rs:
        header = rs.readline()  # assume Time is first column
        header = header.rstrip()
        column_names = header.split(",")

        if "Time" not in column_names:
            raise Exception("'Time' must be a column.")

        if "Species" not in column_names:
            raise Exception("'Species' must be a column to be stratified by species.")

        for base_name in base_column_names:
            if base_name not in column_names:
                raise Exception("'" + base_name + "' is expected to be a column.")

        index_time = column_names.index("Time")
        index_species = column_names.index("Species")

        # Get the unique set of species and save lines
        first_time = -1.0
        first_time_lines = []
        species_list = []
        for line in rs:
            line = line.rstrip()
            values = line.split(",")
            line_time = values[index_time]
            line_species = values[index_species]
            if line_species not in species_list:
                species_list.append(line_species)
            if first_time == -1.0:
                first_time = line_time
            elif first_time == line_time:
                first_time_lines.append(line)
            else:
                break

        # Create sets of channel names for each species
        for species in species_list:
            for base_name in base_column_names:
                channel_name = base_name + "-" + species
                json_data["Channels"][channel_name] = {}
                json_data["Channels"][channel_name]["Units"] = ""
                json_data["Channels"][channel_name]["Data"] = []
                json_data["Channels"][channel_name]["Data"].append(0.0)

        for line in first_time_lines:
            line = line.rstrip()
            values = line.split(",")

            line_time = values[index_time]
            line_species = values[index_species]

            if line_time != first_time:
                raise Exception("line_time != first_time")

            for base_name in base_column_names:
                channel_name = base_name + "-" + line_species
                index = column_names.index(base_name)
                json_data["Channels"][channel_name]["Data"][-1] += float(values[index])

        num_timesteps = 1
        current_time = first_time
        for line in rs:
            line = line.rstrip()
            values = line.split(",")

            line_time = values[index_time]
            line_species = values[index_species]

            if current_time != line_time:
                for channel_name in json_data["Channels"]:
                    json_data["Channels"][channel_name]["Data"].append(0.0)
                num_timesteps += 1
                current_time = line_time

            for base_name in base_column_names:
                channel_name = base_name + "-" + line_species
                index = column_names.index(base_name)
                json_data["Channels"][channel_name]["Data"][-1] += float(values[index])

    num_channels = len(base_column_names) * len(species_list)

    inset_json = CreateHeader(start_time, num_timesteps, timestep, num_channels)

    json_data["Header"] = inset_json

    with open(json_fn, "w") as handle:
        handle.write(json.dumps(json_data, indent=4, sort_keys=False))

    print("Done converting ReportVectorStats")

    return


def CheckInfectiousBites(given_minus_received, output_path):
    total = sum(given_minus_received)
    num = len(given_minus_received)
    avg = float(total) / float(num)

    results_dict = {}
    results_dict["results"] = "success"
    if (avg > 1):
        results_dict["results"] = "failure: avg=" + str(avg)

    results_fn = os.path.join(output_path, "GivenMinusReceivedResults.json")
    with open(results_fn, "w") as handle:
        handle.write(json.dumps(results_dict, indent=4, sort_keys=False))


def GetGenomesByGender(columns):
    males = []
    females = []
    for col in columns:
        if col[0] == "STATE_MALE":
            if col[1].split(":")[1].startswith("Y"):
                males.append(col[1])
            else:
                females.append(col[1])
    return males, females


def ConvertReportVectorGeneticsByGenome(report_fn, output_path):
    csv_fn = os.path.join(output_path, report_fn)

    json_fn = "InsetChart_" + report_fn
    json_fn = json_fn.replace("csv", "json")
    json_fn = os.path.join(output_path, json_fn)

    df = pd.read_csv(csv_fn)

    df = pd.pivot_table(df, index=["Time"], values=["VectorPopulation", "STATE_MALE"], columns=["Genome"],
                        aggfunc=np.sum)

    males, females = GetGenomesByGender(df.columns)

    num_channels = len(males) + len(females)
    num_timesteps = len(df["VectorPopulation", females[0]])

    json_data = {}
    json_data["Header"] = CreateHeader(1, num_timesteps, 1, num_channels)

    json_data["Channels"] = {}

    for m in males:
        json_data["Channels"][m] = {}
        json_data["Channels"][m]["Units"] = "count"
        json_data["Channels"][m]["Data"] = df["STATE_MALE", m].tolist()
    for f in females:
        json_data["Channels"][f] = {}
        json_data["Channels"][f]["Units"] = "count"
        json_data["Channels"][f]["Data"] = df["VectorPopulation", f].tolist()

    with open(json_fn, "w") as handle:
        handle.write(json.dumps(json_data, indent=4, sort_keys=False))


def JustMaleFemalePopulations(output_path, filename="ReportVectorStats.csv"):
    csv_fn = os.path.join(output_path, filename)
    df = pd.read_csv(csv_fn)

    # retrieve and aggregate data
    sexes = ["male", "female"]
    times = list(set(df.Time.tolist()))
    times.sort()
    start_time = times[0]
    end_time = times[-1]

    all_nodes = {}
    all_species = []
    if "Species" in df.columns:
        all_species = set(df.Species.tolist())

    for sex in sexes:
        all_nodes[sex] = {}
        if all_species:
            for species in all_species:
                all_nodes[sex][species] = {int(node): [] for node in set(df.NodeID.tolist())}
        else:
            all_nodes[sex] = {int(node): [] for node in set(df.NodeID.tolist())}

    for time in range(start_time, end_time):
        on_this_day = df.loc[df.Time == time]
        for sex in sexes:
            if sex == "male":
                if all_species:
                    for species in all_species:
                        this_species = on_this_day[on_this_day.Species == species]
                        for node, data in all_nodes[sex][species].items():
                            # number of vectors in each node that day
                            data.append(int(this_species[this_species.NodeID == node]["STATE_MALE"].iloc[0]))
                else:
                    for node, data in all_nodes[sex].items():
                        # number of vectors in each node that day
                        data.append(int(on_this_day[on_this_day.NodeID == node]["STATE_MALE"].iloc[0]))
            else:
                if all_species:
                    for species in all_species:
                        this_species = on_this_day[on_this_day.Species == species]
                        for node, data in all_nodes[sex][species] .items():
                            # number of vectors in each node that day
                            data.append(int(this_species[this_species.NodeID == node]["VectorPopulation"].iloc[0]))
                else:
                    for node, data in all_nodes[sex].items():
                        # number of vectors in each node that day
                        data.append(int(on_this_day[on_this_day.NodeID == node]["VectorPopulation"].iloc[0]))


    # data for header
    num_timesteps = end_time - start_time
    timestep = 1  # assume vectors so assume 1
    num_channels = len(set(df.NodeID)) * 2  # for males and females
    inset_json = CreateHeader(start_time, num_timesteps, timestep, num_channels)
    if all_species:
        for species in all_species:
            # add data to the json
            json_data = {}
            json_data["Channels"] = {}
            json_data["Header"] = inset_json

            for sex in sexes:
                for node, data in all_nodes[sex][species].items():
                    channel_name = f"{sex} in {node}"
                    json_data["Channels"][channel_name] = {}
                    json_data["Channels"][channel_name]["Units"] = "Vectors"
                    json_data["Channels"][channel_name]["Data"] = data

            json_fn = os.path.join(output_path, "InsetChart_" + os.path.splitext(filename)[0] + f"_{species}.json")
            with open(json_fn, "w") as handle:
                handle.write(json.dumps(json_data, indent=4, sort_keys=False))
            print(f"Done creating {json_fn}")
    else:
        # add data to the json
        json_data = {}
        json_data["Channels"] = {}
        json_data["Header"] = inset_json

        for sex in sexes:
            for node, data in all_nodes[sex].items():
                channel_name = f"{sex} in {node}"
                json_data["Channels"][channel_name] = {}
                json_data["Channels"][channel_name]["Units"] = "Vectors"
                json_data["Channels"][channel_name]["Data"] = data

        json_fn = os.path.join(output_path, "InsetChart_" + os.path.splitext(filename)[0] + f".json")
        with open(json_fn, "w") as handle:
            handle.write(json.dumps(json_data, indent=4, sort_keys=False))
        print(f"Done creating {json_fn}")



