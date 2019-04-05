#!/usr/bin/python
# This SFT test the following statements:
# 1. Time of Report (Year): Reporting is accumulated on a yearly basis. This value represents the reporting time
# and the year that stats are accumulated over.
#    Example: A value of 1969.997 indicates that reporting is for the year 1969.
# 3. Pre-Patent (Inc), Chronic (Inc), Sub-Clinical (inc) and Acute (inc) represent new cases occurring
# over the year time period.
# note: Incidence. We calculate this all year for a given year of recording.
#       The age used is the snapshot age at the time of the infection event.
#       As a result, we could record a prepat event for someone age X and then if they had
#       their birthday before becoming acute, they would roll up in the next age bin for the acute incidence.

import json
import math
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
    simulation_duration = cdj["Simulation_Duration"]
    timestep = start_time
    lines = []
    year_of_report = []
    Chr_inc = []
    Sub_inc = []
    Acu_inc = []
    Pre_inc = []
    dict_id_age = {}
    dict_id_sex = {}
    with open("test.txt") as logfile:
        for line in logfile:
            if "Update(): Time:" in line:
                # calculate time step
                timestep += 1
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            elif "Infection stage transition" in line:
                # append time step and all Infection stage transition to list
                line = "TimeStep: "+str(timestep) + " " + line
                lines.append(line)
            elif "AcquireNewInfection" in line:
                # append time step and all new Infection to list
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            elif "went chronic" in line:
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            elif "Created human" in line:
                ind_id = sft.get_val("Created human ", line)
                # human created at timestep 0, so age at time step 1 will need be age+1
                age = float(sft.get_val("age=", line)) - timestep + 1
                sex = sft.get_char("sex=", line)
                sex = 1 if sex == 'Female' else 0
                if ind_id not in dict_id_age:
                    # dictionary for id and age
                    dict_id_age[ind_id] = age
                if ind_id not in dict_id_sex:
                    # dictionary for id and sex
                    dict_id_sex[ind_id] = sex

    if debug:
        with open('filtered_lines.txt',  'w') as outfile:
            for line in lines:
                outfile.write(line)
        with open('dict_id_age.txt', 'w') as outfile:
            for key in dict_id_age:
                outfile.write("Individual {}: age {}, sex: {} \n".format(key, dict_id_age[key]/365.0, dict_id_sex[key]))

    success = True
    with open(sft.sft_output_filename, "w") as report_file:
        with open("output/ReportTyphoidByAgeAndGender.csv") as csvfile:
            # may want to sort the csv before processing
            reader = csv.DictReader(csvfile)
            for row in reader:
                year = int(str(row['Time Of Report (Year)']).split('.')[0])
                chr_inc = int(row[' Chronic (Inc) '])
                sub_inc = int(row[' Sub-Clinical (Inc)'])
                acu_inc = int(row[' Acute (Inc)'])
                pre_inc = int(row[' Pre-Patent (Inc)'])
                year_of_report.append(year)
                Chr_inc.append(chr_inc)
                Sub_inc.append(sub_inc)
                Acu_inc.append(acu_inc)
                Pre_inc.append(pre_inc)

            sum_Chr_inc = 0
            sum_Sub_inc = 0
            sum_Acu_inc = 0
            sum_Pre_inc = 0
            for i in range(0, len(Chr_inc)):
                sum_Chr_inc += Chr_inc[i]
                sum_Sub_inc += Sub_inc[i]
                sum_Acu_inc += Acu_inc[i]
                sum_Pre_inc += Pre_inc[i]
            if sum_Chr_inc == 0:
                success = False
                report_file.write("Bad: Found no Chronic incidence.\n")
            if sum_Sub_inc == 0:
                success = False
                report_file.write("Bad: Found no SubClinical incidence.\n")
            if sum_Acu_inc == 0:
                success = False
                report_file.write("Bad: Found no Acute incidence.\n")
            if sum_Pre_inc == 0:
                success = False
                report_file.write("Bad: Found no Prepatent incidence.\n")

            simulation_duration_year = int(math.floor((simulation_duration+start_time)/365.0))
            if len(year_of_report) != simulation_duration_year * 200:
                success = False
                report_file.write("Bad: the report # of year is {0}, expected {1}."
                                  "\n".format(len(year_of_report)/200, simulation_duration_year))
            for i in range(0, len(year_of_report)):
                year = year_of_report[i]
                if i > 0 and i % 200 == 0:
                    base_year += 1
                if year != base_year:
                    success = False
                    report_file.write("Bad: at {0}th row, Time Of Report (Year) is {1}, expected {2}.\n".format(i, year,base_year))
            # initialize 100*2 arrays
            new_infected_count = [[0 for i in range(100)] for j in range(2)]
            new_acute_count = [[0 for i in range(100)] for j in range(2)]
            new_subc_count = [[0 for i in range(100)] for j in range(2)]
            new_chr_count = [[0 for i in range(100)] for j in range(2)]
            base_year = cdj["Base_Year"]

            new_infected_count_debug = []
            new_acute_count_debug = []
            new_subc_count_debug = []
            new_chr_count_debug = []
            Pre_inc_debug = []
            Acu_inc_debug = []
            Sub_inc_debug = []
            Chr_inc_debug = []
            for line in lines:
                timestep = int(sft.get_val("TimeStep: ", line))
                time_year = timestep/365 + base_year - 1
                index = 0
                if "Update(): Time:" in line and (timestep % 365 == 0):
                    # cross check logging and report at the last day of year
                    # recalculate index for a new year
                    index += int((timestep / 365 - 1) * 200)
                    for i in range(2):
                        for j in range(100):
                            if new_infected_count[i][j] != Pre_inc[index]:
                                success = False
                                report_file.write("BAD:In year {0} time {5}, Sex {1}, Age {2} Pre-Patent (Inc) in "
                                                  "ReportTyphoidByAgeAndGender.csv is {3}, expected {4} based on "
                                                  "test.txt.\n".format(time_year, i, j, Pre_inc[index],
                                                                       new_infected_count[i][j], timestep))
                            if new_acute_count[i][j] != Acu_inc[index]:
                                success = False
                                report_file.write("BAD:In year {0} time {5}, Sex {1}, Age {2} Acute (Inc) in"
                                                  " ReportTyphoidByAgeAndGender.csv is {3}, expected {4} based on "
                                                  "test.txt.\n".format(time_year, i, j, Acu_inc[index],
                                                                       new_acute_count[i][j], timestep))
                            if new_subc_count[i][j] != Sub_inc[index]:
                                success = False
                                report_file.write("BAD:In year {0} time {5}, Sex {1}, Age {2} Sub-Clinical (Inc) in "
                                                  "ReportTyphoidByAgeAndGender.csv is {3}, expected {4} based on "
                                                  "test.txt.\n".format(time_year, i, j, Sub_inc[index],
                                                                       new_subc_count[i][j], timestep))
                            if new_chr_count[i][j] != Chr_inc[index]:
                                success = False
                                report_file.write("BAD:In year {0} time {5}, Sex {1}, Age {2} Chronic (Inc) in"
                                                  " ReportTyphoidByAgeAndGender.csv is {3}, expected {4} based on "
                                                  "test.txt.\n".format(time_year, i, j, Chr_inc[index],
                                                                       new_chr_count[i][j], timestep))
                            new_infected_count_debug.append(new_infected_count[i][j])
                            new_acute_count_debug.append(new_acute_count[i][j])
                            new_subc_count_debug.append(new_subc_count[i][j])
                            new_chr_count_debug.append(new_chr_count[i][j])
                            Pre_inc_debug.append(Pre_inc[index])
                            Acu_inc_debug.append(Acu_inc[index])
                            Sub_inc_debug.append(Sub_inc[index])
                            Chr_inc_debug.append(Chr_inc[index])
                            index += 1

                    # reset counters(2*100 arrays) at the last day of year
                    new_infected_count = [[0 for i in range(100)] for j in range(2)]
                    new_acute_count = [[0 for i in range(100)] for j in range(2)]
                    new_subc_count = [[0 for i in range(100)] for j in range(2)]
                    new_chr_count = [[0 for i in range(100)] for j in range(2)]
                elif "AcquireNewInfection" in line:
                    ind_id = sft.get_val("individual=", line)
                    age = dict_id_age[ind_id] + timestep
                    age_year = int(round(age/365, 5))
                    if age_year > 99:
                        age_year = 99
                    sex = dict_id_sex[ind_id]
                    new_infected_count[int(sex)][age_year] += 1
                elif "->Acute" in line:
                    ind_id = sft.get_val("Individual=", line)
                    age = dict_id_age[ind_id] + timestep
                    age_year = int(round(age/365, 5))
                    if age_year > 99:
                        age_year = 99
                    sex = dict_id_sex[ind_id]
                    new_acute_count[int(sex)][age_year] += 1
                elif "->SubClinical" in line:
                    ind_id = sft.get_val("Individual=", line)
                    age = dict_id_age[ind_id] + timestep
                    age_year = int(round(age/365, 5))
                    if age_year > 99:
                        age_year = 99
                    sex = dict_id_sex[ind_id]
                    new_subc_count[int(sex)][age_year] += 1
                elif "went chronic" in line:
                    ind_id = sft.get_val("Individual ", line)
                    age = dict_id_age[ind_id] + timestep
                    age_year = int(round(age/365, 5))
                    if age_year > 99:
                        age_year = 99
                    sex = dict_id_sex[ind_id]
                    new_chr_count[int(sex)][age_year] += 1

        num_age_bins = 100
        xticks = np.arange(num_age_bins, len(new_acute_count_debug)+1, num_age_bins)
        xtickslabel = []
        gender = 'male'
        for i in range(len(xticks)):
            xtickslabel.append("{0}_Year {1}".format(gender, (i+2)//2))
            gender = 'female' if gender == 'male' else 'male'
        sft.plot_data(new_infected_count_debug, Pre_inc_debug,
                      label1="New Prepatent incidence from stdout",
                      label2="New Prepatent incidence from Report",
                      title="TyphoidByAgeAndGender New Prepatent Infections", xlabel="Gender-Age-Year bins",
                      ylabel="Number of incidences",
                      category='report_typhoidbyageandgender_prepatent_incidence',
                      alpha = 0.5, overlap = True,
                      xticks=xticks, xtickslabel=xtickslabel)
        sft.plot_data(new_acute_count_debug, Acu_inc_debug,
                      label1="New Acute incidence from stdout",
                      label2="New Acute incidence from Report",
                      title="TyphoidByAgeAndGender New Acute Infections", xlabel="Gender-Age-Year bins",
                      ylabel="Number of incidences",
                      category='report_typhoidbyageandgender_acute_incidence',
                      alpha = 0.5, overlap = True,
                      xticks=xticks, xtickslabel=xtickslabel)
        sft.plot_data(new_subc_count_debug, Sub_inc_debug,
                      label1="New SubClinical incidence from stdout",
                      label2="New SubClinical incidence from Report",
                      title="TyphoidByAgeAndGender SubClinical Infections", xlabel="Gender-Age-Year bins",
                      ylabel="Number of incidences",
                      category='report_typhoidbyageandgender_subcliical_incidence',
                      alpha = 0.5, overlap = True,
                      xticks=xticks, xtickslabel=xtickslabel)
        sft.plot_data(new_chr_count_debug, Chr_inc_debug,
                      label1="New Chronic incidence from stdout",
                      label2="New Chronic incidence from Report",
                      title="TyphoidByAgeAndGender New Chronic Infections", xlabel="Gender-Age-Year bins",
                      ylabel="Number of incidences",
                      category='report_typhoidbyageandgender_chronic_incidence',
                      alpha = 0.5, overlap = True,
                      xticks=xticks, xtickslabel=xtickslabel)

        report_file.write(sft.format_success_msg(success))


if __name__ == "__main__":
    # execute only if run as a script
    application("")
