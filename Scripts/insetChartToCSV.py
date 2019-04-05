import csv
import json
import os
import argparse
import shutil


def insetChartToCSV(foldername, transpose, no_newline):
    newline = '\r\n'
    if no_newline or transpose:
        newline = ''

    input_file = os.path.join(foldername,'output','insetchart.json')
    with open(input_file,'r') as jf:
        j = json.loads(jf.read())

    output_file = os.path.join(foldername,'output','insetchart.csv')
    with open(output_file,'w', newline=newline) as csvfile:
        writer = csv.writer(csvfile)

        for channel_name,channel in j['Channels'].items():
            data = channel['Data']
            writer.writerow([channel_name]+data)

    if transpose:
        output_file_transpose = output_file + "_transpose"
        with open(output_file) as input_csv, open(output_file_transpose, 'w', newline=newline) as output_csv:
            csv.writer(output_csv).writerows(zip(*csv.reader(input_csv)))
        shutil.move(output_file_transpose, output_file)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('working_directory', help='Directory containing ./output/InsetChart.json')
    parser.add_argument('-t', '--transpose', action='store_true', help='Transpose data (one channel is contained '
                                                                       'in one column), assumes --no-newline')
    parser.add_argument('-n', '--no-newline', action='store_true', help='Omit empty channels between channels')
    args = parser.parse_args()

    insetChartToCSV(args.working_directory, args.transpose, args.no_newline)
