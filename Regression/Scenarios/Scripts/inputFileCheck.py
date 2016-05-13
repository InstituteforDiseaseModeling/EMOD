import argparse
import json
import os


def get_input_files(inputdir):
    inputs = []
    for entry in os.listdir(inputdir):
        fullpath = os.path.join(inputdir, entry)
        if os.path.isfile(fullpath):
            inputs.append(entry.lower())
    
    return inputs


def check_config(fullpath, input_files):
    handle = open(fullpath)
    config_json = json.load(handle)
    handle.close()
    parameters = config_json['parameters']
    for key in parameters:
        if (key.lower().count('filename') > 0) & (key != 'Campaign_Filename'):
            value = parameters[key].lower()
            if value:
                for filename in value.split(';'):
                    if not filename in input_files:
                        print("***** Input file '%s' referenced in parameter '%s' in '%s' not found among input files." % (filename, key, fullpath))


def process_directories(directory_list, input_files):
    while directory_list:
        directory = directory_list[0]
        directory_list = directory_list[1:]
        for entry in os.listdir(directory):
            fullpath = os.path.join(directory, entry)
            if os.path.isdir(fullpath):
                directory_list.append(fullpath)
            elif os.path.isfile(fullpath):
                if entry.lower() == 'config.json':
                    check_config(fullpath, input_files)


def main(directory, inputdir):
    print("Looking in '%s' for config files, checking against files in '%s'." % (directory, inputdir))
    input_files = get_input_files(inputdir)
    process_directories( [ directory ], input_files )


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--directory', default='.', type=str, required=False)
    parser.add_argument('-i', '--inputdir',    type=str, required=True)
    args = parser.parse_args()
    main(args.directory, args.inputdir)