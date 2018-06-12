#!/usr/bin/python

import dtk_sft as sft
import dtk_Immunity_Initialization_Support as dtk_iis


"""
This test (for Complex Initialization) operates differently:
1. Load a config file with a demographics overlay
 that declares a particular complex immunity profile
 and has loglevel_Susceptibility set to VALID
2. Capture stdout (Test.txt) on the first day of the sim
3. In dtk_post_process.py, load the stdout into a dataframe
4. Compare this dataframe to expected distributions from
 the complex immunity profile from step (1)
5. Fail if... something is wrong

NOTE: Always make sure that the expected immunity isn't close to 50%
"""


def application(output_folder="output", config_filename="config.json",
                jsonreport_name="InsetChart.json", stdout_filename="Test.txt",
                report_name=sft.sft_output_filename, debug=False):
    if debug:
        print("output_folder: " + output_folder + "\n")
        print("config_filename: " + config_filename + "\n")
        print("insetchart_name: " + jsonreport_name + "\n")
        print("stdout filename:" + stdout_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()
    config_object = dtk_iis.load_config_file(config_filename=config_filename,
                                             debug=debug)
    demographics_overlay_name = config_object[dtk_iis.ConfigKeys.DEMOGRAPHIC_FILENAMES][-1]
    campaign_name = config_object[dtk_iis.ConfigKeys.CAMPAIGN_FILENAME]
    test_config_name = config_object[dtk_iis.ConfigKeys.CONFIG_NAME]

    demographics_object = dtk_iis.load_demographics_file(
        demographics_filename=demographics_overlay_name,
        immunity_initialization_type=config_object[dtk_iis.ConfigKeys.IMM_DIST_TYPE],
        debug=debug)

    sft.start_report_file(report_name, test_config_name)

    i_df = dtk_iis.parse_stdout_file(stdout_filename, debug=debug)
    distribution_object = demographics_object[dtk_iis.DemographicFileKeys.SusceptibilityDistribution.KEY]
    distro_values = \
        distribution_object[dtk_iis.DemographicFileKeys.SusceptibilityDistribution.ComplexKeys.DistributionValues][0]
    distro_results = \
        distribution_object[dtk_iis.DemographicFileKeys.SusceptibilityDistribution.ComplexKeys.ResultValues][0]
    if debug:
        print("Imm distribution type: {0}\n".format(config_object[dtk_iis.ConfigKeys.IMM_DIST_TYPE]))
        print("Distribution object: {0}\n".format(distribution_object))
        print("Distro values: {0}\n".format(distro_values))
    with open(report_name, "a") as outfile:
        messages = dtk_iis.validate_complex_initialization_numpy(
            individual_dataframe=i_df, susceptibility_distribution=distribution_object,
            outfile=outfile, debug=debug)
    pass

    # dtk_iis.create_report_file(expected_infections_obj, actual_infections,
    #                            outbreak_day, report_name,
    #                            debug=debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-j', '--jsonreport', default="InsetChart.json", help="Json report to load (InsetChart.json)")
    parser.add_argument('-s', '--stdout', default="Test.txt", help="Name of stdoutfile to parse (Test.txt")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    args = parser.parse_args()

    application(output_folder=args.output, config_filename=args.config,
                jsonreport_name=args.jsonreport,
                report_name=args.reportname, debug=True)
