#!/usr/bin/python
import os
if __name__ == '__main__':
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_HIVDeath_Support as hiv_s
from dtk_test.dtk_StdOut import StdOut, SearchType
from scipy import stats


"""
This test is testing the HIV Infection to death duration. Test 2: testing the child Rapid progressors.

Without treatment, progression of HIV disease in infants is generally more rapid than in adults, and can be further 
sub-divided into rapid progressor sand slower progressors following a model published by Ferrand et al.
The proportion of children who are rapid progressors,α,is configured by the parameter 
HIV_Child_Survival_Rapid_Progressor_Fraction. Rapid progressors have an exponentially distributed survival time with 
decay rate β configured by the parameter HIV_Child_Survival_Rapid_Progressor_Rate. Slower progressors have a 
Weibull-distributed survival time with shape parameters configured by HIV_Child_Survival_Slow_Progressor_Shape and 
scale parameter μ configured by HIV_Child_Survival_Slow_Progressor_Scale. The Weibull distribution is a generalized 
form of the exponential distribution that is specified by two parameters, one for shape,the other for scale. 

Data to test: StdOut debug logging.

main test: test infection to death durations counter for child rapid progressors is drawn from the expected 
exponential distribution. 
The simulation runs with 15000+ population for only 2 days to collect the survival duration counter from debug logging. 
The theoretical data is generated based on the exponential cdf. The test is using AD k sample method to 
test the survival duration counter with theoretical data.

Parameters to test:
            "HIV_Child_Survival_Rapid_Progressor_Rate"
            "HIV_Child_Survival_Rapid_Progressor_Fraction"
            "HIV_Age_Max_for_Child_Survival_Function"
            
            
Output: scientific_feature_report.txt
        expon_cdf.png
        infection_to_death_durations.png
        
"""


filter_string_list = ["Update(): Time:",
                      "days_until_death_by_HIV"]


load_df_param = [[hiv_s.Constant.ind_id, hiv_s.Constant.total_duration, hiv_s.Constant.age],
                 ["Individual id=", "days_until_death_by_HIV=", "age_years="],
                 [SearchType.VAL, SearchType.VAL, SearchType.VAL]]


def create_report_file(duration_df, total_pop, config_filename, report_name):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        # Load parameters to test from config
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")

        rapid_rate, max_child_age, rapid_fraction = \
            dtk_sft.get_config_parameter(config_filename,
                                         [hiv_s.ConfigParam.rapid_rate,
                                          hiv_s.ConfigParam.max_child_age,
                                          hiv_s.ConfigParam.rapid_fraction])
        rapid_rate_day = rapid_rate / dtk_sft.DAYS_IN_YEAR
        output_report_file.write(f"{hiv_s.ConfigParam.rapid_rate} = {rapid_rate} per years({rapid_rate_day} per days).\n")
        output_report_file.write(f"{hiv_s.ConfigParam.max_child_age} = {max_child_age} years.\n")

        if rapid_fraction < 1:
            succeed = False
            output_report_file.write(f"BAD: {hiv_s.ConfigParam.rapid_fraction} must be 1 in this test, please check the "
                                     f"test.\n")
        else:
            output_report_file.write(f"{hiv_s.ConfigParam.rapid_fraction} = {rapid_fraction}, so all child will be rapid "
                                     f"progressors in this test.\n")

        # Main test
        # Get infection to death counter for all children.
        duration_df.sort_values(by=[hiv_s.Constant.total_duration], inplace=True)
        infection_to_death_durations = duration_df[
            duration_df[hiv_s.Constant.age] < max_child_age][
            hiv_s.Constant.total_duration].values.tolist()
        output_report_file.write(f"{len(infection_to_death_durations)} children out of {total_pop} individuals.\n")
        output_report_file.write(f"Test data point size = {len(infection_to_death_durations)}.\n")

        # comment out this block since the duration is at least one week(7 days), it becomes a discrete distribution.
        # # Test if the durations under test are drawn from a standard Exponential distribution:
        # result = dtk_sft.test_exponential(infection_to_death_durations, rapid_rate_day,
        #                                   report_file=output_report_file,
        #                                   integers=True, roundup=True, round_nearest=False)
        # if not result:
        #     succeed = False

        # duration is at least one week
        output_report_file.write("Duration is at least one week(7 days), draw theoretical data and modify it.\n")
        # draw theoretical data from scipy
        dist_expon_scipy = stats.expon.rvs(loc=0, scale=1/rapid_rate_day, size=len(infection_to_death_durations))
        dist_expon_scipy = [x if x > 7 else 7 for x in dist_expon_scipy]

        # Using AD k sample test.
        result = stats.anderson_ksamp([infection_to_death_durations, dist_expon_scipy])
        p_value = result.significance_level
        s = result.statistic
        msg = f"anderson_ksamp() with rapid_rate = {rapid_rate}({rapid_rate_day} per days) return p value = " \
            f"{p_value} and statistic = {s}.\n"
        if p_value < 5e-2:
            succeed = False
            output_report_file.write("BAD: statistical test on infection to death durations failed! " + msg)
        else:
            output_report_file.write("GOOD: statistical test on infection to death durations passed! " + msg)
        # Plot with theoretical distribution
        dtk_sft.plot_data(infection_to_death_durations, dist_expon_scipy,
                          sort=True, category="infection_to_death_durations", overlap=True,
                          label1="infection_to_death_durations",label2="expon from scipy(min=7 days)",
                          xlabel="Data Points", ylabel="Duration",
                          title="infection_to_death_durations")

        # Plot with cdf function
        dtk_sft.plot_cdf_w_fun(infection_to_death_durations, name="expon_cdf", cdf_function=stats.expon.cdf,
                               args=(0, 1/rapid_rate_day), show=True)

        output_report_file.write(dtk_sft.format_success_msg(succeed))

        return succeed


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done(stdout_filename)
    stdout = StdOut(stdout_filename, filter_string_list, load_df_param)
    duration_df = stdout.df

    total_pop = 0
    for line in stdout.filtered_lines:
        if "Time: 1" in line:
            total_pop = int(dtk_sft.get_val("StatPop: ", line))
            break
    if total_pop <= 0:
        raise ValueError(f"total population is {total_pop}, please check the test.")

    create_report_file(duration_df, total_pop, config_filename, report_name)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

