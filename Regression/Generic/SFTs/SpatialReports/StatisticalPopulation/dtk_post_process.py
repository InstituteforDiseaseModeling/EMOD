#!/usr/bin/python

import dtk_test.dtk_sft as sft
import json
import glob
import os
import numpy as np
import re
import pandas as pd
import dtk_test.dtk_General_Support as dtkgs

from dtk_SFTReport import SFTReport
from dtk_SpatialReportReader import Spatial_Report_Channels, SpatialReportData
from dtk_DemographicsFile import DemographicsKeys, DemographicsFile

with open("config.json") as infile:
    run_number=json.load(infile)['parameters']['Run_Number']
np.random.seed(run_number)


# -- support code

class ConfigKeys:
    Spatial_Output_Channels = "Spatial_Output_Channels"


# ---- test code

def verify_node_ids(report, channel_data, demographics, channel):
    """
    Verify that all node ids in demographics file match those in population spatial report channel

    :param report: SFT report object
    :param population_channel: spatial report channel (SpatialReportData object)
    :param demographics: DemographicsFile object with some loaded node data
    :param channel: channel that's being verified (for good/bad report messages)
    :return: whether the node ids match
    """
    if set(demographics.node_data.keys()) == set(channel_data.node_ids):
        report.add_good_msg(f'All node ids from demographics file found in {channel} spatial report.')
        return True
    else:
        report.add_bad_msg(f"Node ids in demographics file and {channel} spatial report don't match")
        return False


def compare_init_population_per_node(report, population_report, demographics):
    """
    Compare populations at first timestep from SR population channel to demographics file init. pops, adds good/bad
    messages to report depending on success or failure

    :param report: SFT report object
    :param population_report: population spatial report channel (SpatialReportData object)
    :param demographics: DemographicsFile object with initial population data loaded
    """
    success = True
    for node_id, node_data in demographics.node_data.items():
        expected_population = node_data[DemographicsKeys.INITIAL_POPULATION]
        report_population = population_report.node_data[node_id][0]
        if abs(expected_population - report_population) < 0.2:
            # instead of writing one good message for every single node, just write one message
            success &= True
        else:
            success &= False
            report.add_bad_msg(f'population for node {node_id} in report {report_population} differs from demographic initial pop {expected_population}')
    if success:
        report.add_good_msg(f'population for each node in report matches demographic initial pop')


def compare_channel_data_by_timestep(report, channel_data, inset_chart, channel_name, precision=0.1):
    """
    Compare data from inset chart to summed data across all nodes at the same timestep from the spatial report channel
    data. Write good/bad messages to report.

    :param report: SFT report object
    :param population_channel: spatial report channel (SpatialReportData object)
    :param channel_name: name of the channel used (e.g. Births, Population)
    :param inset_chart: inset chart object
    :param precision: precision of comparison check (total size of all diffs between channel data and inset chart data)
    """
    inset_chart_data = inset_chart[channel_name]
    spatial_report_df = pd.DataFrame(channel_data.node_data)
    # sum every row across all nodes (should be identical to inset chart values)
    spatial_report_sums = spatial_report_df.sum(axis=1)

    # subtract sum of SR report data from inset chart data, this should be a column of all zeroes
    channel_diff = inset_chart_data - spatial_report_sums
    # sum the absolute values of the diffs, this should be zero
    diff_sum = channel_diff.abs().sum()

    if diff_sum < precision:
        report.add_good_msg(f'{channel_name} from inset chart matches spatial report at all timesteps')
    else:
        first_nonzero = channel_diff.nonzero()[0][0]
        report.add_bad_msg(f'{channel_name} from inset chart differs from spatial report starting at index {first_nonzero}'
                           ' (inset chart: {inset_chart_data[first_nonzero]} spatial report: {spatial_report_sums[first_nonzero]})')


def track_births_compare_to_pop(report, births_channel, population_channel):
    """
    For this sim births should be the only change to population, since birth data is already a running total the
    population for any given node should be equal to the birth data offset by one timestep plus the initial population
    for that node.

    :param report: SFT report object
    :param births_channel: birth spatial report channel (SpatialReportData object)
    :param population_channel: population spatial report channel (SpatialReportData object)
    """
    births_channel_df = pd.DataFrame(births_channel.node_data)
    population_channel_df = pd.DataFrame(population_channel.node_data)

    # subtract offset births from population channel data (by node and timestep)
    # this should leave a table with many rows that are all equal to the init. pop for each node
    static_pop_df = (population_channel_df - births_channel_df.shift(1)).drop(0)
    # get array for init. pop
    init_pop = population_channel_df.iloc[[0]].values.squeeze()
    # subtract init_pop from all rows of the births corrected pop values, the result should be all zeroes
    pop_diff_df = static_pop_df - init_pop
    # sum absolute value of population diff. across all columns
    timestep_pop_diffs = pop_diff_df.abs().sum(axis=1)
    # sum all population diffs for all timesteps
    population_deviation = timestep_pop_diffs.sum()
    
    print(timestep_pop_diffs)

    if population_deviation < 0.1:
        report.add_good_msg(f'Births + initial pop match node populations for all timesteps')
    else:
        # determine first timestep where population deviated from expected value
        #first_nonzero = timestep_pop_diffs.to_numpy().nonzero()[0][0]
        first_nonzero = np.array(timestep_pop_diffs).nonzero()[0][0]
        # need to offset reported index to to the shift/drop above
        report.add_bad_msg(f'Population deviates from expected via initial pop + births starting at index {first_nonzero + 1}, '
                           'difference: {timestep_pop_diffs[first_nonzero]}')


def generate_report_file(config_object, report_name, output_folder="output", inset_chart={}, debug=False):
    output_fullpath = os.path.join(os.getcwd(), output_folder)
    # collect list of spatial report output files
    spatial_reports_filenames = glob.glob(os.path.join(output_fullpath, "SpatialReport_*.bin"))

    report = SFTReport(report_name)

    with report.use_report():
        # collect list of spatial output channels from config.json
        spatial_channels = set(config_object[ConfigKeys.Spatial_Output_Channels])
        channels_count = len(spatial_channels)
        reports_count = len(spatial_reports_filenames)
        channel_counts_match = ConfigKeys.Spatial_Output_Channels in config_object and channels_count == reports_count

        if channel_counts_match:
            # verify each spatial report from config.json exists and keep track of their filenames
            report.add_good_msg(f'Found {reports_count} spatial reports (expected: {channels_count})')
            spatial_reports = {}
            for spatial_file in spatial_reports_filenames:
                matches = re.match('SpatialReport_(.*)\.bin', os.path.basename(spatial_file))
                if matches:
                    spatial_reports[matches[1]] = spatial_file
            spatial_reports_generated = set(spatial_reports.keys())
            if spatial_reports_generated == spatial_channels:
                report.add_good_msg(f'Found reports for {spatial_reports_generated} channels (expected: {spatial_channels})')
            else:
                report.add_bad_msg(f'Generated reports for {spatial_reports_generated} channels does not match expected: {spatial_channels}')

            # read population, births, and campaign cost channel data
            population_channel = SpatialReportData(spatial_reports[Spatial_Report_Channels.Population], Spatial_Report_Channels.Population)
            births_channel = SpatialReportData(spatial_reports[Spatial_Report_Channels.Births], Spatial_Report_Channels.Births)
            camp_cost_channel = SpatialReportData(spatial_reports[Spatial_Report_Channels.Campaign_Cost], Spatial_Report_Channels.Campaign_Cost)

            # read initial population data from demographics file
            demographics_data = DemographicsFile('Balboa_demographics.json', collect_attributes=[DemographicsKeys.INITIAL_POPULATION])

            # verify that all node-ids from demographics file are in each of the spatial report channels
            if verify_node_ids(report, population_channel, demographics_data, Spatial_Report_Channels.Population) and \
                    verify_node_ids(report, births_channel, demographics_data, Spatial_Report_Channels.Births) and \
                    verify_node_ids(report, camp_cost_channel, demographics_data, Spatial_Report_Channels.Campaign_Cost):

                compare_init_population_per_node(report, population_channel, demographics_data)

                compare_channel_data_by_timestep(report, population_channel, inset_chart,
                                                 dtkgs.InsetKeys.ChannelsKeys.Statistical_Population)
                compare_channel_data_by_timestep(report, births_channel, inset_chart,
                                                 dtkgs.InsetKeys.ChannelsKeys.Births)
                compare_channel_data_by_timestep(report, camp_cost_channel, inset_chart,
                                                 dtkgs.InsetKeys.ChannelsKeys.Campaign_Cost)
                track_births_compare_to_pop(report, births_channel, population_channel)
        else:
            report.add_bad_msg(f'Found {reports_count} spatial reports (expected: {channels_count})')


def application(output_folder="output",
                config_filename="config.json", campaign_filename="campaign.json",
                inset_chart_name="InsetChart.json",
                report_name=sft.sft_output_filename,
                debug=False):

    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("campaign_filename: " + campaign_filename + "\n")
        print("inset_chart_name: " + inset_chart_name + "\n")
        print("debug: " + str(debug) + "\n")

    sft.wait_for_done()

    param_obj = dtkgs.load_config_parameters(config_filename=config_filename,
                                             keys=[dtkgs.ConfigKeys.Demographics_Filenames,
                                                     dtkgs.ConfigKeys.Enable_Spatial_Output,
                                                     ConfigKeys.Spatial_Output_Channels])
    campaign_obj = None
    demographics_obj = None
    report_data_obj = None
    inset_chart_obj = dtkgs.parse_inset_chart(insetkey_list=[dtkgs.InsetKeys.ChannelsKeys.Statistical_Population,
                                                             dtkgs.InsetKeys.ChannelsKeys.Births,
                                                             dtkgs.InsetKeys.ChannelsKeys.Campaign_Cost])

    generate_report_file(param_obj, report_name, output_folder, inset_chart=inset_chart_obj)


if __name__ == "__main__":

    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-i', '--insetchartname', default="InsetChart.json", help="insetchart to test(InsetChart.json)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output,
                inset_chart_name=args.insetchartname,
                config_filename=args.config,
                debug=args.debug)
