#!/usr/bin/python

import re
import json
import math
import dtk_test.dtk_sft as sft

# global variables
treatment_multiplier = None
tai = None
tpri = None
tsri = None
tcri = None


def calc_expected_infectiousness(line, treatment_list):
    """
    calculate the infectiousness based on the state and the params in config.json.
    """
    states = ["state PRE.", "state SUB.", "state CHR.", "state ACU."]
    expected_infectiousness = None
    state = None
    for regex in states:
        match = re.search(regex, line)
        if match:
            if regex == "state PRE.":
                state = "Prepatent"
                expected_infectiousness = tai * tpri
            elif regex == "state SUB.":
                state = "Subclinical"
                expected_infectiousness = tai * tsri
            elif regex == "state CHR.":
                state = "Chronic"
                expected_infectiousness = tai * tcri
            elif regex == "state ACU.":
                state = "Acute"
                expected_infectiousness = tai
                ind_id = int(sft.get_val("Individual ", line))
                if ind_id in treatment_list:
                    expected_infectiousness *= treatment_multiplier
            return {'expected_infectiousness': expected_infectiousness, 'state': state}

    
def application(report_file):
    sft.wait_for_done()

    # get params from config.json
    cdj = json.loads(open("config.json").read())["parameters"]
    global treatment_multiplier
    global tai
    global tpri
    global tsri
    global tcri
    treatment_multiplier = 0.5 # hard-coded inside dtk
    tai = cdj["Typhoid_Acute_Infectiousness"]
    tpri = cdj["Typhoid_Prepatent_Relative_Infectiousness"]
    tsri = cdj["Typhoid_Subclinical_Relative_Infectiousness"]
    tcri = cdj["Typhoid_Chronic_Relative_Infectiousness"]
    start_time = cdj["Start_Time"]

    lines = []
    timestep = start_time
    # collect the following logs and add time step in list lines
    # 00:00:01 [0] [V] [IndividualTyphoid] Individual 9927 depositing 20000.000000 to route contact:
    # (antigen=0, substrain=2) at time 9.000000 in state SUB.
    # 00:00:01 [0] [V] [InfectionTyphoid] Individual ID: 4016, State: Acute, GetTreatment: True.
    with open("test.txt") as logfile:
        for line in logfile:
            if "Update(): Time:" in line:
                # calculate time step
                timestep += 1
            elif "depositing" in line:
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            elif "GetTreatment: True" in line:
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            elif "->Acute" in line:
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)

    success = True
    treatment_list = []
    actual_infectiousness_data = []
    expected_infectiousness_data = []
    with open(sft.sft_output_filename, "w") as report_file:
        if len(lines) == 0:
            success = False
            report_file.write("BAD: Found no data matching test case.\n")
        else:
            pre_count = 0
            acu_count = 0
            sub_count = 0
            chr_count = 0
            for line in lines:
                if "GetTreatment" in line:
                    # collect a list of individual Ids that get treatment
                    ind_id = int(sft.get_val("Individual ID: ", line))
                    treatment_list.append(ind_id)
                elif "state SUS." in line:
                    success = False
                    report_file.write("BAD: Found individual in Susceptible state has infectiousness. "
                                      "See details: {}.\n".format(line))
                elif "->Acute" in line:
                    ind_id = int(sft.get_val("Individual=", line))
                    if ind_id in treatment_list:
                        # remove ind id from treatment list if an individual move to Acute state again.
                        treatment_list.remove(ind_id)
                else:
                    if "state PRE" in line:
                        pre_count += 1
                    elif "state ACU" in line:
                        acu_count += 1
                    elif "state SUB" in line:
                        sub_count += 1
                    elif "state CHR" in line:
                        chr_count += 1
                    # validate infectiousness
                    if "to route contact:" in line:
                        route = "contact"
                    elif "to route environmental:" in line:
                        route = "environment"

                    actual_infectiousness = float(sft.get_val("depositing ", line))
                    actual_infectiousness_data.append(actual_infectiousness)
                    result = calc_expected_infectiousness(line, treatment_list)
                    expected_infectiousness = result['expected_infectiousness']
                    expected_infectiousness_data.append(expected_infectiousness)
                    state = result['state']
                    ind_id = int(sft.get_val("Individual ", line))
                    if math.fabs(actual_infectiousness - expected_infectiousness) > 1e2:
                        success = False
                        report_file.write("BAD: Individual {0} depositing {1} to route {2} in state: {3}. Expected "
                                          "infectiousness: {4}\n".format(ind_id, actual_infectiousness,
                                                                         route, state, expected_infectiousness))
            if pre_count == 0:
                success = False
                report_file.write("BAD: Found no infectiousness data in Prepatent state.\n")
            if acu_count == 0:
                success = False
                report_file.write("BAD: Found no infectiousness data in Acute state.\n")
            if sub_count == 0:
                success = False
                report_file.write("BAD: Found no infectiousness data in SubClinical state.\n")
            if chr_count == 0:
                success = False
                report_file.write("BAD: Found no infectiousness data in Chronic state.\n")

            report_file.write("pre_count = {0}, acu_count = {1}, sub_count = {2}, chr_count = {3}."
                              "\n".format(pre_count, acu_count, sub_count, chr_count))

            report_file.write(sft.format_success_msg(success))

            sft.plot_data(actual_infectiousness_data, expected_infectiousness_data, label1="Actual",
                          label2="Expected", title="Infectiousness \nTAI={0}, TPRI={1}, TSRI={2}, TCRI={3}, "
                                                   "TM={4}".format(
                    tai, tpri, tsri,tcri, treatment_multiplier
                ),
                          xlabel="Occurrence",
                          ylabel="Infectiousness",
                          category='infectiousness',
                          alpha=0.5, overlap=True, sort=True)

if __name__ == "__main__":
    # execute only if run as a script
    application("")
