#!/usr/bin/python

import json
import os.path as path
import dtk_test.dtk_sft as sft
import dtk_test.dtk_TBHIV_Support as dts
import numpy
import math

matches = [ "Died from active disease on drugs", "Time:" ]


"""
Requirements description goes here.
This test scenario consists of a population of 1000 individuals.
It starts off by giving everyone TB and HIV.
Then it distributes Anti-TB drugs with a "TB_Drug_Mortality_Rate_HIV"
rate to all population.
We then observe whether the death time of individuals who died after receieved the TBdrugs
is draw from exponential distibution with rate = "TB_Drug_Mortality_Rate_HIV".
"""

def load_emod_parameters(config_filename="config.json", campaign_filename="campaign.json"):
    """reads config file and campaign file and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_START_TIMESTEP, etc., keys (e.g.)
    load some drug params from config.json and some coverage params from campaign.json
    """
    codj = None
    with open(config_filename) as infile:
        codj = json.load(infile)["parameters"]
    cadj = None
    with open(campaign_filename) as infile:
        cadj = json.load(infile)["Events"]
    param_obj = {}
    param_obj[dts.ConfigKeys.KEY_SimulationDuration] = codj[dts.ConfigKeys.KEY_SimulationDuration]


    test_event = dts.get_test_event(cadj)
    drug_type = dts.get_drug_type(test_event)

    param_obj[dts.ConfigKeys.DrugParams.KEY_TbDrugMortalityRateHIV] = \
        codj[dts.ConfigKeys.KEY_DrugParams][drug_type][dts.ConfigKeys.DrugParams.KEY_TbDrugMortalityRateHIV]
    param_obj[dts.ParamKeys.KEY_DrugStartTime] = test_event[dts.CampaignKeys.Start_Day]
    param_obj[dts.ParamKeys.KEY_DrugRegimenLength] = \
        codj[dts.ConfigKeys.KEY_DrugParams][drug_type][dts.ConfigKeys.DrugParams.KEY_TbDrugPrimaryDecayConstant]
    return param_obj


def parse_stdout_file( start_timestep, duration_of_interest, stdout_filename="test.txt", debug=False):
    """creates cum_death and death_times array

    :param start_timestep:   drug start time
    :param stdout_filename: file to parse (test.txt)
    :return:                cum_deaths, death_times
    """

    filtered_lines = []
    with open(stdout_filename) as logfile:
        for line in logfile:
            if sft.has_match(line, matches):
                filtered_lines.append(line)
    if debug:
        with open("filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    # initialize variables
    cum_deaths = []
    deaths = []
    death_times = []
    timestep = 0
    cum = death_daily = infected = 0
    infected_individuals = []
    for line in filtered_lines:
        if "Time:" in line:
            timestep += 1
            infected_individuals.append(infected)
            cum_deaths.append(cum)
            deaths.append(death_daily)
            death_daily = 0
            infected = int(sft.get_val("Infected: ", line))
        else:
            cum += 1
            death_daily += 1
            # cum_deaths[-1] = cum
            if timestep <= duration_of_interest + start_timestep:
                death_times.append( timestep-start_timestep )
    if debug:
        with open("cum_deaths.txt","w") as file:
            file.writelines(  str( cum_deaths ) )
        with open("death_times.txt","w") as file:
            file.writelines( str( death_times ) )
    return cum_deaths, deaths, infected_individuals, death_times

def parse_json_report(output_folder="output", chart_name="InsetChart.json", debug=False):
    """creates disease_deaths array

    :param dts.InsetChart_name: dts.InsetChart.json file with location (output/dts.InsetChart.json)
    :returns: disease_deaths array
    """
    chart_path = path.join(output_folder, chart_name)
    with open(chart_path) as infile:
        icj = json.load(infile)[dts.InsetChart.KEY_channels]

    diseas_deaths = icj[dts.InsetChart.Channels.KEY_DiseaseDeaths][dts.InsetChart.KEY_data]

    if debug:
        with open("diseas_deaths.txt", "w") as outfile:
            outfile.writelines(str(diseas_deaths))

    return diseas_deaths


def create_report_file(drug_start_timestep, disease_deaths, cum_deaths, deaths, infected_individuals, death_times, drug_mortality_rate_HIV, report_name ):
    with open(report_name, "w") as outfile:
        success = True
        length = len(cum_deaths)
        if sum(disease_deaths)==0 or sum(cum_deaths)==0 or len(death_times)==0:
            success = False
            outfile.write(sft.no_test_data)
        for x in range(length):
            if disease_deaths[x] != cum_deaths[x]:
                success = False
                outfile.write("BAD: at timestep {0}, disease deaths is {1} in InsetChart.json and {2} in stdout.txt.\n".format(x+1, disease_deaths[x], cum_deaths[x]))
        # ks exponential test doesn't work very well with large rate, use chi squared test instead
        # while rate is small ks test for exponential distribution is more sensitive to catch the difference
        if drug_mortality_rate_HIV < 0.1:
            outfile.write("Testing death times as draws from exponential distrib with rate {0}. "
                          "Dataset size = {1}.\n".format(drug_mortality_rate_HIV, len(death_times)))
            ks_result = sft.test_exponential( death_times, drug_mortality_rate_HIV, report_file = outfile,
                                                  integers=True, roundup=True, round_nearest=False )
            if not ks_result:
                success = False
                outfile.write("BAD: ks test reuslt is False.\n")
            size = len(death_times)
            scale = 1.0 / drug_mortality_rate_HIV
            dist_exponential_np = numpy.random.exponential(scale, size)
            dist_exponential_np = [math.ceil(x) for x in dist_exponential_np]
            sft.plot_data_sorted(death_times, dist_exponential_np,
                              label1="death times", label2="numpy data",
                              title="death_times_actual_vs_numpy",
                              xlabel="data points", ylabel="death times",
                              category="death_times", show=True, line = True, overlap=True)
            sft.plot_cdf(death_times, dist_exponential_np,
                             label1="death times", label2="numpy data",
                             title="death_times_cdf",
                             xlabel="days", ylabel="probability",
                             category="death_times_cdf", show=True)
        else:
            outfile.write("Testing death count per day with rate {0}. \n".format(drug_mortality_rate_HIV))
            expected_mortality = []
            for t in range( len(deaths)):
                if t < drug_start_timestep + 1:
                    if deaths[t] > 0:
                        success = False
                        outfile.write("BAD: expected no disease death on drugs before day {0}, get {1} cases at timestep {2}.\n"
                                      "".format(drug_start_timestep + 1, deaths[t], t))
                elif infected_individuals[t] > 0:
                    expected_mortality.append(drug_mortality_rate_HIV * infected_individuals[t])
            expected_mortality.pop(0) # the Infected is off by one day
            test_death_dates = deaths[drug_start_timestep + 1:drug_start_timestep + 1 + len(expected_mortality)]
            sft.plot_data(test_death_dates, expected_mortality,
                                     label1="actual death", label2="expected death",
                                     title="death per day",
                                     xlabel="date after drug start day", ylabel="death per day",
                                     category="death_counts", show=True, line=True, overlap=True, sort=False)

            chi_result = sft.test_multinomial(dist=test_death_dates, proportions=expected_mortality,
                                                  report_file=outfile, prob_flag=False)
            if not chi_result:
                success = False
                outfile.write("BAD: Chi-squared test reuslt is False.\n")

        outfile.write(sft.format_success_msg(success))
        return success


def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json",campaign_filename="campaign.json",
                 chart_name="InsetChart.json",
                 report_name=sft.sft_output_filename,
                 debug=True):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "chart_name: " + chart_name + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )

    sft.wait_for_done()

    param_obj = load_emod_parameters(config_filename,campaign_filename)
    drug_mortality_rate_HIV = param_obj[dts.ConfigKeys.DrugParams.KEY_TbDrugMortalityRateHIV]
    drug_start_timestep = param_obj[dts.ParamKeys.KEY_DrugStartTime]
    duration_of_interest = param_obj[dts.ParamKeys.KEY_DrugRegimenLength]
    cum_deaths, deaths, infected_individuals, death_times = parse_stdout_file(drug_start_timestep, duration_of_interest, stdout_filename, debug)
    disease_deaths = parse_json_report(output_folder, chart_name, debug)

    sft.plot_data( cum_deaths, disease_deaths,
                       label1="stdout", label2="insetchart",
                       title="Cumulative Deaths stdout vs. insetchart",
                       category="Cumulative_Deaths",
                       xlabel="Timestep", ylabel="Deaths", show=True, line=True, overlap=True)

    # Now we've ingested all relevant inputs, let's do analysis
    create_report_file(drug_start_timestep, disease_deaths, cum_deaths, deaths, infected_individuals, death_times, drug_mortality_rate_HIV, report_name)

if __name__ == "__main__":
    # execute only if run as a script
    application( "output" )
