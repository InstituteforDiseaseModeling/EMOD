#!/usr/bin/python

import dtk_test.dtk_sft as sft
import math
import dtk_test.dtk_General_Support as General_Support
from dtk_test.dtk_General_Support import ConfigKeys, InsetKeys
import numpy as np
import dtk_test.dtk_HINT_Support as hint_support
from scipy import stats

"""

This SFT is testing environmental feature 2.2 Infections and 2.5.6 Reporting: InsetChart without Multi Route HINT. The 
HINT feature with the statements are tested in \Regression\Environmental\SFTs\Multi_Route_HINT.

"Superinfection. 
    Superinfection is not supported, so individuals may not be infected from both the environmental and the contact 
    route. Individuals are exposed to the contagion first from one route, then the other route, only one of which may 
    cause an infection.
Order of exposures. 
    The order of exposure (which contagion route is first) is determined by the HINT specification. By default, 
    exposure via the contact route occurs prior to the environmental route."

"2.5.6 Reporting
    InsetChart
    InsetChart inherits all channels from GENERIC_SIM. In addition, it will contain "New Infections By Route 
    (ENVIRONMENT)," "New Infections By Route (CONTACT)," ""Contact Contagion Population," and 
    "Environmental Contagion Population.""

Test data is loaded from InsetChart.json. The test calculated the expected values for "New Infections By Route (CONTACT)"
and "New Infections By Route (ENVIRONMENT)" based in data in "Contact Contagion Population" and 
"Environmental Contagion Population" channels. 
    Test 1 compares the expected values for new infection by route XX with InsetChart data for every time step. 
    Test 2 compares the sum of expected value during the whole simulation with InsetChart data.
    Test 3 checks if "New Infections =  "New Infections By Route (CONTACT)" + "New Infections By Route (ENVIRONMENT)"
           for every time step.

Suggested sweep parameters: Base_Infectivity, Node_Contagion_Decay_Rate


"""

channels = [InsetKeys.ChannelsKeys.Contact_Contagion_Population,
            InsetKeys.ChannelsKeys.Environmental_Contagion_Population,
            InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT,
            InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT,
            InsetKeys.ChannelsKeys.New_Infections,
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
        for param in param_obj:
            outfile.write("{0} = {1}\n".format(param, param_obj[param]))

        # base_infectivity = param_obj[ConfigKeys.Base_Infectivity]

        if param_obj[ConfigKeys.Enable_Heterogeneous_Intranode_Transmission] == 1:
            outfile.write("WARNING: {0} = {1}, expected this feature is disabled".format(
                ConfigKeys.Enable_Heterogeneous_Intranode_Transmission,
            param_obj[ConfigKeys.Enable_Heterogeneous_Intranode_Transmission]))

        success = True

        outfile.write("Test 1: Testing new infections for every time step:\n")
        duration = param_obj[ConfigKeys.Simulation_Duration]

        new_infection_list_c = []
        new_infection_list_e = []

        failed_count_c = failed_count_e = 0

        with open("DEBUG_binomial_test.txt", "w") as binomial_test_file:
            for t in range(duration):
                infected = inset_chart_obj[InsetKeys.ChannelsKeys.Infected][t]
                stat_pop = inset_chart_obj[InsetKeys.ChannelsKeys.Statistical_Population][t]
                susceptible = stat_pop * (1.0 - infected)

                contagion_e = inset_chart_obj[InsetKeys.ChannelsKeys.Environmental_Contagion_Population][t]
                contagion_c = inset_chart_obj[InsetKeys.ChannelsKeys.Contact_Contagion_Population][t]

                probability_e = 1.0 - math.exp(-1 * contagion_e)
                probability_c = 1.0 - math.exp(-1 * contagion_c)

                expected_new_infections_c = probability_c * susceptible
                actual_new_infection_c = inset_chart_obj[InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT][t]
                if actual_new_infection_c > math.ceil(susceptible):
                    success = False
                    outfile.write("BAD: at time step {0} the {1} is {2} while calculated susceptible individual is"
                                  " {3}, {1} should be less than calculated susceptible individual, please check the "
                                  "test.\n".format(t, InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT,
                                                   actual_new_infection_c, susceptible))

                susceptible_e = susceptible - actual_new_infection_c
                expected_new_infections_e = probability_e * susceptible_e
                actual_new_infection_e = inset_chart_obj[InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT][t]
                if actual_new_infection_e > math.ceil(susceptible_e):
                    success = False
                    outfile.write("BAD: at time step {0} the {1} is {2} while calculated susceptible individual is"
                                  "{3}, {1} should be less than calculated susceptible individual, please check the "
                                  "test.\n".format(t, InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT,
                                                   actual_new_infection_e, susceptible_e))

                new_infection_list_c.append([actual_new_infection_c, expected_new_infections_c])
                new_infection_list_e.append([actual_new_infection_e, expected_new_infections_e])

                failed_count_c = test_new_infections(expected_new_infections_c, actual_new_infection_c, susceptible,
                                                     probability_c, failed_count_c, routes[1],insetchart_name,
                                                     binomial_test_file, t)
                failed_count_e = test_new_infections(expected_new_infections_e, actual_new_infection_e, susceptible_e,
                                                     probability_e, failed_count_e, routes[0], insetchart_name,
                                                     binomial_test_file, t)

                # actual_total_new_infection = inset_chart_obj[InsetKeys.ChannelsKeys.New_Infections][t + 1]
                # new_infection_list.append(actaul_new_infection_c + actaul_new_infection_e)
                # if actaul_new_infection_c + actaul_new_infection_e != actual_total_new_infection:
                #     success = False
                #     outfile.write("BAD: at time step {0}, in {1} the {2} channel reports {3} new infections, while "
                #                   "{4} and {5} are {6} and {7} respectively.\n".format(
                #                     t + 1, insetchart_name, InsetKeys.ChannelsKeys.New_Infections,
                #                     actual_total_new_infection,
                #                     InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT,
                #                     InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT,
                #                     actaul_new_infection_e,
                #                     actaul_new_infection_c))

        message_template = "\t{0}: (route {1}) binomial test for new infections channel failed {2} times within {3} " \
                           "total timesteps, which is {4}% fail rate, test is {5}. Please see " \
                           "'DEBUG_binomial_test.txt' for details.\n"
        for failed_count, route in [(failed_count_e, routes[0]),
                                    (failed_count_c, routes[1])]:
            if failed_count / duration > 5e-2:
                success = False
                outfile.write(message_template.format("BAD", route,
                                                      failed_count, duration, (failed_count / duration) * 100,
                                                      "failed"))
            else:
                outfile.write(message_template.format("GOOD", route,
                                                      failed_count, duration, (failed_count / duration) * 100,
                                                      "passed"))
        # plot actual and expected values for new infections
        sft.plot_data(np.array(new_infection_list_c)[:, 0], np.array(new_infection_list_c)[:, 1],
                          label1=insetchart_name, label2="expected new infections",
                          title=InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT,
                          xlabel='day', ylabel='new infections', category="new_infections_{}".format(routes[1]),
                          line=True, alpha=0.5, overlap=True)

        sft.plot_data(np.array(new_infection_list_e)[:, 0], np.array(new_infection_list_e)[:, 1],
                          label1=insetchart_name, label2="expected new infections",
                          title=InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT,
                          xlabel='day', ylabel='new infections', category="new_infections_{}".format(routes[0]),
                          line=True, alpha=0.5, overlap=True)
        outfile.write("Test 1: result is {}.\n".format(success))

        outfile.write("Test 2: Testing sums of new infections by routes match expected values.\n")
        result = True
        message_template = "\t{0}: sum of {1} is {2}, it should be closed to expected value {3}.\n"
        for new_infection_list, channel_name in [(new_infection_list_c,
                                                  InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT),
                                                 (new_infection_list_e,
                                                  InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT)]:
            new_infection_sum = np.array(new_infection_list)[:, 0].sum()
            expected_new_infections_sum = np.array(new_infection_list)[:, 1].sum()
            if math.fabs(new_infection_sum - expected_new_infections_sum) > 5e-2 * expected_new_infections_sum:
                success = result = False
                outfile.write(message_template.format("BAD", channel_name, new_infection_sum,
                                                      expected_new_infections_sum))
            else:
                outfile.write(message_template.format("GOOD", channel_name, new_infection_sum,
                                                      expected_new_infections_sum))
        outfile.write("Test 2: result is {}.\n".format(result))

        outfile.write("Test 3: Testing if {0} = {1} + {2} in {3} for every time step.\n".format(
            InsetKeys.ChannelsKeys.New_Infections,
            InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT,
            InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT,
            insetchart_name
        ))
        result = True
        expected_new_infection_list = []
        # skip the first time step t = 0, when outbreak happens, this is OK for now, next step:
        # ToDO: load stdout.txt for outbreak new infections and add it to expected_total_new_infection
        for t in range(1, duration):
            actual_total_new_infection = inset_chart_obj[InsetKeys.ChannelsKeys.New_Infections][t]
            actaul_new_infection_c = inset_chart_obj[InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT][t]
            actaul_new_infection_e = inset_chart_obj[InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT][t]
            expected_total_new_infection = actaul_new_infection_c + actaul_new_infection_e
            expected_new_infection_list.append(expected_total_new_infection)
            if expected_total_new_infection != actual_total_new_infection:
                success = result = False
                outfile.write("\tBAD: at time step {0}, in {1} the {2} channel reports {3} new infections, while "
                              "{4} and {5} are {6} and {7} respectively.\n".format(
                    t + 1, insetchart_name, InsetKeys.ChannelsKeys.New_Infections,
                    actual_total_new_infection,
                    InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT,
                    InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT,
                    actaul_new_infection_e,
                    actaul_new_infection_c))

        outfile.write("Test 3: '{0} = {1} + {2}' is {3}.\n".format(
            InsetKeys.ChannelsKeys.New_Infections,
            InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT,
            InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT,
            result
        ))

        sft.plot_data(inset_chart_obj[InsetKeys.ChannelsKeys.New_Infections][1:], expected_new_infection_list,
                          label1=InsetKeys.ChannelsKeys.New_Infections, label2="{0} + \n{1}".format(
                                InsetKeys.ChannelsKeys.New_Infections_By_Route_CONTACT,
                                InsetKeys.ChannelsKeys.New_Infections_By_Route_ENVIRONMENT),
                          title="{0}_{1}".format(InsetKeys.ChannelsKeys.New_Infections, insetchart_name),
                          xlabel='day(t-1, skip the first time step)', ylabel='new infections', category="total_new_infections",
                          line=True, alpha=0.5, overlap=True)

        outfile.write(sft.format_success_msg(success))
    if debug:
        print(sft.format_success_msg(success))
    return success


def test_new_infections(expected_new_infection, actual_new_infection, susceptible_pop,
                        probability, failed_count, route, insetchart_name, binomial_test_file, t):
    if expected_new_infection < 5 or susceptible_pop * (1 - probability) < 5:
        binom_pmf = stats.binom.pmf(k=actual_new_infection, n=susceptible_pop, p=probability)
        if binom_pmf < 5e-2:
            failed_count += 1
            binomial_test_file.write("BAD: at timestep {0}, total new infections for route {1} is "
                                     "{2} in {3}, expected = {4}, calculated binomial pmf is {5}.\n"
                                     "".format(t + 1, route, actual_new_infection, insetchart_name,
                                               expected_new_infection, binom_pmf))

    elif not sft.test_binomial_99ci(num_success=actual_new_infection, num_trials=susceptible_pop,
                                        prob=probability, report_file=binomial_test_file,
                                        category="new infections by route({0}) at time {1}".format(route, t + 1)):
        failed_count += 1
        # standard_deviation = math.sqrt(
        #     probability * (1 - probability) * susceptible_pop)
        # # 99% confidence interval
        # lower_bound = expected_new_infection - 3 * standard_deviation
        # upper_bound = expected_new_infection + 3 * standard_deviation
        # binomial_test_file.write("WARNING: at timestep {0}, total new infections for route {1} is {2} in {3},"
        #                          " expected within 99% binomial interval ({4}, {5}) with mean = {6}\n"
        #                          "".format(t, route, insetchart_name, actual_new_infection,
        #                                    lower_bound, upper_bound, expected_new_infection))
    return failed_count

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

