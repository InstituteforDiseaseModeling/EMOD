import dtk_test.dtk_sft as dtk_sft
import json
import matplotlib
from sys import platform
if platform == "linux" or platform == "linux2":
    print('Linux OS. Using non-interactive Agg backend')
    matplotlib.use('Agg')
import numpy as np
import pandas as pd


def load_insetchart_to_df(insetchart_file):
    with open(insetchart_file) as infile:
        icj = json.load(infile)["Channels"]
    data = {}
    for channel in icj.keys():
        channel_name = channel
        chars = "\\/`*_{}[]()>#+-.!$<= "
        for c in chars:
            channel_name = channel_name.replace(c, "")
        # channel_name = channel.replace(" ", "")
        data[channel_name] = icj[channel]["Data"]
    insetchart_pd = pd.DataFrame.from_dict(data)
    return insetchart_pd


def get_dist_param(inv_config):
    dist = inv_config['Usage_Duration_Distribution']

    """
    :param dist: CONSTANT_DISTRIBUTION
    :return: Usage_Duration_Constant

    :param dist: GAUSSIAN_DISTRIBUTION
    :return: Usage_Duration_Gaussian_Mean, Usage_Duration_Gaussian_Std_Dev

    :param dist: EXPONENTIAL_DISTRIBUTION
    :return: Usage_Duration_Exponential
    """

    if dist == 'CONSTANT_DISTRIBUTION':
        return 'CONSTANT_DISTRIBUTION', [inv_config['Usage_Duration_Constant']]
    elif dist == 'GAUSSIAN_DISTRIBUTION':
        return 'GAUSSIAN_DISTRIBUTION', [inv_config['Usage_Duration_Gaussian_Mean'], inv_config['Usage_Duration_Gaussian_Std_Dev']]
    elif dist == 'EXPONENTIAL_DISTRIBUTION':
        return 'EXPONENTIAL_DISTRIBUTION', [inv_config['Usage_Duration_Exponential']]
    else:
        raise ValueError(f'Distribution {dist} not matched any of these distributions: CONSTANT_DISTRIBUTION, '
                         f'GAUSSIAN_DISTRIBUTION, EXPONENTIAL_DISTRIBUTION.')


def check_dist(list_time_event_raised, dist, list_param, output_report_file):
    if dist == 'CONSTANT_DISTRIBUTION':
        return all(x == list_param[0] for x in list_time_event_raised)
    elif dist == 'GAUSSIAN_DISTRIBUTION':
        return dtk_sft.test_gaussian_chisquare(list_time_event_raised, list_param[0], list_param[1], output_report_file)
        # return dtk_sft.test_gaussian(list_time_event_raised, list_param[0], list_param[1], report_file=output_report_file, round=True, integers=True, roundup=True, round_nearest=False)
    elif dist == 'EXPONENTIAL_DISTRIBUTION':
        return dtk_sft.test_exponential(list_time_event_raised, 1/list_param[0], output_report_file, integers=True, roundup=True)
    else:
        raise ValueError(f'Distribution {dist} not matched any of these distributions: CONSTANT_DISTRIBUTION, '
                         f'GAUSSIAN_DISTRIBUTION, EXPONENTIAL_DISTRIBUTION.')


def check_pregnancy_within_duration(id, report_df, usage_expiration_event):
    """
    Check person's pregnancy time was after contraception expired.
    :return: boolean
    """
    rows = report_df.loc[report_df['Individual_ID'] == id]
    time_expire = rows[rows['Event_Name'] == usage_expiration_event]['Time'].values
    time_pregnant = rows[rows['Event_Name'] == 'Pregnant']['Time'].values
    return time_expire <= min(time_pregnant)


def parse_config_file(config_filename='config.json'):
    with open(config_filename) as infile:
        parameters = json.load(infile)['parameters']
    config_params = {}
    config_params['Config_Name'] = parameters['Config_Name']
    config_params['Campaign_Filename'] = parameters['Campaign_Filename']
    return config_params


def parse_campaign_file(campaign_filename="campaign.json"):
    """
    :param campaign_filename: Used for finding campaign name
    :return: campaign dictionary with 'broadcast_name, duration, efficacy, expiration_event' keys
    for contraception intervention
    """
    campaign_obj = {}
    # Consider: If you are using event 0 for something interesting, you could load that here too.
    with open(campaign_filename) as infile:
        campaign_json = json.load(infile)
        event_broadcast = campaign_json["Events"][0]  # TODO: Explain why event 0.
        event_intervention = campaign_json["Events"][1] # TODO: Explain why event 1.
        broadcast_config = event_broadcast['Event_Coordinator_Config']['Intervention_Config']
        campaign_obj['broadcast_name'] = broadcast_config['Broadcast_Event']
        intervention_config = event_intervention['Event_Coordinator_Config']['Intervention_Config']
        dist, list_param = get_dist_param(intervention_config)
        # get waning_effect_const from campaign.json
        waning_effect = intervention_config['Waning_Config']['Initial_Effect']
        waning_distribution = intervention_config['Waning_Config']['class']
        campaign_obj['duration'] = {}
        campaign_obj['duration']['distribution'] = dist
        campaign_obj['duration']['list_param'] = list_param
        campaign_obj['efficacy'] = {}
        campaign_obj['efficacy']['initial_effect'] = waning_effect
        campaign_obj['efficacy']['distribution'] = waning_distribution
        campaign_obj['expiration_event'] = intervention_config['Usage_Expiration_Event']
    return campaign_obj
    pass


def create_report_file(report_df, config_params, campaign_obj, csv_report_name, report_name=dtk_sft.sft_output_filename,
                       debug=False):
    succeed = True
    with open(report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {config_params['Config_Name']}\n")
        output_report_file.write(f"Testing {csv_report_name}: \n")

        # check whether Broadcast_Event and Usage_Expiration_Event is raised for each individual
        # count unique number of female
        broadcast_intervention_name = campaign_obj['broadcast_name']
        list_female = report_df.loc[report_df['Event_Name'] == broadcast_intervention_name][
            'Individual_ID'].values
        # check if Broadcast_Event raised only once per female
        if not len(np.unique(list_female)) == len(list_female):
            unique_broadcast_count_event = len(np.unique(list_female))
            output_report_file.write(f"BAD: duplicate Broadcast_Event raised for the same individual.\n")
            output_report_file.write(f'Number of unique Broadcast_Event raised: {unique_broadcast_count_event}.\n')
        else:
            output_report_file.write(
                f"GOOD: No duplicate Broadcast_Event raised for the same individual.\n")
        # count number of unique females get contraception intervention
        count_female = len(np.unique(list_female))
        output_report_file.write(f'Number of females with contraception intervention distributed: {count_female}.\n')
        # count number of times that Usage_Expiration_Event raised
        usage_expiration_event = campaign_obj['expiration_event']
        list_female_event_raised = report_df.loc[report_df['Event_Name'] == usage_expiration_event][
            'Individual_ID'].values

        count_event_raised = len(list_female_event_raised)
        unique_count_event_raised = len(np.unique(list_female_event_raised))
        output_report_file.write(f'Number of Usage_Expiration_Event ({usage_expiration_event}) raised: {count_event_raised}.\n')
        # check if Usage_Expiration_Event raised only once per female
        if not count_event_raised == unique_count_event_raised:
            output_report_file.write(f"BAD: duplicate Usage_Expiration_Event raised for the same individual.\n")
            output_report_file.write(f'Number of unique Usage_Expiration_Event raised: {unique_count_event_raised}.\n')
        else:
            output_report_file.write(
                f"GOOD: No duplicate Usage_Expiration_Event raised for the same individual.\n")
        # check if Usage_Expiration_Event raised for every female
        if not count_event_raised == count_female:
            output_report_file.write(f"BAD: Usage_Expiration_Event not raised for some females who use contraception.\n")
            list_id_event_not_raised = np.setdiff1d(list_female, list_female_event_raised)
            df = report_df[report_df['Individual_ID'].isin(list_id_event_not_raised)]
            df.to_csv('event_not_raised.csv', mode='a', index=False)
        else:
            output_report_file.write(f"GOOD: Usage_Expiration_Event raised for all females who use contraception.\n")
        # time of event raised (assuming Usage_Expiration_Event raised only once for each female)
        list_time_event_raised = report_df.loc[report_df['Event_Name'] == usage_expiration_event]['Time'].values
        # once duration expires, should have some pregnancy
        list_id_pregnant = report_df.loc[report_df['Event_Name'] == "Pregnant"]['Individual_ID'].values
        if len(list_id_pregnant) > 0:
            output_report_file.write(f"GOOD: There are pregnancies over the whole period of simulation.\n")
        # when waning_effect is constant 1 over the whole usage duration, no one should pregnant before duration expires
        waning_effect = campaign_obj['efficacy']['initial_effect']
        waning_distribution = campaign_obj['efficacy']['distribution']
        if waning_effect == 1 and waning_distribution == 'WaningEffectConstant':
            output_report_file.write(f"Expect: contraception used by all intervened female is Constant distribution "
                                     f"with efficacy being 1.0, so should be no pregnancy before one's usage "
                                     f"duration expires.\n")
            pregnancy_checked = True
            for id in list_id_pregnant:
                if not check_pregnancy_within_duration(id, report_df, usage_expiration_event):
                    pregnancy_checked = False
                    output_report_file.write(f"BAD: {id} pregnant before Constant Usage_Duration_Distribution expires.\n")
            if pregnancy_checked:
                output_report_file.write(f"GOOD: No pregnancy before Constant Usage_Duration_Distribution expires.\n")
        # check Usage_Duration_Distribution
        dist = campaign_obj['duration']['distribution']
        list_param = campaign_obj['duration']['list_param']
        result = check_dist(list_time_event_raised, dist, list_param, output_report_file)
        if not result:
            succeed = False
            output_report_file.write(f"BAD: Usage_Duration_Distribution in {csv_report_name} has some problem.\n")
        else:
            output_report_file.write(f"GOOD: Usage_Duration_Distribution in {csv_report_name} looks good.\n")
        output_report_file.write(dtk_sft.format_success_msg(succeed))
        return succeed