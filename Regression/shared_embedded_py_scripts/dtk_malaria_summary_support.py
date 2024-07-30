import itertools
from collections import OrderedDict
import logging

import pandas as pd
import numpy as np

logger = logging.getLogger(__name__)


def summary_channel_to_pandas(data, channel):
    """
    A function to return a hierarchical binned pandas.Series for a specified MalariaSummaryReport.json channel
    :param data: parsed data from summary report
    :param channel: channel in summary report
    :return: pd.Series with MultiIndex binning taken from summary metadata
    """

    grouping = get_grouping_for_summary_channel(data, channel)
    bins = get_bins_for_summary_grouping(data, grouping)

    channel_series = json_to_pandas(data[grouping][channel], bins, channel)

    # Append some other useful metadata to the output Series
    metadata = data['Metadata']
    channel_series.Start_Day = metadata.get('Start_Day')
    channel_series.Reporting_Interval = metadata.get('Reporting_Interval')
    return channel_series


def json_to_pandas(channel_data, bins, channel=None):
    """
    A function to convert nested array channel data from a json file to
    a pandas.Series with the specified MultiIndex binning.
    """

    logger.debug("Converting JSON data from '%s' channel to pandas.Series with %s MultiIndex.", channel, bins.keys())
    bin_tuples = list(itertools.product(*bins.values()))
    multi_index = pd.MultiIndex.from_tuples(bin_tuples, names=bins.keys())

    channel_series = pd.Series(np.array(channel_data).flatten(), index=multi_index, name=channel)
    logger.debug('\n%s', channel_series)
    return channel_series


def get_grouping_for_summary_channel(data, channel):
    """
    A function to find the grouping to which a channel belongs in MalariaSummaryReport.json
    :param data: parsed data from summary report
    :param channel: channel to find
    :return: grouping or exception if not found

    Example:

    >>> get_grouping_for_summary_channel(data, channel='Average Population by Age Bin')
    'DataByTimeAndAgeBins'
    """

    for group, group_data in data.items():
        if channel in group_data.keys():
            return group

    raise Exception('Unable to find channel %s in groupings %s' % (channel, data.keys()))


def get_bins_for_summary_grouping(data, grouping):
    """
    A function to get the dimensions and binning of data for a specified MalariaSummaryReport.json grouping
    :param data: parsed data from summary report
    :param grouping: group name
    :return: an OrderedDict of dimensions and bins

    Example:

    >>> get_bins_for_summary_grouping(data, grouping='DataByTimeAndAgeBins')
    OrderedDict([('Time', [31, 61, 92, ..., 1095]), ('Age Bin', [0, 10, 20, ..., 1000])])
    """

    metadata = data['Metadata']
    time = data['DataByTime']['Time Of Report']

    if grouping == 'DataByTime':
        return OrderedDict([
            ('Time', time)
        ])
    elif grouping == 'DataByTimeAndAgeBins':
        return OrderedDict([
            ('Time', time),
            ('Age Bin', metadata['Age Bins'])
        ])
    elif grouping == 'DataByTimeAndPfPRBinsAndAgeBins':
        return OrderedDict([
            ('Time', time),
            ('PfPR Bin', metadata['Parasitemia Bins']),
            ('Age Bin', metadata['Age Bins'])
        ])

    raise Exception('Unable to find grouping %s in %s' % (grouping, data.keys()))
