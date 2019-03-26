#!/usr/bin/python
# This SFT test the following statements:
# 2. Population: Number of Individuals in the population at specified Age and Gender at reporting time.
# 4. Carrier (prev) represents the number of existing chronic carriers present at the time of reporting.
# note: Prevalence. We calculate prevalence based on infected status at the last timestep of the reporting period.
#       Population. The population (of an age bin) is also calculated at the last timestep of the reporting period.
#  This of course means that the incidence ages will not necessarily match the populations.


import re
import json
import dtk_test.dtk_sft as sft
import csv
import numpy as np


def application(report_file, debug=False):
    # pdb.set_trace()
    # print( "Post-processing: " + report_file )
    sft.wait_for_done()

    cdj = json.loads(open("config.json").read())["parameters"]
    start_time = cdj["Start_Time"]
    base_year = cdj["Base_Year"]
    timestep = start_time
    lines = []
    Population = []
    Chr_prev = []
    Sub_prev = []
    Acu_prev = []
    Pre_prev = []
    dict_id_age = {}
    dict_id_sex = {}
    with open("test.txt") as logfile:
        for line in logfile:
            if re.search("Update\(\): Time:", line):
                # calculate time step
                timestep += 1
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            if re.search("state_to_report", line):
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            if re.search("Created human", line):
                ind_id = sft.get_val("Created human ", line)
                # human created at timestep 0, so age at time step 1 will need be age+1
                age = float(sft.get_val("age=", line)) - timestep + 1
                sex = sft.get_char("sex=", line)
                sex = 1 if sex == 'Female' else 0
                if ind_id not in dict_id_age:
                    dict_id_age[ind_id] = age
                if ind_id not in dict_id_sex:
                    dict_id_sex[ind_id] = sex

    success = True
    if debug:
        with open('filtered_lines.txt', 'w') as outfile:
            for line in lines:
                outfile.write(line)
        with open('id_age_sex.txt', 'w') as outfile:
            for key in dict_id_age:
                outfile.write("ID: {0}, Age: {1}, Sex: {2}\n".format(key, dict_id_age[key]/365.0, dict_id_sex[key]))

    with open(sft.sft_output_filename, "w") as report_file:
        with open("output/ReportTyphoidByAgeAndGender.csv") as csvfile:
            # may want to sort the csv before processing
            reader = csv.DictReader(csvfile)
            for row in reader:
                population = int(row[' Population'])
                chr_prev = int(row[' Chronic (Prev)'])
                sub_prev = int(row[' Sub-Clinical (Prev)'])
                acu_prev = int(row[' Acute (Prev)'])
                pre_prev = int(row[' Pre-Patent (Prev)'])
                Population.append(population)
                Chr_prev.append(chr_prev)
                Sub_prev.append(sub_prev)
                Acu_prev.append(acu_prev)
                Pre_prev.append(pre_prev)

            sum_Chr_prev = 0
            sum_Sub_prev = 0
            sum_Acu_prev = 0
            sum_Pre_prev = 0
            for i in range(0, len(Chr_prev)):
                sum_Chr_prev += Chr_prev[i]
                sum_Sub_prev += Sub_prev[i]
                sum_Acu_prev += Acu_prev[i]
                sum_Pre_prev += Pre_prev[i]
            if sum_Chr_prev == 0:
                success = False
                report_file.write("Bad: Found no Chronic prevalence.\n")
            if sum_Sub_prev == 0:
                success = False
                report_file.write("Bad: Found no SubClinical prevalence.\n")
            if sum_Acu_prev == 0:
                success = False
                report_file.write("Bad: Found no Acute prevalence.\n")
            if sum_Pre_prev == 0:
                success = False
                report_file.write("Bad: Found no Prepatent prevalence.\n")

            # initialize 100*2 arrays
            pre_infected_count = [[0 for i in range(100)] for j in range(2)]
            pre_acute_count = [[0 for i in range(100)] for j in range(2)]
            pre_subc_count = [[0 for i in range(100)] for j in range(2)]
            pre_chr_count = [[0 for i in range(100)] for j in range(2)]
            population_count = [[0 for i in range(100)] for j in range(2)]

            pre_infected_count_debug = []
            pre_acute_count_debug = []
            pre_subc_count_debug = []
            pre_chr_count_debug = []
            Pre_prev_debug = []
            Acu_prev_debug = []
            Sub_prev_debug = []
            Chr_prev_debug = []
            for line in lines:
                timestep = int(sft.get_val("TimeStep: ", line))
                time_year = timestep/365 + base_year - 1
                index = 0
                if (timestep+1) % 365 == 0:
                    # collect logging info for Prevalence at the last day of year
                    if re.search("state_to_report", line):
                        ind_id = sft.get_val("individual ", line)
                        # get age and calculate current age in year
                        age = dict_id_age[ind_id] + timestep
                        age_year = int(age/365.0)
                        if age_year > 99:
                            age_year = 99
                        sex = dict_id_sex[ind_id]
                        if re.search("PRE", line):
                            pre_infected_count[int(sex)][age_year] += 1
                        elif re.search("SUB", line):
                            pre_subc_count[int(sex)][age_year] += 1
                        elif re.search("ACU", line):
                            pre_acute_count[int(sex)][age_year] += 1
                        elif re.search("CHR", line):
                            pre_chr_count[int(sex)][age_year] += 1
                elif re.search("Update\(\): Time:", line) and (timestep) % 365 == 0:
                    # cross check logging and report at the end of last day of year
                    # recalculate index for a new year
                    index += int((timestep / 365 - 1) * 200)
                    for key in dict_id_age:
                        # get age and calculate current age in year
                        age = dict_id_age.get(key) + timestep - 1
                        age_year = int(age/365.0)
                        sex = dict_id_sex.get(key)
                        # list of population for age and sex buckets
                        population_count[sex][age_year] += 1
                    for i in range(2):
                        for j in range(100):
                            if pre_infected_count[i][j] != Pre_prev[index]:
                                success = False
                                report_file.write("BAD:In year {0} time {5}, Sex {1}, Age {2}  Pre-Patent (Prev) in "
                                                  "ReportTyphoidByAgeAndGender.csv is {3}, expected {4} based on "
                                                  "test.txt.\n".format(time_year, i, j, Pre_prev[index],
                                                                       pre_infected_count[i][j], timestep-1))
                            if pre_acute_count[i][j] != Acu_prev[index]:
                                success = False
                                report_file.write("BAD:In year {0} time {5}, Sex {1}, Age {2}  Acute (Prev) in "
                                                  "ReportTyphoidByAgeAndGender.csv is {3}, expected {4} based on "
                                                  "test.txt.\n".format(time_year, i, j, Acu_prev[index],
                                                                       pre_acute_count[i][j], timestep-1))
                            if pre_subc_count[i][j] != Sub_prev[index]:
                                success = False
                                report_file.write("BAD:In year {0} time {5}, Sex {1}, Age {2}   Sub-Clinical (Prev) in "
                                                  "ReportTyphoidByAgeAndGender.csv is {3}, expected {4} based on "
                                                  "test.txt.\n".format(time_year, i, j, Sub_prev[index],
                                                                       pre_subc_count[i][j], timestep-1))
                            if pre_chr_count[i][j] != Chr_prev[index]:
                                success = False
                                report_file.write("BAD:In year {0} time {5}, Sex {1}, Age {2}   Chronic (Prev) in "
                                                  "ReportTyphoidByAgeAndGender.csv is {3}, expected {4} based on "
                                                  "test.txt.\n".format(time_year, i, j, Chr_prev[index],
                                                                       pre_chr_count[i][j], timestep-1))
                            if population_count[i][j] != Population[index]:
                                success = False
                                report_file.write("BAD:In year {0} time {5}, Sex {1}, Age {2}  Population in "
                                                  "ReportTyphoidByAgeAndGender.csv is {3}, expected {4} based on "
                                                  "test.txt.\n".format(time_year, i, j, Population[index],
                                                                       population_count[i][j], timestep-1))

                            pre_infected_count_debug.append(pre_infected_count[i][j])
                            pre_acute_count_debug.append(pre_acute_count[i][j])
                            pre_subc_count_debug.append(pre_subc_count[i][j])
                            pre_chr_count_debug.append(pre_chr_count[i][j])
                            Pre_prev_debug.append(Pre_prev[index])
                            Acu_prev_debug.append(Acu_prev[index])
                            Sub_prev_debug.append(Sub_prev[index])
                            Chr_prev_debug.append(Chr_prev[index])
                            # next row
                            index += 1

                    # reset counters(2*100 arrays) at the first day of year
                    pre_infected_count = [[0 for i in range(100)] for j in range(2)]
                    pre_acute_count = [[0 for i in range(100)] for j in range(2)]
                    pre_subc_count = [[0 for i in range(100)] for j in range(2)]
                    pre_chr_count = [[0 for i in range(100)] for j in range(2)]
                    population_count = [[0 for i in range(100)] for j in range(2)]

        num_age_bins = 100
        xticks = np.arange(num_age_bins, len(pre_infected_count_debug) + 1, num_age_bins)
        xtickslabel = []
        gender = 'male'
        for i in range(len(xticks)):
            xtickslabel.append("{0}_Year {1}".format(gender, (i + 2) // 2))
            gender = 'female' if gender == 'male' else 'male'
        sft.plot_data(pre_infected_count_debug, Pre_prev_debug,
                      label1="Prepatent prevalence from stdout",
                      label2="Prepatent prevalence from Report",
                      title="TyphoidByAgeAndGender Prepatent Prevalence", xlabel="Gender-Age-Year bins",
                      ylabel="Prevalence",
                      category='report_typhoidbyageandgender_prepatent_prevalence',
                      alpha=0.5, overlap=True,
                      xticks=xticks, xtickslabel=xtickslabel)
        sft.plot_data(pre_acute_count_debug, Acu_prev_debug,
                      label1="Acute prevalence from stdout",
                      label2="Acute prevalence from Report",
                      title="TyphoidByAgeAndGender Acute Prevalence", xlabel="Gender-Age-Year bins",
                      ylabel="Prevalence",
                      category='report_typhoidbyageandgender_acute_prevalence',
                      alpha=0.5, overlap=True,
                      xticks=xticks, xtickslabel=xtickslabel)
        sft.plot_data(pre_subc_count_debug, Sub_prev_debug,
                      label1="SubClinical prevalence from stdout",
                      label2="SubClinical prevalence from Report",
                      title="TyphoidByAgeAndGender Subclinical Prevalence",
                      xlabel="Gender-Age-Year bins",
                      ylabel="Prevalence",
                      category='report_typhoidbyageandgender_subclinical_prevalence',
                      alpha=0.5, overlap=True,
                      xticks=xticks, xtickslabel=xtickslabel)
        sft.plot_data(pre_chr_count_debug, Chr_prev_debug,
                      label1="Chronic prevalence from stdout",
                      label2="Chronic prevalence from Report",
                      title="TyphoidByAgeAndGender Chronic Prevalence", xlabel="Gender-Age-Year bins",
                      ylabel="Prevalence",
                      category='report_typhoidbyageandgender_chronic_prevalence',
                      alpha=0.5, overlap=True,
                      xticks=xticks, xtickslabel=xtickslabel)

        report_file.write(sft.format_success_msg(success))


if __name__ == "__main__":
    # execute only if run as a script
    application("")
