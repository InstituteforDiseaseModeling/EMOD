#!/usr/bin/python

import dtk_InfectivityScalingBoxcar_Support as isb_support
import dtk_sft

KEY_SIMULATION_TIMESTEP = "Simulation_Timestep"

def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json",
                 insetchart_name="InsetChart.json",
                 report_name=dtk_sft.sft_output_filename,
                 debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "insetchart_name: " + insetchart_name + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )
    dtk_sft.wait_for_done()
    param_obj = isb_support.load_emod_parameters(config_filename)
    simulation_timestep = param_obj[KEY_SIMULATION_TIMESTEP]
    output_df = isb_support.parse_output_file(stdout_filename, simulation_timestep, debug)
    report_df = isb_support.parse_json_report(output_folder, insetchart_name, debug)
    isb_support.create_report_file(param_obj, output_df, report_df, report_name, debug)

if __name__ == "__main__":
    # execute only if run as a script
    application( "output" )