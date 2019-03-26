#!/usr/bin/python
# This SFT test the following statements:
# All infections begin in prepatent.
# The proportion of individuals who move to acute infections is determined by the config parameter Config:TSF.
#  The remainder shall move to subclinical.
# All new acute cases and subclinical cases are transited from prepatent state only.

from dtk_test.dtk_Typhoid_SFT_Support import AcuToChr as ATP
from dtk_test.dtk_Typhoid_SFT_Support import ConfigParameters as ConfigParameters
import dtk_test.dtk_sft as sft

def application(output_folder="output", stdout_filename=sft.sft_test_filename,
                 config_filename="config.json",
                 insetchart_name="InsetChart.json",
                 report_name=sft.sft_output_filename,
                 debug=False):
    sft.wait_for_done()
    atp = ATP()
    param_obj = atp.load_emod_parameters(config_filename=config_filename)
    output_df = atp.parse_output_file(output_filename=sft.sft_test_filename,
                                       start_time=param_obj[ConfigParameters.KEY_Start_Time])
    atp.create_report_file(param_obj, output_df, report_name=report_name, age_index_1=2, age_index_2=3)

if __name__ == "__main__":
    # execute only if run as a script
    application("")
