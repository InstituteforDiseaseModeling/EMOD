#!/usr/bin/python

import os
import sys

"""
This is the embedded Python code that is used with the VectorSurveillanceEventCoordinator/VSEC.
The main purpose of this code is to determine if new Coordinator Evens should be broadcasted
based on the vectors that were sampled.

The create and delete methods are available incase you want to create instances of the python
version of the responder that contain state.  For example, if you have several responders/VSECs
and your next decision was based on your previous, you might need each python responder to remember
what it last did.  In that case, you could create a python class and create a new one for each
instance of VSEC.

The "respond()" method is the main method called from VSEC.  It is called each time VSEC samples
the vectors.  You want to modify this method so that Coordinator Events are returned (then broadcasted)
that will cause the respons to happen that you want.  For example, you could return an event that
causes mosquitoes to be released.

responder_id - Each responder gets a unique ID.  this allows you to tell each one apart.
coordinator_name - This the name of the VSEC that owns the resonder.  You could have multiple
    instances of VSEC's with the same name.  The responder_id allows you to tell them apart.  For example,
    you might have one VSEC that is sampling via ALLELE_FREQ and one by GENOME_FRACTION.  You could use
    the name to tell them apart.
"""

header_not_needed = []


def write_csv_report(time, coordinator_name, num_vectors_sampled, list_data_names, list_data_values, filename=None):
    """
        Write a csv report with the data passed in.
        This assumes that order of names in list_data_names is always the same and always the same length.
        (which is true)
        If the file does not exist, it will be created.
        The data is gotten from "respond()" which gets it from EMOD.

    Args:
        time: timestamp of the data
        coordinator_name: unique name of the coordinator
        num_vectors_sampled: number of vectors that were used for this data
        list_data_names: list of names of alleles or genes possible in the vector population
        list_data_values: list of the fraction of each allele or gene in the vector population matching the
            location in list_data_names
        filename: name of the file to write to.  If None, it will write to "<coordinator_name>_py_log.csv"

    Returns:
        Nothing
    """
    if not filename:
        filename = f"{coordinator_name}_py_log.csv"
    with open(filename, "a") as csv_log:
        line = f"{time}, {coordinator_name}, {num_vectors_sampled}"
        if coordinator_name not in header_not_needed:  # write header: this is the first time us seeing this coordinator
            header = "time, coordinator_name, num_vectors_sampled"
            for i in range(len(list_data_names)):
                header += f",{list_data_names[i]}"
                line += f",{round(list_data_values[i], 5)}"
            csv_log.write(header + "\n")
            csv_log.write(line + "\n")
            header_not_needed.append(coordinator_name)  # set flag to false so we don't write the header again
        else:
            for i in range(len(list_data_values)):
                line += f",{round(list_data_values[i], 5)}"
            csv_log.write(line + "\n")


def create_responder(responder_id, coordinator_name):
    print(f"py: creating responder: {responder_id} - {coordinator_name}")
    with open(f"create_responder_{coordinator_name}.txt", "a") as csv_log:
        csv_log.write(f"create_responder: {coordinator_name}\n")


def delete_responder(responder_id, coordinator_name):
    print(f"py: deleting responder: {responder_id} - {coordinator_name}")
    with open(f"delete_responder_{coordinator_name}.txt", "a") as csv_log:
        csv_log.write(f"create_responder: {coordinator_name}\n")


def respond(time, responder_id, coordinator_name, num_vectors_sampled, list_data_names, list_data_values):
    print(f"py: respond: {time} - {responder_id} - {coordinator_name} - {num_vectors_sampled}")
    for i in range(len(list_data_names)):
        print(f"py: {list_data_names[i]}: {list_data_values[i]}")
    with open(f"respond_{coordinator_name}.txt", "a") as csv_log:
        csv_log.write(f"respond: {time}\n")

    event_names = []
    if coordinator_name == "Frequency_Counter":
        write_csv_report(time=time, coordinator_name=coordinator_name, num_vectors_sampled=num_vectors_sampled,
                         list_data_names=list_data_names, list_data_values=list_data_values,
                         filename="freq_log.csv")
        for i in range(len(list_data_names)):
            if (list_data_names[i] == "a1") and (list_data_values[i] < 0.3):
                event_names.append("Release_More_Mosquitoes_a1a1")
    else:
        write_csv_report(time=time, coordinator_name=coordinator_name, num_vectors_sampled=num_vectors_sampled,
                         list_data_names=list_data_names, list_data_values=list_data_values)
        for i in range(len(list_data_names)):
            if (list_data_names[i] == "X-a0:X-a0") and (list_data_values[i] < 0.7):
                event_names.append("Release_More_Mosquitoes_a0a0")

    return event_names
