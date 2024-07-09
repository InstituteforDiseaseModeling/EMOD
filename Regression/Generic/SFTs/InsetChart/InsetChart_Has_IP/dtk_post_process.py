import os

if __name__ == '__main__':
    from pathlib import Path
    import sys

    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append(str(Path('../../../shared_embedded_py_scripts').resolve().absolute()))
    sys.path.append(str(Path('../../../shared_embedded_py_scripts/dev').resolve().absolute()))

from dtk_test.dtk_sft_class import SFT, arg_parser
import dtk_test.dtk_sft as dtk_sft
from dtk_test.dtk_General_Support import ConfigKeys


"""
This test is testing the InsetChat.json channels HasIP_*, making sure the proportions of the population are correct
and that IPs not asked for are not listed. 

    Data collected: 
        1. InsetChat.json
        
        
IPs collected: ["Health", "FiftyFifty"]
Health: "Healthy", "Ok", "Sick"
FiftyFifty:  "OneHalf", "OtherHalf"

"""

dict_expected = {"HasIP_FiftyFifty:OneHalf": 0.5, "HasIP_FiftyFifty:OtherHalf": 0.5, "HasIP_Health:Healthy": 0.4,
                 "HasIP_Health:Ok": 0.5, "HasIP_Health:Sick": 0.1}

class InsetChartTest(SFT):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.params_keys = [ConfigKeys.Simulation_Duration,
                            ConfigKeys.Demographics_Filenames,
                            ConfigKeys.Config_Name]

    def load_config(self):
        super(InsetChartTest, self).load_config(params_keys=self.params_keys)

    @staticmethod
    def write_result(self, success, test_channel_name, expected_value, actual_value):
        if not success:
            self.msg.append(
                f"BAD: We expected {test_channel_name} to have proportion of population very close to "
                f"{expected_value}, but it is {actual_value}.\n")
        else:
            self.msg.append(f"GOOD: {test_channel_name} value of {actual_value} does match "
                            f" the expected value of  {expected_value}.\n")

    # overwrite the test method
    def test(self):
        self.parse_json_report(channel_names=list(dict_expected.keys()))
        inset_chart_df = self.json_report.df
        tolerance = 0.01
        for channel in dict_expected.keys():
            actual_proportions = round(inset_chart_df[channel][0], 2)
            expected_proportions = dict_expected[channel]
            if abs(actual_proportions - expected_proportions) < tolerance:
                self.write_result(self, True, channel, expected_proportions, actual_proportions)
            else:
                self.success = False
                self.write_result(self, False, channel, expected_proportions, actual_proportions)


def application(output_folder="output", my_arg=None):
    if not my_arg:
        my_sft = InsetChartTest()
    else:
        my_sft = InsetChartTest(
            output=my_arg.output, stdout=my_arg.stdout, json_report=my_arg.json_report, event_csv=my_arg.event_csv,
            config=my_arg.config, campaign=my_arg.campaign, report_name=my_arg.report_name, debug=my_arg.debug)
    my_sft.run()


if __name__ == "__main__":
    # execute only if run as a script
    my_arg = arg_parser()
    application(my_arg=my_arg)
