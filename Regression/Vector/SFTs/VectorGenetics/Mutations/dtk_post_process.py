#!/usr/bin/python

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_Vector_Support as veds
import pandas as pd
import os

"""
This is the test that the genes mutate with correct probability as marked in config file. 


"""


def create_biting_report_file(param_obj,output_folder, genetics_report, report_name, debug=False):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Test name: {str(param_obj[veds.ConfigKeys.CONFIG_NAME])} \n"
                      f"Run number: {str(param_obj[veds.ConfigKeys.RUN_NUMBER])} \n")
        rsv_path = os.path.join(output_folder, genetics_report)
        genetics_df = pd.read_csv(rsv_path)
        days = 12
        egg = "STATE_EGG"
        a_mutation = 0.01
        b_mutation = 0.03
        error_tolerance = 0.001
        for day in range(days):
            today = genetics_df[genetics_df["Time"] == day]
            a0 = today[today['Alleles'] == 'a0'][egg].iloc[0]
            a1 = today[today['Alleles'] == 'a1'][egg].iloc[0]
            a3 = today[today['Alleles'] == 'a3'][egg].iloc[0]
            b0 = today[today['Alleles'] == 'b0'][egg].iloc[0]
            b1 = today[today['Alleles'] == 'b1'][egg].iloc[0]
            b2 = today[today['Alleles'] == 'b2'][egg].iloc[0]
            if b1 != 0 or a1 != 0:
                success = False
                outfile.write(f"BAD: We shouldn't have any b1 or a1 alleles, but there are. Please check.\n")
            if abs(a0/(a3+a0) - a_mutation) > error_tolerance:
                success = False
                outfile.write(f"BAD: We are expecting {a_mutation} of a0 out of total eggs, but we got {a0/(a3+a0)}, "
                              f"which is not within allowable error of {error_tolerance}. Please check.\n")
            else:
                outfile.write(f"GOOD: We are expecting about {a_mutation} of a0 out of total eggs, and we got it - "
                              f"{a0/(a3+a0)}.\n")
            if abs(b0/(b2+b0) - b_mutation) > error_tolerance:
                success = False
                outfile.write(f"BAD: We are expecting {b_mutation} of a0 out of total eggs, and we got {b0/(b2+b0)}, "
                              f"which is not within allowable error of {error_tolerance}. Please check.\n")
            else:
                outfile.write(f"GOOD: We are expecting about {b_mutation} of b0 out of total eggs, and we got it - "
                              f"{a0/(a3+a0)}.\n")

        outfile.write(dtk_sft.format_success_msg(success))


def application(output_folder="output",
                config_filename="config.json",
                genetics_report="ReportVectorGenetics_TestVector_ALLELE_FREQ.csv",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    dtk_sft.wait_for_done()
    param_obj = veds.load_emod_parameters(config_filename=config_filename)
    create_biting_report_file(param_obj=param_obj, output_folder=output_folder, genetics_report=genetics_report, report_name=report_name, debug=debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-gr', '--genticsreport', default="ReportVectorGenetics_TestVector_ALLELE_FREQ.csv", help="Genetics report")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application(output_folder=args.output, config_filename=args.config,
                report_name=args.reportname, debug=args.debug)
