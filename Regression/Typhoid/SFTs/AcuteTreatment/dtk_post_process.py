#!/usr/bin/python

import json
import math
import dtk_test.dtk_sft as sft
import scipy.stats as stats
import os.path as path

KEY_START_TIME = "Start_Time"
UPDATE_TIME = "Update(): Time:"
TO_ACUTE = "Prepatent->Acute"
TREATMENT = "State: Acute, GetTreatment: True"
RECOVERED = "just recovered (from acute)"
CHRONIC = "just went chronic (from acute)"
IND_ID_1 = "Individual="
IND_ID_2 = "Individual ID: "
IND_ID_3 = "Individual "


def load_emod_parameters(config_filename="config.json"):
    """reads config file and populates params_obj
    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_START_TIME
    """
    cdj = None
    param_obj ={}
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
        param_obj[KEY_START_TIME] = cdj[KEY_START_TIME]
    return param_obj


def parse_stdout_file(start_time=1, stdout_filename="test.txt", filtered_filename="filtered_lines.txt", debug=False):
    """ Goes through the file and collects information on daily new acute infections (to verify with InsetChart.json),
    acute infection to treatment times and exceptions (if person goes acute while already acute, or treated after being
    recovered,etc)
    :param start_time:   first time step from config
    :param stdout_filename: file to parse (test.txt)
    :param filtered_filename:   file to parse to (only happens in debug)
    :param debug: whether or not data gets saved to to the external file
    :return:     acute_to_treatment_times, acute_infections_daily, exceptions
    """

    with open(stdout_filename) as infile:
        time_step = start_time
        exceptions = []
        filtered_lines = []
        acute_to_treatment_times = []
        acute_infections = {}
        acute_infections_daily = []
        acute_infections_count_daily = 0
        # no one dies in current application from the infection, we will have to change some things if people start
        # dying. Counting up time steps between individuals going acute to treatment, throwing out individuals
        # who self-cure or go chronic
        for line in infile:
            if UPDATE_TIME in line:
                filtered_lines.append(line)
                # calculate time step, count add daily infection count, reset daily infection count
                time_step += 1
                acute_infections_daily.append(acute_infections_count_daily)
                acute_infections_count_daily = 0
            elif TO_ACUTE in line:
                filtered_lines.append(line)
                acute_infections_count_daily += 1
                ind_id = int(sft.get_val(IND_ID_1, line))
                if ind_id not in acute_infections:
                    acute_infections[ind_id] = time_step
                else:
                    exceptions.append("Individual {} at time {} just became acute while already being "
                                      "acute\n".format(ind_id, time_step))
            elif TREATMENT in line:
                filtered_lines.append(line)
                # when person gets treated, calculate the time from becoming acute to being treated
                ind_id = int(sft.get_val(IND_ID_2, line))
                if ind_id in acute_infections:
                    acute_to_treatment_times.append(time_step - acute_infections.get(ind_id))
                else:
                    exceptions.append(
                        "Individual {} at time {} got treatment without being acute\n".format(ind_id, time_step))
            elif RECOVERED in line:
                filtered_lines.append(line)
                # when person is recovered, kick them out of acute_infections dictionary, stop keeping track of them
                ind_id = int(sft.get_val(IND_ID_3, line))
                if ind_id in acute_infections:
                    del acute_infections[ind_id]
                else:
                    exceptions.append(
                        "Individual {} at time {} recovered from acute without being acute\n".format(ind_id, time_step))
            elif CHRONIC in line:
                filtered_lines.append(line)
                # when person goes chronic, kick them out of acute_infections dictionary, stop keeping track of them
                ind_id = int(sft.get_val(IND_ID_3, line))
                if ind_id in acute_infections:
                    del acute_infections[ind_id]
                else:
                    exceptions.append(
                        "Individual {} at time {} just went chronic from acute without "
                        "being acute\n".format(ind_id, time_step))
        if debug:
            with open(filtered_filename, "w") as outfile:
                outfile.writelines(filtered_lines)

    return [acute_to_treatment_times, acute_infections_daily, exceptions]


def parse_json_report(start_time=1, output_folder="output", insetchart_name="InsetChart.json", debug=False):
    """creates inset_days structure with number of new acute infections
    :param insetchart_name: InsetChart.json file with location (output/InsetChart.json)
    :returns: inset_days structure
    """
    insetchart_path = path.join(output_folder, insetchart_name)
    with open(insetchart_path) as infile:
        icj = json.load(infile)["Channels"]
    new_acute_infections = icj["Number of New Acute Infections"]["Data"]
    inset_days = {}
    end_time = start_time + len(new_acute_infections)
    for x in range(start_time, end_time):
        day = {"New_Acute_Infections": new_acute_infections[x]}
        inset_days[x] = day

    if debug:
        with open("inset_days.json", "w") as outfile:
            json.dump(inset_days, outfile, indent=4)

    return inset_days


def create_report_file(data, inset_days, report_name=sft.sft_output_filename):

    acute_to_treatment_times = data[0]
    acute_infections_daily = data[1]
    exceptions = data[2]
    # mu and sigma for treatment times distribution
    MU_TREAT = 2.33219066
    SIGMA_TREAT = 0.5430
    # mu and sigma for acute sickness times distribution for people 30+
    MU_SICK = 1.2584990
    SIGNMA_SICK = 0.7883767
    with open(report_name, "w") as report_file:
        # comparing the data logged to the data new acute infection data in the inset chart.
        # per Jillian's request, Typhoid InsetChart skips the first time step (starts at Time: 2)
        for i in range(1, len(inset_days)+1):
            if not int(inset_days[i-1]["New_Acute_Infections"]) == int(acute_infections_daily[i]):
                report_file.write("At Time: {} InsetChar's New Acute Infections - {} "
                                  "doesn't match logged infections - {}. \n".format(i,
                                  int(inset_days[i-1]["New_Acute_Infections"]), int(acute_infections_daily[i])))
        report_file.write("There were {} data points used for this test.\n".format(len(acute_to_treatment_times)))
        if not len(exceptions) == 0:
            success = False
            outfile.write("BAD: There were {} exceptions in this test, they are as follows:\n".format(len(exceptions)))
            for exception in exceptions:
                outfile.write(exception)
        if len(acute_to_treatment_times) == 0:
            success = False
            report_file.write("BAD: No acute to treatment times were found.\n")
        else:
            theoretical_acute_to_treatment_times = []
            # generating a lognorm distribution with integers for treatment times
            # generating a lognorm distribution for sickness duration and removing combinations where person self-cures
            # to make data closer to what happens in the code, everyone assumed to be over 30
            # (using 35 year olds overlay)
            while len(theoretical_acute_to_treatment_times) <= len(acute_to_treatment_times):
                treatment_time = int(stats.lognorm.rvs(SIGMA_TREAT, loc=0, scale=math.exp(MU_TREAT), size=1))
                cure_time = int(stats.lognorm.rvs(SIGNMA_SICK, loc=0, scale=math.exp(MU_SICK), size=1) * 7)
                if treatment_time <= cure_time:
                    # offset by one because of how our code treats the time, everything is 1 step shorter.
                    theoretical_acute_to_treatment_times.append(int(treatment_time) - 1)

            result = stats.ks_2samp(theoretical_acute_to_treatment_times, acute_to_treatment_times)
            p = sft.get_p_s_from_ksresult(result)['p']
            s = sft.get_p_s_from_ksresult(result)['s']

            critical_value_s = sft.calc_ks_critical_value(len(acute_to_treatment_times))
            p_epsilon = 5e-2

            if p >= p_epsilon or s <= critical_value_s:
                success = True
            else:
                report_file.write("BAD: Either p-value of {} is less than {} and/or s-value of {} is smaller "
                                  "than {}.\n".format(p, p_epsilon, s, critical_value_s))
                success = False

        report_file.write(sft.format_success_msg(success))
        return [acute_to_treatment_times, theoretical_acute_to_treatment_times]


def graph_the_data(actual_data, theoretical_data):

    sft.plot_data(sorted(actual_data), sorted(theoretical_data), label1="Actual", label2="Expected",
                  title="Treatment Times", xlabel="Treatment Occurrences",
                  ylabel="Treatment Time (in days)",
                  category='acute_treatment_times_plot',
                  alpha=0.5,
                  overlap=True
                  )


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json",
                insetchart_name="InsetChart.json",
                report_name=sft.sft_output_filename,
                debug=False):

    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename + "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "insetchart_name: " + insetchart_name + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )
    sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename)

    parsed_data = parse_stdout_file(param_obj[KEY_START_TIME], stdout_filename, debug=debug)
    inset_days = parse_json_report(param_obj[KEY_START_TIME], output_folder, insetchart_name, debug)
    processed_data = create_report_file(parsed_data, inset_days,
                                        report_name=sft.sft_output_filename)
    graph_the_data(processed_data[0], processed_data[1])


if __name__ == "__main__":
    # execute only if run as a script
    application("output")
