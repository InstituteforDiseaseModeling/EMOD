#!/usr/bin/python

import dtk_ImportPressure_Support as ips
import dtk_sft as sft

KEY_NEW_INFECTIONS = "New Infections"
KEY_STATISTICAL_POPULATION  = "Statistical Population"

def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json",campaign_filename="campaign.json",
                 demographics_filename = "demographics_100_overlay.json",
                 insetchart_name="InsetChart.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    if debug:
        print "output_folder: " + output_folder
        print "stdout_filename: " + stdout_filename+ "\n"
        print "config_filename: " + config_filename + "\n"
        print "campaign_filename: " + campaign_filename + "\n"
        print "demographics_filename: " + demographics_filename + "\n"
        print "insetchart_name: " + insetchart_name + "\n"
        print "report_name: " + report_name + "\n"
        print "debug: " + str(debug) + "\n"

    sft.wait_for_done()

    param_obj = ips.load_emod_parameters(config_filename, debug)
    campaign_obj =  ips.load_campaign_file(campaign_filename, debug)
    demographics_obj =  ips.load_demographics_file(demographics_filename, debug)
    report_data_obj =  ips.parse_json_report(output_folder, insetchart_name, debug)

    sft.plot_data_unsorted(report_data_obj[KEY_NEW_INFECTIONS],
                           title="new infections",
                           label1= "New Infections",
                           label2 = "NA",
                           xlabel="time steps", ylabel="new infection",
                           category = 'New_infections',
                           show = True )
    sft.plot_data_unsorted(report_data_obj[KEY_STATISTICAL_POPULATION],
                           title="Statistical Population",
                           label1= "Statistical Population",
                           label2 = "NA",
                           xlabel = "time steps", ylabel="Statistical Population",
                           category = 'Statistical_popupation',
                           show = True, line = True)

    ips.create_report_file(param_obj, campaign_obj, demographics_obj, report_data_obj, report_name, debug)

if __name__ == "__main__":
    # execute only if run as a script
    application( "output" )
