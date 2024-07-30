import os
if __name__ == '__main__':
    from pathlib import Path
    import sys

    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append(str(Path('../../../shared_embedded_py_scripts').resolve().absolute()))
    sys.path.append(str(Path('../../../shared_embedded_py_scripts/dev').resolve().absolute()))

from dtk_test.dtk_INDDRM_Support import INDDRMTest
from dtk_test.dtk_sft_class import arg_parser
"""
IndividualNonDiseaseDeathRateModifier
This test is testing the new campaign intervention: IndividualNonDiseaseDeathRateModifier

Modifier of the IndividualNonDiseaseDeathRateModifier intervention during the usage duration distribution in the campaign.json
files are set to be the following:
"Intervention_Config": {
                    "class": "IndividualNonDiseaseDeathRateModifier",
                    "Cost_To_Consumer": 1,
                    "Duration_To_Modifier" : {
                        "Times" : [ 0.0 ],
                        "Values": [ 0.0 ]
                    },
                    "Expiration_Duration_Distribution": "CONSTANT_DISTRIBUTION",
                    "Expiration_Duration_Constant": 200,
                    "Expiration_Event": "Stopped_Death_Modification"
                }
    So the NonDiseaseDeathRate rate is 0 during the usage duration.

Please see the test comment in dtk_test.dtk_INDDRM_Support.py for the things that this set of tests are doing.

"""


class ConstantTest(INDDRMTest):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)


def application(output_folder="output", my_arg=None):
    if not my_arg:
        my_sft = ConstantTest()
    else:
        my_sft = ConstantTest(
            output=my_arg.output, stdout=my_arg.stdout, json_report=my_arg.json_report, event_csv=my_arg.event_csv,
            config=my_arg.config, campaign=my_arg.campaign, report_name=my_arg.report_name, debug=my_arg.debug)
    my_sft.run()


if __name__ == "__main__":
    # execute only if run as a script
    my_arg = arg_parser()
    application(my_arg=my_arg)

