#!/usr/bin/python

from dtk_test.dtk_OutputFile import TextFile
import dtk_test.dtk_sft as dtk_sft
import pandas as pd
import matplotlib
from sys import platform
if platform == "linux" or platform == "linux2":
    print('Linux OS. Using non-interactive Agg backend')
    matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import random
from enum import Enum, auto


class SearchType(Enum):
    CHAR = auto()
    VAL = auto()


class StdOut(TextFile):
    """
    StdOut class that inherits from TextFile. It will load StdOut.txt(Test.txt) as a dataframe and save it as
    StdOut.df.
    """
    def __init__(self, file='StdOut.txt', filter_string_list=None, load_df_param: list = None,
                 first_only: bool = False):
        super().__init__(file, filter_string_list)
        self.df = self.load_df(load_df_param, first_only)

    def load_df(self, load_df_param: list = None, first_only: bool = False):
        if not isinstance(load_df_param, list) or len(load_df_param) < 2:
            return pd.DataFrame()
        else:
            column_names = load_df_param[0]
            search_strings = load_df_param[1]
            if isinstance(column_names, list) and isinstance(search_strings, list):
                if len(column_names) != len(search_strings):
                    raise ValueError("column_names and search_strings should have the same length.")
            else:
                raise ValueError("column_names and search_strings should be list of string objects.")

            if len(load_df_param) > 2:
                search_types = load_df_param[2]
            else:
                search_types = [SearchType.CHAR] * len(column_names) # default search_type is char which will return a string object

            time_string = "Update(): Time: "
            column_data = {"Time": []}
            for column_name in column_names:
                column_data[column_name] = []

            # replace special characters for searching
            actual_search_strings = search_strings[:]
            for i in range(len(actual_search_strings)):
                string = actual_search_strings[i]
                for char in ['[', ']', '(', ')']:
                    index = string.find(char)
                    if index != -1:
                        string = string[:index] + '\\' + string[index:]
                        actual_search_strings[i] = string

            lines = self.filtered_lines if self.filtered_lines else self.lines
            time = 0
            for line in lines:
                if time_string in line:
                    time = float(dtk_sft.get_val("Time: ", line))
                else:
                    if first_only and time in column_data["Time"]:
                        continue
                    if search_strings[0] in line:
                        value_list = []
                        for actual_search_string, search_string, search_type in \
                                zip(actual_search_strings, search_strings, search_types):
                            try:
                                if search_type == SearchType.VAL:
                                    value = float(dtk_sft.get_val(actual_search_string, line))
                                elif search_type == SearchType.CHAR:
                                    value = dtk_sft.get_char(actual_search_string, line)
                                else:
                                    raise ValueError(f"search_type must be {SearchType.VAL} or {SearchType.CHAR}, "
                                                     f"got {search_type}.")
                                value_list.append(value)
                            except LookupError as ex:
                                raise ValueError(f"Can't find {search_string} with search_type = {search_type} in logging "
                                                 f"line '{line}'. Got exception: {ex}")
                        column_data["Time"].append(time)
                        for value, column_name in zip(value_list, column_names):
                            column_data[column_name].append(value)

            df = pd.DataFrame.from_dict(column_data, orient='columns')
            df["Time"] = pd.to_numeric(df["Time"])
            return df

    def count_incidence(self):
        if self.df.empty:
            return pd.DataFrame()
        else:
            df = self.df.groupby("Time").size().reset_index(name='count').sort_values(by=['Time'])
            return df

    def count_prevalence(self):
        if self.df.empty:
            return pd.DataFrame()
        else:
            try:
                self.df = self.df.apply(pd.to_numeric)
                df = self.df.groupby("Time").mean().reset_index()
                return df
            except ValueError as ve:
                raise ValueError(f"Can't use count_prevalence on non-numerical values, got exception: {ve}.")

    def plot_df(self, df=pd.DataFrame):
        if df.empty:
            df = self.df
        fig = plt.figure()
        ax = fig.add_axes([0.12, 0.15, 0.76, 0.76])
        column_names = df.columns
        cmap = dtk_sft.get_cmap(len(column_names))
        for i, column_name in enumerate(column_names[1:]):
            ax.scatter(x=df["Time"], y=df[column_name], c=cmap(i))
        ax.set_title("data from StdOut vs. Time")
        ax.set_xlabel("Time")
        plt.legend(loc=0)
        if len(column_names) == 2:
            plt.savefig(f"{column_names[1]}(StdOut).png")
        else:
            plt.savefig(f"data(StdOut).png")
        if dtk_sft.check_for_plotting():
            plt.show()
        plt.close(fig)





