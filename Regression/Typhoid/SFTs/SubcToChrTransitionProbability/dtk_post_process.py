#!/usr/bin/python
# This SFT test the following statements:
# All infections begin in prepatent.
# The proportion of individuals who move to acute infections is determined by the config parameter Config:TSF.
# The remainder shall move to subclinical.
# All new acute cases and subclinical cases are transited from prepatent state only.


import json
import math
import dtk_test.dtk_sft as sft
import numpy as np


def application(report_file):
    sft.wait_for_done()
    # pdb.set_trace()
    # print( "Post-processing: " + report_file )

    cdj = json.loads(open("config.json").read())["parameters"]
    start_time = cdj["Start_Time"]
    timestep = start_time
    stat_pop = None
    chronic_list = []
    recovered_list = []
    with open("test.txt") as logfile:
        for line in logfile:
            if "Update(): Time:" in line:
                # calculate time step
                timestep += 1
                if stat_pop is None:
                    stat_pop = sft.get_val("StatPop: ", line)
            elif "just went chronic" in line and ("from subclinical" in line or "from Subclinical" in line):
                # append time step and all Infection stage transition to list
                age = float(sft.get_val(" age ", line))
                sex = "female" if ("sex 1" in line) or ("sex Female" in line) else "male"
                chronic_list.append((age, sex))
            elif "just recovered" in line and ("from subclinical" in line or "from Subclinical" in line):
                # append time step and all Infection stage transition to list
                age = float(sft.get_val(" age ", line))
                sex = "female" if ("sex 1" in line) or ("sex Female" in line) else "male"
                recovered_list.append((age, sex))

    # 4*10 list to store the count for cases [0][]: Chr_male, [1][]: Chr_female, [2][]: Sus_male. [3][]: Sus_female
    count = [[0]*9 for _ in range(4)]

    tcpm = cdj["Typhoid_Carrier_Probability_Male"]
    tcpf = cdj["Typhoid_Carrier_Probability_Female"]

    gpag_male = [0.0, 0.0, 0.045, 0.134, 0.167, 0.198, 0.247, 0.435, 0.4]
    gpag_female = [0.0, 0.097, 0.234, 0.431, 0.517, 0.60, 0.692, 0.692, 0.555]

    success = True
    with open(sft.sft_output_filename, "w") as report_file:
        if not len(chronic_list) or not len(recovered_list):
            success = False
            report_file.write(sft.sft_no_test_data)
        else:
            for age, sex in chronic_list:
                # to Chronic
                #  python 2.7 the (int / int) operator is integer division
                i = int(age) // 10
                # for age > 80, put them into the last age group
                if i > 8:
                    i = 8
                if sex == "male":
                    count[0][i] += 1
                else:
                    count[1][i] += 1
            for age, sex in recovered_list:
                # to Susceptible
                # python 2.7 the (int / int) operator is integer division
                i = int(age) // 10
                # for age > 80, put them into the last age group
                if i > 8:
                    i = 8
                if sex == "male":
                    count[2][i] += 1
                else:
                    count[3][i] += 1

            # calculate theoretic probability of becoming a Chronic carrier in two 1*9 lists
            theoretic_p_male = [x*tcpm for x in gpag_male]
            theoretic_p_female = [x*tcpf for x in gpag_female]
            # calculate actual probability of becoming a Chronic carrier in four 1*9 lists
            actual_p_male = [x/float(x+y) if (x+y) != 0 else -1 for x, y in zip(count[0], count[2])]
            actual_p_female = [x/float(x+y) if (x+y) != 0 else -1 for x, y in zip(count[1],count[3])]
            # calculate tolerance from theoretic and actual probabilities and store then in two 1*9 list
            # tolerance_male = [math.fabs(x - y) / y if y != 0 else x for x, y in zip(theoretic_p_male, actual_p_male)]
            # tolerance_female = [math.fabs(x - y) / y if y != 0 else x for x, y in zip(theoretic_p_female,
            # actual_p_female)]
            age = ["0-9", "10-19", "20-29", "30-39", "40-49", "50-59", "60-69", "70-79", "80+"]

            for x in range(len(age)):
                # calculate these counts for error logging
                actual_chr_count_male = count[0][x]
                actual_count_male = count[0][x] + count[2][x]
                actual_chr_count_female = count[1][x]
                actual_count_female = count[1][x] + count[3][x]
                # calculate the mean and standard deviation for binormal distribution
                sd_male = math.sqrt(theoretic_p_male[x] * (1 - theoretic_p_male[x]) * actual_count_male)
                mean_male = actual_count_male * theoretic_p_male[x]
                sd_female = math.sqrt(theoretic_p_female[x] * (1 - theoretic_p_female[x]) * actual_count_female)
                mean_female = actual_count_female * theoretic_p_female[x]
                # 99.73% confidence interval
                lower_bound_male = mean_male - 3 * sd_male
                upper_bound_male = mean_male + 3 * sd_male
                lower_bound_female = mean_female - 3 * sd_female
                upper_bound_female = mean_female + 3 * sd_female
                # Male
                sex = "male"
                if not actual_count_male:
                    success = False
                    report_file.write("Found no male in age group {0} went to Chronic state or was recovered from "
                                      "SubClinical state.\n".format(age[x]))
                elif mean_male != 0 and (mean_male <= 5 or actual_count_male - mean_male <= 5):
                    success = False
                    report_file.write("There is not enough sample size in age group {0}, sex {1}: mean = {2}, sample "
                                      "size - mean = {3}.\n".format( age[x], sex, mean_male,
                                                                     actual_count_male - mean_male))
                elif actual_chr_count_male < lower_bound_male or actual_chr_count_male > upper_bound_male:
                    success = False
                    report_file.write("BAD: The probability of becoming a chronic carrier from SubClinical stage for "
                                      "individual age group {0}, sex {1} is {2}, expected {3}. The {1} Chronic cases is"
                                      " {4}, expected 99.73% confidence interval ( {5}, {6})."
                                      "\n".format(age[x], sex, actual_p_male[x], theoretic_p_male[x],
                                                  actual_chr_count_male, lower_bound_male, upper_bound_male))
                else:
                    report_file.write("Sample size in age group {0}, sex {1}: mean = {2}, actual = {3}, "
                                      "sample size = {4}.\n".format(age[x], sex, mean_male,
                                                                    actual_chr_count_male, actual_count_male))
                # Female
                sex = "female"
                if not actual_count_female:
                    success = False
                    report_file.write("Found no female in age group {0} went to Chronic state or was recovered from "
                                      "SubClinical state.\n".format(age[x]))
                elif mean_female != 0 and (mean_female <= 5 or actual_count_female - mean_female <= 5):
                    success = False
                    report_file.write(
                        "There is not enough sample size in age group {0}, sex {1}: mean = {2}, sample size - mean"
                        " = {3}.\n".format(age[x], sex, mean_female, actual_count_female - mean_female))
                elif actual_chr_count_female < lower_bound_female or actual_chr_count_female > upper_bound_female:
                    success = False
                    report_file.write(
                        "BAD: The probability of becoming a chronic carrier from SubClinical stage for individual age "
                        "group {0}, sex {1} is {2}, expected {3}. The {1} Chronic cases is {4}, expected 99.73% "
                        "confidence interval ( {5}, {6}).\n".format(
                            age[x], sex, actual_p_female[x], theoretic_p_female[x], actual_chr_count_female,
                            lower_bound_female, upper_bound_female))
                else:
                    report_file.write("Sample size in age group {0}, sex {1}: mean = {2}, actual = {3}, "
                                      "sample size = {4}.\n".format(age[x], sex, mean_female,
                                                                    actual_chr_count_female, actual_count_female))

            xticks = np.arange(len(age))
            sft.plot_data(actual_p_male, theoretic_p_male,
                          label1="Actual Probability",
                          label2="Expected Probability",
                          title="Subclinical to Chronic Probability Male\n"
                                "Total population = {0}, TCPM = {1}".format(stat_pop, tcpm),
                          xlabel="Age bins",
                          ylabel="Subclinical -> Chronic Probability",
                          category='sublinical_to_chronic_male',
                          overlap=True, alpha=0.5,
                          xticks=xticks, xtickslabel=age)

            sft.plot_data(actual_p_female, theoretic_p_female,
                          label1="Actual Probability",
                          label2="Expected Probability",
                          title="Subclinical to Chronic Probability Female\n"
                                "Total population = {0}, TCPF = {1}".format(stat_pop, tcpf),
                          xlabel="Age bins",
                          ylabel="Subclinical -> Chronic Probability",
                          category='sublinical_to_chronic_female',
                          overlap=True, alpha=0.5,
                          xticks=xticks, xtickslabel=age)

        report_file.write(sft.format_success_msg(success))


if __name__ == "__main__":
    # execute only if run as a script
    application("")
