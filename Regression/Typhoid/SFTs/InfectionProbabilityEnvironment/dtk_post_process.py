#!/usr/bin/python
# This SFT test the following statements:
# probability of infection from route environment is driven by this formula:
# P(infection) = 1- (1- immunity * P(response))#exposures
# It's a beta-binomial distribution
# the actual number of infection from route environment is within the 95% confidence interval of binomial distribution

if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../shared_embedded_py_scripts').resolve().absolute()) )

import re
import json
import math
import dtk_test.dtk_sft as sft


def application(report_file):
    sft.wait_for_done()
    # print( "Post-processing: " + report_file )
    cdj = json.loads(open("config.json").read())["parameters"]
    start_time = cdj["Start_Time"]
    timestep = start_time
    lines_e = []
    count_enviro = 0

    with open("test.txt") as logfile:
        for line in logfile:
            if "Update(): Time:" in line:
                timestep = int(float(sft.get_val("Time: ", line)))
            elif ("Exposing " in line) and ("route 'environment'" in line):
                # collect dose_response probabilities and dose for route environment
                line = "TimeStep: " + str(timestep) + " " + line
                lines_e.append(line)
            # route=0, outbreak; route=1, contact; route=2, environment
            elif ("AcquireNewInfection:" in line) and ("route=2" in line):
                count_enviro += 1

    success = True
    infection_prob_e_all = []
    infection_prob_e_theoretic_all= []

    with open(sft.sft_output_filename, "w") as report_file:
        if len(lines_e) == 0:
            success = False
            report_file.write("Found no individual exposed from route environment.\n")
        else:
            prob_per_num_exposures = {}
            prob_per_num_exposures_expected = {}
            random_dose_response = None

            for line in lines_e:
                dose_response = float(sft.get_val("infects=", line))
                # some version of logging as a typo
                ind_id = int(sft.get_val("individual ", line)) if "individual " in line else \
                    int(sft.get_val("inividual ", line))
                immunity = float(sft.get_val("immunity=", line))
                num_exposures = float(sft.get_val("num_exposures=", line))
                infection_prob = float(sft.get_val("prob=", line))
                timestep = int(sft.get_val("TimeStep: ", line))
                infection_prob_theoretic = 1.0 - math.pow((1.0 - immunity * dose_response), num_exposures)
                infection_prob_e_all.append(infection_prob)
                infection_prob_e_theoretic_all.append(infection_prob_theoretic)

                if math.fabs(infection_prob_theoretic - infection_prob) > 5e-2:
                    success = False
                    report_file.write(
                        "BAD: Infection probability for individual {0} at time {1}, route environment is {2}, "
                        "expected {3}.\n".format(ind_id, timestep, infection_prob, infection_prob_theoretic))

                # pick a random dose_response, for plot to be added to spec
                if random_dose_response is None:
                    random_dose_response = dose_response

                if immunity == 1.0 and dose_response == random_dose_response:
                    if num_exposures not in prob_per_num_exposures:
                        prob_per_num_exposures[num_exposures] = infection_prob
                        prob_per_num_exposures_expected[num_exposures] = infection_prob_theoretic

            if success:
                report_file.write("GOOD: Infection probability matches expected value for every exposure.\n")
            else:
                report_file.write("BAD: Infection probability doesn't match expected value for every exposure.\n")

            # plot for Spec
            # sort dictionary by keys
            prob_per_num_exposures = dict(sorted(prob_per_num_exposures.items()))
            prob_per_num_exposures_expected = dict(sorted(prob_per_num_exposures_expected.items()))
            sft.plot_data([v for k,v in prob_per_num_exposures.items()],
                          [v for k,v in prob_per_num_exposures_expected.items()],
                          label1="Actual",
                          label2="Expected",
                          title="Infection Probability(Env)\nimmunity=1, dose_response={}".format(
                              random_dose_response),
                          xlabel="num_exposures",
                          ylabel="Infection Probability",
                          category='infection_probability_Env',
                          alpha=0.5, overlap=True, xticks=range(len(prob_per_num_exposures)),
                          xtickslabel=[str(int(i)) for i in prob_per_num_exposures]
                          )

            # sum of independent Bernoulli trials is Poisson binomial distribution
            # contact is calculated first
            # mean = average number of events per interval
            mean_e = sft.calc_poisson_binomial(infection_prob_e_all)['mean']
            sd_e = sft.calc_poisson_binomial(infection_prob_e_all)['standard_deviation']
            num_trials_e = len(infection_prob_e_all)
            prob_e = mean_e / float(num_trials_e)
            # report_file.write("environment: tarils = {2}, num_infection = {3}, proc_e= {4}, mean= {0},
            # sd = {1}.\n".format(mean_e, sd_e,num_trails_e,count_enviro,prob_e))

            # TODO: comment out line 102 to 117 once #2895 is fixed. we are going to test New Infections By Route
            # channels in NewInfection SFT, we only test the logging in this test.
            isj = json.loads(open("output/InsetChart.json").read())["Channels"]
            new_infection_enviro = isj["New Infections By Route (ENVIRONMENT)"]["Data"]
            insetchart_total_enviro_infection = sum(new_infection_enviro)
            message_template = "{0}: {1} is {2}, while total number of exposure = {3}, sum of contact " \
                               "infection_probability = {4}, expected total contact infections = {5} " \
                               "with standard_deviation = {6}.\n"
            if not sft.test_binomial_95ci(insetchart_total_enviro_infection, num_trials_e, prob_e, report_file,
                                          'environmental infection'):
                success = False
                report_file.write(message_template.format("BAD", "sum of 'New Infections By Route (ENVIRONMENT)' in "
                                                                 "InsetChart.json", insetchart_total_enviro_infection,
                                                          num_trials_e, prob_e, mean_e, sd_e))
            else:
                report_file.write(message_template.format("GOOD", "sum of 'New Infections By Route (ENVIRONMENT)' in "
                                                                 "InsetChart.json", insetchart_total_enviro_infection,
                                                          num_trials_e, prob_e, mean_e, sd_e))

            if not sft.test_binomial_95ci(count_enviro, num_trials_e, prob_e, report_file, 'environmental infection'):
                success = False
                report_file.write(message_template.format("BAD", "total environmental infections from StdOut logging",
                                                          count_enviro,
                                                          num_trials_e, prob_e, mean_e, sd_e))
            else:
                report_file.write(message_template.format("GOOD", "total environmental infections from StdOut logging",
                                                          count_enviro,
                                                          num_trials_e, prob_e, mean_e, sd_e))

            sft.plot_data(infection_prob_e_all, infection_prob_e_theoretic_all,
                          # filter(lambda a: a != 0, infection_prob_e_all),
                          # filter(lambda a: a != 0, infection_prob_e_theoretic_all),
                          label1="Actual",
                          label2="Expected", title="Infection Probability Environmental",
                          xlabel="Occurrence",
                          ylabel="Infection Probability",
                          category='immunity_probability_environment',
                          alpha=0.5, overlap=True, sort=False)

        report_file.write(sft.format_success_msg(success))




if __name__ == "__main__":
    # execute only if run as a script
    application("")
