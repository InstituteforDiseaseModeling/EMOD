#!/usr/bin/python
import os
if __name__ == '__main__':
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../shared_embedded_py_scripts').resolve().absolute()) )

import json
import math
import matplotlib
from sys import platform
if platform == "linux" or platform == "linux2":
    print('Linux OS. Using non-interactive Agg backend')
    matplotlib.use('Agg')
import matplotlib.pyplot as plt
from functools import partial

import dtk_test.dtk_sft as sft
from dtk_test.dtk_OutputFile import CsvOutput

"""
The ReferenceTrackingEventCoordinator coordinator class defines a particular coverage of an individual-level persistent 
intervention that should be present in a population over time. The coordinator tracks the actual coverage with the 
desired coverage; it will poll the population of nodes it has been assigned to determine how many people have the 
distributed intervention. When coverage is less than the desired coverage, it will distribute enough interventions to 
reach the desired coverage.

Parameter under test:
    Time_Value_Map, Start_Year and End_Year of intervention
This test is also looking at these parameters:
    Update_Period, Intervention_Name, Duration_To_Wait_Before_Revaccination of this intervention

Output: scientific_feature_report.txt
        Vaccine_Coverage.png
        Vaccine_Coverage_Gap.png

"""


class Campaign:
    events = "Events"
    class_name = "class"
    ReferenceTrackingEventCoordinator = "ReferenceTrackingEventCoordinator"
    start_year = "Start_Year"
    end_year = "End_Year"
    event_coordinator_config = "Event_Coordinator_Config"
    update_period = "Update_Period"
    time_value_map = "Time_Value_Map"
    times = "Times"
    values = "Values"
    intervention_config = "Intervention_Config"
    intervention_name = "Intervention_Name"
    distributed_event_trigger = "Distributed_Event_Trigger"
    duration_to_wait_before_revaccination = "Duration_To_Wait_Before_Revaccination"


class ReportColumns:
    year = "Year"
    pop = " Population"
    vaccinated_coverage = "Vaccinated Coverage"
    expected_vaccinated_coverage = "Expected_Vaccinated Coverage"
    diff = "diff"

def load_campaign(campaign_filename):
    with open(campaign_filename, 'r') as campaign_file:
        cf = json.load(campaign_file)
        try:
            events = cf[Campaign.events]
            campaign_object = []
            for event in events:
                ecc = event[Campaign.event_coordinator_config]
                if ecc[Campaign.class_name] == Campaign.ReferenceTrackingEventCoordinator:
                    start_year = event[Campaign.start_year]
                    end_year = ecc[Campaign.end_year]
                    update_period = ecc[Campaign.update_period]
                    time_value_map = ecc[Campaign.time_value_map]

                    iv = ecc[Campaign.intervention_config]
                    intervention_name = iv[Campaign.intervention_name]
                    distributed_event = iv[Campaign.distributed_event_trigger]
                    duration_to_wait_before_revaccination = iv[Campaign.duration_to_wait_before_revaccination]

                    campaign_object = [start_year, end_year, update_period, time_value_map, intervention_name,
                                       distributed_event,duration_to_wait_before_revaccination]
                    break

            return campaign_object

        except KeyError as ke:
            raise KeyError(f"{ke} is not found in {campaign_filename}.\n")


def create_report_file(report_df, hiv_report_name, config_filename, report_name):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {sft.get_config_name(config_filename)}\n")
        simulation_timestep = sft.get_simulation_timestep(config_filename)
        campaign_filename = sft.get_config_parameter(config_filename, "Campaign_Filename")[0]
        campaign_object = load_campaign(campaign_filename=campaign_filename)
        if not len(campaign_object):
            succeed = False
            output_report_file.write(f"BAD: There is no {Campaign.ReferenceTrackingEventCoordinator} in "
                                     f"{campaign_filename}, please fix the test.\n")
        else:
            start_year, end_year, update_period, time_value_map, intervention_name, distributed_event, \
            duration_to_wait_before_revaccination = campaign_object

            if duration_to_wait_before_revaccination != 0:
                succeed = False
                output_report_file.write(f'BAD: Having {Campaign.duration_to_wait_before_revaccination}=0 is key to '
                                         f'the test. It implies that intervention will allow someone to get the '
                                         f'intervention more than once.\n')
            elif update_period != simulation_timestep:
                output_report_file.write(f"WARNING: It's recommended to have {Campaign.update_period} = Simulation_Timestep "
                                         f"in this test.\n")
            if succeed:
                columns = report_df.columns
                intervention_column = None
                for column in columns:
                    if intervention_name in column:
                        intervention_column = column
                        break
                if not intervention_column:
                    succeed = False
                    output_report_file.write(f"BAD: HasIntervention({intervention_name}) should be in the "
                                             f"{hiv_report_name}, can't find this column.\n")
                else:
                    report_df = report_df[[ReportColumns.year, ReportColumns.pop, intervention_column, distributed_event]]
                    report_df[ReportColumns.vaccinated_coverage] = report_df[intervention_column] / \
                                                                   report_df[ReportColumns.pop]

                    # Create partial function from calculate_expected_coverage with fixed arguments
                    partial_calculate_expected_coverage = partial(calculate_expected_coverage,
                                                                  time_value_map, start_year, end_year)
                    # Calculate expected vaccine coverage for each report time period
                    report_df[ReportColumns.expected_vaccinated_coverage] = \
                        report_df[ReportColumns.year].apply(partial_calculate_expected_coverage)
                    # Calculate difference between actual and expected vaccine coverage
                    report_df[ReportColumns.diff] = report_df[ReportColumns.expected_vaccinated_coverage] - \
                                                    report_df[ReportColumns.vaccinated_coverage]
                    # plot the actual and expected vaccine coverage over time with Time_Values_Map
                    plot_name = "Vaccine_Coverage.png"
                    diff_plot_name = "Vaccine_Coverage_Gap.png"
                    output_report_file.write(f"Plotting, please take a look at {plot_name} and {diff_plot_name}.\n")
                    plot_coverage(report_df, time_value_map, plot_name)
                    plot_diff_coverage(report_df, diff_plot_name)

                    # Sum of all difference should be about 0, if not, fail the test.
                    sum_diff = sum(report_df[ReportColumns.diff])
                    if -1e-2 < sum_diff < 1e-2:
                        output_report_file.write(f"GOOD: the sum of all difference is {sum_diff}.\n")
                    else:
                        succeed = False
                        output_report_file.write(f"BAD: the sum of all difference is {sum_diff}.\n")

                    # Fail the test if difference is greater than half percent at any time step.
                    diff_df = report_df[(report_df[ReportColumns.diff] > 5e-3) ^ (report_df[ReportColumns.diff] < -5e-3)]
                    if not diff_df.empty:
                        succeed = False
                        output_report_file.write(f"BAD: These time steps have vaccine coverage gap larger that 0.5%: "
                                                 f"{diff_df[ReportColumns.year]}.\n")
                    else:
                        output_report_file.write(f"GOOD: All time steps have vaccine coverage gap less that 0.5%.\n")

                    # Verify that it's only distributed to individual without this intervention in order to replace
                    # HIV/54_HIV_RTEC_DistributeToWithout
                    result = True
                    pre_has_intervention = 0
                    for index, row in report_df.iterrows():
                        has_intervention = row[intervention_column]
                        intervention_distributed = row[distributed_event]
                        if intervention_distributed != has_intervention - pre_has_intervention:
                            year = row[ReportColumns.year]
                            result = succeed = False
                            output_report_file.write(f"BAD: {year} should distribute "
                                                     f"{has_intervention - pre_has_intervention} {distributed_event} "
                                                     f"instead of {intervention_distributed} events.\n")
                        pre_has_intervention = has_intervention
                    if result:
                        output_report_file.write("GOOD: the intervention only revaccinate the individual who haven't "
                                                 "received the vaccine.\n")
                    else:
                        output_report_file.write("BAD: the intervention may revaccinate the individual who had "
                                                 "received the vaccine.\n")

        output_report_file.write(sft.format_success_msg(succeed))

        return succeed


def calculate_expected_coverage(time_value_map, start_year, end_year, cur_year):
    times = time_value_map[Campaign.times]
    values = time_value_map[Campaign.values]
    if cur_year < start_year:
        return 0
    elif start_year <= cur_year <= end_year:
        if cur_year <= times[0]:
            return values[0]
        for i in range(1, len(times)):
            pre_t = times[i - 1]
            target_t = times[i]
            pre_value = values[i - 1]
            target_value = values[i]
            if cur_year < target_t:
                pre_max_value = max(values[:i]) # maximum value by previous year
                max_value = max(values[:i+1]) # maximum value by target year
                return max_value if max_value > target_value else \
                    value_interpolation(pre_t, target_t, pre_value, target_value, pre_max_value, cur_year)
        if cur_year >= times[-1]:
            return max(values)
    else:
        return max(values)

    return -1 # this means error


def value_interpolation(pre_t, target_t, pre_value, target_value, pre_max_value, cur_year):
    if target_value > pre_value:
        value = ((cur_year - pre_t) / (target_t - pre_t)) * (target_value - pre_value) + pre_value
        return value if value > pre_max_value else pre_max_value
    elif target_value == pre_t:
        return pre_t
    else:
        return -1


def plot_coverage(df, time_value_map, plot_name="Vaccine_Coverage.png"):
    fig = plt.figure()
    ax = fig.add_axes([0.12, 0.15, 0.76, 0.76])

    df.plot(x=ReportColumns.year, y=ReportColumns.vaccinated_coverage,
            linestyle='-', marker="o", color='b', alpha=0.5,
            ax=ax, label="Actual")
    df.plot(x=ReportColumns.year, y=ReportColumns.expected_vaccinated_coverage,
            linestyle='-', marker="^", color='r', alpha=0.5,
            ax=ax, label="Expected")
    ax.plot(time_value_map[Campaign.times], time_value_map[Campaign.values],
            linestyle='-', marker="*", color='g', alpha=0.5,
            label=Campaign.time_value_map)
    ax.set_title("Vaccine Coverage")
    ax.legend(loc=0)

    plt.savefig(plot_name)

    if sft.check_for_plotting():
        plt.show()
    plt.close(fig)


def plot_diff_coverage(df, plot_name):
    fig = plt.figure()
    ax = fig.add_axes([0.12, 0.15, 0.76, 0.76])

    df.plot(x=ReportColumns.year, y=ReportColumns.diff,
            linestyle='-', marker="o", color='b', alpha=0.5,
            ax=ax)
    ax.set_title("Vaccine Coverage Gap")

    plt.savefig(plot_name)

    if sft.check_for_plotting():
        plt.show()
    plt.close(fig)


def application(output_folder="output", stdout_filename="test.txt",
                HIV_report="ReportHIVByAgeAndGender.csv",
                config_filename="config.json",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("HIV_report: " + HIV_report + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done(stdout_filename)
    report_df = CsvOutput(file=os.path.join(output_folder, HIV_report)).df
    create_report_file(report_df, HIV_report, config_filename, report_name)

    pass

if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-e', '--HIV_report', default="ReportHIVByAgeAndGender.csv",
                        help="Report HIV By Age and Gender to parse (ReportHIVByAgeAndGender.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                HIV_report=args.HIV_report,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

