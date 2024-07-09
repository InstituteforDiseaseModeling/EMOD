import json

import dtk_test.dtk_vector_genetics_support as genetics_support
import dtk_test.dtk_sft as dtk_sft

"""
This test distributes hrp2-negative infections to all 1k people and 300 hrp2-positive infections
to 300 of the all 1k people. We then make sure that the MalariaSurveyJSONAnalyzer and BinnedReport reports reflect that. 
MalariaSqlReport.db and SpatialReportMalariaFiltered were manually checked. 
"""


def check_malariasurveyjsonanalyzer(outfile):
    success = True
    report = "MalariaSurveyJSONAnalyzer_HRP2_0.json"
    with open("output/" + report) as msja:
        patient_array = json.load(msja)["patient_array"]
    malaria_status = {"pcr_positive_only": 0, "hrp2_positive_only": 0, "both_positive": 0, "neither_positive": 0}
    for patient in patient_array:
        pcr_parasites = patient["pcr_parasites"][0]
        pfhrp2 = patient["pfhrp2"][0]
        if pcr_parasites > 0 and pfhrp2 > 0:
            malaria_status["both_positive"] += 1
        elif pcr_parasites > 0 and pfhrp2 == 0:
            malaria_status["pcr_positive_only"] += 1
        elif pcr_parasites == 0 and pfhrp2 > 0:
            malaria_status["hrp2_positive_only"] += 1
        else:
            malaria_status["neither_positive"] += 1

    # checking expectations
    if malaria_status["both_positive"] != 300:
        success = False
        outfile.write(f"BAD: {report} - we expected 300 people to test positive for "
                      f"pcr and hrp2 and got {malaria_status['both_positive']} \n ")
    if malaria_status["neither_positive"] > 0:
        success = False
        outfile.write(f"BAD: {report} - we expected 0 people to test negative for "
                      f"pcr and hrp2 and got {malaria_status['neither_positive']} \n ")
    if malaria_status["pcr_positive_only"] != 700:
        success = False
        outfile.write(f"BAD: {report} - we expected 700 people to test positive for "
                      f"pcr and negative for hrp2 and got {malaria_status['pcr_positive_only']} \n ")
    if malaria_status["hrp2_positive_only"] > 0:
        success = False
        outfile.write(f"BAD: {report} - there shouldn't be any hrp2 positive without "
                      f"pcr positive but we got {malaria_status['hrp2_positive_only']} \n ")
    return success


def check_binned_report(outfile):
    success = True
    with open("output/BinnedReport.json", "r") as output:
        output_data = json.load(output)["Channels"]
    day = 20
    index = day - 1
    outbreak_day = 1
    data = "Data"

    infected = "Infected"
    infected_expected = 1000

    new_infections = "New Infections"
    new_infections_expected = 1000

    pcr_parasites_positive = "PCR Parasites Positive"
    pcr_parasites_positive_expected = 1000

    pfhrp2_positive = "PfHRP2 Positive"
    pfhrp2_positive_expected = 300

    # Extraction of data and counting
    actuals = {}
    for channel in [infected, pcr_parasites_positive, pfhrp2_positive]:
        counts = 0
        for age_bin in output_data[channel][data]:
            counts += age_bin[index]
        actuals[channel] = counts

    counts = 0
    for age_bin in output_data[new_infections][data]:
        counts += age_bin[outbreak_day]
    actuals[new_infections] = counts

    # checking numbers against expected
    if actuals[pfhrp2_positive] != pfhrp2_positive_expected:
        success = False
        outfile.write(f"BAD: BinnedReport.json for {pfhrp2_positive} were expecting {pfhrp2_positive_expected}, "
                      f"but got {actuals[pfhrp2_positive]} .\n")
    if actuals[new_infections] != new_infections_expected:
        success = False
        outfile.write(f"BAD: BinnedReport.json for {new_infections} were expecting {new_infections_expected}, "
                      f"but got {actuals[new_infections]} .\n")
    if actuals[pcr_parasites_positive] != pcr_parasites_positive_expected:
        success = False
        outfile.write(
            f"BAD: BinnedReport.json for {pcr_parasites_positive} were expecting {pcr_parasites_positive_expected}, "
            f"but got {actuals[pcr_parasites_positive]} .\n")
    if actuals[infected] != infected_expected:
        success = False
        outfile.write(f"BAD: BinnedReport.json for {infected} were expecting {infected_expected}, "
                      f"but got {actuals[infected]} .\n")
    return success


def create_report_file(report_name):
    success = True
    with open("config.json", "r") as config_file:
        cf = json.load(config_file)
    test_name = cf["parameters"]["Config_Name"]
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {test_name}  \n")

        if not check_binned_report(outfile):
            success = False
        if not check_malariasurveyjsonanalyzer(outfile):
            success = False

        outfile.write(dtk_sft.format_success_msg(success))


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done()
    create_report_file(report_name)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                config_filename=args.config, report_name=args.reportname, debug=args.debug)
