#!/usr/bin/python

import dtk_sft
import dtk_TBHIV_Reporter_Support as trs

"""
Reporter: DiseaseDeaths

This SFT is testing the DiseaseDeaths column in the Report_TBHIV_ByAge.csv match the count for deaths in Active Symptomatic 
state in stdout.txt.

"""


class ReportColumn:
    year = "Year"
    diseasedeaths = " DiseaseDeaths"
    agebin = " AgeBin"


matches = ["Update(): Time: ",
           "moved from Active Symptomatic to Death"
           ]


def application( output_folder="output", stdout_filename="test.txt", reporter_filename="Report_TBHIV_ByAge.csv",
                 config_filename="config.json",
                 report_name=dtk_sft.sft_output_filename,
                 debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "reporter_filename: " + reporter_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )

    dtk_sft.wait_for_done()
    param_obj = trs.load_emod_parameters(config_filename, debug)
    output_dict = trs.parse_output_file(matches, ReportColumn.diseasedeaths, stdout_filename,
                                        param_obj[trs.Config.simulation_timestep], debug)
    report_column_list = [ReportColumn.year, ReportColumn.agebin, ReportColumn.diseasedeaths]
    reporter_df = trs.parse_custom_reporter(report_column_list, reporter_path=output_folder, reporter_filename=reporter_filename, debug=debug)
    trs.create_report_file_incidence(ReportColumn.diseasedeaths, ReportColumn.year, param_obj, output_dict, reporter_df,
                                     report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-j', '--reportername', default="Report_TBHIV_ByAge.csv", help="reporter to test(Report_TBHIV_ByAge.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', default=False, help="Debug = True or False")
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout, reporter_filename=args.reportername,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)
