import csv
import json
import os
import sys

def insetChartToCSV(foldername):
    with open(os.path.join(foldername,'output','insetchart.json'),'r') as jf:
        j = json.loads(jf.read())

    with open(os.path.join(foldername,'output','insetchart.csv'),'wb') as csvfile:
        writer = csv.writer(csvfile)

        for channel_name,channel in j['Channels'].items():
            data = channel['Data']
            writer.writerow([channel_name]+data)

if __name__ == "__main__":
    if len( sys.argv ) != 2:
        print( "Usage: insetChartToCSV.py <working_directory>" )
        sys.exit(0)

    insetChartToCSV( sys.argv[1] )
