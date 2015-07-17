"""
/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/
"""

#from __future__ import print_function

import argparse
import datetime
import json
import logging
import os
import re
import shutil
import sys
import xml.etree.ElementTree as et


PROGRESS = 25

globals = {}
# the entries in 'directory_whitelist' should all be lower case (regardless of the directory name casing on disk)
globals['directory_whitelist'] = [ 'basereportlib', 'cajun', 'campaign', 'componenttests', 'dependencies', 'eradication', 'interventions', 'jsonspirit', 'quickstart', 'rapidjson', 'regression', 'reporters', 'scripts', 'snappy', 'unittest++', 'utils' ]
globals['directory_blacklist'] = [ '.svn', 'cajun/test', 'Regression/reports', 'Regression/Polio', 'Regression/NotreDame' ]
# the entries in 'reporter_whitelist' should be tuples of solution files, the associated directory, and the project file
globals['reporter_whitelist'] = [ ('TBCustomReporterRelease.sln', 'lib_custom_tb_reporter_Scenarios', 'lib_custom_tb_reporter_Scenarios.vcxproj'), ('BasicReportPlugin.sln', 'libreportpluginbasic', 'BaseReportLib.vcxproj'), ('BasicReportPlugin.sln', 'libreportpluginbasic', 'libreportpluginbasic.vcxproj'), ('BasicReportPlugin.sln', 'libreporteventcounter', 'libreporteventcounter.vcxproj') ]
# the entries in 'file_whitelist' and 'file_blacklist' should all be lower case (regardless of the file name casing on disk)
globals['file_whitelist'] = [ 'eradicationkernel.sln', 'license.txt', 'notices.txt' ]
globals['file_blacklist'] = [ 'sconscript', 'status.txt', 'time.txt', 'transitions.json' ]
globals['project_filter_exclude'] = set(['environmental', 'polio', 'tbhiv', 'hivtb'])
globals['excluded_preprocessor_defines'] = set(['ENABLE_POLIO', 'ENABLE_TBHIV', 'ENABLE_PYTHON'])
globals['regression_directory_file_whitelist'] = [ 'hiv.json', 'malaria.json', 'multicore.json', 'param_sweep.json', 'plotallcharts.py',
   'plotnewinfectionsbypool.py', 'plotsirchannels.py', 'prettyprintjson.py',
   'primaryscenarios.json', 'regression_test.cfg', 'regression_test.py',
   'regression_utils.py', 'samples.json', 'sanity.json', 'sti.json', 'sti_hiv_samples.json', 'tb_samples.json', 'update_baselines.py',
   'vector.json', 'warning.txt', 'win_v2_0.json' ]
globals['allowed_sim_types'] = [ 'GENERIC_SIM', 'VECTOR_SIM', 'MALARIA_SIM', 'AIRBORNE_SIM', 'TB_SIM', 'STI_SIM', 'HIV_SIM' ]


def process_directory_entries(predicate, path, whitelist, action):
    for entry in os.listdir(path):
        full_path = os.path.join(path, entry)
        if predicate(full_path):
            if not entry.lower() in whitelist:
                logging.getLogger(__name__).info("Calling '{0}' on '{1}'".format(action.__name__, full_path))
                action(full_path)
            else:
                logging.getLogger(__name__).debug("Skipping '{0}' on '{1}' (found in whitelist).".format(action.__name__, full_path))

    pass


def filter_directories(path, whitelist):
    process_directory_entries(os.path.isdir, path, whitelist, shutil.rmtree)
    pass


def filter_files(path, whitelist):
    process_directory_entries(os.path.isfile, path, whitelist, os.remove)
    pass


def filter_reporters(root_directory):
    reporter_directory = os.path.join(root_directory, 'reporters')
    file_whitelist = [ entry[0].lower() for entry in globals['reporter_whitelist'] ]
    filter_files(reporter_directory, file_whitelist)
    directory_whitelist = [ entry[1].lower() for entry in globals['reporter_whitelist'] ]
    filter_directories(reporter_directory, directory_whitelist)

    pass


def clean_directories(root_directory):
    logging.getLogger(__name__).log(PROGRESS, "Cleaning directories below '{0}'".format(root_directory))

    # remove directories we don't ship
    whitelist = globals['directory_whitelist']
    filter_directories(root_directory, whitelist)

    blacklist = globals['directory_blacklist']
    for directory in blacklist:
        full_path = os.path.join(root_directory, directory)
        if os.path.exists(full_path):
            if os.path.isdir(full_path):
                logging.getLogger(__name__).info("Removing directory '{0}'".format(full_path))
                shutil.rmtree(full_path)
            else:
                logging.getLogger(__name__).error("Found '{0}' but it isn't a directory?".format(full_path))
        else:
            logging.getLogger(__name__).debug("os.path.exists('{0}') returned false.">format(full_path))

    filter_reporters(root_directory)

    # clean build directories if present
    for root, directories, files in os.walk(root_directory):
        for directory in directories:
            full_path = os.path.join(root, directory)
            if directory.lower() == 'x64':
                logging.getLogger(__name__).info("Cleaning build directory '{0}'".format(full_path))
                shutil.rmtree(full_path)
            else:
                logging.getLogger(__name__).debug("Ignoring directory'{0}'".format(full_path))

    pass


def clean_files(root_directory):
    logging.getLogger(__name__).log(PROGRESS, "Cleaning files below '{0}'".format(root_directory))

    # remove files we don't ship
    whitelist = globals['file_whitelist']
    filter_files(root_directory, whitelist)

    # remove lib*vcxproj files we don't need for building Eradication.exe
    proj = re.compile('lib.+\.vcxproj$')
    project_whitelist = [ entry[2].lower() for entry in globals['reporter_whitelist'] ]
    filtered_files = globals['file_blacklist']
    for root, directories, files in os.walk(root_directory):
        for file in files:
            full_path = os.path.join(root, file)
            p = proj.match(file.lower())
            if p:
                if not file.lower() in project_whitelist:
                    logging.getLogger(__name__).info("Removing project file '{0}'".format(full_path))
                    os.remove(full_path)
                else:
                    logging.getLogger(__name__).debug("Keeping project file '{0}' found in project whitelist.".format(file))
            elif file.lower() in filtered_files:
                logging.getLogger(__name__).info("Removing file '{0}'".format(full_path))
                os.remove(full_path)
            else:
                logging.getLogger(__name__).debug("Ignoring file '{0}' (doesn't match project pattern or filtered_files list)".format(full_path))

    pass


def gather_source_files(directory):
    logging.getLogger(__name__).log(PROGRESS, "Gathering source files (and project files) below '{0}'".format(directory))

    sources = []
    projects = []
    code = re.compile('.+\.(cpp|h|hpp|tlh|inl|rc|cc)$') # definition of "source file"
    proj = re.compile('.+\.vcxproj$')                   # definition of "project file"
    for root, directories, files in os.walk(directory):
        for file in files:
            c = code.match(file)
            p = proj.match(file)
            full_path = os.path.abspath(os.path.join(root, file)).lower()
            if c:
                logging.getLogger(__name__).debug("Found source file '{0}'".format(full_path))
                sources.append(full_path)
            elif p:
                logging.getLogger(__name__).debug("Found project file '{0}'".format(full_path))
                projects.append(full_path)
            else:
                logging.getLogger(__name__).debug("Ignoring file '{0}' (neither code nor project)".format(full_path))

    sources.sort()
    projects.sort()

    return sources, projects


def get_namespace(element):
    m = re.match('\{.*\}', element.tag)
    namespace = m.group(0) if m else ''
    logging.getLogger(__name__).debug("Found namespace '{0}'".format(namespace))
    return namespace


def update_project_files(project_files):
    logging.getLogger(__name__).log(PROGRESS, 'Updating project files.')

    et.register_namespace('',"http://schemas.microsoft.com/developer/msbuild/2003") # this is the default namespace in Msft project files
    exclude = globals['project_filter_exclude']
    for project_file in project_files:
        logging.getLogger(__name__).debug("Updating project file '{0}'".format(project_file))
        update = False
        filters_file = project_file + '.filters'
        project = et.parse(project_file)
        filters = et.parse(filters_file)
        
        # map each element to its parent which we need if we want to remove the element
        project_map = {c:p for p in project.iter() for c in p}
        filters_map = {c:p for p in filters.iter() for c in p}
        
        project_root = project.getroot()
        filters_root = filters.getroot()
        namespace = get_namespace(project_root)

        # look for elements of these types with an 'Include' attribute
        for type in [ 'ClCompile', 'ClInclude', 'None', 'ResourceCompile' ]:
            for element in project_root.findall('.//{0}{1}[@Include]'.format(namespace, type)):
                file = element.attrib['Include']
                filter_parent = filters_root.find('.//{0}{1}[@Include="{2}"]'.format(namespace, type, file))
                filter = filter_parent.find('{0}Filter'.format(namespace)) if filter_parent is not None else None
                if filter is not None:
                    folders = set(filter.text.lower().split("\\"))
                    if folders & exclude:
                        logging.getLogger(__name__).info("Removing element '{0}' ({1})".format(et.tostring(element), filter.text))
                        project_map[element].remove(element)
                        filters_map[filter_parent].remove(filter_parent)
                        update = True
                    else:
                        logging.getLogger(__name__).debug("Skipping element '{0}' ('{1}') as its folder wasn't in the exclude list".format(et.tostring(element), filter.text))
                else:
                    logging.getLogger(__name__).debug("Skipping element '{0}' as no entry was found in the filters file".format(et.tostring(element)))

        logging.getLogger(__name__).info('Checking PreprocessorDefinitions:')
        for element in project_root.findall('.//{0}PreprocessorDefinitions'.format(namespace)):
            definitions = element.text.split(';')
            for define in globals['excluded_preprocessor_defines']:
                if define in definitions:
                    logging.getLogger(__name__).info("Removing '{0}' from preprocessor definitions '{1}'".format(define, element.text))
                    definitions.remove(define)
                    update = True
                else:
                    logging.getLogger(__name__).debug("'{0}' not found in preprocessor definitions '{1}'".format(define, element.text))

            element.text = ';'.join(definitions)

        if update:
            logging.getLogger(__name__).info("Writing new project file '{0}'".format(project_file))
            project.write(project_file, xml_declaration=True, encoding='utf-8')
            logging.getLogger(__name__).info("Writing new project filters file '{0}'".format(filters_file))
            filters.write(filters_file, xml_declaration=True, encoding='utf-8')
        else:
            logging.getLogger(__name__).debug("Skipping write of project '{0}' and filters '{1}' files as there were no updates.".format(project_file, filters_file))

    pass


def gather_source_references(project_files):
    logging.getLogger(__name__).log(PROGRESS, 'Gathering source references from project files.')
    references = set()
    for project in project_files:
        logging.getLogger(__name__).debug("Processing project file '{0}'".format(project))
        root_directory = os.path.dirname(project)
        tree = et.parse(project)
        root_element = tree.getroot()
        ns = get_namespace(root_element)
        for type in [ 'ClCompile', 'ClInclude', 'None', 'ResourceCompile' ]:
            for element in root_element.findall('.//%s%s[@Include]' % (ns, type)):
                references.add(os.path.abspath(os.path.join(root_directory, element.attrib['Include'])).lower())

    reference_list = list(references)
    reference_list.sort()

    return reference_list


def mismatches(files, references):
    file_set = set(files)
    ref_set = set(references)

    return list(file_set - ref_set), list(ref_set - file_set)


def show_files(header, files):
    logging.getLogger(__name__).info('\n' + header)
    for file in files:
        logging.getLogger(__name__).info(file)

    pass


def remove_files(file_list):
    for file in file_list:
        logging.getLogger(__name__).info("Removing '{0}'".format(file))
        os.remove(file)

    pass


def filter_regression_directory(root_directory):
    regression_path = os.path.join(root_directory, 'Regression')
    logging.getLogger(__name__).log(PROGRESS, "\nFiltering regression directory '{0}'".format(regression_path))
    whitelist = globals['regression_directory_file_whitelist']
    filter_files(regression_path, whitelist)

    allowed_sims = globals['allowed_sim_types']
    for root, directories, files in os.walk(regression_path):
        for file in files:
            if file.lower() == 'config.json':
                handle = open(os.path.join(root, file))
                json_data = json.load(handle)
                handle.close()
                if ('parameters' in json_data) and \
                   ('Simulation_Type' in json_data['parameters']) and \
                   (not json_data['parameters']['Simulation_Type'] in allowed_sims):
                       logging.getLogger(__name__).info("Removing regression directory '{0}' (simulation type {1})".format(root, json_data['parameters']['Simulation_Type']))
                       shutil.rmtree(root)

    pass


def main(root_directory):
    logging.getLogger(__name__).log(PROGRESS, "Root directory: '{0}'".format(root_directory))

    clean_directories(root_directory)
    clean_files(root_directory)

    source_files, project_files = gather_source_files(root_directory)

    update_project_files(project_files)

    source_refs = gather_source_references(project_files)
    unreferenced_files, missing_references = mismatches(source_files, source_refs)

    show_files('---===### Source files found on disk ###===---', source_files)
    show_files('---===### Source files referenced in projects ###===---', source_refs)
    show_files('---===### Unreferenced files on disk ###===---', unreferenced_files)
    show_files('---===### Missing references from projects ###===---', missing_references)

    remove_files(unreferenced_files)

    filter_regression_directory(root_directory)

    pass


def validate_root(directory):
    solution_file = os.path.join(directory, 'EradicationKernel.sln')
    return os.path.exists(solution_file)


def setup_logging(log_file, verbosity):
    log_level = getattr(logging, verbosity.upper(), None)
    if not isinstance(log_level, int):
        raise ValueError('Invalid log level: %s' % verbosity)
    logger = logging.getLogger(__name__)
    logger.setLevel(log_level)
    file_handler = logging.FileHandler(log_file)
    logger.addHandler(file_handler)
    std_error = logging.StreamHandler(sys.stderr)
    std_error.setLevel(PROGRESS)    # above INFO and below WARNING
    logger.addHandler(std_error)
    logging.getLogger(__name__).log(PROGRESS, datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'))

    pass


if __name__ == "__main__":
    exit_value = 0
    parser = argparse.ArgumentParser()
    parser.add_argument("-d", "--directory", type=str, default=os.getcwd(), help='Root directory of source tree (contains EradicationKernel.sln)')
    parser.add_argument("-l", "--logfile", type=str, default='filter.log', help='Set logging file [filter.log]')
    parser.add_argument("-v", "--verbosity", type=str, default='INFO', help='Set logging level/verbosity [INFO]')
    args = parser.parse_args()
    setup_logging(args.logfile, args.verbosity)
    if (validate_root(args.directory)):
        main(args.directory)
    else:
        logging.getLogger(__name__).error("Didn't find 'EradicationKernel.sln' in directory '{0}'.".format(os.path.abspath(args.directory)))
        exit_value = 1

    sys.exit(exit_value)
