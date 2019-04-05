#!/usr/bin/python

if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as sft
import math
import dtk_test.dtk_General_Support as General_Support
from dtk_test.dtk_General_Support import ConfigKeys, DemographicsKeys, InsetKeys
import numpy as np
import dtk_test.dtk_HINT_Support as hint_support

"""

This SFT is testing environmental feature 2.1 Environmental route,  2.2 Infections: Calculating total contagion,
2.5.5 Contagion normalization and 2.5.6 Reporting: PropertyReportEnvironmental:

"2.1 Environmental route. 
    Contagion is transmitted over a second route, called environmental, just like it is over the 
    contact route in Generic. Unlike for the contact route, the environmental route contagion doesn't necessarily decay to 
    zero after each timestep.

Node_Contagion_Decay_Rate. 
    The size of the environmental contagion pool for a node shall equal the previous day times 1 
    minus the Node Decay Rate, plus any new shedding for that timestep:

    Previous_pool_size * (1 - Node_Decay_Rate) + New_contagion"

"2.2 Infections
Calculating total contagion. 
    The model returns the sum across all antigens and all groups for the normalized contagion population. For example, 
    if an individual is exposed to N units of contagion on the given route, then the reported value is the sum of that 
    exposure across all possible target groups."
    
"2.5.5 Contagion normalization

    Group population. The environmental route will be normalized by group population."

"2.5.6 Reporting
        InsetChart
            InsetChart inherits all channels from GENERIC_SIM. In addition, it will contain "New Infections By Route 
            (ENVIRONMENT)," "New Infections By Route (CONTACT)," ""Contact Contagion Population," and 
            "Environmental Contagion Population.
    	PropertyReportEnvironmental
	        This report shall be produced if Enable_Property_Output is set to 1. It shall include channels from the base 
	        class (Generic), broken down by HINT group, as well as the following channels: New Infections By Route 
	        (contact), New Infections By Route (environment), Contagion (contact) and Contagion (environment)."

This test is testing the above statement with Multi Routes HINT feature.

Test data is loaded from InsetChart.json and PropertyReportEnvironmental.json. 

Suggested sweep parameters: Node_Contagion_Decay_Rate, Base_Infectivity

"""

channels = [InsetKeys.ChannelsKeys.Infected,
            "New Infections By Route (ENVIRONMENT)",
            "New Infections By Route (CONTACT)",
            "Contagion (Environment)",
            "Contagion (Contact)",
            InsetKeys.ChannelsKeys.Statistical_Population]

inset_channels = [InsetKeys.ChannelsKeys.Contact_Contagion_Population,
                  InsetKeys.ChannelsKeys.Environmental_Contagion_Population,
                  InsetKeys.ChannelsKeys.Infected,
                  InsetKeys.ChannelsKeys.Statistical_Population]

config_keys = [ConfigKeys.Config_Name, ConfigKeys.Simulation_Timestep,
               ConfigKeys.Simulation_Duration, ConfigKeys.Base_Infectivity,
               ConfigKeys.Run_Number, ConfigKeys.Demographics_Filenames,
               ConfigKeys.Enable_Heterogeneous_Intranode_Transmission,
               ConfigKeys.Node_Contagion_Decay_Rate]

routes = [
    "environmental",
    "contact"
]

def create_report_file(param_obj, property_df, property_obj, inset_chart_obj, insetchart_name, property_report_name,
                       report_name, debug):
    with open(report_name, "w") as outfile:
        config_name = param_obj[ConfigKeys.Config_Name]
        decay_rate = param_obj[ConfigKeys.Node_Contagion_Decay_Rate]
        outfile.write("Config_name = {}\n".format(config_name))
        outfile.write("{0} = {1} {2} = {3}\n".format(
            ConfigKeys.Node_Contagion_Decay_Rate, decay_rate,
            ConfigKeys.Run_Number, param_obj[ConfigKeys.Run_Number]))

        success = True

        outfile.write("Testing: Testing contagion and decay in each groups for every time step:\n")
        duration = param_obj[ConfigKeys.Simulation_Duration]
        transmission_matrix_e = property_obj[DemographicsKeys.PropertyKeys.TransmissionMatrix][routes[0]]
        base_infectivity = param_obj[ConfigKeys.Base_Infectivity]
        groups = property_obj[DemographicsKeys.PropertyKeys.Values]
        contagion = []
        for group in groups:
            outfile.write("  Testing group: {}.\n".format(group))
            contagion_list_e = []
            # get list of column names that contains the group that we are testing
            cols = [c for c in property_df.columns if group in c]
            # get the list of the other group
            other_group = [x for x in groups if x is not group][0]
            # get all the column names that contains the other group
            cols_others = [c for c in property_df.columns if group not in c]

            # test data for test group
            infected = property_df[[c for c in cols if channels[0] in c]]
            contagion_e = property_df[[c for c in cols if channels[3] in c]]
            population = property_df[[c for c in cols if channels[5] in c]]


            # test data for the other group
            infected_other = property_df[[c for c in cols_others if channels[0] in c]]
            # population_other = property_df[[c for c in cols_others if channels[5] in c]]

            pre_contagion = 0
            for t in range(duration - 1):
                if t == 0:
                    contagion_list_e.append([contagion_e.iloc[t][0], 0])
                    if contagion_e.iloc[t][0]:
                        success = False
                        outfile.write("    BAD: at time step {0}, for group {1} route {2}, the contagion is {3}, "
                                      "expected {4}.\n".format(t, group, routes[0], contagion_e.iloc[t][0],
                                                               0
                                                               ))

                infectivity = base_infectivity * infected.iloc[t][0] / population.iloc[t][0]
                infectivity_other = base_infectivity * infected_other.iloc[t][0] / population.iloc[t][0]

                # calculate contagion for environmental group
                new_contagion = infectivity * transmission_matrix_e[groups.index(group)][
                    groups.index(group)] + infectivity_other * transmission_matrix_e[groups.index(other_group)][
                    groups.index(group)]

                current_contagion = pre_contagion * (1 - decay_rate) + new_contagion

                # for next time step
                pre_contagion = current_contagion

                # get contagion from property report
                actual_contagion_e = contagion_e.iloc[t + 1][0]

                contagion_list_e.append([actual_contagion_e, current_contagion])

                if math.fabs(current_contagion - actual_contagion_e) > 5e-2 * current_contagion:
                    success = False
                    outfile.write("    BAD: at time step {0}, for group {1} route {2}, the contagion is {3}, "
                                  "expected {4}.\n".format(t + 1, group, routes[0], actual_contagion_e,
                                                           current_contagion
                    ))


            contagion.append(contagion_list_e)
            # plot actual and expected values for contagion
            sft.plot_data(np.array(contagion_list_e)[:, 0], np.array(contagion_list_e)[:, 1],
                              label1=property_report_name, label2="calculated contagion",
                              title="contagion for group {0}\n route {1}".format(group, routes[0]),
                              xlabel='day', ylabel='contagion', category="contagion_{0}_{1}".format(group, routes[0]),
                              line=True, alpha=0.5, overlap=True)

        outfile.write("Checking if contagion data from insetchart matches sum of group contagion from property report"
                      " for every time step:\n")
        total_contagion_list = []
        for t in range(duration - 1):
            total_contagion = 0
            for contagion_list_e in contagion:
                total_contagion += contagion_list_e[t][0]
            total_contagion_list.append(total_contagion)
            insetchart_contagion = inset_chart_obj[InsetKeys.ChannelsKeys.Environmental_Contagion_Population][t]
            if math.fabs(total_contagion - insetchart_contagion) > 5e-2 * total_contagion:
                success = False
                outfile.write("    BAD: at time step {0}, for route {1}, the total contagion from {2} is {3}, "
                              "expected {4}(sum of group contagion in {5}).\n".format(
                                    t, routes[0], insetchart_name, insetchart_contagion,
                                    total_contagion, property_report_name
                                                       ))
        sft.plot_data(inset_chart_obj[InsetKeys.ChannelsKeys.Environmental_Contagion_Population],
                          total_contagion_list,
                          label1=insetchart_name, label2="sum calculated from {}".format(property_report_name),
                          title=InsetKeys.ChannelsKeys.Environmental_Contagion_Population,
                          xlabel='day', ylabel='contagion',
                          category=InsetKeys.ChannelsKeys.Environmental_Contagion_Population,
                          line=True, alpha=0.5, overlap=True)

        outfile.write(sft.format_success_msg(success))
    if debug:
        print(sft.format_success_msg(success))
    return success


def application(output_folder="output", stdout_filename="test.txt",
                property_report_name="PropertyReportEnvironmental.json",
                insetchart_name="InsetChart.json",
                config_filename="config.json",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("property_report_name: " + property_report_name + "\n")
        print("insetchart_name: " + insetchart_name + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done(stdout_filename)

    config_obj = hint_support.load_config_parameters(config_filename, config_keys, debug)

    demo_path = "Assets" if stdout_filename == "StdOut.txt" else ""
    property_list = hint_support.load_demo_mr_overlay_file(
        config_obj[ConfigKeys.Demographics_Filenames][-1], demo_path, debug)
    property_keys = hint_support.build_channel_string_for_property(property_list, channels, debug)
    property_df = hint_support.parse_property_report_json(property_report_name, output_folder, property_keys, debug)
    property_obj = property_list[0] # this test only has one property object

    inset_chart_obj = General_Support.parse_inset_chart(output_folder, insetchart_name, inset_channels, debug)
    create_report_file(config_obj, property_df, property_obj, inset_chart_obj, insetchart_name, property_report_name,
                       report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-p', '--propertyreport', default="PropertyReportEnvironmental.json",
                        help="Property report to load (PropertyReportEnvironmental.json)")
    parser.add_argument('-j', '--insetchart', default="InsetChart.json",
                        help="json report to load (InsetChart.json)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                property_report_name=args.propertyreport, insetchart_name=args.insetchart,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

