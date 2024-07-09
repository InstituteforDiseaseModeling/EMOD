import dtk_test.dtk_sft as dtk_sft
import json
import matplotlib
from sys import platform
if platform == "linux" or platform == "linux2":
    print('Linux OS. Using non-interactive Agg backend')
    matplotlib.use('Agg')
from dtk_test.dtk_sft_class import SFT
import dtk_test.dtk_sft as dtk_sft
from dtk_test.dtk_OutputFile import ReportEventRecorder
import numpy as np
import json

"""
IndividualNonDiseaseDeathRateModifier
This test is testing the Expiration_Duration_Distribution in new campaign intervention: IndividualNonDiseaseDeathRateModifier
There is another test for the Duration_To_Modifier, please see test comment in this SFT: 
/Generic/SFTs/IndividualNonDiseaseDeathRateModifier/Duration_To_Modifier

Data for test are from:
    1. ReportEventRecorder.csv:
        Events: 
            1. NonDiseaseDeaths event.
            2. An event that is broadcast when intervention expires ("Stopped_Death_Modification")
            
Here are the things that the test is doing:
    The Duration_To_Modifier of intervention was set to 0 during the whole intervention period. In this test we are focus
    on testing the Usage_Duration_Distribution: including Constant, Gaussian, Exponential

    Steps to prepare the simulation for testing: 
        1. Use the campaign to distribute IndividualNonDiseaseDeathRateModifier to everyone
        2. run simulation and get time in between start day to Stopped_Death_Modification in ReportEventRecorder per individual

    Things the test check:
        1. Expiration_Event raised:
            1.1. Check if everyone will raise Expiration_Event
            1.2. Check if Expiration_Event only raise once
        2. intervention in effect:
            2.1 Check if there are non-disease deaths before and after the intervention
            2.2 Check if there is no non-disease death during the intervention 
        3. Usage_Duration_Distribution:    
            3.1. check if usage durations follows a certain distribution (ks/chi-square tests)
"""


class Channels:
    StatPop = "Statistical Population"


class INDDRMTest(SFT):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.json_report_name = "InsetChart.json"

    @staticmethod
    def get_expiration_duration_dist_param(inv_config):
        """
        Based on the distribution under test, return the corresponding parameters as follow:
        CONSTANT_DISTRIBUTION -- return: CONSTANT_DISTRIBUTION, Usage_Duration_Constant, Expiration_Event

        GAUSSIAN_DISTRIBUTION -- return: GAUSSIAN_DISTRIBUTION, (Usage_Duration_Gaussian_Mean,
                                                                 Usage_Duration_Gaussian_Std_Dev), Expiration_Event

        EXPONENTIAL_DISTRIBUTION -- return: EXPONENTIAL_DISTRIBUTION, Usage_Duration_Exponential, Expiration_Event
        Args:
            inv_config ():

        Returns:

        """

        dist = inv_config['Expiration_Duration_Distribution']

        if dist == 'CONSTANT_DISTRIBUTION':
            return 'CONSTANT_DISTRIBUTION', [inv_config['Expiration_Duration_Constant']], inv_config['Expiration_Event']
        elif dist == 'GAUSSIAN_DISTRIBUTION':
            return 'GAUSSIAN_DISTRIBUTION', [inv_config['Expiration_Duration_Gaussian_Mean'],
                                             inv_config['Expiration_Duration_Gaussian_Std_Dev']], inv_config[
                       'Expiration_Event']
        elif dist == 'EXPONENTIAL_DISTRIBUTION':
            return 'EXPONENTIAL_DISTRIBUTION', [inv_config['Expiration_Duration_Exponential']], inv_config[
                'Expiration_Event']
        else:
            raise ValueError(f'Distribution {dist} not matched any of these distributions: CONSTANT_DISTRIBUTION, '
                             f'GAUSSIAN_DISTRIBUTION, EXPONENTIAL_DISTRIBUTION.')

    @staticmethod
    def test_dist(start_day, list_time_event_raised, dist, list_param, output_report_file=None):
        list_time_event_raised = [x - start_day for x in list_time_event_raised]
        if dist == 'CONSTANT_DISTRIBUTION':
            return all(x == list_param[0] for x in list_time_event_raised)
        elif dist == 'GAUSSIAN_DISTRIBUTION':
            return dtk_sft.test_gaussian_chisquare(list_time_event_raised, list_param[0], list_param[1],
                                                   output_report_file)
        elif dist == 'EXPONENTIAL_DISTRIBUTION':
            return dtk_sft.test_exponential(list_time_event_raised, 1 / list_param[0], output_report_file,
                                            integers=True, roundup=True)
        else:
            raise ValueError(f'Distribution {dist} not matched any of these distributions: CONSTANT_DISTRIBUTION, '
                             f'GAUSSIAN_DISTRIBUTION, EXPONENTIAL_DISTRIBUTION.')

    def parse_campaign(self):
        campaign_obj = {}
        # Consider: If you are using event 0 for something interesting, you could load that here too.
        with open(self.campaign_filename) as infile:
            campaign_json = json.load(infile)
            event_intervention = campaign_json["Events"][0]
            intervention_config = event_intervention['Event_Coordinator_Config']['Intervention_Config']
            dist, list_param, event = self.get_expiration_duration_dist_param(intervention_config)
            campaign_obj['duration'] = {}
            campaign_obj['duration']['distribution'] = dist
            campaign_obj['duration']['list_param'] = list_param
            campaign_obj['expiration_event'] = event
            campaign_obj['start_day'] = event_intervention['Start_Day']
        return campaign_obj

    # overwrite the test method
    def test(self):
        self.parse_json_report(channel_names=[Channels.StatPop])
        inset_chart_df = self.json_report.df

        self.parse_report_event_recorder()
        event_df = self.csv.df

        self.msg.append(f"Testing {self.event_report_name}: \n")

        # check whether Expiration_Event is raised for each individual
        # count unique number of IDs
        campaign_obj = self.parse_campaign()
        expiration_event = campaign_obj['expiration_event']
        list_event_raised = event_df.loc[event_df[ReportEventRecorder.Column.Event_Name.name] == expiration_event][
            ReportEventRecorder.Column.Individual_ID.name]

        count_event_raised = len(list_event_raised)
        unique_count_event_raised = len(np.unique(list_event_raised))
        self.msg.append(
            f'Number of Expiration_Event ({expiration_event}) raised: {count_event_raised}.\n')
        # check if Expiration_Event raised only once per person
        if count_event_raised != unique_count_event_raised:
            self.success = False
            self.msg.append(f"BAD: duplicate Expiration_Event raised for the same individual.\n")
            self.msg.append(f'BAD: Number of unique Expiration_Event raised: {unique_count_event_raised}, while total '
                            f'number of Expiration_Event raised: {count_event_raised}.\n')
        else:
            self.msg.append(
                f"GOOD: No duplicate Expiration_Event raised for the same individual.\n")
        # check if Expiration_Event raised for everyone
        count_pop = inset_chart_df[Channels.StatPop].iloc[campaign_obj['start_day']]  # intervention happens at day 50

        if count_event_raised != count_pop:
            self.success = False
            self.msg.append(
                f"BAD: Expiration_Event not raised for some people who get the intervention.\n")
        else:
            self.msg.append(f"GOOD: Expiration_Event raised for people who get the intervention.\n")
        # time of event raised (assuming Usage_Expiration_Event raised only once for each female)
        list_time_event_raised = event_df.loc[event_df[ReportEventRecorder.Column.Event_Name.name] == expiration_event][
            ReportEventRecorder.Column.Time.name].values
        # once duration expires, should have some non-disease death
        ind_ids = event_df[event_df[ReportEventRecorder.Column.Event_Name.name] ==
                           ReportEventRecorder.Event.NonDiseaseDeaths.name][
            ReportEventRecorder.Column.Individual_ID.name]
        if len(ind_ids) > 0:
            self.msg.append(f"GOOD: There are non-disease death over the whole period of simulation.\n")
        else:
            self.success = False
            self.msg.append(f"BAD: There are no non-disease death over the whole period of simulation.\n")
        # check Expiration_Duration_Distribution
        dist = campaign_obj['duration']['distribution']
        list_param = campaign_obj['duration']['list_param']
        start_day = campaign_obj['start_day']
        with open('debug_dist_test.txt', 'w') as output_report_file:
            result = self.test_dist(start_day, list_time_event_raised, dist, list_param, output_report_file)
        if not result:
            self.success = False
            self.msg.append(f"BAD: Expiration_Duration_Distribution has some problem.\n")
        else:
            self.msg.append(f"GOOD: Expiration_Duration_Distribution looks good.\n")

