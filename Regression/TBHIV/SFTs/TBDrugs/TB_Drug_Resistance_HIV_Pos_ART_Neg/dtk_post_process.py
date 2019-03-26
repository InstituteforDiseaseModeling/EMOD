#!/usr/bin/python

import json
import os.path as path
import dtk_test.dtk_sft as sft
import dtk_test.dtk_TBHIV_Support as dts
import math

matches = [ "Evolved drug resistance", "Time:" ]


"""
Requirements description goes here.
This test scenario consists of a population of XXX individuals. 
It starts off by giving everyone (?) TB and HIV. 
Then it distributes Anti-TB drugs with a 50% deactivation rate & 50% Resistance 
rate to half the population.
We then observe whether the individuals who receieved the drugs 'achieve' TB 
Resistance at a 50% greater rate than the control group.
"""

def has_match(target):
    for match in matches:
        if match in target:
            return True
    return False


def load_emod_parameters(config_filename="config.json"):
    """reads config file and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_START_TIMESTEP, etc., keys (e.g.)
    load some drug params from config.json and some coverage params from campaign.json
    """
    codj = None
    with open(config_filename) as infile:
        codj = json.load(infile)["parameters"]
    cadj = None
    with open("campaign.json") as infile:
        cadj = json.load(infile)["Events"]
    param_obj = {}

    test_event = dts.get_test_event(cadj)
    drug_type = dts.get_drug_type(test_event)

    param_obj[dts.ConfigKeys.KEY_SimulationDuration] = codj[dts.ConfigKeys.KEY_SimulationDuration]
    param_obj[dts.ParamKeys.KEY_TbDrugResistanceRate] = \
        codj[dts.ConfigKeys.KEY_DrugParams][drug_type][dts.ConfigKeys.DrugParams.KEY_TbDrugResistanceRate]
    param_obj[dts.ParamKeys.KEY_DrugStartTime] = test_event[dts.CampaignKeys.Start_Day]
    param_obj[dts.ParamKeys.KEY_ResistanceRelativeTime] = \
        codj[dts.ConfigKeys.KEY_DrugParams][drug_type][dts.ConfigKeys.DrugParams.KEY_PrimaryDecayConstant]
    param_obj[dts.ConfigKeys.KEY_StartTime] = codj[dts.ConfigKeys.KEY_StartTime]
    return param_obj


def parse_stdout_file(drug_start_time, stdout_filename="test.txt", debug=False):
    """creates a dictionary of infected mosquitoes per day

    :param curr_timestep:   first timestep from config
    :param stdout_filename: file to parse (test.txt)
    :return:                infected_mosquitoes_per_day dictionary, total_infections int
    """

    filtered_lines = []
    with open(stdout_filename) as logfile:
        for line in logfile:
            if has_match(line):
                filtered_lines.append(line)
    if debug:
        with open("filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    # initialize variables
    # so actually all the will-Resistance calculations are done at time of clearance. 
    # our clearance rate is 100%, so they all happen at once. So we just calculate
    # the number of will-Resistances at time-of-clearance and use 
    # sft.test_binomial_95ci( Resistances, N, 'rate', None, None )
    # to figure out if everything was right.
    # But first we have to fix the bug where fast progressors who get cleared by 
    # drugs bounce back-and-forth between active and latent.
    cum_resistances = []
    initial_resistances = 0
    timestep = 0
    cum = 0
    for line in filtered_lines:
        if "Time:" in line:
            cum_resistances.append(cum)
            timestep += 1
        else:
            if timestep == drug_start_time + 1:
                initial_resistances += 1
            individual = line.split()[10]
            cum += 1
    if debug:
        print( "Initial resistances: {0}".format(str( initial_resistances )))
        with open ("stdout_cumulative_resistances.json","w") as outfile:
            json.dump(cum_resistances, outfile, indent=4)
    return cum_resistances, initial_resistances

def parse_json_report(start_time, output_folder="output",
                      chart_name="InsetChart.json", debug=False):
    chart_path = path.join(output_folder, chart_name)
    with open(chart_path) as infile:
        icj = json.load(infile)[dts.InsetChart.KEY_channels]

    stat_pop = icj[dts.InsetChart.Channels.KEY_StatisticalPopulation][dts.InsetChart.KEY_data]
    mdr_prevalence = icj[dts.InsetChart.Channels.KEY_MdrTbPrevalence][dts.InsetChart.KEY_data]
    active_TB_treatments = icj[dts.InsetChart.Channels.KEY_ActiveTBTreatment][dts.InsetChart.KEY_data]

    end_time = start_time + len(stat_pop)

    inset_days = {}
    for x in range(start_time, end_time):
        day = {}
        day[dts.InsetChart.Channels.KEY_StatisticalPopulation] = stat_pop[x]
        day[dts.InsetChart.Channels.KEY_MdrTbPrevalence] = mdr_prevalence[x]
        day[dts.InsetChart.Channels.KEY_ActiveTBTreatment] = active_TB_treatments[x]
        inset_days[x + start_time] = day

    if debug:
        with open("inset_days.json", "w") as outfile:
            json.dump(inset_days, outfile, indent=4)

    return inset_days

def create_report_file( Resistances, initial_resistances, drug_start_time, param_obj, report_name, inset_days, debug ):
    with open(report_name, "w") as outfile:
        starting_pop = inset_days[0][dts.InsetChart.Channels.KEY_StatisticalPopulation]
        # success = sft.test_binomial_95ci( initial_resistances, starting_pop, param_obj["TB_Drug_Resistance_Rate_HIV"], outfile, "???" )
        success = True
        progression = []
        bad_msgs = []
        for x in range(len(inset_days)):
            inset_day = inset_days[x]
            inset_mdr_prevalence =  inset_day[dts.InsetChart.Channels.KEY_MdrTbPrevalence]
            stdout_resistants = Resistances[x]
            if x >= drug_start_time:
                progression.append(stdout_resistants)
            if debug:
                outfile.write("Day: {0}\n".format(x))
                outfile.write(str(inset_day) + "\n")
                outfile.write("StdOut resistants: {0}\n".format(stdout_resistants))
            stdout_predicted_prevalence = stdout_resistants / float(inset_day[dts.InsetChart.Channels.KEY_StatisticalPopulation])
            if abs(inset_mdr_prevalence - stdout_predicted_prevalence) > 0.03:
                bad_msgs.append("BAD: at timestep {0}, expected MDR prevalence: {1}, InsetChart had: {2}\n".format(
                    x,
                    stdout_predicted_prevalence,
                    inset_mdr_prevalence
                ))

        tb_drug_resistance_rate_hiv = param_obj["TB_Drug_Resistance_Rate_HIV"]
        new_resistances = []
        pre_resistance = 0
        failed_count = 0
        total_test = 0
        for x in range(drug_start_time + 1,len(Resistances)):
            resistance = Resistances[x]
            new_resistance = resistance - pre_resistance
            pre_resistance = resistance
            new_resistances.append(new_resistance)
            expected_mean = (starting_pop - resistance) * tb_drug_resistance_rate_hiv
            total_test += 1
            if expected_mean >= 5: # advoid failing with too small mean
                result = sft.test_binomial_99ci(new_resistance, starting_pop - resistance, tb_drug_resistance_rate_hiv, outfile,category="time step {}".format(x + 1))
                if not result:
                        failed_count += 1
                        outfile.write("Warning: New Resistance test fails for rate = {0} at time step {1}.\n".format(tb_drug_resistance_rate_hiv, x +1))
            else:
                error_tolerance = 3* math.sqrt(tb_drug_resistance_rate_hiv * (1 - tb_drug_resistance_rate_hiv) * (starting_pop - resistance)) # 3 sigma
                result = math.fabs(new_resistance - expected_mean) <= error_tolerance
                if not result:
                        failed_count += 1
                        outfile.write("Warning: New Resistance test fails for rate = {0} at time step {1}, "
                                      "new resistance = {2}, expected mean = {3}, error tolerance = {4}.\n".format(
                            tb_drug_resistance_rate_hiv, x +1, new_resistance, expected_mean, error_tolerance))

        if failed_count > math.ceil(total_test * 0.01):
            success = False
            outfile.write("BAD: test failed {0} times out of {1} timestep, please check the warning message.\n"
                          "".format(failed_count, total_test))
        if debug:
            sft.plot_data(new_resistances, title="new resistance over time", category="new_resistance",show = True)
            series = sft.create_geometric_dis(param_obj["TB_Drug_Resistance_Rate_HIV"], starting_pop, len(progression),
                                              test_decay=False)
            sft.plot_data(progression, series,
                                   label1="progression", label2="geomatric dis",
                                   xlabel="days", ylabel="resistance",
                                   title="progression vs geomatric",
                                   category="progression_vs_geomatric", show=True, line = True)
            sft.plot_cdf(progression, series,
                         label1="progression", label2="geomatric dis",
                         title="progression vs geomatric cdf",
                         category="progression_vs_geomatric_cdf", show=True)
        # success = sft.test_geometric_decay(progression, param_obj["TB_Drug_Resistance_Rate_HIV"], starting_pop, test_decay=False, report_file=outfile, debug=debug)


        if len(bad_msgs) > 0:
            success = False
            outfile.writelines(bad_msgs)

        outfile.write(sft.format_success_msg(success))


def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json",
                 chart_name="InsetChart.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "chart_name: " + chart_name + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )
    sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename)
    total_timesteps = param_obj[dts.ConfigKeys.KEY_SimulationDuration]

    drug_start_time = param_obj["Drug_Start_Time"]
    start_timestep = param_obj[dts.ConfigKeys.KEY_StartTime]

    # Now process log output (probably) and compare to theory (not in this example) or to another report.
    cum_resistances, initial_resistances = parse_stdout_file(drug_start_time, stdout_filename, debug)
    sft.plot_data( cum_resistances, label1="Cumulative Resistances", label2="NA",
                   title="Cumulative Resistances over Time",
                   xlabel="Timestep", ylabel="Resistances",
                   category="Cumulative_Resistances", show = True)
    inset_days = parse_json_report(start_timestep, output_folder, chart_name, debug)

    # Now we've ingested all relevant inputs, let's do analysis

    create_report_file( cum_resistances, initial_resistances, drug_start_time, param_obj,
                        report_name, inset_days, debug)
    return None

if __name__ == "__main__":
    # execute only if run as a script
    application( "output" )
