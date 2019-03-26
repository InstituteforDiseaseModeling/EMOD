#!/usr/bin/python

import json
import dtk_test.dtk_sft as sft
from scipy import stats

KEY_TYPHOID_EXPOSURE_LAMBDA = "Typhoid_Exposure_Lambda"
KEY_SIMULATION_DURATION = "Simulation_Duration"
UPDATE_TIME = "Update(): Time:"
POPULATION = "StatPop: "  # same line as "Update(): Time:"
SUSCEPTIBLE = "being made susceptible"


def inf_calc(age, typhoid_exposure_lambda):
    return float(1.0) - (float(365 * 20 - age) / float(age * typhoid_exposure_lambda + 365 * 20))


def load_emod_parameters(config_filename="config.json"):
    """reads config file and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with KEY_TYPHOID_EXPOSURE_LAMBDA, KEY_SIMULATION_DURATION
    """
    param_obj = {}
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj[KEY_TYPHOID_EXPOSURE_LAMBDA] = cdj[KEY_TYPHOID_EXPOSURE_LAMBDA]
    param_obj[KEY_SIMULATION_DURATION] = cdj[KEY_SIMULATION_DURATION]
    return param_obj


def parse_stdout_file(time_step=0, stdout_filename="test.txt", filtered_filename="filtered_lines.txt", debug=False):
    """ creates a list of total susceptible people daily (each day is new susceptible people + total susceptible
        the day before)
    :param time_step:   first timestep from config
    :param stdout_filename: file to parse (test.txt)
    :param filtered_filename: file to which we write lines of interest when debug = True
    :param debug: flad indicating whether or not we want debug data written out
    :return: a list of total people susceptible each day, total population
    """
    with open(stdout_filename, "r") as infile:
        filtered_lines = []
        total_susceptible_daily = []
        population = 0
        total_susceptible = 0
        for line in infile:
            if UPDATE_TIME in line:
                filtered_lines.append(line)
                total_susceptible_daily.append(total_susceptible)
                time_step += 1
                if population == 0:
                    population = int(sft.get_val(POPULATION, line))
            elif SUSCEPTIBLE in line:
                filtered_lines.append(line)
                total_susceptible += 1

    if debug:
        with open(filtered_filename, "w") as outfile:
            outfile.writelines(filtered_lines)

    return [total_susceptible_daily, population]


def parse_json_report(start_time=1, output_folder="output", insetchart_name="InsetChart.json", debug=False):
    inset_days = {}
    return inset_days


def create_report_file(data):
    total_susceptible_daily = data[0]
    population = data[1]
    simulation_duration = data[2]
    typhoid_exposure_lambda = data[3]
    report_name = data[4]

    # by the end of the sim, everyone should be susceptible, so I'm taking the last data point.
    total_susceptible = total_susceptible_daily[-1]

    theoretical_total_susceptible_daily = []
    # off by one by because newborns are already 1 day old on day 1
    for i in range(1, simulation_duration + 1):
        theoretical_total_susceptible_daily.append(int(population * inf_calc(i, typhoid_exposure_lambda)))

    success = True
    with open(report_name, "w") as report_file:
        if not total_susceptible == population:
            success = False
            report_file.write("Not everyone converted to susceptible! Expecting {}, got {}"
                              "\n".format(population, total_susceptible))
        else:
            # leaving this in case the test fails and you want to look at the data directly, commenting to keep
            # final clean
            # for i in range(0, simulation_duration):
            # report_file.write("data - {}, theory - {} \n".format(total_susceptible_daily[i],
            # theoretical_total_susceptible_daily[i]))

            result = stats.ks_2samp(theoretical_total_susceptible_daily, total_susceptible_daily)
            p = sft.get_p_s_from_ksresult(result)['p']
            s = sft.get_p_s_from_ksresult(result)['s']

            P_EPSILON = 5e-2
            critical_value_s = sft.calc_ks_critical_value(population)
            if p >= P_EPSILON or s <= critical_value_s:
                success = True
            else:
                report_file.write("BAD: Either p-value of {} is less than {} and/or s-value of {} is smaller "
                                  "than {}.\n".format(p, P_EPSILON, s, critical_value_s))
                success = False

        report_file.write(sft.format_success_msg(success))

    return theoretical_total_susceptible_daily


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
    #sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename)

    parsed_data = parse_stdout_file(debug=debug)
    inset_days = parse_json_report()
    parsed_data.extend([param_obj.get(KEY_SIMULATION_DURATION),
                        param_obj.get(KEY_TYPHOID_EXPOSURE_LAMBDA), sft.sft_output_filename])
    theory = create_report_file(parsed_data)
    sft.plot_data_sorted(parsed_data[0], theory, label1="Actual",
                  label2="Expected", title="Percent of Population Susceptible as they age from 0 to 20, TEL={}"
                                           "".format(parsed_data[3]),
                  xlabel="Time",
                  ylabel="Total % Susceptible",
                  category='immunity_newborns',
                         alpha=0.5,overlap=True)


if __name__ == "__main__":
    # execute only if run as a script
    application("output")
