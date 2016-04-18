import argparse
import datetime
import logging
import os
import re
import sys


PROGRESS = 25


def read_solution(solution_filename):
    with open(solution_filename) as handle:
		lines = handle.readlines()

    return lines


def filter_solution(lines):
    version   = re.compile('^Microsoft Visual Studio Solution File, Format Version ([0-9\.]+)$')
    project   = re.compile('^Project\(\"{[^}]+}\"\) = \"([^"]+)\", \"([^"]+)\", \"{([^}]+)}\"$')
    endproj   = re.compile('^EndProject$')
#    libproj   = re.compile('lib.+\.vcxproj$')
    section   = re.compile('^\s*GlobalSection\([^)]+\) = .+$')
    projentry = re.compile('^\s*{([^}]+)}.*$')
    endsect   = re.compile('^\s*EndGlobalSection\s*$')
    passed = []
    filtered_guids = {}

    def version_project_section(line):
        append = True
        fn = version_project_section
        v = version.match(line)
        p = project.match(line)
        s = section.match(line)

        if v is not None:
            format_version = float(v.group(1))
            if format_version != 12.0:
                logging.getLogger(__name__).error('Unsupported solution file version: {0}'.format(format_version))
                raise NotImplementedError()

        elif p is not None:
#            project_file = p.group(2).split('\\')[-1]
#            if libproj.match(project_file):
            project_name = p.group(1)
            project_file = p.group(2)
            if not os.path.isfile(project_file):
                logging.getLogger(__name__).info("Didn't find project file '{0}'".format(project_file))
                project_guid = p.group(3).upper()
                logging.getLogger(__name__).info("Filtering project '{0}' ({1})".format(project_name, project_guid))
                filtered_guids[project_guid] = project_file
                fn = filter_project
                append = False

        elif s is not None:
            fn = filter_section

        return append, fn

    def filter_project(line):
        append = False
        fn = filter_project
        if endproj.match(line):
            fn = version_project_section

        return append, fn

    def filter_section(line):
        append = True
        fn = filter_section
        e = projentry.match(line)
        if e is not None:
            project_guid = e.group(1).upper()
            if project_guid in filtered_guids:
                logging.getLogger(__name__).info("Filtering section entry '{0}' (for {1})".format(project_guid, filtered_guids[project_guid]))
                append = False

        elif endsect.match(line):
            fn = version_project_section

        return append, fn

    fn = version_project_section
    for line in lines:
        (append, fn) = fn(line)

        if append:
            passed.append(line)

    return passed


def write_solution(lines, output_filename):
    with open(output_filename, 'w') as handle:
		handle.writelines(lines)


def main(solution_filename, output_filename):
    logging.getLogger(__name__).log(PROGRESS, "Solution '{0}'".format(solution_filename))
    logging.getLogger(__name__).log(PROGRESS, "Output file '{0}'".format(output_filename))

    lines = read_solution(solution_filename)
    filtered = filter_solution(lines)
    write_solution(filtered, output_filename)


def setup_logging(log_file, verbosity):
    log_level = getattr(logging, verbosity.upper(), None)
    if not isinstance(log_level, int):
        raise ValueError('Invalid log level: {0}'.format(verbosity))
    logger = logging.getLogger(__name__)
    logger.setLevel(log_level)
    file_handler = logging.FileHandler(log_file)
    logger.addHandler(file_handler)
    std_error = logging.StreamHandler(sys.stderr)
    std_error.setLevel(PROGRESS)    # above INFO and below WARNING
    logger.addHandler(std_error)
    logging.getLogger(__name__).log(PROGRESS, datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--solution',  type=str, help='Solution file name [EradicationKernel.sln]', default='EradicationKernel.sln')
    parser.add_argument('-o', '--output',    type=str, help='Output file name [same as solution]',        default=None)
    parser.add_argument('-l', '--logfile',   type=str, help='Set logging file [filter.log]',              default='filter.log')
    parser.add_argument('-v', '--verbosity', type=str, help='Set logging level/verbosity [INFO]',         default='INFO')
    args = parser.parse_args()
    setup_logging(args.logfile, args.verbosity)
    if args.output is None:
        args.output = args.solution
    main(args.solution, args.output)