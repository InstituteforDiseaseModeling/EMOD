#!/usr/bin/python
# This support the following Typoid SFT tests:
# All tests under AcuToChrTransitionProbability
# All tests under SubcToChrTransitionProbability

import re
import json
import math
import pdb
import os
import dtk_test.dtk_sft as sft
import pandas as pd

class ConfigParameters(object):
    KEY_Start_Time = "Start_Time"
    KEY_TCP = "Typhoid_Carrier_Probability"
    KEY_SIMULATION_TIMESTEP = "Simulation_Timestep"


class Constant(object):
    gpag_male = [0.0, 0.0, 0.045, 0.134, 0.167, 0.198, 0.247, 0.435, 0.4]
    gpag_female = [0.0, 0.097, 0.234, 0.431, 0.517, 0.60, 0.692, 0.692, 0.555]
    age = ["0-9", "10-19", "20-29", "30-39", "40-49", "50-59", "60-69", "70-79", "80+"]
    age_initial = [1, 11, 21, 31, 41, 51, 61, 71, 81]


class DemographicsParameters():
    Nodes = "Nodes"
    NodeAttributes = "NodeAttributes"
    InitialPopulation = "InitialPopulation"
    IndividualAttributes = "IndividualAttributes"
    AgeDistributionFlag= "AgeDistributionFlag"
    AgeDistribution1 = "AgeDistribution1"
    AgeDistribution = "AgeDistribution"
    FertilityDistribution = "FertilityDistribution"
    MortalityDistributionFemale = "MortalityDistributionFemale"
    MortalityDistributionMale = "MortalityDistributionMale"


class ChrTransitionProbabilitySupport(object):
    def __init__(self, from_state):
        self.from_state = from_state

    #region pre_process script support
    def get_json_template(self, json_filename="../../../Santiago_Demographics.json"):
        with open(json_filename) as infile:
            j_file_obj = json.load(infile)
        return j_file_obj

    def set_json_file(self, json_object, json_filename="Santiago_Demographics_overlay.json"):
        with open(json_filename, 'w') as outfile:
            json.dump(json_object, outfile, indent=4, sort_keys=True)

    def configure_demo(self, ini_age, ini_pop, demo_template_filename="../../../Santiago_Demographics.json",
                       demo_filename="Santiago_Demographics.json"):
        print("configure demographics json.\n")
        demographics = self.get_json_template(json_filename=demo_template_filename)
        demographics[DemographicsParameters.Nodes][0][DemographicsParameters.NodeAttributes][
            DemographicsParameters.InitialPopulation] = ini_pop
        demographics[DemographicsParameters.Nodes][0][DemographicsParameters.IndividualAttributes][
            DemographicsParameters.AgeDistributionFlag] = 0
        demographics[DemographicsParameters.Nodes][0][DemographicsParameters.IndividualAttributes][
            DemographicsParameters.AgeDistribution1] = ini_age

        # list_to_remove = [DemographicsParameters.AgeDistribution,
        #                   DemographicsParameters.FertilityDistribution,
        #                   DemographicsParameters.MortalityDistributionFemale,
        #                   DemographicsParameters.MortalityDistributionMale]
        # for element in demographics[DemographicsParameters.Nodes][0][DemographicsParameters.IndividualAttributes]:
        #     for to_remove in list_to_remove:
        #         element.pop(to_remove, None)

        del demographics[DemographicsParameters.Nodes][0][DemographicsParameters.IndividualAttributes][
            DemographicsParameters.AgeDistribution]
        del demographics[DemographicsParameters.Nodes][0][DemographicsParameters.IndividualAttributes][
            DemographicsParameters.FertilityDistribution]
        del demographics[DemographicsParameters.Nodes][0][DemographicsParameters.IndividualAttributes][
            DemographicsParameters.MortalityDistributionFemale]
        del demographics[DemographicsParameters.Nodes][0][DemographicsParameters.IndividualAttributes][
            DemographicsParameters.MortalityDistributionMale]

        self.set_json_file(demographics, json_filename=demo_filename)
    #endregion

    #region post_process script support
    def load_emod_parameters(self, config_filename="config.json"):
        """
        reads config file and populates params_obj
        :param config_filename: name of config file (config.json)
        :returns param_obj:     dictionary with KEY_Start_Time, etc., keys (e.g.)
        """
        with open(config_filename) as infile:
            cdj = json.load(infile)["parameters"]
        param_obj = {ConfigParameters.KEY_Start_Time: cdj[ConfigParameters.KEY_Start_Time],
                     ConfigParameters.KEY_TCP: cdj[ConfigParameters.KEY_TCP]}
        return param_obj

    def parse_output_file(self, output_filename=sft.sft_test_filename, start_time=0, debug=False):
        """
        read "test.txt' and filter lines that match keywords, return a dataframe of time step and lines of interest.
        :param output_filename: file to parse (test.txt)
        :param start_time: simulation start time
        :param from_state: the disease state before transiting to Chronic state
        :return: output_df
        """
        output_df = pd.DataFrame(columns=[ConfigParameters.KEY_SIMULATION_TIMESTEP, "Content"])
        time_step = start_time
        output_df.index.name = "index"
        index = 0
        with open(output_filename) as logfile:
            for line in logfile:
                if "Update(): Time:" in line:
                    # calculate time step
                    time_step += 1
                elif "just went chronic" in line and "from {}".format(self.from_state) in line:
                    # append time step and all Infection stage transition to list
                    output_df.loc[index] = [time_step, line]
                    index += 1
                elif "just recovered" in line and "from {}".format(self.from_state) in line:
                    # append time step and all Infection stage transition to list
                    output_df.loc[index] = [time_step, line]
                    index += 1
        if debug:
            with open("df.csv", 'w') as file:
                output_df.to_csv(file)
        return output_df

    def create_report_file(self, param_obj, output_df, report_name=sft.sft_output_filename, age_index_1=0,
                           age_index_2=9):
        """
        Test the probability to become Chronic state and recovered from Acute or Subclinical state, write the
        test result to file.
        :param param_obj: config parameter dictionary
        :param output_df: dataframe which contains test data from stdout file
        :param report_name: file to write test result
        :param age_index_1: index to define age range to test
        :param age_index_2: index to define age range to test
        :return: True or False for test result
        """

        # 4*10 list to store the count for cases [0][]: Chr_male, [1][]: Chr_female, [2][]: Sus_male. [3][]: Sus_female
        count = [[0] * 9 for _ in range(4)]

        tcp = param_obj[ConfigParameters.KEY_TCP]

        gpag_male = Constant.gpag_male
        gpag_female = Constant.gpag_female

        success = True

        with open(report_name, "w") as report_file:
            # if len(output_df) == output_df.ix[len(output_df)-1, ConfigParameters.KEY_SIMULATION_TIMESTEP]:
            if len(output_df) == 0:
                success = False
                report_file.write(sft.sft_no_test_data)
            else:
                time_steps = output_df[ConfigParameters.KEY_SIMULATION_TIMESTEP]
                lines = output_df["Content"]
                for index in range(len(output_df)):
                    time_step = time_steps[index]
                    line = lines[index]
                    age = float(sft.get_val(" age ", line))
                    sex = "female" if ("sex 1" in line or "sex Female" in line) else "male"
                    if "just went chronic" in line:
                        # to Chronic
                        #  python 2.7 the (int / int) operator is integer division
                        i = int(age / 10)
                        # for age > 80, put them into the last age group
                        if i > 8:
                            i = 8
                        if sex == "male":
                            count[0][i] += 1
                        else:
                            count[1][i] += 1
                    else:
                        # to Susceptible
                        # python 2.7 the (int / int) operator is integer division
                        i = int(age / 10 )
                        # for age > 80, put them into the last age group
                        if i > 8:
                            i = 8
                        if sex == "male":
                            count[2][i] += 1
                        else:
                            count[3][i] += 1
                # calculate theoretic probability of becoming a Chronic carrier in two 1*9 lists
                theoretic_p_male = [x * tcp for x in gpag_male]
                theoretic_p_female = [x * tcp for x in gpag_female]
                # calculate actual probability of becoming a Chronic carrier in two 1*9 lists
                actual_p_male = [x / float(x + y) if (x + y) != 0 else -1 for x, y in zip(count[0], count[2])]
                actual_p_female = [x / float(x + y) if (x + y) != 0 else -1 for x, y in zip(count[1], count[3])]

                for x in range(age_index_1, age_index_2):
                    age = Constant.age
                    # calculate the total chronic cases and sample sizes for Male and Female
                    actual_chr_count_male = count[0][x]
                    actual_count_male = count[0][x] + count[2][x]
                    actual_chr_count_female = count[1][x]
                    actual_count_female = count[1][x] + count[3][x]
                    # Male
                    category = 'sex: Male, age: ' + age[x]
                    if actual_count_male == 0:
                        success = False
                        report_file.write(
                            "Found no male in age group {0} went to Chronic state or was recovered from {1} state.\n".format(
                                age[x], self.from_state))
                    elif theoretic_p_male[x] < 5e-2 or theoretic_p_male[x] > 0.95:
                        # for cases that binomial confidence interval will not work: prob close to 0 or 1
                        if math.fabs(actual_p_male[x] - theoretic_p_male[x]) > 5e-2:
                            success = False
                            report_file.write(
                                "BAD: Proportion of {0} {1} cases that become Chronic is {2}, expected {3}.\n".format(
                                    category, self.from_state, actual_p_male[x], theoretic_p_male[x]))
                    elif not sft.test_binomial_95ci(actual_chr_count_male, actual_count_male, theoretic_p_male[x],
                                                    report_file, category):
                        success = False

                    # Female
                    category = 'sex: Female, age: ' + age[x]
                    if actual_count_female == 0:
                        success = False
                        report_file.write(
                            "Found no female in age group {0} went to Chronic state or was recovered from {1} state.\n".format(
                                age[x], from_state))
                    elif theoretic_p_female[x] < 5e-2 or theoretic_p_female[x] > 0.95:
                        # for cases that binomial confidence interval will not work: prob close to 0 or 1
                        if math.fabs(actual_p_female[x] - theoretic_p_female[x]) > 5e-2:
                            success = False
                            report_file.write(
                                "BAD: Proportion of {0} {1} cases that become Chronic is {2}, expected {3}.\n".format(
                                    category, from_state,actual_p_female[x], theoretic_p_female[x]))
                    elif not sft.test_binomial_95ci(actual_chr_count_female, actual_count_female, theoretic_p_female[x],
                                                    report_file, category):
                        success = False
            report_file.write(sft.format_success_msg(success))
            return success
    #endregion


class AcuToChr(ChrTransitionProbabilitySupport):
    def __init__(self):
        ChrTransitionProbabilitySupport.__init__(self, 'acute')


class SubcToChr(ChrTransitionProbabilitySupport):
    def __init__(self):
        ChrTransitionProbabilitySupport.__init__(self, 'subclinical')
