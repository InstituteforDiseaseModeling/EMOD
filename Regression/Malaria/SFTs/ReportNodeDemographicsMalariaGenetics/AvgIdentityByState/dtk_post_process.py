import dtk_test.dtk_malaria_support as mal_sup
import dtk_test.dtk_sft as dtk_sft
import os
import pandas as pd

"""
Verifying that AvgIdentityByStateInLastYearOfUniqueGenomes,
Releasing 4 infections - AAAA, ACAA, AGGA, CCAA - their manually calculated Average Identity by State is 0.541667
Making sure this value matching the AvgIdentityByStateInLastYearOfUniqueGenomes column

In this test, identity by state and identity by descent will not be the same value 
(unlike in the AvgIdentityByDescent test), because the same barcode letter
can come from different outbreaks - for example, assigning AAAA - roots 1111, ACAA - 2222, etc
barcode's AAGA parents could be: the first A could have come from infections 1, 2, or 3; second A from infections
1 or 3, G - only from 2, and third A from infections 1, 2, 3, 4 - so the identity by descent and identity by state
will not be equal (unlike in the AvgIdentityByDescent test)

"Var_Gene_Randomness_Type" must be set to "ALL_RANDOM", otherwise calculated average identity by state is different.
"""

expected_average_identity_by_state = 0.541667
avgid_state = "AvgIdentityByStateInLastYearOfUniqueGenomes"
avgid_descent = "AvgIdentityByDescentInLastYearOfUniqueGenomes"


def create_report_file(report_under_test, output_folder, param_obj, report_name, debug):
    success = True
    with open(report_name, "w") as outfile:
        outfile.write(f"Running test: {param_obj['Config_Name']} : \n")
        report_df = pd.read_csv(os.path.join(output_folder, report_under_test))
        # we are only looking at identity by state at the release of the disease,
        # because we know for sure what it will be, hence looking at index = 1
        reported_average_identity_by_state = report_df[avgid_state].values[1]
        if reported_average_identity_by_state != expected_average_identity_by_state:
            success = False
            outfile.write(f"BAD: We were expecting {avgid_state} value in row 2 to be "
                          f"{expected_average_identity_by_state}, "
                          f" but it is {reported_average_identity_by_state}. \n")
        else:
            outfile.write(f"GOOD: We were expecting {avgid_state} value in row 2 to be "
                          f"{expected_average_identity_by_state}, "
                          f" and it is {reported_average_identity_by_state}. \n")
        # Verifying that identity by state is NOT identity by descent as we expect
        reported_average_identity_by_state = report_df[avgid_state]
        reported_average_identity_by_descent = report_df[avgid_descent]
        for i in range(1, len(reported_average_identity_by_descent)):
            if reported_average_identity_by_state.values[i] == reported_average_identity_by_descent.values[i]:
                success = False
                outfile.write(f"BAD: We were expecting {avgid_state} with in row {i + 1} with value "
                              f"{reported_average_identity_by_state.values[i]} to NOT be "
                              f"equal to {avgid_descent} in row {i + 1} with value "
                              f"{reported_average_identity_by_state.values[i]}. It is incredibly "
                              f"unlikely to happen by itself. \n")
        if success:
            outfile.write(f"GOOD: None of the values for {avgid_descent} are the same as for {avgid_state}, "
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
