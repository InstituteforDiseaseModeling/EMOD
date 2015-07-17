from __future__ import print_function

import argparse
import os
import sys
import uuid


def generate_unique_id(prefix, suffix):
    return prefix + ''.join(str(uuid.uuid1()).split('-')) + suffix


class Directory(object):
    def __init__(self, path):
        self._id = generate_unique_id('DIR', '')
        self._path = path
        self._name = os.path.basename(self._path)
        self._children = []
        self._files = []
        contents = os.listdir(self._path)
        for entry in contents:
            fullpath = os.path.join(self._path, entry)
            if os.path.isdir(fullpath):
                self._children.append(Directory(fullpath))
            elif os.path.isfile(fullpath):
                self._files.append(entry)
            else:
                print("Warning - '%s' is neither a file nor a directory?" % entry, file=sys.stderr)

    def get_id(self):
        return self._id

    def get_name(self):
        return self._name

    def get_children(self):
        return self._children

    def get_files(self):
        return self._files

    def get_fullpath(self):
        return self._path
    
    id = property(get_id)
    name = property(get_name)
    children = property(get_children)
    files = property(get_files)
    fullpath = property(get_fullpath)


def load_template_file(template_filename):
    handle = open(template_filename)
    template = handle.read()
    handle.close()
    print("Successfully loaded '%s'" % template_filename)

    return template


def exclude_file(filename):
    return filename.lower() in ("param_overrides.json", "time.txt", "status.txt", "inputfilecheck.py" )


def dump_directory_contents(directory, registry_path, build_directory):
    def dump_directory(subdirectory, key_path):
        header = "<Directory Id='%s' Name='%s'>" % (subdirectory.id, subdirectory.name)
        subdirectory_entry = [header]
        (child_contents, child_components) = dump_directory_contents(subdirectory, key_path, build_directory)
        subdirectory_entry.extend(child_contents)
        subdirectory_entry.append("</Directory>")

        return subdirectory_entry, child_components

    contents = []
    components = []
    for subdirectory in directory.children:
        (subdirectory_contents, subdirectory_components) = dump_directory(subdirectory, '\\'.join([registry_path, directory.name]))
        contents.extend(subdirectory_contents)
        components.extend(subdirectory_components)

    component_id = generate_unique_id('COMP', '')
    component_guid = uuid.uuid1()
    components.append(component_id)
    component_header = "<Component Id='%s' Guid='%s' >" % (component_id, str(component_guid))
    contents.append(component_header)
    contents.append("<CreateFolder />")
    contents.append("<RemoveFile Id='%s' Name='*.*' On='uninstall' />" % generate_unique_id('REMOVE', ''))
    contents.append("<RemoveFolder Id='%s' On='uninstall' />" % generate_unique_id('REMOVE', ''))
    contents.append("<RegistryKey Root='HKCU' Key='%s'>" % registry_path)
    contents.append("<RegistryValue Name='%s' Value='1' Type='integer' />" % directory.name)
    contents.append("</RegistryKey>")
    if directory.name != 'output':
        for file in directory.files:
            if not exclude_file(file):
                file_id = generate_unique_id('FILE', '')
                path_to_source = os.path.relpath(os.path.join(directory.fullpath, file), build_directory)
                contents.append("<File Id='%s' Source='%s' />" % (file_id, path_to_source))

    contents.append("</Component>")

    return contents, components


def build_samples_component(build_directory, samples_directory):
    samples = Directory(samples_directory)
    (samples_entries, samples_components) = dump_directory_contents(samples, r'Software\EMOD\QuickStart\2.0.0', build_directory)
    samples_text = '\n'.join(samples_entries)
    
    return samples_text, samples_components


def build_component_group(components):
    text = ''
    elements = ["<Fragment>"]
    if components:
        elements.append("<ComponentGroup Id='SamplesComponents'>")
        for component in components:
            elements.append("<ComponentRef Id='%s' />" % str(component))
        elements.append("</ComponentGroup>")
        elements.append("</Fragment>")
        text = '\n'.join(elements)

    return text


def build_input_files_component(build_directory, input_files_directory):
    input_directory = Directory(input_files_directory)
    contents = []
    contents.append("<CreateFolder />")
    contents.append("<RemoveFolder Id='%s' On='uninstall' />" % generate_unique_id('REMOVE', ''))
    contents.append(r"<RegistryKey Root='HKCU' Key='Software\EMOD\QuickStart\2.0.0'>")
    contents.append("<RegistryValue Name='InputFiles' Value='1' Type='integer' />")
    contents.append("</RegistryKey>")
    for file in input_directory.files:
        file_id = generate_unique_id('FILE', '')
        path_to_source = os.path.relpath(os.path.join(input_directory.fullpath, file), build_directory)
        contents.append("<File Id='%s' Source='%s' />" % (file_id, path_to_source))

    text = '\n'.join(contents)
    return text


def write_wix_file(text, output_filename):
    handle = open(output_filename, 'w')
    handle.write(text)
    handle.close()
    print ("Wrote WIX file to '%s'" % output_filename)


def main(template_filename, build_directory, samples_directory, input_files_directory, output_filename):
    template = load_template_file(template_filename)
    (samples_text, samples_components) = build_samples_component(build_directory, samples_directory)
    template = template.replace("<SamplesComponent/>", samples_text)
    component_group_text = build_component_group(samples_components)
    template = template.replace("<SamplesComponentGroup/>", component_group_text)
    input_files_component_text = build_input_files_component(build_directory, input_files_directory)
    template = template.replace("<InputFilesComponent/>", input_files_component_text)
    write_wix_file(template, output_filename)


def display_arguments(args):
    print("Template file '%s'" % args.template)
    print("Installer build directory '%s'" % os.path.abspath(args.builddir))
    print("Samples root directory '%s'" % os.path.abspath(args.samplesdir))
    print("Input files directory '%s'" % os.path.abspath(args.inputdir))
    print("Output file '%s'" % args.output)


if __name__ == "__main__":
    print('%s' % sys.argv[0])
    parser = argparse.ArgumentParser()
    parser.add_argument('-t', '--template', default='product.xml', type=str, required=False)
    parser.add_argument('-b', '--builddir', default='.', type=str, required=False)
    parser.add_argument('-s', '--samplesdir', type=str, required=True)
    parser.add_argument('-i', '--inputdir', type=str, required=True)
    parser.add_argument('-o', '--output', default='product.wxs', type=str, required=False)
    args = parser.parse_args()
    display_arguments(args)
    main(args.template, args.builddir, args.samplesdir, args.inputdir, args.output)
