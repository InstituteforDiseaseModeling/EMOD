import dtk_test.dtk_malaria_support as mal_sup
import dtk_test.dtk_sft as dtk_sft
import os
import pandas as pd

"""
Verifying that AvgIdentityByDescentInLastYearOfUniqueGenomes,
Releasing 3 infections, AAAAA and CCCCC and GGGGG - because each infection has 0 identity by state to others, 
their descendents will have identity by state and identity by descent be equal. 
The identity by descent is tracked by assigning unique ids to roots, so for AAAAA - it looks like roots 11111,
for CCCCC, it looks like roots 22222, and GGGGG - it looks like roots 33333 - so as the allels get mixed up, 
the letter will continue to correspond to its root. So roots combination of 11321 will look like AAGCA on the barcode
and no other possibility and GGCCC can only have roots 33222.
 
This would not be possible if there were multiple roots that could have the same barcode letter. 
ex:outbreaks AAACC with roots 11111 and AAGGG with roots 22222, looking at barcode AAGCC - the first two As
could be coming from infection 1 or 2 in each allele and you can't tell by looking at the letters, 
whereas in the example above, an A could only come from infection/root 1. 

"Var_Gene_Randomness_Type" must be set to "ALL_RANDOM", otherwise calculated average identity by state is different. 

We have verified that Average Identity by State is correct in 
Malaria::SFTs::ReportNodeDemographicsMalariaGenetics:AvgIdentityByState test, therefore, 
we just need to compare identity by state to identity by descent in the report to verify that identity by descent is
correct
"""

avgid_descent = "AvgIdentityByDescentInLastYearOfUniqueGenomes"
avgid_state = "AvgIdentityByStateInLastYearOfUniqueGenomes"


def create_report_file(report_under_test, output_folder, param_obj, report_name, debug):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj['Config_Name']} : \n")
        report_df = pd.read_csv(os.path.join(output_folder, report_under_test))
        reported_average_identity_by_state = report_df[avgid_state]
        reported_average_identity_by_descent = report_df[avgid_descent]
        for i in range(len(reported_average_identity_by_descent)):
            if reported_average_identity_by_state.values[i] != reported_average_identity_by_descent.values[i]:
                success = False
                outfile.write(f"BAD: We were expecting {avgid_descent} with in row {i+1} to be "
                              f"{reported_average_identity_by_state.values[i]},  "
                              f" but it is {reported_average_identity_by_descent.values[i]}. \n")
        if success:
            outfile.write(f"GOOD: All the values for {avgid_descent} are the same as for {avgid_state}, "
                          f"which is what we were expecting with our setup. \n")
        outfile.write(dtk_sft.format_success_msg(success))


def application(output_folder="output", stdout_filename="test.txt",
                config_filename="config.json",
                report_name=dtk_sft.sft_output_filename,
                report_under_test="ReportNodeDemographicsMalariaGenetics.csv",
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("stdout_filename: " + stdout_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done()
    param_obj = mal_sup.load_genetics_parameters(config_filename, debug)
    create_report_file(report_under_test, output_folder, param_obj, report_name, debug)


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
