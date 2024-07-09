#!/usr/bin/python
if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append(str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )


import os.path as path
import dtk_test.dtk_sft as sft
import matplotlib.pyplot as plt
import pandas as pd
import dtk_test.dtk_STI_Support as sti_s
import dtk_test.dtk_ICP_Support as icp_s
import math
import json

"""
InterventionForCurrentPartners\Intervention_Config
This test is testing the InterventionForCurrentPartners with Event_Or_Config set to Config to deliver a HIV intervention
MaleCircumcision to all male partners with Circumcision_Reduced_Acquire set to 1.

An outbreak is distributed to 2% of the population at the beginning of the test with R0 = 4 so the disease is 
circulated in population without other intervention. An InterventionForCurrentPartners is started an year later to 
target all female and distribute a MaleCircumcision to all their male partners with 100% Circumcision_Reduced_Acquire
effect. So we expect the prevalence to drop dramatically after the intervention.

This test is looking at the incidence data over the simulation duration from test.txt/StdOut.txt, it performs the 
following tests:
1. load campaign and make sure test is setup as design.
2. parse test.txt file and get infected population per time step.
3. Calculated average infected count per day for times before intervention happens and after intervention happens. Test
    if average infected count drop dramatically(10 times at least.)
4. Plot infected count per day.

Output:
    scientific_feature_report.txt
    Infected.png

"""


def load_campaigns(campaign_filename):
    with open(campaign_filename, 'r') as campaign_file:
        cp_json = json.load(campaign_file)[icp_s.Constant.events]
    campaign_obj = {}
    for event in cp_json:
        start_day = event[icp_s.Constant.start_day]
        ic = event[icp_s.Constant.event_coordinator_config][icp_s.Constant.intervention_config]
        iv_class = ic["class"]
        campaign_obj[start_day] = {"class": iv_class,
                                   icp_s.Constant.intervention_config: ic}
    return campaign_obj


def create_report_file(campaign_obj, config_filename, output_report_name, stdout_filename, debug):
    succeed = True
    with open(output_report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {sft.get_config_name(config_filename)}\n")

        # check campaign_obj
        num_of_campaign = 2
        if len(campaign_obj) != num_of_campaign:
            output_report_file.write(f"WARNING: we are expecting {num_of_campaign} campaign events got "
                                     f"{len(campaign_obj)}, please check the test.\n")
        icp_start_day = outbreak_start_day = None # these are populated in the loop
        for start_day in campaign_obj.keys():
            iv_class = campaign_obj[start_day]['class']
            if iv_class == icp_s.Constant.OutbreakIndividual:
                outbreak_start_day = start_day
                if outbreak_start_day != 0:
                    output_report_file.write(f"WARNING: {icp_s.Constant.start_day} for "
                                             f"{icp_s.Constant.OutbreakIndividual} is {outbreak_start_day} expected 0, "
                                             f"please check the test.\n")
            elif iv_class == icp_s.Constant.InterventionForCurrentPartners:
                icp_start_day = start_day
                if icp_start_day != 365:
                    output_report_file.write(f"WARNING: {icp_s.Constant.start_day} for "
                                             f"{icp_s.Constant.InterventionForCurrentPartners} is {icp_start_day} "
                                             f"expected 365, please check the test.\n")
                iv_to_broadcast = campaign_obj[start_day][icp_s.Constant.intervention_config]\
                                  [icp_s.Constant.intervention_config]["class"]
                if iv_to_broadcast != icp_s.Constant.MaleCircumcision:
                    succeed = False
                    output_report_file.write(f"BAD: Intervention to broadcast for "
                                             f"{icp_s.Constant.InterventionForCurrentPartners} is {iv_to_broadcast} "
                                             f"expected {icp_s.Constant.MaleCircumcision}, please check the test.\n")
            else:
                output_report_file.write(f"WARNING: Campaign class is {iv_class}, expected "
                                         f"{icp_s.Constant.InterventionForCurrentPartners} or "
                                         f"{icp_s.Constant.OutbreakIndividual}, please check the test.\n")
        output_report_file.write(f"{icp_s.Constant.OutbreakIndividual} starts at day {outbreak_start_day} and "
                                 f"{icp_s.Constant.InterventionForCurrentPartners} with "
                                 f"{icp_s.Constant.MaleCircumcision} starts at ")

        # load test data from test.txt
        infected = []
        with open(stdout_filename, "r") as in_file:
            for line in in_file:
                if "[Simulation] Update()" in line:
                    infected.append(int(sft.get_val("Infected: ", line)))

        # compare infected count before and after the intervention
        infected_avg_before_mc = sum(infected[:icp_start_day])/len(infected[:icp_start_day])
        infected_avg_after_mc = sum(infected[icp_start_day:])/len(infected[icp_start_day:])
        ratio = infected_avg_before_mc / infected_avg_after_mc
        plot_name = 'Infected.png'
        msg = f"HIV Intervention should reduce the prevalence dramatically. Before intervention we got average " \
              f"infected count per day = {infected_avg_before_mc} and after intervention it's " \
              f"{infected_avg_after_mc}. You can see {plot_name} for the infected data.\n"
        if ratio < 10:
            succeed = False
            output_report_file.write("BAD: " + msg)
        else:
            output_report_file.write("GOOD: " + msg)

        # plot infected data
        fig = plt.figure()
        ax = fig.add_axes([0.12, 0.12, 0.76, 0.76])
        # x = np.arange(len(infected))
        # print(len(x), len(infected_base), len(infected), max(infected_base), max(infected))
        # ax.plot(infected_base, 'y*--', label="baseline", alpha=0.5)
        ax.plot(infected, 'r^--', label=icp_s.Constant.MaleCircumcision, alpha=0.5)
        # indicate when MaleCircumcision happens
        ax.plot([icp_start_day], [infected[icp_start_day + 1]], marker='o', markersize=5, color="blue")
        ax.text(icp_start_day, infected[icp_start_day + 1],
                f"{icp_s.Constant.MaleCircumcision} started at day {icp_start_day}")
        ax.legend(loc=0)
        ax.set_title('Infected')
        ax.set_xlabel("Days")
        ax.set_ylabel("Infected")
        fig.savefig(plot_name)
        if sft.check_for_plotting():
            plt.show()
        plt.close()

        output_report_file.write(sft.format_success_msg(succeed))
    return succeed


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json",
                output_report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("output_report_name: " + output_report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done(stdout_filename)
    # load campaign
    campaign_filename = sft.get_config_parameter(config_filename=config_filename,
                                                 parameters=icp_s.Constant.campaign_filename)[0]
    campaign_obj = load_campaigns(campaign_filename)
    create_report_file(campaign_obj, config_filename, output_report_name, stdout_filename, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                config_filename=args.config,
                output_report_name=args.reportname, debug=args.debug)

