#!/usr/bin/python

# SUMMARY: Plot HIV prevalence, incidence rate, and mortality rate for ages 15-49 by gender
# INPUT ARGUMENTS: (1) Path to ReportHIVByAgeAndGender.csv
# OUTPUT: figure drawn on screen


#from __future__ import print_function
import argparse
import copy
import csv
import math
import matplotlib.pyplot as pyplot
import sys


MALE_ENUM = 0
FEMALE_ENUM = 1

ROWS = 'rows'
COLUMNS = 'columns'
INDEX = 'index'
LAMBDA = 'lambda'
TITLE = 'title'
XLABEL = 'x-label'
YLABEL = 'y-label'
LEGEND = 'legend'

YEAR_IDX = 0
POPULATION_IDX = 1
INFECTED_IDX = 2
NEWLY_INFECTED_IDX = 3
ON_ART_IDX = 4
DIED_IDX = 5
DIED_FROM_HIV_IDX = 6
TESTED_PAST_YEAR_IDX = 7
TESTED_EVER_IDX = 8

CUMULATIVE_MODE = 0
INSTANTANEOUS_AND_CUMULATIVE_MODE = 1

DEBUG = False

plot_definitions = [
    { ROWS: 1, COLUMNS: 4, INDEX: 1, TITLE: 'Prevalence', XLABEL: 'Year', YLABEL: 'Prevalence 15-49 (%)',                               LEGEND: True },
    { ROWS: 1, COLUMNS: 4, INDEX: 2, TITLE: 'Incidence',  XLABEL: 'Year', YLABEL: 'Incidence Rate 15-49 (Infections / Susceptible-PY)', LEGEND: False },
    { ROWS: 1, COLUMNS: 4, INDEX: 3, TITLE: 'Mortality',  XLABEL: 'Year', YLABEL: 'HIV-caused Mortality Rate 15-49 (Deaths/PY)',        LEGEND: False },
    { ROWS: 1, COLUMNS: 4, INDEX: 4, TITLE: 'ART',        XLABEL: 'Year', YLABEL: 'Anti-retroviral Therapy Coverage 15-49 (%)',         LEGEND: False },
]


def main(filename):
    data = []
    with open(filename, 'rb') as handle:
        header = None
        reader = csv.reader(handle)
        for row in reader:
            if header is None:
                header = map(str.strip, row)
                age_index = header.index('Age')
                sex_index = header.index('Gender')
            else:
                values = map(float, row)
                if (values[age_index] >= 15.0) and (values[age_index] < 50.0):
                    data.append(tuple(values))

    plot_data(data, sex_index)

    pass


def plot_data(data, sex_index):

    male_data = filter(lambda row: row[sex_index] == MALE_ENUM, data)
    female_data = filter(lambda row: row[sex_index] == FEMALE_ENUM, data)

    def prevalence_fn(year): 
        #print "Pop: {0}, Inf: {1}".format(year[POPULATION_IDX], year[INFECTED_IDX])
        return 100 * year[INFECTED_IDX] / year[POPULATION_IDX] if (year[POPULATION_IDX] > 0) else 0

    def incidence_fn(pair): 
        susceptible_population = pair[1][POPULATION_IDX] - pair[1][INFECTED_IDX]
        if susceptible_population == 0:
            if pair[0][NEWLY_INFECTED_IDX] != 0:
                print "WARNING: Returning 0 incidence because the number of susceptible individuals at the center of the reporting interval was 0."
            return 0
        return pair[0][NEWLY_INFECTED_IDX] / susceptible_population

    def mortality_fn(pair): return pair[0][DIED_FROM_HIV_IDX] / pair[1][POPULATION_IDX] if (pair[1][POPULATION_IDX] > 0) else 0

    def on_art_fn(year): return 100 * year[ON_ART_IDX] / year[INFECTED_IDX] if (year[INFECTED_IDX] > 0) else 0

    (cumulative, instantaneous) = aggregate_and_extract(male_data)

    male_prevalence = map(prevalence_fn, instantaneous)
    male_incidence = map(incidence_fn, zip(cumulative, instantaneous))
    male_mortality = map(mortality_fn, zip(cumulative, instantaneous))
    males_on_art = map(on_art_fn, instantaneous)

    (cumulative, instantaneous) = aggregate_and_extract(female_data)

    female_prevalence = map(prevalence_fn, instantaneous)
    female_incidence = map(incidence_fn, zip(cumulative, instantaneous))
    female_mortality = map(mortality_fn, zip(cumulative, instantaneous))
    females_on_art = map(on_art_fn, instantaneous)

    timeline = map(lambda row: row[0], cumulative)

    figure = pyplot.figure('HIV Summary')

    do_plot(male_prevalence, female_prevalence, timeline, plot_definitions[0])
    do_plot(male_incidence,  female_incidence,  timeline, plot_definitions[1])
    do_plot(male_mortality,  female_mortality,  timeline, plot_definitions[2])
    do_plot(males_on_art,    females_on_art,    timeline, plot_definitions[3])

    pyplot.tight_layout()

    pyplot.show()

    pass


def aggregate_and_extract(data):
    cumulative = []
    instantaneous = []
    timestamps = []

    def delta(timestamp): return math.fabs((timestamp - math.floor(timestamp)) - 0.5)

    def accumulate(entry, row):
        for i in range(4, 12): entry[i] += row[i]
        pass

    mode = CUMULATIVE_MODE #CUMULATIVE_MODE, INSTANTANEOUS_AND_CUMULATIVE_MODE
    prev_timestamp = -1

    for row in data:
        timestamp = row[0]
        if DEBUG: print "--> N ts {0} @ {1}".format(timestamp, row)
        # Year (n,n+1] belong to year n
        #year = int(math.floor(timestamp) if (timestamp != int(timestamp)) else (timestamp - 1))

        # If we haven't seen this timestamp yet, append to cumulative and instantaneous with zeros
        if timestamp != prev_timestamp:
            if DEBUG: print "New timestamp {}".format(timestamp)
            ## New (or first) timestamp
            mode = 1 - mode     # switch mode
            prev_timestamp = timestamp
            if DEBUG: print "Switched mode to {}".format(mode)

            if DEBUG: print "CUMULATIVE {0}: {1}".format(timestamp, cumulative)
            if DEBUG: print "INSTANTANEOUS {0}: {1}".format(timestamp, instantaneous)

            if mode == INSTANTANEOUS_AND_CUMULATIVE_MODE:
                cumulative.append([0] * len(row))
                instantaneous.append([0] * len(row))
                timestamps.append(timestamp)

        accumulate(cumulative[-1], row)

        if mode == INSTANTANEOUS_AND_CUMULATIVE_MODE:
            accumulate(instantaneous[-1], row)

    if DEBUG: print "CUMULATIVE: {}".format(cumulative)
    if DEBUG: print "INSTANTANEOUS: {}".format(instantaneous)
    if DEBUG: print "TIMESTAMPS: {}".format(timestamps)

    cumulative = [[timestamps[i]]+v[4:] for (i,v) in enumerate(cumulative)]
    instantaneous = [[timestamps[i]]+v[4:] for (i,v) in enumerate(instantaneous)]

    if DEBUG: print "CUMULATIVE: {}".format(cumulative)
    if DEBUG: print "INSTANTANEOUS: {}".format(instantaneous)

    return cumulative, instantaneous


def do_plot(male, female, timeline, plot_definition):

    pyplot.subplot(plot_definition[ROWS], plot_definition[COLUMNS], plot_definition[INDEX])

    pyplot.plot(timeline, male, 'b', label='Male', linewidth=2.0)
    pyplot.plot(timeline, female, 'r', label='Female', linewidth=2.0)

    pyplot.title(plot_definition[TITLE])
    pyplot.xlabel(plot_definition[XLABEL])
    pyplot.ylabel(plot_definition[YLABEL])

    locs, labels = pyplot.xticks()
    pyplot.setp(labels, rotation=90)

    if plot_definition[LEGEND]:
        pyplot.legend(loc='upper right')

    pass


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('filename', default='ReportHIVByAgeAndGender.csv')
    args = parser.parse_args()
    main(args.filename)
