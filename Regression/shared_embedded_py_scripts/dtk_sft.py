#!/usr/bin/python

import math
import time
import re
import os
from hashlib import md5
from scipy import stats
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import warnings

"""
This module centralizes some small bits of functionality for SFT tests
to make sure we are using the same strings for messages and filenames.
"""

sft_output_filename = "scientific_feature_report.txt"
sft_no_test_data = "BAD: Relevant test data not found.\n"
sft_test_filename = "test.txt"
SFT_EOF = "Finalizing 'InsetChart.json' reporter"
DAYS_IN_MONTH = 30
DAYS_IN_YEAR = 365
MONTHS_IN_YEAR = 12

def start_report_file(output_filename, config_name):
    with open(output_filename, 'w') as outfile:
        outfile.write("Beginning validation for {0} at time {1}.\n".format(
            config_name,
            time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
        ))


def format_success_msg(success):
    report_msg = "SUMMARY: Success={0}\n".format(success)
    return report_msg


def add_time_stamp(time_stamp, line):
    temp_array = line.split()
    temp_array.append("time= " + str(time_stamp))
    new_line = " ".join(temp_array) + "\n"
    return new_line


def test_binomial_95ci(num_success, num_trials, prob, report_file, category):
    """
    -------------------------------------------------------
        This function test if a binomial distribution falls within the 95% confidence interval
            :param num_success: the number of successes
            :param num_trials: the number of trial
            :param prob: the probability of success for one trial
            :param report_file: for error reporting
            :param category: for error reporting
            :return: True, False
    -------------------------------------------------------
    """
    # calculate the mean and  standard deviation for binomial distribution
    mean = num_trials * prob
    standard_deviation = math.sqrt(prob * (1 - prob) * num_trials)
    # 95% confidence interval
    lower_bound = mean - 2 * standard_deviation
    upper_bound = mean + 2 * standard_deviation
    success = True
    if mean < 5 or num_trials * (1 - prob) < 5:
        # The general rule of thumb for normal approximation method is that
        # the sample size n is "sufficiently large" if np >= 5 and n(1-p) >= 5
        # for cases that binomial confidence interval will not work
        success = False
        report_file.write(
            "There is not enough sample size in group {0}: mean = {1}, sample size - mean = {2}.\n".format(category,
                                                                                                           mean,
                                                                                                           num_trials * (
                                                                                                           1 - prob)))
    elif num_success < lower_bound or num_success > upper_bound:
        success = False
        report_file.write(
            "BAD: For category {0}, the success cases is {1}, expected 95% confidence interval ( {2}, {3}).\n".format(
                category, num_success, lower_bound, upper_bound))
    return success


def test_binomial_99ci(num_success, num_trials, prob, report_file, category):
    """
    -------------------------------------------------------
        This function test if a binomial distribution falls within the 99.73% confidence interval
            :param num_success: the number of successes
            :param num_trials: the number of trial
            :param prob: the probability of success for one trial
            :param report_file: for error reporting
            :param category: for error reporting
            :return: True, False
    -------------------------------------------------------
    """

    # calculate the mean and  standard deviation for binomial distribution
    mean = num_trials * prob
    standard_deviation = math.sqrt(prob * (1 - prob) * num_trials)
    # 99.73% confidence interval
    lower_bound = mean - 3 * standard_deviation
    upper_bound = mean + 3 * standard_deviation
    success = True
    if mean < 5 or num_trials * (1 - prob) < 5:
        # The general rule of thumb for normal approximation method is that
        # the sample size n is "sufficiently large" if np >= 5 and n(1-p) >= 5
        success = False
        report_file.write(
            "There is not enough sample size in group {0}: mean = {1}, sample size - mean = {2}.\n".format(category,
                                                                                                           mean,
                                                                                                           num_trials * (
                                                                                                           1 - prob)))
    elif num_success < lower_bound or num_success > upper_bound:
        success = False
        report_file.write(
            "BAD: For category {0}, the success cases is {1}, expected 99.75% confidence interval ( {2}, {3}).\n".format(
                category, num_success, lower_bound, upper_bound))
    return success


def calc_poisson_binomial(prob):
    """
    ------------------------------------------------------
        By definition, a Poisson binomial distribution is a sum of n independent Bernoulli distribution
        this function calculate the mean, standard deviation and variance based on probabilities from n independent Bernoulli distribution
            :param prob: list of probabilities from n independent Bernoulli distribution
            :return: mean, standard deviation and variance
    ------------------------------------------------------
    """
    mean = 0
    variance = 0
    standard_deviation = 0
    for p in prob:
        mean += p
        variance += (1 - p) * p
        standard_deviation = math.sqrt(variance)
    return {'mean': mean, 'standard_deviation': standard_deviation, 'variance': variance}


def calc_poisson_prob(mean, num_event):
    """
    ----------------------------------------------------
        This function calculates approximate probability of obeserving num_event events in an interval for a Poisson distribution
            :param mean: the average number of events per interval
            :param num_event: the number of event happened in an interval
            :return: probability of obeserving num_event events in an interval
    ----------------------------------------------------
    """
    a = math.exp(-1.0 * mean)
    b = math.pow(mean, float(num_event))
    c = math.factorial(int(num_event))
    prob = a * b / float(c)
    return prob


def plot_poisson_probability(rate, num, file, category='k vs. probability', xlabel='k', ylabel='probability',
                             show=True):
    """
    -----------------------------------------------------
    This function plot and save the actual and expected poisson probability and save them in a file when the error is
    larger than the tolerance
            :param rate:
            :param num:
            :param file:
            :param category:
            :param xlabel:
            :param ylabel:
            :return:
    -----------------------------------------------------
    """
    if not check_for_plotting():
        show = False

    d = {}
    x = []
    y = []
    z = []
    for n in num:
        if n in d:
            d[n] += 1
        else:
            d[n] = 1
    for n in sorted(num):
        if n in d:
            if rate < 10:
                p = calc_poisson_prob(rate, n)
                # p = stats.poisson.pmf(n,rate)
            else:
                loc = rate  # mean
                scale = math.sqrt(rate)  # standard deviation
                p = stats.norm.pdf(n, loc, scale)
            x.append(n)
            y.append(p)
            z.append(d[n] / float(len(num)))
            # file.write("Poisson Probability of {0} is {1}, expected {2}.
            # \n".format(n, p, d[n] / float(len(num_exposures_enviro))))
            if math.fabs(p - d[n] / float(len(num))) > 0.001:
                file.write("{3} : Probability for NumOfExposures {0} is {1}, expected {2}."
                           "\n".format(n, d[n] / float(len(num)), p, category))
                file.write("{3} : Count for NumOfExposures {0} is {1}, expected {2}."
                           "\n".format(n, d[n], p * float(len(num)), category))
            d.pop(n)
    fig = plt.figure()
    plt.plot(x, y, 'r--', x, z, 'bs')
    plt.title("{0}:rate = {1}, sample size = {2}".format(category, rate, len(num)))
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    if show:
        plt.show()
    fig.savefig(str(category) + "_rate" + str(rate) + ".png")
    plt.close(fig)
    return None


def plot_probability(dist1, dist2=None, precision=1, label1="test data", label2="scipy data",
                     title='probability mass function', xlabel='k', ylabel='probability', category='test', show=True,
                     line=False):
    """
    -----------------------------------------------------
    This function plot and the probability mass function of two distributions
            :param dist1:
            :param dist1:
            :param precision:
            :param title:
            :param xlabel:
            :param ylabel:
            :return:
    -----------------------------------------------------
    """

    if not check_for_plotting():
        show = False

    d = {}
    x = []
    y = []
    x2 = []
    y2 = []
    for n in sorted(dist1):
        # round n to x number of decimal value, x = precision
        i = round(n, precision)
        if i in d:
            d[i] += 1
        else:
            d[i] = 1
    for key in sorted(d):
        x.append(key)
        y.append(d[key] / float(len(dist1)))
        d.pop(key)
    fig = plt.figure()
    if line:
        color1 = 'r'
        color2 = 'b'
    else:
        color1 = 'rs'
        color2 = 'bs'
    if dist2 is not None:  # solve the future warning with comparison to 'None'.
        # if dist2: doesn't work for dataframe in Bamboo environment.
        for n in sorted(dist2):
            i = round(n, precision)
            if i in d:
                d[i] += 1
            else:
                d[i] = 1
        for key in sorted(d):
            x2.append(key)
            y2.append(d[key] / float(len(dist2)))
            d.pop(key)
        plt.plot(x, y, color1, x2, y2, color2)
    else:
        plt.plot(x, y, color1)
    # plt.plot(x, y, 'r--')
    plt.title("{0}".format(title))
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    red_patch = mpatches.Patch(color='red', label=label1)
    blue_patch = mpatches.Patch(color='blue', label=label2)
    plt.legend(handles=[red_patch, blue_patch])
    fig.savefig(str(category) + '.png')
    if show:
        plt.show()
    plt.close(fig)
    return None


def plot_hist(dist1, dist2=None, label1="test data 1", label2="test data 2", title=None, xlabel=None, ylabel=None,
              category='histogram', show=True):
    """
    -----------------------------------------------------
    This function plot and the histogram of one\two distributions
            :param dist1:
            :param dist1:
            :param title:
            :param xlabel:
            :param ylabel:
            :return:
    -----------------------------------------------------
    """

    if not check_for_plotting():
        show = False

    fig = plt.figure()
    if title:
        plt.title(title)
    if xlabel:
        plt.xlabel(xlabel)
    if ylabel:
        plt.ylabel(ylabel)
    if dist2 is not None:
        plt.hist([dist1, dist2], color=['r', 'b'], alpha=0.8)
    else:
        plt.hist(dist1, color='r', alpha=0.8)
    red_patch = mpatches.Patch(color='red', label=label1)
    blue_patch = mpatches.Patch(color='blue', label=label2)
    plt.legend(handles=[red_patch, blue_patch])
    fig.savefig(str(category) + '.png')
    if show:
        plt.show()
    plt.close(fig)
    return None


def calc_cdf(dist, num_bin=20):
    min_num = min(dist)
    max_num = max(dist)
    step = float(max_num - min_num) / num_bin
    bin_range = np.arange(min_num, max_num + step, step)
    # Use the histogram function to bin the data
    counts, bin_edges = np.histogram(dist, bins=bin_range, normed=True)
    # Now find the cdf
    cdf = np.cumsum(counts)
    max_p = float(max(cdf))
    cdf = [x / max_p for x in cdf]
    return cdf, bin_edges[1:]


def plot_cdf(dist1, dist2=None, label1="test data", label2="scipy data", title='Cumulative distribution function',
             xlabel='k', ylabel='probability', category='Cumulative_distribution_function', show=True, line=False):
    """
    -----------------------------------------------------
    This function plot and the Cumulative distribution function of two distributions
            :param dist1:
            :param dist1:
            :param title:
            :param xlabel:
            :param ylabel:
            :return:
    -----------------------------------------------------
    """

    if not check_for_plotting():
        show = False

    fig = plt.figure()
    if line:
        color1 = 'r'
        color2 = 'b'
    else:
        color1 = 'ro'
        color2 = 'bo'

    num_bin = 20
    cdf, bin = calc_cdf(dist1, num_bin)

    if dist2 is None:
        plt.plot(bin, cdf, color1)
    else:
        cdf_2, bin_2 = calc_cdf(dist2, num_bin)
        plt.plot(bin, cdf, color1, bin_2, cdf_2, color2)

    plt.title("{0}".format(title))
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    red_patch = mpatches.Patch(color='red', label=label1)
    blue_patch = mpatches.Patch(color='blue', label=label2)
    plt.legend(handles=[red_patch, blue_patch])
    # plt.ylim((0.0, 1.01))
    fig.savefig(str(category) + '.png')
    if show:
        plt.show()
    plt.close(fig)
    return None


def check_for_plotting():
    homepath = None
    if os.environ.get("HOME"):
        homepath = os.getenv("HOME")
    elif os.environ.get("HOMEDRIVE"):
        homepath = os.path.join(os.getenv("HOMEDRIVE"), os.getenv("HOMEPATH"))
    else:
        # HPC case: if none of the env vars are present, we're on the HPC and won't plot anything.
        return False

    if (os.path.exists(os.path.join(homepath, ".rt_show.sft"))):
        return True
    else:
        return False

def plot_data_unsorted(dist1, dist2=None, label1="test data 1", label2="test data 2", title=None, xlabel=None, ylabel=None,
              category='plot_data_unsorted', show=True, line=False, alpha=1, overlap=False):
    """
    -----------------------------------------------------
    This function plot the data of one\two distributions
            :param dist1:
            :param dist1:
            :param title:
            :param xlabel:
            :param ylabel:
            :return:
    -----------------------------------------------------
    """
    exception = "plot_data_unsorted is deprecated, please use plot_data() with sort argument instead."
    warnings.warn(exception, FutureWarning)

    if not check_for_plotting():
        show = False

    plot_data(dist1=dist1, dist2=dist2, label1=label1, label2=label2, title=title,
                  xlabel=xlabel, ylabel=ylabel, category=category, show=show, line=line,alpha=alpha, overlap=overlap, sort=False)

def plot_data_sorted(dist1, dist2=None, label1="test data 1", label2="test data 2", title=None, xlabel=None, ylabel=None,
              category='plot_data_sorted', show=True, line=False, alpha=1, overlap=False):
    """
    -----------------------------------------------------
    This function sort and plot the data of one\two distributions
            :param dist1:
            :param dist1:
            :param title:
            :param xlabel:
            :param ylabel:
            :return:
    -----------------------------------------------------
    """
    exception = "plot_data_sorted is deprecated, please use plot_data() with sort argument instead."
    warnings.warn(exception, FutureWarning)

    if not check_for_plotting():
        show = False

    plot_data(dist1=dist1, dist2=dist2, label1=label1, label2=label2, title=title,
                  xlabel=xlabel, ylabel=ylabel, category=category, show=show, line=line,alpha=alpha, overlap=overlap, sort=True)



def plot_data(dist1, dist2=None, label1="test data 1", label2=None, title=None,
                       xlabel=None, ylabel=None, xmin=None, xmax=None, ymin=None, ymax=None,
                       category ='plot_data', show=True, line=False,alpha=1, overlap=False, marker1='s', marker2='o', sort=False):
    """
    -----------------------------------------------------
    This function plot the data of one\two distributions
            :param dist1:
            :param dist1:
            :param title:
            :param xlabel:
            :param ylabel:
            :return:
    -----------------------------------------------------
    """
    if not check_for_plotting():
        show = False

    if sort: # use not in-place method
        dist1 = sorted(dist1)
        if dist2 is not None:
            dist2 = sorted(dist2)

    fig = plt.figure()
    ax= fig.add_axes([0.12,0.12,0.76,0.76])
    if title:
        ax.set_title(title)
    if xlabel:
        ax.set_xlabel(xlabel)
    if ylabel:
        ax.set_ylabel(ylabel)
    if xmin and xmax:
        ax.set_xlim(xmin, xmax)
    else:
        if xmax:
            ax.set_xlim(xmax=xmax)
        if xmin:
            ax.set_xlim(xmin=xmin)
    if ymin and ymax:
        ax.set_ylim(ymin, ymax)
    else:
        if ymax:
            ax.set_ylim(ymax=ymax)
        if xmin:
            ax.set_ylim(ymin=ymin)
    if overlap:
        color1 = 'r'+marker1
        color2 = 'b'+marker2
    else:
        color1 = 'y'+marker1
        color2 = 'g'+marker2
    if line:
        color1 += '-'
        color2 += '-'
    ax.plot(dist1, color1, alpha=alpha, label=label1, lw= 0.5, markersize=4)
    if dist2 is not None: # "if dist2:" will not work with numpy.ndarray
        plt.plot(dist2, color2, alpha=alpha, label=label2, lw=0.5, markersize=3)
    ax.legend(loc=0)

    fig.savefig(str(category) + '.png')
    if show:
        plt.show()
    plt.close(fig)
    return None

def plot_pie(sizes, labels, category='plot_pie', show=True):
    """
    plot a pie chart based on sizes
    :param sizes:
    :param labels:
    :param show:
    :return:
    """
    plt.pie(sizes, labels=labels, autopct='%1.1f%%', shadow=True, startangle=140)
    plt.axis('equal')
    plt.savefig(str(category) + '.png')
    if show:
        plt.show()
    plt.close()
    return None

def plot_scatter_with_fit_lines(dataframe, xlabel, ylabel, fit_data_segments,
                                est_data_segments, fit_data_label="fits to data",
                                est_data_label="estimated fits", category="plot_data_fits",
                                show=True):
    """
    Plots a scatterplot of data, as well as fitted and estimated segments. This is written with
    Immunity Initialization in mind
    :param dataframe:
    :param xlabel: What is the x-axis? For example, 'age'
    :param ylabel: what is the y-axis? For example, 'mod_acquire'
    :param fit_data_segments: array of data-fit line segments in this format ([startx, endx],[starty, endy])
    :param est_data_segments: array of esimated (ideal) line segments as above
    :param fit_data_label: ("fits to data")
    :param est_data_label: ("esitmated fits")
    :param category: name of file without extension ("plot_data_fits")
    :param show:
    :return:
    """
    if not check_for_plotting():
        return
    fig = plt.figure()
    plt.scatter(dataframe[xlabel], dataframe[ylabel], s=20, alpha=0.02, lw=0)
    for segment in est_data_segments:
        plt.plot(segment[0], segment[1], 'r') # Red is reference, or expected data
    for segment in fit_data_segments:
        plt.plot(segment[0], segment[1], 'b') # Blue is data under test
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    fig.savefig(str(category) + '.png')
    if show:
        plt.show()
    plt.close(fig)

def calc_ks_critical_value(num_trials):
    """
    -----------------------------------------------------
    This function returns the critical values for kstest Statistic test based on KS table (reference: http://www.cas.usf.edu/~cconnor/colima/Kolmogorov_Smirnov.htm)
            :param num_trials: number of trials = length of distribution
            :return: critical values
    -----------------------------------------------------
    """
    # KS table
    ks_table = [0.975, 0.842, 0.708, 0.624, 0.565, 0.521, 0.486, 0.457, 0.432, 0.410, 0.391, 0.375, 0.361, 0.349, 0.338,
                0.328, 0.318, 0.309, 0.301, 0.294, 0.270, 0.240, 0.230]
    critical_value_s = 0
    if num_trials <= 20:
        critical_value_s = ks_table[num_trials - 1]
    elif num_trials <= 25:
        critical_value_s = ks_table[20]
    elif num_trials <= 30:
        critical_value_s = ks_table[21]
    elif num_trials <= 35:
        critical_value_s = ks_table[22]
    else:
        critical_value_s = 1.36 / math.sqrt(num_trials)
    return critical_value_s


def get_p_s_from_ksresult(result):
    # print( str( result ) )
    p = s = 0
    # NOTE: different versions of kstest seem to produce different output.
    if "pvalue" in result:
        p = float(get_val("pvalue=", str(result)))
        s = float(get_val("statistic=", str(result)))
    else:
        s = result[0]
        p = result[1]
        # report_file.write("s is {0}, p is : {1} for {2}.\n".format(s, p, category))
    return {'p': p, 's': s}


def create_geometric_dis(rate, scale, size, test_decay=True):
    curr_change = 0
    curr_count = scale
    series = []
    for _ in range(size):
        curr_count -= curr_change
        curr_change = math.floor(curr_count * rate)
        if test_decay:
            series.append(curr_count)
        else:
            series.append(scale - curr_count)
    return series


def test_geometric_decay(distribution_under_test, rate, scale, test_decay=True, report_file=None, debug=False):
    """
    Tests if the given distribution is a geometric distribution
    with the given rate and scale
    :param distribution_under_test: array of integers to test against
    :param rate:  rate per timestep at which the value decays
    :param scale:  number of things to decay
    :param test_decay: True assumes that your distribution is being decayed, False assumes it is growing
    :return:
    """
    size = len(distribution_under_test)
    series = create_geometric_dis(rate, scale, size, test_decay)

    result = stats.ks_2samp(series, distribution_under_test)
    if debug and report_file:
        report_file.write(str(result) + "\n")

    p = get_p_s_from_ksresult(result)['p']
    s = get_p_s_from_ksresult(result)['s']

    critical_value_s = calc_ks_critical_value(len(distribution_under_test))
    report_file.write("distribution under test\n")
    report_file.write(str(distribution_under_test) + "\n")
    report_file.write("series I made\n")
    report_file.write(str(series) + "\n")
    if p >= 5e-2 or s <= critical_value_s:
        success = True
    else:
        if report_file is not None:
            report_file.write(
                "BAD: Two sample kstest result for {0} is: statistic={1}, pvalue={2}, expected s less than {3} and p larger than 0.05.\n".format(
                    "geometric decay", s, p, critical_value_s))
        success = False
    return success


def test_poisson(trials, rate, report_file=None, route=None, normal_approximation=True):
    """
    -----------------------------------------------------
        This function test if a distribution is a Poisson distribution with given rate
        For rate < 10, I am testing it using the two-sample kstest since one-sample kstest test for poisson distribution has a bug
            previously, I am test it based on probability of events for a Possion distribution, this test is too sensitive and may not be a good fit in our case
        For rate >= 10, I am using Normal Approximation with continuity correction
        If normal_approcimation = False, I am testing it using the two-sample kstest
            :param trials: distribution to test, it contains values 0, 1, 2, ...
            :param rate: the average number of events per interval
            :param report_file:
            :param route:
            :param normal_approximation: if false, don't use normal approximation for rate >=10, only use poisson kstest for all rates.
            :return: True/False
    -----------------------------------------------------
    """
    success = True
    size = len(trials)
    if not normal_approximation or rate < 10:
        # rate less than 10 is calculated as a Poisson Draw
        # ps is Poisson distribution
        ps = np.random.poisson(rate, size)
        result = stats.ks_2samp(ps, trials)

        p = get_p_s_from_ksresult(result)['p']
        s = get_p_s_from_ksresult(result)['s']

        critical_value_s = calc_ks_critical_value(size)
        if p >= 5e-2 or s <= critical_value_s:
            success = True
        else:
            if report_file is not None:
                report_file.write(
                    "BAD: Poisson two sample kstest result for {0} is: statistic={1}, pvalue={2}, expected s less than {3} and p larger than 0.05.\n".format(
                        route, s, p, critical_value_s))
            success = False
    else:
        # rate >= 10 is using Normal Approximation with continuity correction
        # stats.kstest(trials, 'norm', args=(loc, scale)) will fail since kstest is too sensitive for this approximation
        d = {}
        count = 0
        for n in sorted(trials):
            if d.get(n):
                d[n] += 1
            else:
                d[n] = 1
        for key in d:
            count += d[key]
            loc = rate  # mean
            scale = math.sqrt(rate)  # standard deviation
            p = stats.norm.cdf(key + 0.5, loc, scale)  # continuity correction
            actual_p = (count / float(size))  # calculate actual cumulative probability
            if math.fabs(p - actual_p) > 0.02:
                success = False
                if report_file != None:
                    report_file.write(
                        "BAD: Poisson cumulative probability for {0} and {1} is {2}, expected {3}.\n".format(route, key,
                                                                                                             actual_p,
                                                                                                             p))
    return success


def test_lognorm(timers, mu, sigma, report_file=None, category=None, round=False):
    """
    -----------------------------------------------------
        kstest for lognormal distribution
            :param timers: the distribution to test
            :param mu: mean, which is equal to sqrt(scale)
            :param sigma: the standard deviation
            :param report_file: for error reporting
            :param category: for error reporting
            :return: True, False
    -----------------------------------------------------
    """
    # print( "Running test_lognorm for " + category )
    scale = math.exp(mu) # median
    size = len(timers)
    dist_lognormal = stats.lognorm.rvs(sigma, 0, scale, size)
    # dist_lognormal = np.random.lognormal(mu, sigma, size)

    if round:
        dist_lognormal_2 = []
        for n in dist_lognormal:
            dist_lognormal_2.append(round_to_n_digit(n, 7))
        result = stats.ks_2samp(dist_lognormal_2,
                                timers)  # switch to 2 sample kstest so I can round the data from scipy
    else:
        result = stats.kstest(timers, 'lognorm', args=(sigma, 0, scale))

    p = get_p_s_from_ksresult(result)['p']
    s = get_p_s_from_ksresult(result)['s']

    critical_value_s = calc_ks_critical_value(size)

    if p >= 5e-2 or s <= critical_value_s:
        if report_file is not None:
            report_file.write(
                "GOOD: log normal kstest result for {0} is: statistic={1}, pvalue={2}, expected s less than {3} and p larger than 0.05.\n".format(
                    category, s, p, critical_value_s))
        return True
    else:
        if report_file is not None:
            report_file.write(
                "BAD: log normal kstest result for {0} is: statistic={1}, pvalue={2}, expected s less than {3} and p larger than 0.05.\n".format(
                    category, s, p, critical_value_s))
        # print("BAD: log normal kstest result for {0} is: statistic={1}, pvalue={2}, expected s less than {3} and p larger than 0.05.\n".format(category, s, p, critical_value_s))
        return False


def test_uniform(dist, p1, p2, report_file=None, round=False, significant_digits=7):
    """
     -----------------------------------------------------
        kstest for uniform distribution
            :param p1: loc
            :param p2: loc + scale
            :param dist: The distribution to be tested
            :return: True, False
    -----------------------------------------------------
    """
    loc = p1
    scale = p2 - p1
    size = len(dist)
    dist_uniform_scipy = stats.uniform.rvs(loc, scale, size)
    # dist_uniform_np = np.random.uniform(p1, p2, size)
    if round:
        dist_uniform_scipy_r = []
        for s in dist_uniform_scipy:
            dist_uniform_scipy_r.append(round_to_n_digit(s, significant_digits))
        result = stats.ks_2samp(dist_uniform_scipy_r, dist)
    else:
        result = stats.ks_2samp(dist_uniform_scipy,dist)
        # result = stats.kstest(dist, 'uniform', args=(loc, scale)) this is not compatible for Python 3, only Python 2 works
    p = get_p_s_from_ksresult(result)['p']
    s = get_p_s_from_ksresult(result)['s']
    critical_value_s = calc_ks_critical_value(size)

    # return p >= 5e-2 or s <= critical_value_s
    if p >= 5e-2 or s <= critical_value_s:
        return True
    else:
        if report_file is not None:
            report_file.write(
                "BAD: ({0},{1})failed with statistic={2}, pvalue={3}, expected s less than {4} and p larger than 0.05.\n".format(
                    p1, p2, s, p, critical_value_s))
        return False


def test_gaussian(dist, p1, p2, allow_negative=True, report_file=None, round=False):
    """
     -----------------------------------------------------
        kstest for gaussian distribution
            :param dist: The distribution to be tested
            :param p1: mean, loc
            :param p2: width(standard deviation), scale
            :param allow_negative: allow negative value in normal distribution, if False, turn all negative value to 0.0
            :param round: wether
            :return: True, False
    -----------------------------------------------------
    """
    size = len(dist)
    if round or not allow_negative:
        # dist_gaussian_np = np.random.normal(p1, p2, size)
        dist_gaussian_scipy = stats.norm.rvs(p1, p2, size)
        dist_gaussian_scipy2 = []
        for n in dist_gaussian_scipy:
            if (not allow_negative) and n < 0:
                n = 0
            if round:
                dist_gaussian_scipy2.append(round_to_n_digit(n, 7))
            else:
                dist_gaussian_scipy2.append(n)
        result = stats.ks_2samp(dist_gaussian_scipy2, dist)
    else:
        result = stats.kstest(dist, "norm", args=(p1, p2))

    p = get_p_s_from_ksresult(result)['p']
    s = get_p_s_from_ksresult(result)['s']
    critical_value_s = calc_ks_critical_value(size)

    # return p >= 5e-2 or s <= critical_value_s
    if p >= 5e-2 or s <= critical_value_s:
        return True
    else:
        if report_file is not None:
            report_file.write(
                "BAD: ({0},{1})failed with statistic={2}, pvalue={3}, expected s less than {4} and p larger than 0.05.\n".format(
                    p1, p2, s, p, critical_value_s))
        return False


def round_down(num, precision):
    multiplier = math.pow(10.0, precision)
    return math.floor(num * multiplier) / multiplier


def round_up(num, precision):
    multiplier = math.pow(10.0, precision)
    return math.ceil(num * multiplier) / multiplier


def test_exponential(dist, p1, report_file=None, integers=False, roundup=False, round_nearest=False):
    """
     -----------------------------------------------------
        kstest for exponential distribution
            :param p1: decay rate = 1 / decay length , lambda, >0,
            :param dist: The distribution to be tested
            :param report_file: report file to which write the error if such exists
            :param integers: Indicates whether the distribution is rounded up or down to integers or not
            :return: True, False
    -----------------------------------------------------
    """
    size = len(dist)
    scale = 1.0 / p1
    dist_exponential_np = np.random.exponential(scale, size)
    if integers:
        if round_nearest:
            dist_exponential_np = [round(x) for x in dist_exponential_np]
        elif roundup:
            dist_exponential_np = [round_up(x, 0) for x in dist_exponential_np]
        else:
            dist_exponential_np = [round_down(x, 0) for x in dist_exponential_np]
    # loc = 0
    # dist_exponential_scipy = stats.expon.rvs(loc, scale, size)
    result = stats.ks_2samp(dist_exponential_np, list(dist))
    # ?? result = stats.kstest(dist, "exponential", args=(p1))

    p = get_p_s_from_ksresult(result)['p']
    s = get_p_s_from_ksresult(result)['s']
    critical_value_s = calc_ks_critical_value(size)

    if p >= 5e-2 or s <= critical_value_s:
        if report_file is not None:
            report_file.write(
                "GOOD: ({0})succeed with statistic={1}, pvalue={2}, expected s less than {3} and p larger "
                "than 0.05.\n".format(p1, s, p, critical_value_s))
        return True
    else:
        if report_file is not None:
            report_file.write("BAD: ({0})failed with statistic={1}, pvalue={2}, expected s less than {3} and p larger "
                              "than 0.05.\n".format(p1, s, p, critical_value_s))
        return False


def test_bimodal(dist, p1, p2, report_file=None):
    """
     -----------------------------------------------------
        Test for bimodal distribution. This bimodal distribution is not a true bimodal distribution, which is defined as the overlap of two Gaussians.
        The definition of bimodal in DTK mathfunctions.cpp is a function that gives an output of either 1 or the value of param2. The param1 controls the fraction.
            :param p1: faction of param 2
            :param p2: multiplier
            :param dist: The distribution to be tested
            :return: True, False
    -----------------------------------------------------
    """
    size = len(dist)
    count1 = 0
    count2 = 0
    for n in dist:
        if n == 1.0:
            count1 += 1
        elif n == p2:
            count2 += 1
        else:
            if report_file is not None:
                report_file.write(
                    "BAD: Binomal distribution contains value = {0}, expected 1.0 or {1}.\n".format(n, p2))
            return False
    actual_faction = count2 / float(size)
    if math.fabs(p1 - actual_faction) <= 5e-2:
        return True
    else:
        if report_file is not None:
            report_file.write(
                "BAD: test Binomal failed with actual fraction = {0}, expected {1}.\n".format(actual_faction, p1))
        return False


def test_weibull(dist, p1, p2, report_file=None, round=False):
    """
     -----------------------------------------------------
        kstest for weibull distribution
            :param p1: scale, lambda > 0
            :param p2: shape, kappa > 0
            :param dist: The distribution to be tested
            :return: True, False
    -----------------------------------------------------
    """
    size = len(dist)
    # s = np.random.weibull(p2, size)
    # dist_weibull_np = map(lambda x : x * p1, s)
    dist_weibull_scipy = stats.weibull_min.rvs(c=p2, loc=0, scale=p1, size=size)
    if round:
        dist_weibull_scipy2 = []
        for n in dist_weibull_scipy:
            dist_weibull_scipy2.append(round_to_n_digit(n, 7))
        result = stats.ks_2samp(dist_weibull_scipy2, dist)
    else:
        result = stats.ks_2samp(dist_weibull_scipy, dist)

    p = get_p_s_from_ksresult(result)['p']
    s = get_p_s_from_ksresult(result)['s']
    critical_value_s = calc_ks_critical_value(size)

    # return p >= 5e-2 or s <= critical_value_s
    if p >= 5e-2 or s <= critical_value_s:
        return True
    else:
        if report_file is not None:
            report_file.write(
                "BAD: ({0},{1})failed with statistic={2}, pvalue={3}, expected s less than {4} and p larger than 0.05.\n".format(
                    p1, p2, s, p, critical_value_s))
        return False

def test_multinomial(dist, proportions, report_file=None, prob_flag=True):
    """
    Chi-squared test for multinomial data
    :param dist: array_like, number in each categories
    :param p: array_like, proportions in each categories
    :param report_file:
    :param report_file: flag that indicates whether p are proportions or the expected values
    :return: True or False for test result
    """
    if prob_flag:
        n = sum(dist)
        prob = sum(proportions)
        total = int(n/prob)
        result = stats.chisquare(dist, np.array(proportions) * total, ddof=0 ) #returns chi-square statistic and p value
    else:
        result = stats.chisquare(dist, proportions, ddof=0)
    p = get_p_s_from_ksresult(result)['p']
    s = get_p_s_from_ksresult(result)['s']
    if p >= 5e-2:
        if report_file is not None:
            report_file.write(
                "GOOD: Chi-squared test for multinomial data passed with statistic={0}, pvalue={1}, expected p larger"
                " than 0.05.\ndata for test is {2} and proportion is {3}.\n".format(s, p, dist, proportions))
        return True
    else:
        if report_file is not None:
            report_file.write(
                "BAD: Chi-squared test for multinomial data failed with statistic={0}, pvalue={1}, expected p larger"
                " than 0.05.\ndata for test is {2} and proportion is {3}.\n".format(s, p, dist, proportions))
        return False

def is_stats_test_pass(fail_count, pass_count, report_file=None):
    """
    -------------------------------------------------------
        This function determine whether a set of statistic tests(kstest basically) pass
        The threshold for p value to determine whether the ks test pass is hardcoded as 0.05
            :param fail_count: total number of failing statistic tests
            :param pass_count: total number of passing statistic tests
            :param report_file: optional
            :return: True, False
    -------------------------------------------------------
    """
    count = fail_count + pass_count
    # hardcoded threshold for p value
    threshold_pvalue = 5e-2
    # probability for small probability event
    small_prob_event = 1e-3
    # this is the theoretical average failing count
    mean_pvalue = count * threshold_pvalue

    if fail_count < mean_pvalue:
        if report_file is not None:
            report_file.write("mean_pvalue = {0}, fail_count = {1}.\n".format(mean_pvalue, fail_count))
        return True
    else:
        # could use Normal Approximation with continuity correction (mean > 25) and 6 sigma rule.
        # calculate the cummulative density function
        prob = stats.poisson.cdf(fail_count - 1, mean_pvalue)
        if report_file is not None:
            report_file.write(
                "prob = {0}, mean_pvalue = {1}, fail_count = {2}.\n".format(prob, mean_pvalue, fail_count))
        # test passes when the survival function (1 - cdf) >= probability for samll probability event(0.001, 0.01 or 0.05)
        # higher value is stricter than lower value
        # if <=, which means the small probability event happens in real life so we determine the test fails
        return (1.0 - prob) >= small_prob_event


def round_to_1_digit(x):
    """
    -----------------------------------------------------
        Round number x to 1 significant digit
            :param x: number to be rounded
            :return:
    -----------------------------------------------------
        """
    if x == float("inf") or x == float("-inf") or x == 0.0:
        return x
    # elif math.fabs(x - 0.0) < 1e-1:
    #     return 0.0
    else:
        return round(x, -int(math.floor(math.log10(abs(x)))))


def round_to_n_digit(x, n):
    """
    -----------------------------------------------------
        Round number x to n significant digits
            :param x: number to be rounded
            :param n: # of significant digits
            :return:
    -----------------------------------------------------
    """
    if x == float("inf") or x == float("-inf") or x == 0.0:
        return x
    else:
        return round(x, -int(math.floor(math.log10(abs(x)))) + (n - 1))


def get_val(key, line):
    regex = key + "(\d*\.*\d*)"
    match = re.search(regex, line)
    if match is not None:
        return match.group(1)
    else:
        raise LookupError


def wait_for_done():
    with open("test.txt", "r") as test_file:
        while 1:
            where = test_file.tell()
            line = test_file.readline()
            if not line:
                time.sleep(1)
                test_file.seek(where)
            else:
                if SFT_EOF in line:
                    output_file_md5 = md5_hash_of_file("test.txt")
                    with open("touchfile", "w") as touchfile:
                        touchfile.write("Test.txt file completely written. Move on to read.\n")
                        touchfile.write(line)
                        touchfile.write(str(output_file_md5))
                        # print( "Last line read = " + line )
                    return


def md5_hash(handle):
    handle.seek(0)
    md5calc = md5()
    while True:
        data = handle.read(10240)  # reasonable value from examples
        if len(data) == 0:
            break
        md5calc.update(data.encode("UTF-8"))
    hash = md5calc.hexdigest()
    return hash


def md5_hash_of_file(filename):
    # print( "Getting md5 for " + filename )
    file_handle = open(filename)
    hash = md5_hash(file_handle)
    # md5calc = md5()
    # while True:
    # file_bytes = file_handle.read( 10240 ) # value picked from example!
    # if len(file_bytes) == 0:
    # break
    # md5calc.update( file_bytes )
    file_handle.close()
    # hash = md5calc.hexdigest()
    return hash


def has_match(target, matches):
    for match in matches:
        if match in target:
            return True
    return False


def get_char(key, line):
    regex = key + "(\w*\d*\w*\d*)"
    match = re.search(regex, line)
    if match != None:
        return match.group(1)
    else:
        raise LookupError


def convert_barchart_to_interpolation(population_groups, result_values):
    for i in range(len(population_groups)):
        for j in range(0, 2 * len(population_groups[i]), 2):
            age_or_year = population_groups[i][j]
            population_groups[i].insert(j + 1, age_or_year + 0.9999999)
    for i in range(0, 2 * len(result_values), 2):
        age_value = result_values[i]
        age_value_copy = [p for p in age_value]
        result_values.insert(i + 1, age_value_copy)
    for age_value in result_values:
        for i in range(0, 2 * len(age_value), 2):
            age_value.insert(i + 1, age_value[i])
