#!/usr/bin/python

import json
import os.path as path
import dtk_sft as sft
import dtk_TBHIV_Support as dts

matches = [ "will relapse in the future", "Time:" ]

"""
Requirements description goes here.
This test scenario consists of a population of 2000 individuals.
It starts off by giving everyone TB and HIV.
Then it distributes Anti-TB drugs with a 0% deactivation rate & 50% relapse
rate to all population.
We then observe whether the individuals who receieved the drugs 'achieve' TB 
relapse at a 50% rate.
"""

def load_emod_parameters(config_filename="config.json", campaign_filename="campaign.json"):
    """reads config and campaign files and populates params_obj

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
    param_obj[dts.ConfigKeys.DrugParams.KEY_TbDrugRelapseRateHIV] = \
        codj[dts.ConfigKeys.KEY_DrugParams][drug_type][dts.ConfigKeys.DrugParams.KEY_TbDrugRelapseRateHIV]
    param_obj[dts.ParamKeys.KEY_DrugStartTime] = test_event[dts.CampaignKeys.Start_Day]
    param_obj[dts.ParamKeys.KEY_RelapseRelativeTime] = \
        codj[dts.ConfigKeys.KEY_DrugParams][drug_type][dts.ConfigKeys.DrugParams.KEY_TbDrugPrimaryDecayConstant]
    return param_obj


def parse_stdout_file( start_timestep, stdout_filename="test.txt", debug=False ):
    """creates a dictionary of infected mosquitoes per day

    :param curr_timestep:   first timestep from config
    :param stdout_filename: file to parse (test.txt)
    :return:                infected_mosquitoes_per_day dictionary, total_infections int
    """

    filtered_lines = []
    with open(stdout_filename) as logfile:
        for line in logfile:
            if sft.has_match(line,matches):
                filtered_lines.append(line)
    if debug:
        with open("filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    # initialize variables
    # so actually all the will-relapse calculations are done at time of clearance. 
    # our clearance rate is 100%, so they all happen at once. So we just calculate
    # the number of will-relapses at time-of-clearance and use 
    # sft.test_binomial_95ci( relapses, N, 'rate', None, None )
    # to figure out if everything was right.
    # But first we have to fix the bug where fast progressors who get cleared by 
    # drugs bounce back-and-forth between active and latent.
    cum_relapses = []
    relapses = []
    timestep = 0
    initial_relapses = 0
    pre_cum = 0
    cum = 0
    for line in filtered_lines:
        if "Time:" in line:
            timestep += 1
            cum_relapses.append(cum)
            relapses.append(cum - pre_cum)
            pre_cum = cum
        else:
            if timestep == start_timestep + 1:
                initial_relapses += 1
            cum += 1
    if debug:
        with open("relapses.txt", "w") as file:
            file.writelines(str(relapses))
        with open("cum_relapses.txt", "w") as file:
            file.writelines(str(cum_relapses))
        print( "initial relapses is {}.\n".format(initial_relapses ) )
    return  relapses, cum_relapses

def parse_json_report( output_folder="output", chart_name="InsetChart.json", debug=False):
    """creates active_TB_treatment array

    :param chart_name: InsetChart.json file name (InsetChart.json)
    :returns: active_TB_treatment array
    """
    chart_path = path.join(output_folder, chart_name)
    with open(chart_path) as infile:
        icj = json.load(infile)[dts.InsetChart.KEY_channels]

    active_TB_treatments = icj[dts.InsetChart.Channels.KEY_ActiveTBTreatment][dts.InsetChart.KEY_data]

    if debug:
        with open("active_TB_treatments.json", "w") as outfile:
            json.dump(active_TB_treatments, outfile, indent=4)

    return active_TB_treatments

def create_report_file( active_TB_treatments,relapses, tb_drug_relapse_rate_hiv,drug_start_timestep, report_name ):
    with open(report_name, "w") as outfile:
        print(str(relapses), str(tb_drug_relapse_rate_hiv))
        success = True
        if sum(active_TB_treatments)==0 or sum(relapses)==0:
            success = False
            outfile.write(sft.sft_no_test_data)
        for x in range (drug_start_timestep + 1, len(active_TB_treatments) - 1):
            active_TB_treatment = int(active_TB_treatments[x])
            relapse = relapses[x + 1]
            result = sft.test_binomial_99ci( relapse, active_TB_treatment, tb_drug_relapse_rate_hiv, outfile, category="time step {}".format(x+1) )
            if not result:
                success = False
                outfile.write("BAD: test fails for rate = {0} at time step {1}.\n".format(tb_drug_relapse_rate_hiv, x +1))

        outfile.write(sft.format_success_msg(success))
        return success

def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json",
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
    param_obj = load_emod_parameters(config_filename)

    drug_start_timestep = param_obj[dts.ParamKeys.KEY_DrugStartTime]
    tb_drug_relapse_rate_hiv = float(param_obj[dts.ConfigKeys.DrugParams.KEY_TbDrugRelapseRateHIV])

    # Now process log output (probably) and compare to theory (not in this example) or to another report.
    relapses, cum_relapses = parse_stdout_file(drug_start_timestep, stdout_filename, debug)
    sft.plot_data( relapses, cum_relapses, label1="Relapses",
                   label2="Cumulative Relapses", title="Relapses vs. Cumulative Relapses over Time",
                   xlabel="Timestep", ylabel="Relapses",
                   category="Relapses_vs_Cumulative_Relapse", show=True, line = True)

    active_TB_treatments = parse_json_report( output_folder, chart_name, debug)
    mean_relapses = [x * tb_drug_relapse_rate_hiv for x in active_TB_treatments]
    sft.plot_data( relapses[1:len(relapses) - 1], mean_relapses, label1="Relapses",
                   label2="calculated relapses", title="Relapses vs. calculated relapses over Time",
                   xlabel="Timestep", ylabel="Relapses",
                   category="Relapses_actual_vs_calculated", show=True, line=True, overlap=True )

    # Now we've ingested all relevant inputs, let's do analysis

    create_report_file( active_TB_treatments,relapses, tb_drug_relapse_rate_hiv,drug_start_timestep, report_name)

if __name__ == "__main__":
    # execute only if run as a script
    application( "output" )
