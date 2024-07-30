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
Waning effect distribution
This test is testing the new property of individual: Waning effect distributions in Contraception

Data for test are from:
    1. InsetChart.json:

Here are the things that the test is doing:
    
Steps:
    1. Get parameters: waning effect distribution params, birth rate without contraception, total simulation time
    2. Calculate (and plot) expected efficacy per time step of the contraception based on waning effect params
    3. Use the expected efficacy per time step to calculate birth rate with contraception per time step
    4. Read in InsetChart.json for Channels: 'Currently Pregnant', 'Possible Mothers', 'New Pregnancies'
    5. Total possible mother per time step = Possible Mothers - Currently Pregnant
    6. Use binomial distribution with probability being birth rate with contraception per time step and total number of
       trails being total possible mother per time step to get (and plot) confidence interval of new pregnancies per 
       time step. Also count the number of time steps where new pregnancies is out of the confidence interval
    7. Use poisson distribution to test if the cumulative new pregnancies follows the theoretical birth rate, where the
       theoretical birth rate is calculated as follows:
       1. birth_rate_with_contraception_per_time_step = 
              (1 - expected_efficacy_per_time_step) * birth_rate_without_contraception
       2. theoretical_birth_rate = 
              sum of (total_possible_mother_per_time_step * birth_rate_with_contraception_per_time_step) 
              over the simulation duration
       
Assumptions made on the config/campaign configurations:
    1. Assuming possibility of pregnancy for currently pregnant women is 0
    2.1 Make sure usage_duration_distribution in of the contraceptive is "CONSTANT_DISTRIBUTION"
    2.2 Make sure usage_duration_distribution has "Usage_Duration_Constant" greater than "Simulation_Duration" in config
        By configuring the campaign like this, we can make sure the contraceptive never expires, so that the change in 
        pregnancy rates only come from the change of efficacy of the contraception.
    2.3 Make sure "Box_Duration" in Box and BoxExponential distributions smaller than "Simulation_Duration" in config,
        so that we can see the difference in pregnancy rates before and after the Box_Duration reaches.
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

