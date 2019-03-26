import argparse
import json
import jsontools as jt
import sys


def replace_chunk(data):
    # While we can still find Choices...
    found_context = jt.find_key_context("Choices", data)
    while found_context:

        # Draw out all the values
        choices = found_context["Choices"]
        choice_names = [choice for choice in choices]
        choice_values = [choices[choice] for choice in choices]

        # Replace it with the right structure
        found_context["Choice_Names"] = choice_names
        found_context["Choice_Probabilities"] = choice_values

        # Delete Choices
        jt.delete_key("Choices", data, 1)
        print("Replaced Choices")

        # Find the next Choices
        found_context = jt.find_key_context("Choices", data)


def replace_chunk_in_file(file_path):
    # Get data from json file
    with open(file_path) as file:
        file_data = json.load(file)

    # Fix up json file
    replace_chunk(file_data)

    # Write data back to json file
    with open(file_path, 'w') as f:
        json.dump(file_data, f, sort_keys=True, indent=4, separators=(',', ': '))


def replace_chunks_in_collection(file_path):
    files = []
    # Read in the collection
    with open(file_path, "r") as file:
        for line in file:
            files.append(line.strip())

    # Replace chunks in every file listed
    for file in files:
        replace_chunk_in_file(file)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--file_path", help="Path to the JSON file for replacement")
    parser.add_argument("--collection_path", help="Path to the file pointing to JSON files for replacement")
    args = parser.parse_args()

    if args.file_path and args.collection_path or not args.file_path and not args.collection_path:
        print("Exactly one of file_path or collection_path must be specified")
        sys.exit(2)
    if args.file_path:
        replace_chunk_in_file(args.file_path)
    elif args.collection_path:
        replace_chunks_in_collection(args.collection_path)
