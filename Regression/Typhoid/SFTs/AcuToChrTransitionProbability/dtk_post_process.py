#!/usr/bin/python
# This SFT test the following statements:
# All Acute infections go to Chronic or Susceptible state.
# The proportion of individuals who move to chronic infections is determined by the config parameter Config:CPG multiply by hardcoded Gallstones table. The remainder shall move to Susceptible.
# This test passes when the number of went to Chronic cases is within binomial 95% confidence interval
# for each test, there is 5% of chance that we will reject the hypothesis while it's true

import re
import json
import math
import dtk_test.dtk_sft as sft
import numpy as np

def get_val( key, line ):
    regex = key + "(\d*\.*\d*)"
    match = re.search(regex, line)
    if match != None:
        return match.group(1)
    else:
        raise LookupError


def application( report_file ):
    #pdb.set_trace()
    #print( "Post-processing: " + report_file )

    sft.wait_for_done()
    cdj = json.loads( open( "config.json" ).read() )["parameters"]
    start_time=cdj["Start_Time"]
    stat_pop = None
    timestep=start_time
    chronic_list = []
    recovered_list = []
    with open( sft.sft_test_filename ) as logfile:
        for line in logfile:
            if re.search("Update\(\): Time:",line):
                #calculate time step
                timestep+=1
                if stat_pop is None:
                    stat_pop = sft.get_val("StatPop: ", line)
            if re.search( "just went chronic", line) and re.search("from acute", line) :
                #append time step and all Infection stage transition to list
                age = float(sft.get_val(" age ", line))
                sex = "female" if ("sex 1" in line) or ("sex Female" in line) else "male"
                chronic_list.append((age, sex))
            if re.search("just recovered", line) and re.search("from acute", line) :
                # append time step and all Infection stage transition to list
                age = float(sft.get_val(" age ", line))
                sex = "female" if ("sex 1" in line) or ("sex Female" in line) else "male"
                recovered_list.append((age, sex))

    # 4*10 list to store the count for cases [0][]: Chr_male, [1][]: Chr_female, [2][]: Sus_male. [3][]: Sus_female
    count=[[0]*9 for _ in range(4)]

    tcpm=cdj["Typhoid_Carrier_Probability_Male"]
    tcpf=cdj["Typhoid_Carrier_Probability_Female"]

    gpag_male=[0.0, 0.0, 0.045, 0.134, 0.167, 0.198, 0.247, 0.435, 0.4]
    gpag_female=[0.0, 0.097, 0.234, 0.431, 0.517, 0.60, 0.692, 0.692, 0.555]

    success = True
    with open( sft.sft_output_filename, "w" ) as report_file:
        if not len(chronic_list) or not len(recovered_list):
            success = False
            report_file.write(sft.sft_no_test_data)
        else:
            for age, sex in chronic_list:
                # to Chronic
                #  python 2.7 the (int / int) operator is integer division
                i = int(age) // 10
                # for age > 80, put them into the last age group
                if i > 8: i = 8
                if sex=="male":
                    count[0][i] += 1
                else:
                    count[1][i] += 1
            for age, sex in recovered_list:
                # to Susceptible
                # python 2.7 the (int / int) operator is integer division
                i = int(age) // 10
                # for age > 80, put them into the last age group
                if i > 8: i = 8
                if sex == "male":
                    count[2][i] += 1
                else:
                    count[3][i] += 1
            # calculate theoretic probability of becoming a Chronic carrier in two 1*9 lists
            theoretic_p_male=[x*tcpm for x in gpag_male]
            theoretic_p_female=[x*tcpf for x in gpag_female]
            # calculate actual probability of becoming a Chronic carrier in two 1*9 lists
            actual_p_male=[x/float(x+y) if (x+y)!=0 else -1 for x, y in zip(count[0],count[2])]
            actual_p_female=[x/float(x+y) if (x+y)!=0 else -1 for x, y in zip(count[1],count[3])]

            age = ["0-9", "10-19", "20-29", "30-39", "40-49", "50-59", "60-69", "70-79", "80+"]

            for x in range(len(age)):
                # calculate the total chronic cases and sample sizes for Male and Female
                actual_chr_count_male = count[0][x]
                actual_count_male = count[0][x] + count[2][x]
                actual_chr_count_female = count[1][x]
                actual_count_female = count[1][x] + count[3][x]
                # Male
                category='sex: Male, age: ' + age[x]
                if actual_count_male== 0:
                    success=False
                    report_file.write("Found no male in age group {0} went to Chronic state or was recovered from Acute state.\n".format(age[x]))
                elif  theoretic_p_male[x] < 5e-2 or theoretic_p_male[x] >0.95:
                    # for cases that binomial confidence interval will not work: prob close to 0 or 1
                    if math.fabs( actual_p_male[x] - theoretic_p_male[x])>5e-2 :
                        success = False
                        report_file.write("BAD: Proportion of {0} Acute cases that become Chronic is {1}, expected {2}.\n".format(category,actual_p_male[x], theoretic_p_male[x]))
                elif not sft.test_binomial_95ci(actual_chr_count_male,actual_count_male, theoretic_p_male[x],report_file, category):
                    success = False

                #Female
                category = 'sex: Female, age: ' + age[x]
                if actual_count_female == 0:
                    success = False
                    report_file.write("Found no female in age group {0} went to Chronic state or was recovered from Acute state.\n".format(age[x]))
                elif  theoretic_p_female[x] < 5e-2 or theoretic_p_female[x] >0.95:
                    # for cases that binomial confidence interval will not work: prob close to 0 or 1
                    if math.fabs( actual_p_female[x] - theoretic_p_female[x])>5e-2 :
                        success = False
                        report_file.write("BAD: Proportion of {0} Acute cases that become Chronic is {1}, expected {2}.\n".format(category,actual_p_female[x], theoretic_p_female[x]))
                elif not sft.test_binomial_95ci(actual_chr_count_female, actual_count_female, theoretic_p_female[x], report_file,category):
                    success = False

            xticks = np.arange(len(age))
            sft.plot_data(actual_p_male, theoretic_p_male,
                          label1="Actual Probability",
                          label2="Expected Probability",
                          title="Acute to Chronic Probability Male\n"
                                "Total population = {0}, TCPM = {1}".format(stat_pop, tcpm),
                          xlabel="Age bins",
                          ylabel="Acute -> Chronic Probability",
                          category='acute_to_chronic_male',
                          overlap=True, alpha=0.5,
                          xticks=xticks, xtickslabel=age)

            sft.plot_data(actual_p_female, theoretic_p_female,
                          label1="Actual Probability",
                          label2="Expected Probability",
                          title="Acute to Chronic Probability Female\n"
                                "Total population = {0}, TCPF = {1}".format(stat_pop, tcpf),
                          xlabel="Age bins",
                          ylabel="Acute -> Chronic Probability",
                          category='acute_to_chronic_female',
                          overlap=True, alpha=0.5,
                          xticks=xticks, xtickslabel=age)
        if success:
            report_file.write( sft.format_success_msg(success))


if __name__ == "__main__":
    # execute only if run as a script
    application( "" )
