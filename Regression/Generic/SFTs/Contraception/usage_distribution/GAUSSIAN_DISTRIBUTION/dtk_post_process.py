import os
if __name__ == '__main__':
    from pathlib import Path
    import sys

    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append(str(Path('../../../shared_embedded_py_scripts').resolve().absolute()))
    sys.path.append(str(Path('../../../shared_embedded_py_scripts/dev').resolve().absolute()))

import dtk_test.dtk_sft as dtk_sft
import dtk_test.dtk_Contraception_Support as fp_support
import pandas as pd

"""
Contraception
This test is testing the new campaign intervention: Contraception

Waning_Config (efficacy of the contraceptive intervention during the usage duration distribution) in the campaign.json
files are set to be the following:
    "Waning_Config": {
        "Initial_Effect": 1.0,
        "class": "WaningEffectConstant"
    } 
    So the contraception is 100% effective in preventing an individual being pregnant during her usage duration.

Data for test are from:
    1. ReportEventRecorder.csv:
        Events: 
            1. An event that is broadcast to distribute the intervention (the first event in the campaign file)
            2. An event that is broadcast when intervention expires (the second event in the campaign file)
"""

"""
Here are the things that the test is doing:
    Without considering the event pregnant, test Usage_Duration_Distribution: including Constant, Gaussian, 
    Exponential
    
    Steps to prepare the simulation for testing: 
        1. Use the campaign to distribute contraception to all female
        2. run simulation and get time in between start day to use_expiration_event in ReportEventRecorder per individual
    
    Things the test check:
        1. Usage_Expiration_Event raised:
            1.1. Check if all females with contraception intervention will raise Usage_Expiration_Event
            1.2. Check if each female with contraception intervention will raise Usage_Expiration_Event only once
        2. Contraception intervention in effect:
            2.1 Check if there are pregnancies over the whole period of simulation
            2.2 Check if there is no pregnancy before each female's contraception usage duration expires 
            (assuming we set the waning effect of the contraception to be constant 1.0 efficacy)
        3. Usage_Duration_Distribution:    
            3.1. check if usage durations of the females follows a certain distribution (ks/chi-square tests)
"""

def application(output_folder="output", stdout_filename="test.txt",
                csv_report_name="ReportEventRecorder.csv",
                insetchart_filename="InsetChart.json",
                config_filename="config.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder+ "\n")
        print("stdout_filename: " + stdout_filename + "\n")
        print("csv_report_name: " + csv_report_name + "\n")
        print("insetchart_filename: " + insetchart_filename + "\n")
        print("config_filename: " + config_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done(stdout_filename)
    report_df = pd.read_csv(os.path.join(output_folder, csv_report_name))
    config_params = fp_support.parse_config_file(config_filename)
    campaign_obj = fp_support.parse_campaign_file(config_params['Campaign_Filename']) # local method will figure out campaign name
    fp_support.create_report_file(report_df, config_params, campaign_obj, csv_report_name, report_name, debug)

if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-s', '--stdout', default="test.txt", help="Name of stdoutfile to parse (test.txt)")
    parser.add_argument('-e', '--csv_report', default="ReportEventRecorder.csv",
                        help="ReportEventRecorder report to parse (ReportEventRecorder.csv)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-i', '--insetchart', default="InsetChart.json", help="Config name to load (config.json)")
    parser.add_argument('-d', '--debug', help="debug flag", action='store_true')
    args = parser.parse_args()

    application(output_folder=args.output, stdout_filename=args.stdout,
                csv_report_name=args.csv_report,
                insetchart_filename=args.insetchart,
                config_filename=args.config,
                report_name=args.reportname, debug=args.debug)

