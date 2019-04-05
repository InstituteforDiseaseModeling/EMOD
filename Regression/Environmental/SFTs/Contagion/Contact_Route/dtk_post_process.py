#!/usr/bin/python

if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir( str(Path(sys.argv[0]).parent) )
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as sft
import math
import dtk_test.dtk_General_Support as General_Support
from dtk_test.dtk_General_Support import ConfigKeys, InsetKeys
import numpy as np
import dtk_test.dtk_HINT_Support as hint_support

"""

This SFT is testing the following statements when individual property is not enabled:

    "The contact route contagion decays to zero after each timestep."

    "The contact route will be normalized by total population.(2.5.5 Contagion normalization)"
    
    ""2.5.6 Reporting
        InsetChart
            InsetChart inherits all channels from GENERIC_SIM. In addition, it will contain "New Infections By Route 
            (ENVIRONMENT)," "New Infections By Route (CONTACT)," ""Contact Contagion Population," and 
            "Environmental Contagion Population."

In this test, the environmental route transmission is disabled(HINT multipliers are all 0). We only look at Contact
route contagion in this SFT.

Test data is loaded from InsetChart.json. (Infected, Contact_Contagion_Population and Statistical_Population channels)

Suggested sweep parameter: Base_Infectivity

"""

channels = [InsetKeys.ChannelsKeys.Contact_Contagion_Population,
            InsetKeys.ChannelsKeys.Environmental_Contagion_Population,
            InsetKeys.ChannelsKeys.Infected,
            InsetKeys.ChannelsKeys.Statistical_Population]

config_keys = [ConfigKeys.Config_Name,
               ConfigKeys.Simulation_Timestep,
               ConfigKeys.Simulation_Duration,
               ConfigKeys.Base_Infectivity,
               ConfigKeys.Run_Number,
               ConfigKeys.Enable_Heterogeneous_Intranode_Transmission]

routes = [
    "environmental",
    "contact"
]


def create_report_file(param_obj, inset_chart_obj, report_name, insetchart_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[ConfigKeys.Config_Name]
        base_infectivity = param_obj[ConfigKeys.Base_Infectivity]
        outfile.write("Config_name = {}\n".format(config_name))
        outfile.write("{0} = {1} {2} = {3}\n".format(
            ConfigKeys.Base_Infectivity, base_infectivity,
            ConfigKeys.Run_Number, param_obj[ConfigKeys.Run_Number]))

        if param_obj[ConfigKeys.Enable_Heterogeneous_Intranode_Transmission] == 1:
            outfile.write("WARNING: {0} = {1}, expected this feature is disabled".format(
                ConfigKeys.Enable_Heterogeneous_Intranode_Transmission,
            param_obj[ConfigKeys.Enable_Heterogeneous_Intranode_Transmission]))

        success = True

        outfile.write("Testing: Testing contagion for every time step:\n")
        duration = param_obj[ConfigKeys.Simulation_Duration]

        contagion_list_c = []

        for t in range(duration - 1):
            infected = inset_chart_obj[InsetKeys.ChannelsKeys.Infected][t]
            # population = inset_chart_obj[InsetKeys.ChannelsKeys.Statistical_Population][t]
            # calculate contagion
            # The Infected channel in InsetChart is normalized data
            calculated_contagion = base_infectivity * infected # / population

            # get contagion from insetchart file
            actual_contagion_c = inset_chart_obj[InsetKeys.ChannelsKeys.Contact_Contagion_Population][t]

            contagion_list_c.append([actual_contagion_c, calculated_contagion])

            if math.fabs(calculated_contagion - actual_contagion_c) > 5e-2 * calculated_contagion:
                success = False
                outfile.write("    BAD: at time step {0}, for route {1}, the total contagion is {2}, "
                              "expected {3}.\n".format(t, routes[1], actual_contagion_c,
                                                       calculated_contagion
                ))

        # plot actual and expected values for contagion
        sft.plot_data(np.array(contagion_list_c)[:, 0], np.array(contagion_list_c)[:, 1],
                          label1=insetchart_name, label2="calculated contagion",
                          title=InsetKeys.ChannelsKeys.Contact_Contagion_Population,
                          xlabel='day',ylabel='contagion',category="contagion_{}".format(routes[1]),
                          line=True, alpha=0.5, overlap=True)

        outfile.write(sft.format_success_msg(success))
    if debug:
        print(sft.format_success_msg(success))
    return success


def application(output_folder="output", stdout_filename="test.txt",
                insetchart_name="InsetChart.json",
                config_filename="config.json",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("insetchart_name: " + insetchart_name + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done(stdout_filename)

    config_obj = hint_support.load_config_parameters(config_filename, config_keys, debug)
    inset_chart_obj = General_Support.parse_inset_chart(output_folder, insetchart_name, channels, debug)
    create_report_file(config_obj, inset_chart_obj, report_name, insetchart_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-p', '--insetchart', default="InsetChart.json",
                        help="json report to load (InsetChart.json)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                insetchart_name=args.insetchart,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

