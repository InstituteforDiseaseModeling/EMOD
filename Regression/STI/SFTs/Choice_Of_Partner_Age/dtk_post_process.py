#!/usr/bin/python

if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../shared_embedded_py_scripts').resolve().absolute()) )

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_STI_Support as sti_support
import json
import matplotlib.pyplot as plt
import numpy as np
"""
This test is developed from STI scenario: 01_STI_Network\C_Choice_Of_Partner_Age.

STI model allows user to configure the mixing patterns in the population by prescribing the proportion of relationship
that should occur between different age of partners. The pattern are configured in the demographics file. In this test,
it's defined in pfa_transitory_2x2.json. Values being tested here are:

"Pair_Formation_Parameters" :
                {
                    "Number_Age_Bins_Male"   : 2,
                    "Number_Age_Bins_Female" : 2,
                    "Age_of_First_Bin_Edge_Male"   : 40,
                    "Age_of_First_Bin_Edge_Female" : 40,
                    "Years_Between_Bin_Edges_Male"   : 25,
                    "Years_Between_Bin_Edges_Female" : 25,
                    "Joint_Probabilities" :
                    [
                        [ 0.7, 0.0],
                        [ 0.1, 0.2]
                    ]
                }
                

In this test, we read the configurations from pfa_transitory_2x2.json and load data from output files: 
RelationshipStart.csv, perform the following tests:

1. In this test, we only test a simpler 2x2 joint probabilities matrix. It first make sure it's configured correctly.
2. It compares the actual relationship count for each male-female agebin with expected value.(Note: It's hard to find 
a good tolerance here, when there is no enough people in the relationship queue, the actual value could have a large gap
compared to expected value. In this test, we have "Formation_Rate_Constant": 0.001 try to avoid this situation)

Output: scientific_feature_report.txt
        TRANSITORY.png
        INFORMAL.png(if there is any)
        MARITAL.png(if there is any)
        wrong_rel_{agebin_string}_{rel_name}.csv(if test failed in step 2)

"""


def parse_demo_file(demo_file_name="../../../../STI/SFTs/Inputs/pfa_transitory_2x2.json"):
    """
    Parse demo file and get Joint_Probabilities for transitory, informal and
    marital relationships.
    :param demo_file_name:
    :return: demo_object
    """
    with open(demo_file_name, 'r') as demo_file:
        society = json.load(demo_file)[sti_support.DemoPfaKeys.defaults][sti_support.DemoPfaKeys.society]
        demo_object = {}
        for rel_name in sti_support.inv_relationship_table:
            pfa_params = society[rel_name][sti_support.DemoPfaKeys.pair_formation_parameters]
            param = {sti_support.DemoPfaKeys.joint_probabilities:
                         pfa_params[sti_support.DemoPfaKeys.joint_probabilities],
                     sti_support.DemoPfaKeys.first_agebin_male:
                         pfa_params[sti_support.DemoPfaKeys.first_agebin_male],
                     sti_support.DemoPfaKeys.first_agebin_female:
                         pfa_params[sti_support.DemoPfaKeys.first_agebin_female],
                     sti_support.DemoPfaKeys.num_agebin_male:
                         pfa_params[sti_support.DemoPfaKeys.num_agebin_male],
                     sti_support.DemoPfaKeys.num_agebin_female:
                         pfa_params[sti_support.DemoPfaKeys.num_agebin_female]
                     }
            demo_object[rel_name] = param

    return demo_object


def autolabel(ax, rects):
    """
    Attach a text label above each bar displaying its height
    """
    for rect in rects:
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width()/2., 1.005*height,
                '%d' % int(height),
                ha='center', va='bottom')


def create_report_file(start_df, demo_object, start_report_name, output_report_name, debug):
    succeed = True
    with open(output_report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name()}\n")
        for rel_type, rel_name in sti_support.relationship_table.items():
            if start_df[start_df[sti_support.ReportHeader.rel_type] == rel_type].empty:
                # the reporter is tested in another SFT, we assume this reporter is working fine here.
                output_report_file.write(f"There is no relationship for {rel_name}, skip it.\n")
                continue

            output_report_file.write(f"Checking if actual relationship counts match "
                                     f"{sti_support.DemoPfaKeys.joint_probabilities} for {rel_name}:\n")

            num_agebin_male = demo_object[rel_name][sti_support.DemoPfaKeys.num_agebin_male]
            num_agebin_female = demo_object[rel_name][sti_support.DemoPfaKeys.num_agebin_female]
            if num_agebin_male != 2 or num_agebin_female != 2:
                succeed = False
                output_report_file.write(f"\tBAD: the test assuming there are two agebin for both genders in "
                                         f"{sti_support.DemoPfaKeys.pair_formation_parameters}, but we got {num_agebin_female} for"
                                         f"female and {num_agebin_male} for male, please check your demo file.\n")
                continue

            # turn male and female age into two agebins based on the age of the age bin defined in demo file.
            first_agebin_male = demo_object[rel_name][sti_support.DemoPfaKeys.first_agebin_male]
            first_agebin_female = demo_object[rel_name][sti_support.DemoPfaKeys.first_agebin_female]
            # for example, if first_agebin_male = 40, columns are <40 and 40+
            start_df[sti_support.ReportHeader.a_agebin] = start_df[sti_support.ReportHeader.a_age].apply(
                lambda x: f"<{first_agebin_male}" if x < first_agebin_male else f"{first_agebin_male}+")
            start_df[sti_support.ReportHeader.b_agebin] = start_df[sti_support.ReportHeader.b_age].apply(
                lambda x: f"<{first_agebin_female}" if x < first_agebin_female else f"{first_agebin_female}+")

            joint_probabilities = demo_object[rel_name][sti_support.DemoPfaKeys.joint_probabilities]
            if np.shape(joint_probabilities) != (2, 2):
                succeed = False
                output_report_file.write(f"\tBAD: the {sti_support.DemoPfaKeys.joint_probabilities} in "
                                         f"{sti_support.DemoPfaKeys.pair_formation_parameters} should be a 2 * 2 array, but we got "
                                         f"shape({sti_support.DemoPfaKeys.joint_probabilities}) = {np.shape(joint_probabilities)}.\n")
                continue

            # groupby male and female agebin for each relationship and get the count for each bins.
            df_c = start_df[start_df[sti_support.ReportHeader.rel_type] == rel_type]. \
                groupby([sti_support.ReportHeader.a_agebin, sti_support.ReportHeader.b_agebin]). \
                size().reset_index(name=sti_support.ReportHeader.counts)
            # calculate the percentage.
            df_c[sti_support.ReportHeader.percent] = df_c[sti_support.ReportHeader.counts] / df_c[sti_support.ReportHeader.counts].sum()

            expected_agebin_counts = []
            actual_agebin_counts = []
            agebin_name = []
            agebin_catogories = [[f"<{first_agebin_male}", f"{first_agebin_male}+"],
                                 [f"<{first_agebin_female}", f"{first_agebin_female}+"]]
            # compare actual count vs. expected count for each bins
            for i in range(2):
                for j in range(2):
                    expected_agebin_count = joint_probabilities[i][j] * df_c[sti_support.ReportHeader.counts].sum()
                    expected_agebin_counts.append(expected_agebin_count)
                    df_count = df_c[(df_c[sti_support.ReportHeader.a_agebin] == agebin_catogories[0][i]) &
                         (df_c[sti_support.ReportHeader.b_agebin] == agebin_catogories[1][j])][sti_support.ReportHeader.counts]
                    count = df_count.iloc[0] if not df_count.empty else 0
                    actual_agebin_counts.append(count)
                    agebin = f"M{agebin_catogories[0][i]}, F{agebin_catogories[1][j]}"
                    agebin_name.append(agebin)

                    # make sure we get 0 count if expected 0 count
                    if expected_agebin_count == 0:
                        # if we don't get 0 count, save those relationship into csv file.
                        if count != 0:
                            df_wrong = \
                            start_df[(start_df[sti_support.ReportHeader.a_agebin] == agebin_catogories[0][i]) &
                                     (start_df[sti_support.ReportHeader.b_agebin] == agebin_catogories[1][j])]
                            succeed = False
                            agebin_string = agebin.replace("<", "-")
                            wrong_rel_file = f"wrong_rel_{agebin_string}_{rel_name}.csv"
                            output_report_file.write(f"\tBAD: {sti_support.DemoPfaKeys.joint_probabilities} for {agebin} is "
                                                     f"{joint_probabilities[i][j]}, expected 0 relationship but we got "
                                                     f"{count} relationships in {start_report_name}. Please see "
                                                     f"{wrong_rel_file} for details of these {count} relationships.\n")
                            df_wrong.to_csv(wrong_rel_file, header=True, index=False, line_terminator="")
                        else:
                            output_report_file.write(f"\tGOOD: found {count} relationships for {agebin}, expected 0 "
                                                     f"relationships.\n")
                    # make sure actual count is close enough to expected count when it's not 0
                    else:
                        # tolerance is 3% of the total relationships counts.
                        tolerance = 3e-2 * df_c[sti_support.ReportHeader.counts].sum()
                        message = f"found {count} relationships for {agebin}, expected within " \
                                  f"[{expected_agebin_count - tolerance}, " \
                                  f"{expected_agebin_count + tolerance}].\n"
                        if abs(count - expected_agebin_count) > tolerance:
                            succeed = False
                            output_report_file.write("\tBAD: " + message)
                        else:
                            output_report_file.write("\tGOOD: " + message)

            # plot bar chart
            output_report_file.write(f"\tPlotting relationship counts for each agebin groups for {rel_name}.\n")
            fig = plt.figure()
            ax = fig.add_axes([0.12, 0.15, 0.76, 0.76])
            x_ind = np.arange(len(agebin_name))
            width = 0.35
            rectangles1 = ax.bar(x_ind, expected_agebin_counts, width, color="orange")
            rectangles2 = ax.bar(x_ind + width, actual_agebin_counts, width, color="seagreen")
            ax.set_xticks(x_ind + width / 2)
            ax.set_xticklabels(agebin_name)
            ax.set_ylabel('Relationship counst')
            ax.set_title(f'{rel_name}')
            ax.legend((rectangles1[0], rectangles2[0]), ('expected values', 'actual values'))
            autolabel(ax, rectangles1)
            autolabel(ax, rectangles2)
            if dtk_sft.check_for_plotting():
                plt.show()
            fig.savefig(f'{rel_name}.png')
            plt.close(fig)

        output_report_file.write(dtk_sft.format_success_msg(succeed))

        return succeed


def application(output_folder="output", stdout_filename="test.txt",
                start_report_name="RelationshipStart.csv",
                config_filename="config.json", pfa_filename="pfa_transitory_2x2.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("start_report_name: " + start_report_name + "\n")
        print("config_filename: " + config_filename + "\n")
        print("pfa_filename: " + pfa_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done(stdout_filename)

    # demo_filenames = dtk_sft.get_config_parameter(config_filename, ["Demographics_Filenames"])[0]
    start_df = sti_support.parse_relationship_start_report(report_path=output_folder, report_filename=start_report_name)
    demo_obj = parse_demo_file(demo_file_name=pfa_filename)
    create_report_file(start_df, demo_obj, start_report_name, report_name, debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-S', '--start_report', default="RelationshipStart.csv",
                        help="Relationship start report to parse (RelationshipStart.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-p', '--pfa', default="pfa_transitory_2x2.json",
                        help="PFA file name to load (pfa_transitory_2x2.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                start_report_name=args.start_report,
                config_filename=args.config, pfa_filename=args.pfa,
                report_name=args.reportname, debug=args.debug)

