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
This test is testing the HIV Infection to death duration. Test 3: testing the adult, Weibull scale is a function of age.

For Adult, survival time with untreated HIV infection depends on the age of the individual at the time of infection. 
At infection, survival time is drawn from a Weibull distribution with shape parameter κ and scale parameter λ. The 
scale parameter λ is allowed to vary linearly with age as follows:
λ = HIV_Adult_Survival_Scale_Parameter_Intercept + HIV_Adult_Survival_Scale_Parameter_Slope *Age (in years). 
Because survival time with HIV becomes shorter with increasing age,HIV_Adult_Survival_Scale_Parameter_Slope should be 
set to a negative number.

Data to test: StdOut debug logging.

main test: test infection to death durations counter for adult is drawn from the expected weibull distribution. The 
simulation runs with 15000+ population for only 2 days to collect the survival duration counter from debug logging. 
The population is divided into several age group and durations are grouped by age and tested with corresponding scale 
value. For each age group, the theoretical data is generated based on the Weibull cdf function. The test is using AD k 
sample method to test the survival duration counter with theoretical data.

Parameters to test:
            "HIV_Adult_Survival_Scale_Parameter_Intercept"
            "HIV_Adult_Survival_Scale_Parameter_Slope"
            "HIV_Adult_Survival_Shape_Parameter"
            "HIV_Age_Max_for_Adult_Age_Dependent_Survival"
            "HIV_Age_Max_for_Child_Survival_Function"
            
Output: scientific_feature_report.txt
        infection_to_death_durations_Age-1.0.png
        infection_to_death_durations_Age-20.0.png
        infection_to_death_durations_Age-40.0.png
        infection_to_death_durations_Age-60.0.png
        infection_to_death_durations_Age-80.0.png
        weibull_cdf_Age-1.0.png
        weibull_cdf_Age-20.0.png
        weibull_cdf_Age-40.0.png
        weibull_cdf_Age-60.0.png
        weibull_cdf_Age-80.0.png
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
        adult_intercept, adult_slope, adult_shape, max_adult_age, max_child_age = \
            dtk_sft.get_config_parameter(config_filename,
                                         [hiv_s.ConfigParam.adult_intercept,
                                          hiv_s.ConfigParam.adult_slope,
                                          hiv_s.ConfigParam.adult_shape,
                                          hiv_s.ConfigParam.max_adult_age,
                                          hiv_s.ConfigParam.max_child_age])

        output_report_file.write(f"{hiv_s.ConfigParam.adult_intercept} = {adult_intercept}.\n")
        output_report_file.write(f"{hiv_s.ConfigParam.adult_slope} = {adult_slope}.\n")
        output_report_file.write(f"{hiv_s.ConfigParam.adult_shape} = {adult_shape}.\n")
        output_report_file.write(f"{hiv_s.ConfigParam.max_adult_age} = {max_adult_age}.\n")

        if max_child_age > 0:
            succeed = False
            output_report_file.write(f"BAD: {hiv_s.ConfigParam.max_child_age} must be 0 in this test, please check the "
                                     f"test.\n")
        else:
            output_report_file.write(f"{hiv_s.ConfigParam.max_child_age} = {max_child_age}, so all individuals will be"
                                     f" treated as adult progressor in this test.\n")

        # Main test
        # Get infection to death counter for all children.
        duration_df.sort_values(by=[hiv_s.Constant.age], inplace=True)
        output_report_file.write("Running test on durations grouped by age:\n")
        age_groups = sorted(duration_df[hiv_s.Constant.age].unique())
        if len(age_groups) > 10:
            output_report_file.write("WARNING: too many age groups in this test. There are may not be enough population"
                                     "in each age group for test.\n")
        test_count = fail_count = 0
        for age in age_groups:
            infection_to_death_durations = duration_df[
                duration_df[hiv_s.Constant.age] == age][
                hiv_s.Constant.total_duration].values.tolist()
            infection_to_death_durations = [x / dtk_sft.DAYS_IN_YEAR for x in infection_to_death_durations]
            output_report_file.write(f"\t{len(infection_to_death_durations)} {age}-years-old 'adults' out of "
                                     f"{total_pop} individuals.\n")
            if age < max_adult_age:
                scale = adult_intercept + adult_slope * age
                output_report_file.write(
                    f"\tAge = {age} is less than {max_adult_age}, calculating scale using: "
                    f"intercept({adult_intercept}) + slope({adult_slope})*age({age}) = {scale}.\n")
            else:
                scale = adult_intercept + adult_slope * max_adult_age
                output_report_file.write(
                    f"\tAge = {age} is larger than {max_adult_age}, calculating scale using: "
                    f"intercept({adult_intercept}) + slope({adult_slope})*max_adult_age({max_adult_age}) = {scale}.\n")

            output_report_file.write(f"\tTest data point size = {len(infection_to_death_durations)} for age = {age}, "
                                     f"scale = {scale} and shape = {adult_shape}.\n")

            # Test if the durations under test are drawn from a standard Weibull distribution
            output_report_file.write("\t")
            result = dtk_sft.test_weibull(infection_to_death_durations, scale, adult_shape,
                                          report_file=output_report_file, integer=False, round=False)
            test_count += 1
            if not result:
                fail_count += 1

            output_report_file.write('\tPlot test data with scipy data and cdf.\n\n')
            # Plot with standard scipy weibull distribution
            dist_weibull_scipy = stats.weibull_min.rvs(c=adult_shape, loc=0, scale=scale,
                                                       size=len(infection_to_death_durations))
            dtk_sft.plot_data(infection_to_death_durations, dist_weibull_scipy,
                              sort=True, category=f"infection_to_death_durations_Age-{age}", overlap=True,
                              label1="infection_to_death_durations",label2="weibull from scipy",
                              xlabel="Data Points", ylabel="Duration(Years)",
                              title=f"infection_to_death_durations_Age-{age}")

            # Plot with cdf function
            dtk_sft.plot_cdf_w_fun(infection_to_death_durations, name=f"weibull_cdf_Age-{age}",
                                   cdf_function=stats.weibull_min.cdf,
                                   args=(adult_shape, 0, scale), show=True)

        if fail_count >= 2:
            succeed = False
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

