import dtk_test.dtk_compareReportSerialization_Immunity as cSI
import dtk_test.dtk_compareReportSerialization_Summary as cSS
import dtk_test.dtk_sft as sft
import json
import sys, os, json, collections, struct, datetime

def application(output_folder="output",
                config_filename="config.json",
                stdout="test.txt",
                report_name=sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("config_filename: " + config_filename + "\n")
        print("stdout_filename: " + stdout + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    print("Post procesing waiting for done...")
    sft.wait_for_done()
    print("Done waiting")
    
    print( "reading serialized population and generating data" )
    messages = []
    result_immunity, immunity_messages = cSI.GenerateReportDataFromPopSerialization(output_folder, debug=debug)
    result_summary, summary_messages = cSS.GenerateReportDataFromPopSerialization(output_folder, debug=debug)
    messages += summary_messages

    success = True
    messages.append("Beginning compare result immunity:\n")
    if result_immunity["comparison_MSP_mean_by_agebin"] == "False":
        messages.append("BAD: 'Immunity:comparison_MSP_mean_by_agebin' did not compare\n")
        messages += immunity_messages
        success=False
    else:
        messages.append("\tGOOD: MalariaImmunityReport compared to the serialized file.\n")
        messages.append("GOOD: 'Immunity:comparison_MSP_mean_by_agebin' compared successfully.\n")

    messages.append("Beginning compare result summary:\n")
    summary_worked = True
    if result_summary["comparison_PfPRs_2to10"] == "False":
        messages.append("\tBAD: 'Summary:comparison_PfPRs_2to10' did not compare.\n")
        summary_worked = False
    else:
        messages.append("\tGOOD: 'Summary:comparison_PfPRs_2to10' compared successfully.\n")
    
    if result_summary["comparison_annual_EIRs"] == "False":
        messages.append("\tBAD: 'Summary:comparison_annual_EIRs' did not compare.\n")
        summary_worked=False
    else:
        messages.append("\tGOOD: 'Summary:comparison_annual_EIRs' compared successfully.\n")
    
    if result_summary["comparison_duration_no_infection_streak"] == "False":
        messages.append("\tBAD: 'Summary:comparison_duration_no_infection_streak' did not compare.\n")
        summary_worked=False
    else:
        messages.append("\tGOOD: 'Summary:comparison_duration_no_infection_streak' compared successfully.\n")
    
    # if we get here, then the summary report was good
    if summary_worked:
        messages.append("GOOD: MalariaSummaryReport compared to the serialized file.\n")
    else:
        messages.append("BAD: Something went wrong in MalariaSummaryReport comparison, see above.\n")
        success=False

    messages += sft.format_success_msg(success)
    
    with open(report_name, 'w') as report_file:
        report_file.writelines(messages)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output,
                stdout=args.stdout,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)
