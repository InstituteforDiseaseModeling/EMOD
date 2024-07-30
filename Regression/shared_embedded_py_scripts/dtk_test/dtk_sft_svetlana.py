import json
import math
import numpy as np
import matplotlib.pyplot as plt
from scipy import stats
import dtk_test.dtk_sft as sft


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
    valid = True
    if mean < 5 or num_trials*(1-prob) < 5:
        #The general rule of thumb for normal approximation method is that
        # the sample size n is "sufficiently large" if np >= 5 and n(1-p) >= 5
        # for cases that binomial confidence interval will not work
        success = False
        valid = False
        report_file.write("There is not enough sample size in group {0}: mean = {1}, sample size - mean = {2}.\n".format(category, mean,num_trials * (1 - prob)))
    elif num_success < lower_bound or num_success > upper_bound:
        success = False
        report_file.write("BAD: For category {0}, the success cases is {1}, expected 95% confidence interval ( {2}, {3})."
                          " The resulting number is {4}."
                          "\n".format(category, num_success, lower_bound, upper_bound,
                                      "TOO LOW" if num_success < lower_bound else "TOO HIGH"))
    return success, valid

    
def plot_scatter_dist_w_avg(actual, xaxis=None, theory=None, actual_avg=None, theory_avg=None,
                            label1="actual", label2="theory", title=None, xlabel=None,
                            ylabel=None, category="plot_data", show=False, line=False, reverse_x=False):
    if not sft.check_for_plotting():
        return

    fig = plt.figure()
    if title:
        plt.title(title)
    if xlabel:
        plt.xlabel(xlabel)
    if ylabel:
        plt.ylabel(ylabel)
    if line:
        act_color = '-ro'
        theo_color = '-bs'
    else:
        act_color = 'ro'
        theo_color = 'bs'

    if theory is not None and xaxis is not None:
        plt.plot(xaxis, theory, theo_color, markersize=3, label=label2)
        plt.plot(xaxis, actual, act_color, markersize=5, label=label1, marker="+")
    elif theory is not None:
        plt.plot(theory, theo_color, markersize=3, label=label2)
        plt.plot(actual, act_color, markersize=5, label=label1, marker="+")
    elif xaxis is not None:
        plt.plot(xaxis, actual, act_color, markersize=5, label=label1)
    else:
        plt.plot(actual, act_color, markersize=5, label=label1)
    if theory_avg is not None:
        plt.axvline(theory_avg, linewidth=2, color='b', label="theory_avg")
    if actual_avg is not None:
        plt.axvline(actual_avg, linewidth=1, color='r', label="actual_avg")
    if reverse_x:
        plt.gca().invert_xaxis()
    plt.legend()
    fig.savefig(str(category))
    if show:
        plt.show()
    plt.close(fig)
    return None


def is_exponential_behavior(dist, report_file, dist_name=""):
    """
    Tests if there is exponential behavior in the distribution, i.e., if the number in the distribution divided by the
    number after it in the distribution provides the same result each time.  This is characteristic of exponential
    behavior and this method tests if the exponential rates are within 1% of each other.
    :param dist: distribution to be tested
    :return: success
    """
    success = True
    rate_list = []
    for i in range(1, len(dist)):
        rate_list.append(dist[i - 1] / dist[i])
    for num in rate_list:
        high = 1e-2 * num + num
        low = num - 1e-2 * num
        for n in rate_list:
            if n > high or n < low:
                success = False
                report_file.write(f"BAD: {n} was not within 1% of {num}, so the rate is not exponential.\n")
    if success:
        report_file.write(f"GOOD: All exponential rates were within 1%; the {dist_name} distribution follows "
                          "exponential "
                          "behavior.\n")
    return success


def convert_binomial_chain_to_geometric(binomial_chain):
    """
    Convert an array of binomial trials results into # of trials before first success, which is a Geometric distribution.
    Example:
        binomial_chain = <class 'list'>: [10, 9, 8, 7, 6, 6, 5, 4, 4]
        geom = <class 'list'>: [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9]
    Args:
        binomial_chain: array of binomial trials results(# of success at each timestep)
     Returns: geom
     """
    geom = []
    for i in range(len(binomial_chain)):
        for _ in range(binomial_chain[i]):
            # index i starts at 0 but Geometric distribution are from set{1, 2, 3, ...}(shifted geometric distribution)
            geom.append(i+1)
    return geom


def test_geometric_binomial(distribution_under_test, prob, report_file=None, category=None):
    """
    kstest for Geometric distribution.
     In the egg hatching scenario, the distribution_under_test should be an array that contains the
    duration(timesteps) until hatch for each egg. If simulation starts at time 0, then distribution_under_test
    = [1] * # of eggs hatch at day 1 + [2] * # of eggs hatch at day 2 + ... + [i] * # of eggs hatch at day i.
     Args:
        distribution_under_test: the distribution to be tested
        prob: success probability for each trial
        report_file: file to write error reporting
        category: name of test for error reporting
    Returns: True, False
    """
    size = len(distribution_under_test)
    geom_dist = np.random.geometric(p=prob, size=size)

    # use 2 sample ks tests since Geometric distribution is discrete.
    result = stats.ks_2samp(distribution_under_test, geom_dist)

    p = sft.get_p_s_from_ksresult(result)['p']
    s = sft.get_p_s_from_ksresult(result)['s']

    critical_value_s = sft.calc_ks_critical_value(size)

    msg = f"Geometric kstest result for {category} is: statistic={s}, p_value={p}, " \
          f"expected statistic less than {critical_value_s} and p_value larger than 0.05.\n"
    if p >= 5e-2 or s <= critical_value_s:
        if report_file is not None:
            report_file.write("GOOD: " + msg)
        return True
    else:
        if report_file is not None:
            report_file.write("BAD: " + msg)
        return False

