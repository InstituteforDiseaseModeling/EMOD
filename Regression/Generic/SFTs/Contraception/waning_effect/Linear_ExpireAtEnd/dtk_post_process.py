import os
if __name__ == '__main__':
    from pathlib import Path
    import sys

    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append(str(Path('../../../shared_embedded_py_scripts').resolve().absolute()))
    sys.path.append(str(Path('../../../shared_embedded_py_scripts/dev').resolve().absolute()))

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_Contraception_WaningEffect_Support as cwe_support

"""
This test is similar to Waning effect distribution MapLinear but has Expire_At_Durability_Map_End set to 1 
to test ticket: https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/4762.

"""

def application(output_folder="output", stdout_filename="test.txt",
                insetchart_filename="InsetChart.json",
                config_filename="config.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("insetchart_filename: " + insetchart_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done(stdout_filename)
    insetchart_df = cwe_support.load_insetchart_to_df(os.path.join(output_folder, insetchart_filename))
    config_params = cwe_support.parse_config_file(config_filename)
    campaign_obj = cwe_support.parse_campaign_file(config_params['Campaign_Filename'])
    cwe_support.create_report_file(insetchart_df, config_params, campaign_obj, report_name, show_plots=True)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-i', '--insetchart', default="InsetChart.json",
                        help="InsetChart json to parse (InsetChart.json)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                insetchart_filename=args.insetchart,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

