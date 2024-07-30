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


def create_responder(responder_id, coordinator_name):
    print(f"py: creating responder: {responder_id} - {coordinator_name}")
    with open(f"create_responder_{coordinator_name}.txt", "a") as csv_log:
        csv_log.write(f"create_responder: {coordinator_name}\n")


def delete_responder(responder_id, coordinator_name):
    print(f"py: deleting responder: {responder_id} - {coordinator_name}")
    with open(f"delete_responder_{coordinator_name}.txt", "a") as csv_log:
        csv_log.write(f"create_responder: {coordinator_name}\n")


def respond(time, responder_id, coordinator_name, num_vectors_sampled, allele_combos, fractions):
    print(f"py: respond: {time} - {responder_id} - {coordinator_name} - {num_vectors_sampled}")
    print(allele_combos)
    print(fractions)
    with open(f"respond_{coordinator_name}.txt", "a") as csv_log:
        csv_log.write(f"respond: {time}\n")

    event_names = []
    if coordinator_name == "Frequency_Counter":
        for i in range(len(allele_combos)):
            if (allele_combos[i] == "a1") and (fractions[i] < 0.3):
                event_names.append("Release_More_Mosquitoes_a1a1")
    else:
        for i in range(len(allele_combos)):
            if (allele_combos[i] == "X-a0:X-a0") and (fractions[i] < 0.7):
                event_names.append("Release_More_Mosquitoes_a0a0")

    return event_names
