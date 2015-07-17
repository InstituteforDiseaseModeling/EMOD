#!/usr/bin/python

# SUMMARY: Plot events that occur during a simulation, disaggregated by gender.  Note that common events, e.g. ARTStaging0 and ARTStaging1 will be displayed in separate rows in a common column with heading ARTStaging.
# INPUT ARGUMENTS: (1) Path to ReportEventRecorder.csv
# OUTPUT: figure drawn on screen

from __future__ import print_function
import argparse
import csv
import math
import matplotlib.pyplot as pyplot
import matplotlib.ticker as ticker
import sets
import sys


def main(filename, logplot):

    data = {}
    events = sets.Set()
    with open(filename, 'rb') as handle:
        header = None
        reader = csv.reader(handle)
        for row in reader:
            if header is None:
                header = map(str.strip, row)
                year_index = header.index('Year')
                event_index = header.index('Event_Name')
                sex_index = header.index('Gender')
            else:
                year = float(row[year_index])
                event_name = row[event_index]
                event = event_name.rstrip('0123456789')
                group = 0 if len(event) == len(event_name) else int(event_name[len(event):])    # convert trailing #s to integer
                sex = row[sex_index]
                if event not in data: data[event] = { group: { year: { 'M': 0, 'F': 0 } } }
                if group not in data[event]: data[event][group] = { year: { 'M': 0, 'F': 0 } }
                if year not in data[event][group]: data[event][group][year] = { 'M': 0, 'F': 0 }
                data[event][group][year][sex] += 1
                events.add(event)

    max_group = 0
    start_year = float('+inf')
    end_year = float('-inf')
    max_count = 0
    min_count = 1e9

    for event in data:
        event_entry = data[event]
        max_group = max(max_group, max(event_entry.keys()))
        for group in event_entry:
            group_entry = event_entry[group]
            keys = group_entry.keys()
            start_year = min(start_year, min(keys))
            end_year = max(end_year, max(keys))
            for year in group_entry:
                year_entry = group_entry[year]
                max_count = max(max_count, year_entry['M'], year_entry['F'])
                min_count = min(min_count, year_entry['M'], year_entry['F'])

    figure = pyplot.figure(filename)

    plot_rows = max_group + 1
    plot_columns = len(events)

    plot_function = pyplot.plot if not logplot else pyplot.semilogy

    pyplot.subplots_adjust(top=0.6, bottom=0.15) # Make space for the titles

    event_index = 0
    for event in sorted(events):

        print('Processing event \'{0}\''.format(event), file=sys.stderr)

        for group in range(plot_rows):

            plot_index = group * plot_columns + event_index + 1 # pyplot indices are 1 based
            pyplot.subplot(max_group+1, len(events), plot_index)

            if group in data[event]:

                print('\tprocessing group {0}'.format(group))

                for year in data[event][group]:
                    value = data[event][group][year]['M']
                    print('\t\tmale counts at year   {0} : {1}'.format(year, value))
                    if (value > 0) or not logplot:
                        male, = plot_function(year, value, 'rx', markeredgewidth=2)
                    value = data[event][group][year]['F']
                    print('\t\tfemale counts at year {0} : {1}'.format(year, value))
                    if (value > 0) or not logplot:
                        female, = plot_function(year, value, 'b+', markeredgewidth=2)

            pyplot.axis([start_year-.0833, end_year+0.0833, max(0, min_count*0.95), max_count*1.05])  # -/+ one month 

            if group == 0:
                pyplot.title(event, rotation=60, verticalalignment='bottom', fontsize=10)

            if group != max_group:
                pyplot.gca().axes.get_xaxis().set_visible(False)
            else:
                pyplot.xlabel('Year')
                locs, labels = pyplot.xticks()
                pyplot.setp(labels, rotation=90)

            if event_index == 0:
                pyplot.ylabel('Count')
            else:
                pyplot.gca().axes.get_yaxis().set_visible(False)

        ax = pyplot.gca()
        ax.ticklabel_format(useOffset=False)

        if event_index == 0:
            pyplot.legend([male, female], ['Male', 'Female'])

        event_index += 1

    pyplot.show()

    pass


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('filename', default='ReportEventRecorder.csv')
    parser.add_argument('-l', '--log', default=False, action='store_true')
    args = parser.parse_args()
    main(args.filename, args.log)

    pass
