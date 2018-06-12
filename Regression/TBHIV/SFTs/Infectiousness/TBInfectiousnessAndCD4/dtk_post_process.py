#!/usr/bin/python

import json
import os.path as path
import dtk_sft

TB_CD4_STRATA_I_S = "TB_CD4_Strata_Infectiousness_Susceptibility"
TB_CD4_S = "TB_CD4_Susceptibility"
TB_CD4_I = "TB_CD4_Infectiousness"
TB_ACTIVE_PS_INF_MULT = "TB_Active_Presymptomatic_Infectivity_Multiplier"
BASE_INF = "Base_Infectivity"
TB_SMEAR_NEG_MULT = "TB_Smear_Negative_Infectivity_Multiplier"


"""
The infectiousness gets modified based on the TB_CD4_Infectiousness, using TB_CD4_Strata_Infectiousness_Susceptibility,
This test can be ran on any config file, assuming there are no drug administrations and everyone is coinfected.
Also, if the cd4 count of the person exceeds the strata's max, currently there is an issue where the multiplier gets
really big (https://github.com/bradwgnr/DtkTrunk/issues/27) and my test catches that.

"""


def tb_cd4_infectiousness_calc(data):
    mod_array = data[0]
    cd4_strata = data[1]
    cd4_count = data[2]
    if cd4_count < cd4_strata[0]:
        return mod_array[0]
    if cd4_count > cd4_strata[-1]:
        return mod_array[-1]
    i = 1
    while cd4_strata[i] < cd4_count:
        i += 1

    m = float(mod_array[i] - mod_array[i-1])/float(cd4_strata[i] - cd4_strata[i-1])
    b = float(mod_array[i] - m * cd4_strata[i])
    return round(m * cd4_count + b, 6)


def load_emod_parameters(config_filename="config.json"):
    """reads config file and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with keys
    """
    cdj = None
    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {TB_CD4_STRATA_I_S: cdj[TB_CD4_STRATA_I_S], TB_CD4_I: cdj[TB_CD4_I], TB_CD4_S: cdj[TB_CD4_S],
                 TB_ACTIVE_PS_INF_MULT: cdj[TB_ACTIVE_PS_INF_MULT], BASE_INF: cdj[BASE_INF],TB_SMEAR_NEG_MULT:cdj[TB_SMEAR_NEG_MULT]}
    return param_obj


def parse_stdout_file(curr_timestep=0, stdout_filename="test.txt", debug=False):
    """
    :param curr_timestep:   first timestep from config
    :param stdout_filename: file to parse (test.txt)
    :param debug:           whether or not we write an additional file that's full of the matched lines
    :return:                an array of filtered lines
    """
    update_infectiousness = "UpdateInfectiousness Individual"
    update_time = "Update(): Time:"
    time = 0
    filtered_lines = []
    with open(stdout_filename) as logfile:
        for line in logfile:
            if update_time in line:
                time += 1
            if update_infectiousness in line:
                new_line = dtk_sft.add_time_stamp(time, line)
                filtered_lines.append(new_line)

    if debug:
        with open("filtered_lines.txt", "w") as outfile:
            outfile.writelines(filtered_lines)

    return filtered_lines


def parse_json_report(start_time=0, output_folder="output", insetchart_name="InsetChart.json", debug=False):
    """creates inset_days structure with "Infectivity" and "Bites" keys

    :param insetchart_name: InsetChart.json file with location (output/InsetChart.json)
    :returns: inset_days structure
    """
    insetchart_path = path.join(output_folder, insetchart_name)
    with open(insetchart_path) as infile:
        icj = json.load(infile)["Channels"]

    prevalence = icj["Infected"]["Data"]
    end_time = start_time + len(prevalence)
    inset_days = {}
    for x in range(start_time, end_time):
        inset_days[x] = x

    if debug:
        with open("inset_days.json", "w") as outfile:
            json.dump(inset_days, outfile, indent=4)

    return inset_days


def create_report_file(data):
    report_name = data[0]
    lines = data[1]
    cd4_strata = data[2]
    mod_array = data[3]
    base_inf = data[4]
    presymp_mult = data[5]
    smear_neg_mult = data[6]

    latent_data_points = 0
    smear_negative_data_points = 0
    smear_positive_data_points = 0
    presymptomatic_data_points = 0
    extrapulmonary_data_points = 0
    success = True
    epsilon = 0.000002
    with open(report_name, "w") as outfile:
        if not lines:
            outfile.write("BAD: No relevant test data found.\n")
            success = False
        for line in lines:
            tot_inf_actual = float(dtk_sft.get_val("total_infectiousness= ", line))
            cd4_count = float(dtk_sft.get_val("CD4count= ", line))
            cd4_mod_actual = float(dtk_sft.get_val("CD4mod= ", line))
            cd4_mod_expected = tb_cd4_infectiousness_calc([mod_array, cd4_strata, cd4_count])
            if "Latent" in line:
                latent_data_points += 1
                if tot_inf_actual != 0:
                    success = False
                    outfile.write("BAD, found Latent infection with total_infectiousness= {} at time= {}. "
                                  "Expected 0. \n".format(tot_inf_actual, dtk_sft.get_val("time= ", line)))
            elif "SmearNegative" in line:
                smear_negative_data_points += 1
                tot_inf_expected = cd4_mod_expected * base_inf * smear_neg_mult
                if abs(tot_inf_expected - tot_inf_actual) > epsilon:
                    success = False
                    outfile.write("BAD, found SmearNegative infection with total_infectiousness= {} at time= {},  "
                                  "Expected {}. \n {} \n ".format(tot_inf_actual, dtk_sft.get_val("time= ", line),
                                                                  tot_inf_expected, line))
            elif "SmearPositive" in line:
                smear_positive_data_points += 1
                tot_inf_expected = cd4_mod_expected * base_inf
                if abs(tot_inf_expected - tot_inf_actual) > epsilon:
                    success = False
                    outfile.write("BAD, found SmearPositive infection with total_infectiousness= {} at time= {},  "
                                  "Expected {}. \n {} \n".format(tot_inf_actual, dtk_sft.get_val("time= ", line),
                                                                 tot_inf_expected, line))
            elif "Presymptomatic" in line:
                presymptomatic_data_points += 1
                tot_inf_expected = cd4_mod_expected * base_inf * presymp_mult
                if abs(tot_inf_expected - tot_inf_actual) > epsilon:
                    success = False
                    outfile.write("BAD, found Presymptomatic infection with total_infectiousness= {} at time= {},  "
                                  "Expected {}. \n {}\n ".format(tot_inf_actual, dtk_sft.get_val("time= ", line),
                                                                 tot_inf_expected, line))
            elif "Extrapulmonary" in line:
                extrapulmonary_data_points += 1
                if tot_inf_actual != 0:
                    success = False
                    outfile.write("BAD, found Extrapulmonary infection with total_infectiousness= {} at time= {}. "
                                  "Should be 0. \n".format(tot_inf_actual, dtk_sft.get_val("time= ", line)))

        outfile.write("Data points for each TBHIV infection state:\nLatent = {} \nPresymptomatic = {} "
                      "\nSmear Negative = {} \nSmear Positive = {} \nExtrapulmonary = {} "
                      "\n".format(latent_data_points, presymptomatic_data_points, smear_negative_data_points,
                                  smear_positive_data_points, extrapulmonary_data_points))
        outfile.write("SUMMARY: Success={0}\n".format(success))


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json",
                insetchart_name="InsetChart.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print( "output_folder: " + output_folder )
        print( "stdout_filename: " + stdout_filename+ "\n" )
        print( "config_filename: " + config_filename + "\n" )
        print( "insetchart_name: " + insetchart_name + "\n" )
        print( "report_name: " + report_name + "\n" )
        print( "debug: " + str(debug) + "\n" )
    dtk_sft.wait_for_done()
    param_obj = load_emod_parameters(config_filename)
    parsed_data = parse_stdout_file()
    inset_days = parse_json_report()
    create_report_file([report_name, parsed_data, param_obj.get(TB_CD4_STRATA_I_S), param_obj.get(TB_CD4_I),
                        param_obj.get(BASE_INF), param_obj.get(TB_ACTIVE_PS_INF_MULT),
                        param_obj.get(TB_SMEAR_NEG_MULT)])


if __name__ == "__main__":
    # execute only if run as a script
    application("output")
