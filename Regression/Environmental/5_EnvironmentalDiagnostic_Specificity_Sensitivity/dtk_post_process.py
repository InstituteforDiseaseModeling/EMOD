#!/usr/bin/python

# This script uses the events from ReportNodeEventRecorder and verifies that the values for
# "Base_Specificity" and "Base_Sensitivity" match the expectation for false negatives/positives.

import csv
import json

report = "ReportNodeEventRecorder.csv"

def application(output_path):
    print("Running Sensitivity/Specificity test")

    Sensitivity = {'false':0, 'correct':0, 'percentage':0.0}
    Specificity = {'false':0, 'correct':0, 'percentage':0.0}

    path = output_path+"\\"+report

    # open report
    with open(path, 'r') as csvfile:
        spamreader = csv.reader(csvfile, delimiter=',')
        next(spamreader, None) #skip header
        for row in spamreader:
            correct_event = row[2]
            row = next(spamreader, None)  # _Event_Node_Sensitivity
            Event_Node_Sensitivity = row[2]
            row = next(spamreader, None)  # _Event_Node_Specificity
            Event_Node_Specificity = row[2]

            if correct_event == 'Positive_Event_Node':
                if Event_Node_Sensitivity == 'Positive_Event_Node_Sensitivity':
                    Sensitivity['correct'] += 1
                else:
                    Sensitivity['false'] += 1

            if correct_event == 'Negative_Event_Node':
                if Event_Node_Specificity == 'Negative_Event_Node_Specificity':
                    Specificity['correct'] += 1
                else:
                    Specificity['false'] += 1

    Sensitivity['percentage'] = Sensitivity['correct']/ sum(Sensitivity.values())
    Specificity['percentage'] = Specificity['correct']/ sum(Specificity.values())

    result = {}
    result['Sensitivity'] = Sensitivity
    result['Specificity'] = Specificity

    result_path = output_path + "\\" + "result.json"
    with open(result_path, 'w') as result_file:
        json.dump(result, result_file, sort_keys=True, indent=4, separators=(',', ': ') )

if __name__ == "__main__":
    # execute only if run as a script
    application("output\\")