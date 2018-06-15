#!/usr/bin/python

import json
import os.path as path
import dtk_sft
import dtk_TBHIV_Support as dts
import numpy
import math

matches = ["TB drug deactivated my",
           "progressing from Latent to Active Presymptomatic while on TB Drugs",
           "Time:",
           "Progress from Latent to Active Presymptomatic",
           "died from non-TB opportunistic infection"
           ]


"""
Requirements description goes here.
"""

def load_emod_parameters(config_filename="config.json", campaign_file="campaign.json"):
    """reads config and campaign files and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_START_TIMESTEP, etc., keys (e.g.)
    """
    codj = None
    with open(config_filename) as infile:
        codj = json.load(infile)["parameters"]
    cadj = None
    with open(campaign_file) as infile:
        cadj = json.load(infile)["Events"]
    param_obj = {}

    test_event = dts.get_test_event(cadj)
    drug_type = dts.get_drug_type(test_event)

    param_obj[dts.ConfigKeys.DrugParams.KEY_TbDrugInactivationRateHIV] = \
        codj[dts.ConfigKeys.KEY_DrugParams][drug_type][dts.ConfigKeys.DrugParams.KEY_TbDrugInactivationRateHIV]
    param_obj[dts.ParamKeys.KEY_DrugStartTime] = test_event[dts.CampaignKeys.Start_Day]
    param_obj[dts.ParamKeys.KEY_DrugRegimenLength] = \
        codj[dts.ConfigKeys.KEY_DrugParams][drug_type][dts.ConfigKeys.DrugParams.KEY_PrimaryDecayConstant]
    param_obj[dts.ConfigKeys.KEY_StartTime] = codj[dts.ConfigKeys.KEY_StartTime]
    return param_obj

def parse_stdout_file(drug_start_time, start_timestep, stdout_filename="test.txt", debug=False):
    """creates cum_inactivations, inactivation_times arrays

    :param drug_start_time: drug start time
    :param start_timestep:   first timestep from config
    :param stdout_filename: file to parse (test.txt)
    :return:                infected_mosquitoes_per_day dictionary, total_infections int
    """

    filtered_lines = []
    with open(stdout_filename) as logfile:
        for line in logfile:
            if dtk_sft.has_match(line, matches):
                filtered_lines.append(line)
    if debug:
        with open("filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    # initialize variables
    cum_inactivations = []
    inactivation_times = []
    timestep = 0
    cum = inactivation_daily = infected = active = 0
    infected_individuals = []
    inactivations = []
    active_count = []
    reactivation_times = {}
    for line in filtered_lines:
        if matches[2] in line:
            cum_inactivations.append(cum)
            infected_individuals.append(infected)
            inactivations.append(inactivation_daily)
            active_count.append(active)
            infected = int(dtk_sft.get_val("Infected: ", line))
            inactivation_daily = 0
            timestep += 1
        elif matches[0] in line:
            # deactivation: have to track individual
            individual = int(dtk_sft.get_val("TB drug deactivated my \(", line))
            # if timestep <= duration_of_interest + start_timestep + drug_start_time:
            inactivation_time = timestep - start_timestep - drug_start_time
            if individual in reactivation_times:
                inactivation_time = timestep-reactivation_times[individual]
                reactivation_times.pop( individual )
                # not including the reactivations: there are some data point lost due to simulation end before timers end
                # inactivation_times.append( inactivation_time )
            else:
                cum += 1
                active -= 1
                inactivation_daily += 1
                inactivation_times.append( inactivation_time )
        elif matches[1] in line: # "progressing from Latent to Active Presymptomatic while on TB Drugs"
            # activation: have to track individual
            individual = int(dtk_sft.get_val("Individual ", line))
            reactivation_times[individual] = timestep
        elif matches[3] in line: # move to active
            active += 1
        else: #die from HIV
            individual = int(dtk_sft.get_val("individual ", line))
            if individual in reactivation_times:
                active -= 1
                reactivation_times.pop(individual)

    if debug:
        with open ("Cum_Inactivations.txt" ,"w") as outfile:
            outfile.write(str( cum_inactivations ) )
        with open ("Inactivations.txt" ,"w") as outfile:
            outfile.write(str( inactivation_times ) )
        print( "there are {} individual in reactivation state at the end of simulation.\n".format(len(reactivation_times)) )
    return cum_inactivations, inactivation_times, active_count, inactivations

def parse_json_report(start_time, output_folder="output",
                      chart_name="InsetChart.json", debug=False):

    chart_path = path.join(output_folder, chart_name)
    with open(chart_path) as infile:
        icj = json.load(infile)[dts.InsetChart.KEY_channels]

    active_prevalence = icj[dts.InsetChart.Channels.KEY_ActiveTbPrevalence][dts.InsetChart.KEY_data]
    latent_prevalence = icj[dts.InsetChart.Channels.KEY_LatentTbPrevalence][dts.InsetChart.KEY_data]
    stat_pop = icj[dts.InsetChart.Channels.KEY_StatisticalPopulation][dts.InsetChart.KEY_data]
    end_time = start_time + len(stat_pop)

    inset_days = {}
    for x in range(start_time, end_time):
        day = {}
        day[dts.InsetChart.Channels.KEY_LatentTbPrevalence] = latent_prevalence[x]
        day[dts.InsetChart.Channels.KEY_ActiveTbPrevalence] = active_prevalence[x]
        day[dts.InsetChart.Channels.KEY_StatisticalPopulation] = stat_pop[x]
        inset_days[x + start_time] = day

    if debug:
        with open("inset_days.json", "w") as outfile:
            json.dump(inset_days, outfile, indent=4)

    return inset_days

def create_report_file(drug_start_timestep, inactivation_times, active_count, inactivations, drug_inactivation_rate, report_name, debug = False):
    with open(report_name, "w") as outfile:
        success = True
        # ks exponential test doesn't work very well with large rate, use chi squared test instead.
        # while rate is small ks test for exponential distribution is more sensitive to catch the difference
        if drug_inactivation_rate < 0.1:
            outfile.write( "Testing inactivation times as draws from exponential distrib with rate {0}. "
                           "Dataset size = {1}.\n".format( drug_inactivation_rate, len( inactivation_times ) ) )
            success = dtk_sft.test_exponential( inactivation_times, drug_inactivation_rate, outfile, integers=True,
                                                roundup=True, round_nearest=False )
            if not success:
                outfile.write("BAD: ks test for rate {} is False.\n".format(drug_inactivation_rate))
            size = len(inactivation_times)
            scale = 1.0 / drug_inactivation_rate
            dist_exponential_np = numpy.random.exponential(scale, size)
            dist_exponential_np = [math.ceil(x) for x in dist_exponential_np]
            dtk_sft.plot_data_sorted(inactivation_times, dist_exponential_np,
                              label1="test times", label2="numpy data",
                              title="inactivation_times_actual_vs_numpy",
                              xlabel="data points", ylabel="Inactivation times",
                              category="inactivation_times", show = True, line = True, overlap=True)
            dtk_sft.plot_cdf(inactivation_times, dist_exponential_np,
                             label1="test times", label2="numpy data",
                             title="inactivation_times_cdf",
                             xlabel="days", ylabel="probability",
                             category="inactivation_times_cdf", show = True)
            dtk_sft.plot_probability(inactivation_times, dist_exponential_np,
                                     label1="test times", label2="numpy data",
                                     title="inactivation_times_pdf",
                                     xlabel="days", ylabel="probability",
                                     category="inactivation_times_pdf", show = True)
        else:
            outfile.write("Testing inactivation count per day with rate {0}. \n".format( drug_inactivation_rate) )
            expected_inactivation = []
            for t in range( len(inactivations)):
                if t < drug_start_timestep :
                    if inactivations[t] > 0:
                        success = False
                        outfile.write("BAD: expected no inactivations on drugs before day {0}, get {1} cases at timestep {2}.\n"
                                      "".format(drug_start_timestep , inactivations[t], t))
                elif active_count[t] > 0:
                    expected_inactivation.append(drug_inactivation_rate * active_count[t])
            if len(inactivations) <= len(expected_inactivation) + drug_start_timestep:
                test_inactivation_dates = inactivations[drug_start_timestep+1:]
                expected_inactivation = expected_inactivation[:len(test_inactivation_dates)]
            else:
                test_inactivation_dates = inactivations[drug_start_timestep + 1:drug_start_timestep + 1 + len(expected_inactivation)]
            #print (len(inactivations), len(test_inactivation_dates), len(expected_inactivation))
            #print (test_inactivation_dates, expected_inactivation)
            dtk_sft.plot_data(test_inactivation_dates, expected_inactivation,
                                     label1="actual inactivation", label2="expected inactivation",
                                     title="inactivation per day",
                                     xlabel="date after drug start day", ylabel="inactivation per day",
                                     category="inactivation_counts", show=True, line=True, overlap=True, sort=False)

            chi_result = dtk_sft.test_multinomial(dist=test_inactivation_dates, proportions=expected_inactivation,
                                                  report_file=outfile, prob_flag=False)
            if not chi_result:
                success = False
                outfile.write("BAD: Chi-squared test reuslt is False.\n")
        outfile.write(dtk_sft.format_success_msg(success))
        if debug:
            print(dtk_sft.format_success_msg(success))
        return success

def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json",campaign_filename="campaign.json",
                 chart_name="InsetChart.json",
                 report_name=dtk_sft.sft_output_filename,
                 debug=True):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "chart_name: " + chart_name + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )
    dtk_sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename, campaign_filename)
    #total_timesteps = param_obj[KEY_TOTAL_TIMESTEPS]

    drug_start_time = param_obj[dts.ParamKeys.KEY_DrugStartTime]
    start_timestep = param_obj[dts.ConfigKeys.KEY_StartTime]
    # duration_of_interest = param_obj[dts.ParamKeys.KEY_DrugRegimenLength]
    drug_inactivation_rate = param_obj[dts.ConfigKeys.DrugParams.KEY_TbDrugInactivationRateHIV]

    cum_inactivations, inactivation_times, active_count, inactivations= parse_stdout_file( drug_start_time, start_timestep, stdout_filename, debug )
    # inset_days = parse_json_report(start_timestep, output_folder, chart_name, debug)
    create_report_file( drug_start_time, inactivation_times, active_count, inactivations, drug_inactivation_rate, report_name, debug)

    return None

if __name__ == "__main__":
    # execute only if run as a script
    application( "output" )
