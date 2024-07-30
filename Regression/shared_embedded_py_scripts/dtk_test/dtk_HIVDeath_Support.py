#!/usr/bin/python
import json
import numpy as np
import dtk_test.dtk_sft as dtk_sft
import matplotlib
from sys import platform
if platform == "linux" or platform == "linux2":
    print('Linux OS. Using non-interactive Agg backend')
    matplotlib.use('Agg')
from matplotlib import pyplot as plt
from scipy import stats


class ConfigParam:
    Sym_to_Death_H = "Days_Between_Symptomatic_And_Death_Weibull_Heterogeneity"
    Sym_to_Death_S = "Days_Between_Symptomatic_And_Death_Weibull_Scale"
    child_slow_shape = "HIV_Child_Survival_Slow_Progressor_Shape"
    child_slow_scale = "HIV_Child_Survival_Slow_Progressor_Scale"
    max_child_age = "HIV_Age_Max_for_Child_Survival_Function"
    rapid_fraction = "HIV_Child_Survival_Rapid_Progressor_Fraction"
    rapid_rate = "HIV_Child_Survival_Rapid_Progressor_Rate"
    adult_intercept = "HIV_Adult_Survival_Scale_Parameter_Intercept"
    adult_slope = "HIV_Adult_Survival_Scale_Parameter_Slope"
    adult_shape = "HIV_Adult_Survival_Shape_Parameter"
    max_adult_age = "HIV_Age_Max_for_Adult_Age_Dependent_Survival"
    base_year = "Base_Year"


class EventReport:
    event_name = "Event_Name"
    year = "Year"
    ind_id = "Individual_ID"
    symptomatic = "NewlySymptomatic"
    new_infection = "NewInfectionEvent"
    death = "DiseaseDeaths"


class Constant:
    total_duration = "total_duration"
    latent_duration = "latent_duration"
    actual_latent_duration = "actual_latent_duration"
    symptomatic_duration = "symptomatic_duration"
    age = "Age"
    ind_id = "Individual id"
    time_of_infection = "Time_of_Infection"
    time_of_symptomatic = "Time_of_Symptomatic"
    year_of_symptomatic = "Year_of_Symptomatic"
    time_of_death = "Time_of_Death"
    year_of_death = "Year_of_Death"
    current_duration = "current_duration"
    actual_total_duration = "actual_total_duration"


def plot_durations(duration_df):
    fig = plt.figure()
    ax = fig.add_axes([0.12, 0.15, 0.76, 0.76])
    duration_df.sort_values(by=[Constant.total_duration], inplace=True)
    column_names = duration_df.columns
    cmap = dtk_sft.get_cmap(len(column_names))
    for i, column_name in enumerate(column_names[1:]):
        if column_name in [Constant.latent_duration, Constant.total_duration, Constant.symptomatic_duration,
                           Constant.current_duration]:
            ax.plot(duration_df[column_name].tolist(), label=column_name, linestyle=':', c=cmap(i))
    ax.set_title("duration from StdOut(Sorted by total duration)")
    ax.set_xlabel("data point")
    ax.set_ylabel("Duration(days)")
    plt.legend(loc=0)
    plt.savefig(f"Duration(StdOut).png")
    if dtk_sft.check_for_plotting():
        plt.show()
    plt.close()