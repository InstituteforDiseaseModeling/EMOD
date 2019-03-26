#!/usr/bin/python

import dtk_test.dtk_sft as sft
import dtk_test.dtk_TBHIV_Support as dts

class ConfigKeys:
    Start_Time = "Start_Time"

def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json", campaign_filename="campaign.json",
                 insetchart_name="InsetChart.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "campaign_filename: " + campaign_filename + "\n" )
        print( "insetchart_name: " + insetchart_name + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )
    sft.wait_for_done()
    param_obj = dts.load_emod_parameters(config_filename, campaign_filename)

    start_timestep = param_obj[ConfigKeys.Start_Time] # get from config

    # Now process log output (probably) and compare to theory (not in this example) or to another report.
    infections = dts.parse_stdout_file(stdout_filename, debug)
    inset_summary = dts.parse_json_report( start_timestep, output_folder, insetchart_name, debug)
    dts.create_report_file( param_obj, infections, inset_summary, report_name )

if __name__ == "__main__":
    # execute only if run as a script
    application( "output" )