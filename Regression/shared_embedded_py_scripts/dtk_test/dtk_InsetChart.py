#!/usr/bin/python

from dtk_test.dtk_OutputFile import JsonOutput
import pandas as pd
from typing import Union
import dtk_test.dtk_sft as dtk_sft
import matplotlib.pyplot as plt
import math
import json
import numpy as np

key_channels = "Channels"
key_data = "Data"


class InsetChart(JsonOutput):
    """
    InsetChart class that inherits from JsonOutput. It will load InsetChart.json as a dataframe and save it as
    InsetChart.df.
    """
    def __init__(self, file='.\\output\\InsetChart.json', channel_names=None):
        super().__init__(file)
        self.channels = self.load_channels()
        self.df = self.load_df(channel_names)

    def load_channels(self):
        if key_channels not in self.json:
            raise ValueError(f"{self.jsonfile} is not a InsetChart json file.")
        else:
            return self.json[key_channels]

    def load_df(self, channel_names: Union[str, list] = None):
        channels_data = {}
        # load all channels if channel_names is not provided
        if not channel_names:
            channel_names = list(self.channels.keys())

        if isinstance(channel_names, str):
            channels_data[channel_names] = self.load_channel_data(channel_names)
        elif isinstance(channel_names, list):
            for channel in channel_names:
                channels_data[channel] = self.load_channel_data(channel)
        else:
            raise ValueError("Expect String or List of string as the channel_names.")

        df = pd.DataFrame.from_dict(channels_data)
        df.index.name = "Time"
        return df

    def load_channel_data(self, channel):
        if channel in self.channels:
            return self.channels[channel][key_data]
        else:
            raise ValueError(f"{channel} in not an InsetChart Channels.")

    def compare_channels(self, expected_data_list, channels, simulation_timestep,
                         output_report_file=None, time_start=0, time_end=None, tolerance_list: Union[float, list]=1e-3):
        succeed = True
        if len(expected_data_list) != len(channels):
            succeed = False
            msg = "BAD: lengths of expected_data_list and channels don't matched."
            if output_report_file:
                output_report_file.write(msg + "\n")
            else:
                print(msg)
        else:
            for i in range(len(channels)):
                channel = channels[i]
                expected_data = expected_data_list[i]
                if isinstance(tolerance_list, float):
                    tolerance = tolerance_list
                elif isinstance(tolerance_list, list) and len(tolerance_list) == len(channels):
                    tolerance = tolerance_list[i]
                else:
                    raise ValueError(f"tolerance_list={tolerance_list} is not supported.")
                result = self.compare_channel(expected_data, channel, simulation_timestep, output_report_file,
                                              time_start, time_end, tolerance)
                if not result:
                    succeed = False
        return succeed

    def compare_channel(self, expected_data, channel, simulation_timestep,
                        output_report_file=None, time_start=0, time_end=None, tolerance=1e-3):
        succeed = True
        messages = [f"\tTesting '{channel}' channel in {self.filename}:"]

        channel_data = self.df[channel].tolist()[time_start:time_end]
        for a, b in {'/': '', '<': 'lt', '>': 'gt', '=': ''}.items():
            if a in channel:
                channel = channel.replace(a, b)
        if len(channel_data) != len(expected_data):
            succeed = False
            messages.append(f"\tBAD: channel '{channel}' reports {len(channel_data)} data points while expected "
                            f"{len(expected_data)} data points.")
        else:
            # plot insetchart data and expected values
            dtk_sft.plot_data(channel_data, expected_data, sort=False,
                              title=channel, label1='InsetChart', label2=f'expected',
                              xlabel=f'timestep/{simulation_timestep} days', category=channel)
            # collect data points that are unmatched in insetchart and expected data.
            diff = [(j * simulation_timestep, channel_data[j], expected_data[j]) for j in range(len(channel_data))
                    if math.fabs(channel_data[j] - expected_data[j]) > tolerance]
            if len(diff):
                succeed = False
                file_name = f"unmatched_{channel}.json"
                messages.append(f"\tBAD: channel '{channel}' reports incorrectly, please see '{channel}.png' and "
                               f"'{file_name}'.")
                with open(file_name, 'w') as debug_file:
                    json.dump(diff, debug_file, indent=4, default=dtk_sft.default)
            else:
                messages.append(f"\tGOOD: channel '{channel}' reports correctly, please see '{channel}.png'.")

        for msg in messages:
            if output_report_file:
                output_report_file.write(msg + "\n")
            else:
                print(msg)
        return succeed

    def plot_channels(self, channels: list = None):
        fig = plt.figure()
        ax = fig.add_axes([0.12, 0.15, 0.76, 0.76])
        if not channels:
            # plot add channels
            self.df.plot(ax=ax, title="InsetChart")
        else:
            # plot selected channels
            self.df[channels].plot(ax=ax, title="InsetChart")
        plt.savefig(f"{self.filename.split('.')[-2]}.png")
        if dtk_sft.check_for_plotting():
            plt.show()
        plt.close(fig)

