#!/usr/bin/python

import json
import os.path as path
import dtk_sft
import dtk_TBHIV_Support as dts

matches = [ "truly cleared", "Time:" ]



"""
Requirements description goes here.
This test scenario consists of a population of XXX individuals. 
It starts off by giving everyone (?) TB and HIV. 
Then it distributes Anti-TB drugs with a 50% deactivation rate & 50% relapse 
rate to half the population.
We then observe whether the individuals who receieved the drugs 'achieve' TB 
relapse at a 50% greater rate than the control group.
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

    param_obj[dts.ConfigKeys.DrugParams.KEY_HivCureRate] = \
        codj[dts.ConfigKeys.KEY_DrugParams][drug_type][dts.ConfigKeys.DrugParams.KEY_HivCureRate]
    param_obj[dts.ParamKeys.KEY_DrugStartTime] = test_event[dts.CampaignKeys.Start_Day]
    param_obj[dts.ParamKeys.KEY_DrugRegimenLength] = \
        codj[dts.ConfigKeys.KEY_DrugParams][drug_type][dts.ConfigKeys.DrugParams.KEY_PrimaryDecayConstant]
    param_obj[dts.ConfigKeys.KEY_StartTime] = codj[dts.ConfigKeys.KEY_StartTime]
    return param_obj


def parse_stdout_file( start_timestep, duration_of_interest, stdout_filename="test.txt", debug=False):
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
    cum_clears = []
    clear_times = []
    clear_days = {}
    timestep = 0
    cum = 0
    clears_today = 0
    for line in filtered_lines:
        if "Time:" in line:
            clear_days[timestep] = clears_today
            clears_today = 0
            timestep += 1
            cum_clears.append(cum)
        else:
            individual = line.split()[10]
            clears_today += 1
            cum += 1
            cum_clears[-1] = cum
            if timestep <= duration_of_interest + start_timestep:
                clear_times.append( timestep-start_timestep )
    if debug:
        with open("stdout_cumulative_clears_by_day.txt", "w") as outfile:
            outfile.write(str(cum_clears))
        with open("stdout_clears_by_day.json", "w") as outfile:
            json.dump(clear_days, outfile, indent=4)
    return cum_clears, clear_times, clear_days

def parse_json_report(start_time, output_folder="output", chart_name="InsetChart.json", debug=False):
    """creates inset_days structure with "Infectivity" and "Bites" keys

    :param dts.InsetChart_name: dts.InsetChart.json file with location (output/dts.InsetChart.json)
    :returns: inset_days structure
    """
    chart_path = path.join(output_folder,chart_name)
    with open(chart_path) as infile:
        icj = json.load(infile)[dts.InsetChart.KEY_channels]

    newly_cleared_infections = icj[dts.InsetChart.Channels.KEY_Newly_Cleared_TB_Infections][dts.InsetChart.KEY_data]
    inset_days = {}
    for x in range(start_time, start_time + len(newly_cleared_infections)):
        inset_days[x] = newly_cleared_infections[x]

    if debug:
        with open("inset_days.json", "w") as outfile:
            json.dump(inset_days, outfile, indent=4)

    return inset_days


def create_report_file( clearance_times, param_obj, report_name, stdout_days, inset_days, debug):

    with open(report_name, "w") as outfile:
        outfile.write("Cleared infections: {0} \n".format(len(clearance_times)))

        # success = dtk_sft.test_exponential( numpy.array( clearance_times ).astype(float), param_obj["TB_Drug_Cure_Rate_HIV"] )
        success = dtk_sft.test_exponential(clearance_times,
                                           param_obj["TB_Drug_Cure_Rate_HIV"],
                                           outfile, integers=True, roundup=True, round_nearest=False)

        for day in sorted(inset_days.keys()):
            inset_count = inset_days[day]
            stdout_count = stdout_days[day]
            if inset_count != stdout_count:
                outfile.write("BAD: day {0} std out has {1} clears, inset chart has {2}\n".format(day, stdout_count, inset_count))
                success = False
        outfile.write("SUMMARY: Success={0}\n".format(success))


def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json",
                 chart_name="InsetChart.json",
                 report_name=dtk_sft.sft_output_filename,
                 debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "chart_name: " + chart_name + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )
    dtk_sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename)
    drug_start_timestep = param_obj[dts.ParamKeys.KEY_DrugStartTime]
    sim_start_timestep = param_obj[dts.ConfigKeys.KEY_StartTime]

    # Now process log output (probably) and compare to theory (not in this example) or to another report.
    duration_of_interest = param_obj[dts.ParamKeys.KEY_DrugRegimenLength]
    cum_clears, clear_times, stdout_days = parse_stdout_file(drug_start_timestep, duration_of_interest, stdout_filename, debug)
    inset_days = parse_json_report(sim_start_timestep, debug=debug)

    if debug:
        print( "trying to plot data\n" )
        dtk_sft.plot_data_sorted(cum_clears, label1="Cumulative Clearances",
                          title="Cumulative Clearances over Time from Drugs (HIV+/No ART)", xlabel="Timestep",
                          ylabel="Clearances", show=True)
    else:
        dtk_sft.plot_data( cum_clears, label1="Cumulative Clearances", title="Cumulative Clearances over Time from Drugs (HIV+/No ART)", xlabel="Timestep", ylabel="Clearances" )
    #inset_days = parse_json_report(start_timestep, output_folder, chart_name, debug)

    # Now we've ingested all relevant inputs, let's do analysis
    if debug:
        print( "trying to create report file\n" )
    create_report_file( clear_times, param_obj, report_name, stdout_days, inset_days, debug)

if __name__ == "__main__":
    # execute only if run as a script
    application( "output" )
