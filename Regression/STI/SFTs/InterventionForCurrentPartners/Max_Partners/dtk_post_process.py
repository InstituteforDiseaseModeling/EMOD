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
import numpy as np

"""
InterventionForCurrentPartners/Max_Partners
This test is testing Max_Partners in InterventionForCurrentPartners with Prioritize_Partners_By set to CHOSEN_AT_RANDOM. 

This test is configured with concurrent relationships and long relationship duration for all types. Multiples ICP 
interventions were distributed at day 30, 60, 90 and 99 with Maximum_Partners set to 0.3, 2, 3.7 and 0 respectively.

Data for test is loaded from ReportEventRecorder.csv, RelationshipStart.csv and RelationshipEnd.csv. 

It performs the following tests:
1. load campaign.json for parameters to test and check if it has the correct setup.
2. load data from RelationshipEnd.csv and make sure it's empty
3. load RelationshipStart.csv, collect how many relationships each female individual has at the times when ICP happens.
4. load ReportEventRecorder.csv and make sure male individuals from step 3 received the exact amount of intervention 
    event for each ICP intervention based on # of females in relationships and value of Maximum_Partners. For example:
        Maximum_Partners = 0.3: 70% of the time a female individual has 0 partner receive the event and 30% of the time
                                has one partner receives the event
        Maximum_Partners = 3.7: 30% of the time a female individual has 3 partners receive the event and 70% of the 
                                time has 4 partners receive the event.
        Basically, the integer part the float value is 100% guaranteed to receive the event and whether to contact 
        one extra partners is a Binomial test of the decimal part.
5. plot histogram for age of male who receives each event.

Output:
    scientific_feature_report.txt
    30.png,
    60.png,
    90.png

"""


def create_report_file(output_folder, report_event_recorder, relationship_start_report, relationship_end_report,
                       config_filename, output_report_name, debug):
    succeed = True
    with open(output_report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {sft.get_config_name(config_filename)}\n")

        # load campaign
        campaign_filename = sft.get_config_parameter(config_filename=config_filename,
                                                     parameters=icp_s.Constant.campaign_filename)[0]
        campaign_obj = icp_s.load_campaigns(campaign_filename)
        num_of_campaign = 4
        if len(campaign_obj) != num_of_campaign:
            output_report_file.write(f"WARNING: we are expecting {num_of_campaign} campaign events got "
                                     f"{len(campaign_obj)}, please check the test.\n")
        for event in campaign_obj.values():
            min_duration = event[icp_s.Constant.minimum_duration_years]
            if min_duration != 0:
                output_report_file.write(f"WARNING: {icp_s.Constant.minimum_duration_years} is {min_duration} expected 0, "
                                         f"please check the test.\n")

        # load data from RelationshipStart.csv
        start_df = sti_s.parse_relationship_start_report(report_path=output_folder,
                                                         report_filename=relationship_start_report)

        female_id = [[] for _ in range(num_of_campaign + 1)]
        relationship_count = [0] * (num_of_campaign + 1)
        start_days = sorted(campaign_obj.keys())
        for index, row in start_df.iterrows():
            time = row[sti_s.ReportHeader.rel_start_time]
            if time in start_days:
                for i in range(num_of_campaign):
                    if time == start_days[i]:
                        female_id[i] = female_id[-1][:]
                        relationship_count[i] = relationship_count[-1]
                        break
            elif time > start_days[-1]:
                break
            f_id = row[sti_s.ReportHeader.b_ID]
            relationship_count[-1] += 1
            if f_id not in female_id[-1]:
                female_id[-1].append(f_id)

        # load data from RelationshipEnd.csv
        end_df = pd.read_csv(path.join(output_folder, relationship_end_report))
        if not end_df.empty:
            output_report_file.write(f"WARNING: {relationship_end_report} is not empty, please check your test.\n")

        # load data from ReportEventRecorder.csv
        report_df = pd.read_csv(path.join(output_folder, report_event_recorder))
        output_report_file.write(f"{len(report_df)} events are broadcast.\n")
        base_year = sft.get_config_parameter(config_filename=config_filename, parameters=[icp_s.Constant.base_year])[0]
        event_counts = {}
        for i in range(len(start_days)):
            start_day = start_days[i]
            start_year = round(start_day / sft.DAYS_IN_YEAR, 2) + base_year
            female_id_i = female_id[i]
            relationship_count_i = relationship_count[i]
            broadcast_event = campaign_obj[start_day][icp_s.Constant.broadcast_event]
            max_partner = campaign_obj[start_day][icp_s.Constant.maximum_partners]

            low_bound = math.floor(max_partner)
            up_bound = low_bound + 1.0
            p_up_bound = max_partner - low_bound
            p_low_bound = 1.0 - p_up_bound

            output_report_file.write(f"at day {start_day} year {start_year}, there are {len(female_id_i)} females have "
                                     f"{relationship_count_i} relationships, each female will have "
                                     f"{'{:.0%}'.format(p_low_bound)} of partners receive {low_bound} "
                                     f"{broadcast_event} events and {'{:.0%}'.format(p_up_bound)} of partners receive "
                                     f"{up_bound} {broadcast_event} events.\n")
            report_df_i = report_df[report_df[icp_s.Constant.year] == start_year]
            if any(report_df_i[icp_s.Constant.event_name] != broadcast_event):
                succeed = False
                output_report_file.write(f"BAD: expected only {broadcast_event} event at day {start_day} year "
                                         f"{start_year}, got other event in {report_event_recorder}, test failed.\n")

            if not report_df_i.empty:
                report_df_i['Age'] = report_df_i['Age'] / sft.DAYS_IN_YEAR
                report_df_i.plot(y='Age', kind='hist')
                plt.title(f"day_{start_day}")
                plt.savefig(f"{start_day}.png")
                if sft.check_for_plotting():
                    plt.show()
                plt.close()

            expected = (p_up_bound * up_bound + p_low_bound * low_bound) * len(female_id_i)
            msg = f"expected {expected} events at day {start_day} year {start_year}, got {len(report_df_i)}.\n"
            if p_up_bound > 0:
                with open("debug_binomial.txt", 'a') as report_file:
                    if sft.test_binomial_99ci(len(report_df_i) - low_bound * len(female_id_i),
                                              len(female_id_i),
                                              p_up_bound, report_file, f"day {start_day} year {start_year}"):
                        output_report_file.write("GOOD: " + msg)
                    else:
                        succeed = False
                        output_report_file.write("BAD: " + msg)
            else:
                if len(report_df_i) != low_bound * len(female_id_i):
                    succeed = False
                    output_report_file.write("BAD: " + msg)
                else:
                    output_report_file.write("GOOD: " + msg)
            event_counts[start_day] = [expected, len(report_df_i)]

        output_report_file.write(sft.format_success_msg(succeed))
        data_to_plot = np.array(list(event_counts.values()))
        # plot bar graph for expected and actual event count for each campaign:
        sft.plot_bar_graph(data=[data_to_plot[:, 0], data_to_plot[:, 1]],
                           xticklabels=start_days,
                           x_label="campaign start_day",
                           y_label="Event Count",
                           legends=["Expected", "Actual"],
                           plot_name="Event_Count",
                           show=True)

    return succeed


def application(output_folder="output", stdout_filename="test.txt",
                report_event_recorder="ReportEventRecorder.csv",
                relationship_start_report="RelationshipStart.csv",
                relationship_end_report="RelationshipEnd.csv",
                config_filename="config.json",
                output_report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("report_event_recorder: " + report_event_recorder + "\n")
        print("relationship_start_report: " + relationship_start_report + "\n")
        print("relationship_end_report: " + relationship_end_report + "\n")
        print("config_filename: " + config_filename + "\n")
        print("output_report_name: " + output_report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done(stdout_filename)
    create_report_file(output_folder, report_event_recorder, relationship_start_report, relationship_end_report,
                       config_filename, output_report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-e', '--event_report', default="ReportEventRecorder.csv",
                        help="Report Event Recorder to parse (ReportEventRecorder.csv)")
    parser.add_argument('-S', '--relationship_start_report', default="RelationshipStart.csv",
                        help="RelationshipStart to parse (RelationshipStart.csv)")
    parser.add_argument('-E', '--relationship_end_report', default="RelationshipEnd.csv",
                        help="RelationshipEnd.csv to parse (RelationshipEnd.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                report_event_recorder=args.event_report,
                relationship_start_report=args.relationship_start_report,
                relationship_end_report=args.relationship_end_report,
                config_filename=args.config,
                output_report_name=args.reportname, debug=args.debug)

