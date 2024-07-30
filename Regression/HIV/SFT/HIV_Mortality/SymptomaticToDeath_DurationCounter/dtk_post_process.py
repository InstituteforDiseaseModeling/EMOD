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
This test is testing the HIV symptomatic to death duration. Test 2: testing the duration counters.

Upon infection or ART discontinuation, the individuals draw from the configured Weibull distribution to
determine the time between symptomatic presentation and untreated AIDS-related death. The duration
is subtracted from the AIDS-related death date(described in the previous scenario) to determine the time
of symptomatic presentation. Small values lead to symptomatic presentation close to the time of
AIDS-related death. Large values lead to symptomatic presentation well before AIDS-related death. If the
drawn time is longer than the total survival time, then the total survival time isused (i.e.,symptomatic
presentation occurs immediately upon infection). The date of symptomatic presentation has no direct
impact on clinical progression; however, it can be used to configure health-seeking behavior

Data to test: StdOut debug logging.

main test: test symptomatic to death durations counter is drawn from the expected weibull distribution. The simulation 
runs with 15000+ population for only 2 days to collect the symptomatic duration counter from debug logging. 

Parameters to test:
            "Days_Between_Symptomatic_And_Death_Weibull_Heterogeneity"
            "Days_Between_Symptomatic_And_Death_Weibull_Scale"
            
Output: scientific_feature_report.txt
        Duration(StdOut).png
        symptomatic_to_death_durations.png
        weibull_cdf.png
        
"""


filter_string_list = ["Update(): Time:",
                      "days_between_sympotmatic_and_death"]

load_df_param = [[hiv_s.Constant.total_duration, hiv_s.Constant.symptomatic_duration],
                 ["days_until_death_by_HIV=", "days_between_sympotmatic_and_death="], [SearchType.VAL, SearchType.VAL]]


def create_report_file(duration_df, total_pop, config_filename, report_name):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        # load parameters to test from config
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")

        sym_to_death_heterogeneity, sym_to_death_scale, base_year = dtk_sft.get_config_parameter(config_filename,
                                                                                 [hiv_s.ConfigParam.Sym_to_Death_H,
                                                                                  hiv_s.ConfigParam.Sym_to_Death_S,
                                                                                  hiv_s.ConfigParam.base_year])
        output_report_file.write(f"{hiv_s.ConfigParam.Sym_to_Death_H} = {sym_to_death_heterogeneity}\n")
        output_report_file.write(f"{hiv_s.ConfigParam.Sym_to_Death_S} = {sym_to_death_scale}\n")

        # calculate the latent_duration from stdout file
        duration_df[hiv_s.Constant.latent_duration] = duration_df[hiv_s.Constant.total_duration] - \
                                                duration_df[hiv_s.Constant.symptomatic_duration]
        # If too many individuals have a total duration less than the duration draw from the weibull distribution under
        # test, the actual symptomatic to death duration is replace by the total duration. It doesn't matter in this
        # test since we are testing the symptomatic to death counters directly.
        non_positive_count = sum(n <= 0 for n in duration_df[hiv_s.Constant.latent_duration].values.tolist())
        if non_positive_count:
            output_report_file.write(f"{non_positive_count} out of {total_pop} individuals have symptomatic "
                                     f"presentation occur immediately upon infection.\n")

        # main test
        symptomatic_to_death_durations = duration_df[hiv_s.Constant.symptomatic_duration].values.tolist()
        #symptomatic_to_death_durations = [x * dtk_sft.DAYS_IN_YEAR for x in symptomatic_to_death_durations]
        output_report_file.write(f"Test data point size = {len(symptomatic_to_death_durations)}.\n")
        result = dtk_sft.test_weibull(symptomatic_to_death_durations, sym_to_death_scale, 1.0/sym_to_death_heterogeneity,
                                      report_file=output_report_file, integer=True)
        if not result:
            succeed = False

        # plotting
        dist_weibull_scipy = stats.weibull_min.rvs(c=1.0/sym_to_death_heterogeneity, loc=0, scale=sym_to_death_scale,
                                                   size=len(symptomatic_to_death_durations))
        # dist_weibull_scipy = [math.ceil(x) for x in dist_weibull_scipy]
        # plot with standard scipy weibull distribution
        dtk_sft.plot_data(symptomatic_to_death_durations, dist_weibull_scipy,
                          sort=True, category="symptomatic_to_death_durations", overlap=True,
                          label1="symptomatic_to_death_durations",label2="weibull from scipy",
                          xlabel="Data Points", ylabel="Duration",
                          title="symptomatic_to_death_durations")

        # plot with cdf function
        dtk_sft.plot_cdf_w_fun(symptomatic_to_death_durations, name="weibull_cdf", cdf_function=stats.weibull_min.cdf,
                               args=(1.0/sym_to_death_heterogeneity, 0, sym_to_death_scale), show=True)

        # plot all type durations in the test
        hiv_s.plot_durations(duration_df)

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

