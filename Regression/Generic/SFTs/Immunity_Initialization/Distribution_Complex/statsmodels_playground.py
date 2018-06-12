# #import numpy as np
# import pandas
# #import statsmodels.api as sm
# #import statsmodels.formula.api as smf
#
# old_df = pandas.read_csv("DEBUG_df_1825_7300.csv")
#
# import seaborn as sns
# #import matplotlib as mpl
# import matplotlib.pyplot as plt
#
# sns.regplot(x="age", y="mod_acquire", data=old_df)
# plt.show()

df_default_name = "DEBUG_df_1826.0_7300.csv"

def load_df(df_name=df_default_name):
    import pandas
    tmp_df = pandas.read_csv(df_name)
    return tmp_df

def seaborn_plot(df_name=df_default_name):
    my_df = load_df(df_name)
    import seaborn as sns
    import matplotlib.pyplot as plt

    sns.regplot(x="age", y="mod_acquire", data=my_df)
    plt.show()

def seaborn_plot_rolling(df_name=df_default_name):
    my_df = load_df(df_name)
    import seaborn as sns
    import matplotlib.pyplot as plt

    sns.regplot(x="age", y="mean_mod_50", data=my_df)
    plt.show()


def sm_try(df_name=df_default_name):
    my_df = load_df(df_name)

    import statsmodels.api as sm
    import statsmodels.formula.api as smf
    results = smf.ols(formula='mod_acquire ~ age', data=my_df).fit()

    print(results.summary())

def sm_try_rolling(df_name=df_default_name):
    my_df = load_df(df_name)

    import statsmodels.formula.api as smf
    results = smf.ols(formula='mean_mod_50 ~ age', data=my_df).fit()

    print(results.summary())

def sm_test_95_confidence(df_name=df_default_name, debug=False):
    my_df = load_df(df_name)

    import statsmodels.formula.api as smf
    results = smf.ols(formula='mean_mod_50 ~ age', data=my_df).fit()

    import json
    overlay = None
    with open("complex_immunity_overlay.json") as infile:
        overlay = json.load(infile)

    distribution = overlay['Defaults']['IndividualAttributes']['SusceptibilityDistribution']

    dist_values = distribution['DistributionValues'][0]
    result_values = distribution['ResultValues'][0]

    age_start = dist_values[-2]
    age_end = dist_values[-1]

    prob_start = result_values[-2]
    prob_end = result_values[-1]

    if debug:
        print("age start {0} age end {1}\n".format(age_start, age_end))
        print("prb start {0} prb end {1}\n".format(prob_start, prob_end))

    expected_slope = (prob_end - prob_start) / (age_end - age_start)
    expected_intercept = prob_start - (expected_slope * age_start)

    intercept = results.params.Intercept
    slope = results.params.age
    std_err = results.bse.age

    if debug:
        print("Expected slope: {0}\n".format(expected_slope))
        print("Expected intercept: {0}\n".format(expected_intercept))
        print("Intercept: {0}".format(intercept))
        print("slope: {0}\n".format(slope))
        print("Std_Err: {0}\n".format(std_err))















