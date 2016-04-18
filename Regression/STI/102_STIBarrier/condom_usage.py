#!/usr/bin/python

import argparse
import csv
import matplotlib.pyplot as pyplot


def main(filename):

    # Load data
    header = None
    time_index = None
    condom_index = None
    data = []
    with open(filename, 'rb') as handle:
        reader = csv.reader(handle)
        for row in reader:
            if header is None:
                header = map(str.strip, row)    # remove trailing spaces
                time_index = header.index('Time')
                condom_index = header.index('Did_Use_Condom')
            else:
                if int(row[condom_index]) != 0:
                    data.append(row)

    # Aggregate data
    aggregate = {}
    for entry in data:
        time = int(entry[time_index])
        if time not in aggregate:
            aggregate[time] = 0
        aggregate[time] += 1

    # Plot data
    series = [0] * (max(aggregate.keys()) + 1)
    for time in aggregate:
        series[time] = aggregate[time]

    figure = pyplot.figure('Condom Usage Over Time')
    pyplot.plot(series)
    pyplot.title('Condom Usage Over Time')
    pyplot.xlabel('Time Step')
    pyplot.ylabel('Condoms Employed')
    
    pyplot.show()

    pass


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('filename', default='RelationshipConsummated.csv')
    args = parser.parse_args()
    main(args.filename)