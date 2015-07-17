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
                data.append(row)

    # Aggregate data
    aggregate = {}
    acts = {}
    print "Condom index is " + str(condom_index)
        
    for entry in data:
        time = int(entry[time_index])
        if time not in aggregate:
            aggregate[time] = 0
            acts[time] = 0
        acts[time] += 1
        if entry[condom_index] == "1":
            aggregate[time] += 1

    total_uses = 0
    for time in aggregate:
        total_uses += aggregate[time]
    print "Total uses: " + str(total_uses)
    # Plot data
    series = [0] * (max(aggregate.keys()) + 1)
    for time in aggregate:
        series[time] = float(aggregate[time])/float(acts[time])

    figure = pyplot.figure('Condom Usage Over Time')
    pyplot.plot(series)
    pyplot.title('Condom Usage Over Time')
    pyplot.xlabel('Time Step')
    pyplot.ylabel('Condom Usage Rate')
    
    pyplot.show()

    pass


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('filename', default='RelationshipConsummated.csv')
    args = parser.parse_args()
    main(args.filename)
