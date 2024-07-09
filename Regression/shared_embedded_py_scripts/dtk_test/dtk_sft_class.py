#!/usr/bin/python
import os
import argparse
import json
from typing import Union, Optional
from abc import ABC, abstractmethod
import dtk_test.dtk_sft as dtk_sft
from dtk_test.dtk_General_Support import ConfigKeys


def arg_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-C', '--campaign', default="campaign.json", help="campaign name to load (campaign.json)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdout file to parse (test.txt)")
    parser.add_argument('-j', '--json_report', default="InsetChart.json",
                        help="Json report to load (InsetChart.json)")
    parser.add_argument('-e', '--event_csv', default="ReportEventRecorder.csv",
                        help="Event report to load (ReportEventRecorder.csv)")
    parser.add_argument('-r', '--report_name', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    return parser.parse_args()


class SFT(ABC):
    """
        A base class carrying the lowest level SFT interfaces called by individual test objects
    """
    def __init__(self, output="output", stdout="test.txt", json_report="InsetChart.json",
                 event_csv="ReportEventRecorder.csv", config="config.json", campaign="campaign.json",
                 report_name=dtk_sft.sft_output_filename, debug=False):
        self.output_folder = output
        self.stdout_filename = stdout
        self.stdout = None
        self.json_report_name = json_report
        self.json_report = None
        self.event_report_name = event_csv
        self.csv = None
        self.config_filename = config
        self.params = dict()
        self.campaign_filename = campaign
        self.report_name = report_name
        self.debug = debug
        self.success = True
        self.msg = list()
        if self.debug:
            print("output_folder: " + self.output_folder + "\n")
            print("stdout_filename: " + self.stdout_filename + "\n")
            print("json_report_name: " + self.json_report_name + "\n")
            print("config_filename: " + self.config_filename + "\n")
            print("campaign_filename: " + self.campaign_filename + "\n")
            print("report_name: " + self.report_name + "\n")
            print("debug: " + str(self.debug) + "\n")
        dtk_sft.wait_for_done(filename=self.stdout_filename)
        super().__init__()

    def load_config(self, params_keys: Optional[list] = None):
        """
                reads config file and save it into self.params as a dictionary with Config_Name, etc., keys (e.g.)
        Args:
            params_keys (list): list of config parameters

        """
        if not params_keys:
            params_keys = [ConfigKeys.Config_Name]
        elif isinstance(params_keys, list):
            if ConfigKeys.Config_Name not in params_keys:
                params_keys.append(ConfigKeys.Config_Name)
        else:
            raise ValueError("params_keys should be a list of config parameters.")

        with open(self.config_filename) as infile:
            cdj = json.load(infile)[ConfigKeys.Parameters]

        for param in params_keys:
            self.params[param] = cdj[param]

        if self.debug:
            with open("DEBUG_param_object.json", 'w') as outfile:
                json.dump(self.params, outfile, indent=4)

    def init_report_file(self):
        with open(self.report_name, "w") as outfile:
            for name, param in self.params.items():
                outfile.write(f"{name} = {param}\n")

    def finish_report_file(self):
        with open(self.report_name, "a") as outfile:
            for s in self.msg:
                outfile.write(s)
            outfile.write(dtk_sft.format_success_msg(self.success))
        if self.debug:
            print(dtk_sft.format_success_msg(self.success))

    def parse_json_report(self, channel_names: Optional[list] = None):
        """
            load data from InsetChart like json file and save them into dataframe
        Args:
            channel_names: list of json channel names

        Returns: InsetChart object

        """
        from dtk_test.dtk_InsetChart import InsetChart
        self.json_report = InsetChart(os.path.join(self.output_folder, self.json_report_name),
                                      channel_names=channel_names)

    def parse_stdout(self, filter_string_list=None, load_df_param: list = None, first_only: bool = False):
        """
            load data from StdOut.txt(test.txt) file and save them into dataframe
        Args:
            filter_string_list: list of string to filter StdOut.txt
            load_df_param:      a list contains 3 parameters: [column_names, search_strings, search_types(optional)]
            first_only:         set to True to load the first line only after each time step

        Returns: StdOut object

        """
        from dtk_test.dtk_StdOut import StdOut
        self.stdout = StdOut(self.stdout_filename, filter_string_list=filter_string_list, load_df_param=load_df_param,
                             first_only=first_only)

    def parse_report_event_recorder(self):
        from dtk_test.dtk_OutputFile import ReportEventRecorder
        self.csv = ReportEventRecorder(file=os.path.join(self.output_folder, self.event_report_name))

    @abstractmethod
    def test(self):
        # actual test method is defined in each SFT
        pass

    def run(self):
        self.load_config()
        self.init_report_file()
        self.test()
        self.finish_report_file()
