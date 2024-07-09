import json
import pandas as pd
import os.path as path
import math
import numpy as np
import collections
import operator
import dtk_test.dtk_sft as dtk_sft
#import matplotlib.pyplot as plt
from dtk_test.dtk_sft import plt
import dtk_test.dtk_STI_Support as sti_s

class Events:
    CHOSEN_AT_RANDOM = 'CHOSEN_AT_RANDOM'
    OLDER_AGE = 'OLDER_AGE'
    YOUNGER_AGE = 'YOUNGER_AGE'
    LONGER_TIME_IN_RELATIONSHIP = 'LONGER_TIME_IN_RELATIONSHIP'
    SHORTER_TIME_IN_RELATIONSHIP = 'SHORTER_TIME_IN_RELATIONSHIP'
    NO_PRIORITIZATION = 'NO_PRIORITIZATION'
    RELATIONSHIP_TYPE = 'RELATIONSHIP_TYPE'


class Constant:
    event_name = "Event_Name"
    campaign_filename = "Campaign_Filename"
    demographics_filenames = "Demographics_Filenames"
    start_day = "Start_Day"
    events = "Events"
    event_coordinator_config = "Event_Coordinator_Config"
    intervention_config = "Intervention_Config"
    minimum_duration_years = "Minimum_Duration_Years"
    maximum_partners = "Maximum_Partners"
    prioritize_partners_by = "Prioritize_Partners_By"
    broadcast_event = "Broadcast_Event"
    base_year = "Base_Year"
    year = "Year"
    age = "Age"
    agebin = "AgeBin"
    individual_id = "Individual_ID"
    relationship_types = "Relationship_Types"
    InterventionForCurrentPartners = "InterventionForCurrentPartners"
    OutbreakIndividual = "OutbreakIndividual"
    MaleCircumcision = "MaleCircumcision"


def load_campaign(campaign_filename, event_index=0):
    with open(campaign_filename, 'r') as campaign_file:
        campaign_obj = json.load(campaign_file)[Constant.events][event_index]
    start_day = campaign_obj[Constant.start_day]
    ic = campaign_obj[Constant.event_coordinator_config][Constant.intervention_config]
    min_duration = ic[Constant.minimum_duration_years]
    max_partners = ic[Constant.maximum_partners]
    prioritize_by = ic[Constant.prioritize_partners_by]

    return int(start_day), float(min_duration), float(max_partners), prioritize_by


def load_campaigns(campaign_filename):
    with open(campaign_filename, 'r') as campaign_file:
        cp_json = json.load(campaign_file)[Constant.events]
    campaign_obj = {}
    for event in cp_json:
        start_day = event[Constant.start_day]
        ic = event[Constant.event_coordinator_config][Constant.intervention_config]
        min_duration = ic[Constant.minimum_duration_years]
        max_partners = ic[Constant.maximum_partners]
        prioritize_by = ic[Constant.prioritize_partners_by]
        broadcast_event = ic[Constant.broadcast_event]
        campaign_obj[start_day] = {Constant.minimum_duration_years : min_duration,
                                   Constant.maximum_partners: max_partners,
                                   Constant.prioritize_partners_by: prioritize_by,
                                   Constant.broadcast_event: broadcast_event}
    return campaign_obj


def load_campaign_relationship_type(campaign_filename):
    start_day, min_duration, max_partners, prioritize_by = load_campaign(campaign_filename)
    if prioritize_by == Events.RELATIONSHIP_TYPE:
        with open(campaign_filename, 'r') as campaign_file:
            campaign_obj = json.load(campaign_file)[Constant.events][0]
        ic = campaign_obj[Constant.event_coordinator_config][Constant.intervention_config]
        relationship_types = ic[Constant.relationship_types]
    else:
        relationship_types = []

    return start_day, min_duration, max_partners, prioritize_by, relationship_types


def create_report_file_relationship_types(output_folder, report_event_recorder, relationship_start_report,
                                          relationship_end_report, config_filename, output_report_name, age_bin, debug,
                                          expected_max_partners=1):
    succeed = True
    with open(output_report_name, 'w') as output_report_file:
        output_report_file.write(f"Config_Name = {dtk_sft.get_config_name(config_filename)}\n")

        # load campaign and test
        succeed, start_day, min_duration, max_partners, prioritize_by, relationship_types = \
            load_and_check_campaign_relationshiptypes(config_filename, succeed, output_report_file, debug,
                                                      expected_max_partners)

        # load pfa overlay file
        relationship_table_male = parse_demo_pfa_file(config_filename, output_report_file, age_bin)

        # load data from RelationshipEnd.csv and test
        parse_and_check_relationship_end_report(output_folder, relationship_end_report, output_report_file)

        # load data from RelationshipStart.csv and test
        relationship_map, male_id = parse_and_check_relationship_start_report(output_folder, relationship_start_report,
                                                                              relationship_types, start_day, age_bin,
                                                                              relationship_table_male,
                                                                              output_report_file)

        # load data from ReportEventRecorder.csv
        succeed, report_df, ids_to_broadcast, ids_to_broadcast_potential = \
            parse_report_event_recorder(output_folder, report_event_recorder, relationship_map, output_report_file,
                                        succeed, relationship_types, max_partners)

        # test expected number of event for each individual
        succeed, report_id_count, expected_counts, counts_upper_bound = \
            test_num_event(report_df, ids_to_broadcast, ids_to_broadcast_potential, succeed, output_report_file, male_id)

        output_report_file.write(dtk_sft.format_success_msg(succeed))

        add_age_plot(report_df)
        add_event_count_plot(report_id_count, expected_counts, counts_upper_bound)

    return succeed


def load_and_check_campaign_relationshiptypes(config_filename, succeed, output_report_file, debug,
                                              expected_max_partners=1):
    """
    :return: succeed, start_day, min_duration, max_partners, prioritize_by, relationship_types
    """
    campaign_filename = dtk_sft.get_config_parameter(config_filename=config_filename,
                                                     parameters=Constant.campaign_filename)[0]
    start_day, min_duration, max_partners, prioritize_by, relationship_types = \
        load_campaign_relationship_type(campaign_filename)
    if max_partners != expected_max_partners:
        output_report_file.write(f"WARNING: {Constant.maximum_partners} is {max_partners} expected {expected_max_partners}, "
                                 f"please check the test.\n")
    if prioritize_by != Events.RELATIONSHIP_TYPE:
        succeed = False
        output_report_file.write(f"BAD: {Constant.prioritize_partners_by} is {prioritize_by} expected "
                                 f"{Events.RELATIONSHIP_TYPE}, "
                                 f"please check the test.\n")
    if min_duration != 0:
        output_report_file.write(f"WARNING: {Constant.minimum_duration_years} is {min_duration} expected 0, "
                                 f"please check the test.\n")
    if not len(relationship_types):
        succeed = False
        output_report_file.write(f"BAD: {Constant.relationship_types} is empty expected "
                                 f"non-empty array of Relationship Types, "
                                 f"please check the test.\n")
    if debug:
        output_report_file.write(f"{Constant.start_day} is {start_day}.\n")
        output_report_file.write(f"{Constant.minimum_duration_years} is {min_duration}.\n")
        output_report_file.write(f"{Constant.maximum_partners} is {max_partners}.\n")
        output_report_file.write(f"{Constant.prioritize_partners_by} is {prioritize_by}.\n")
        output_report_file.write(f"{Constant.relationship_types} is {relationship_types}.\n")

    return succeed, start_day, min_duration, max_partners, prioritize_by, relationship_types


def parse_demo_pfa_file(config_filename, output_report_file, age_bin):
    pfa_filename = dtk_sft.get_config_parameter(config_filename=config_filename,
                                                parameters=Constant.demographics_filenames)[0][-1]
    pfa_obj = sti_s.parse_demo_pfa_file(demo_file_name=pfa_filename)
    relationship_table_male = {}
    for rel_type, pfa_config in pfa_obj.items():
        male_age_bin = pfa_config[sti_s.DemoPfaKeys.year_bt_bin_male]
        if male_age_bin != age_bin:
            output_report_file.write(f"WARNING: {sti_s.DemoPfaKeys.year_bt_bin_male} for {rel_type} should be "
                                     f"{age_bin}, got {male_age_bin}, please fix the test.\n")
        first_age_bin = pfa_config[sti_s.DemoPfaKeys.first_agebin_male]
        relationship_table_male[rel_type] = first_age_bin
    return relationship_table_male


num_relationship_types = 4


def parse_and_check_relationship_start_report(output_folder, relationship_start_report, relationship_types,
                                              start_day, age_bin, relationship_table_male, output_report_file):
    start_df = sti_s.parse_relationship_start_report(report_path=output_folder,
                                                     report_filename=relationship_start_report)

    first_priority = relationship_types[0]
    male_ids, female_ids = [], []
    relationship_map = {}
    for index, row in start_df.iterrows():
        time = row[sti_s.ReportHeader.rel_start_time]
        if time > start_day:
            break
        relationship = int(row[sti_s.ReportHeader.rel_type])
        rel_type = sti_s.relationship_table[relationship]
        if rel_type not in relationship_types:
            # ignore relationships that are not in the priority list
            continue
        m_id = int(row[sti_s.ReportHeader.a_ID])
        f_id = int(row[sti_s.ReportHeader.b_ID])
        if f_id not in relationship_map:
            relationship_map[f_id] = [[] for _ in range(num_relationship_types)]
        relationship_map[f_id][relationship].append(m_id)
        if relationship != sti_s.inv_relationship_table[first_priority]:
            continue
        male_ids.append(m_id)
        female_ids.append(f_id)
        male_age = row[sti_s.ReportHeader.a_age]
        if male_age < relationship_table_male[first_priority] or \
                male_age > relationship_table_male[first_priority] + age_bin:
            # write warning message instead of fail the test, this test is not focus on the Joint_Probabilities
            output_report_file.write(f"WARNING: male {m_id} age {male_age} is in {first_priority} relationship."
                                     f" Something maybe wrong in the pfa Joint_Probabilities matrix.\n")

    output_report_file.write(f"{len(male_ids)} {first_priority} relationships started at day {start_day} with "
                             f"{len(set(male_ids))} male individuals and {len(set(female_ids))} female individuals.\n")
    output_report_file.write(f"{len(start_df)} total relationships started at day {start_day} with "
                             f"{len(start_df[sti_s.ReportHeader.a_ID].unique().tolist())} male individuals and "
                             f"{len(start_df[sti_s.ReportHeader.b_ID].unique().tolist())} female individuals.\n")
    return relationship_map, male_ids


def parse_and_check_relationship_end_report(output_folder, relationship_end_report, output_report_file):
    end_df = pd.read_csv(path.join(output_folder, relationship_end_report))
    if not end_df.empty:
        output_report_file.write(f"WARNING: {relationship_end_report} is not empty, please check your test.\n")


def parse_report_event_recorder(output_folder, report_event_recorder, relationship_map, output_report_file, succeed,
                                relationship_types, max_partner=1):
    """
    :return: succeed, report_df, ids_to_broadcast, ids_to_broadcast_potential
    """
    report_df = pd.read_csv(path.join(output_folder, report_event_recorder))
    expected_event = len(relationship_map) * max_partner
    if isinstance(max_partner, int) or max_partner.is_integer():
        tolerance = 0
    else:
        tolerance = dtk_sft.cal_tolerance_binomial(expected_value=expected_event,
                                                   binomial_p=max_partner - math.floor(max_partner),
                                                   prob=0.01)
    if math.fabs(len(report_df) - expected_event) > tolerance * expected_event:
        succeed = False
        output_report_file.write(
            f"BAD: {len(report_df)} events are broadcast, expected {expected_event} with tolerance = {tolerance}.\n")
    else:
        output_report_file.write(
            f"GOOD: {len(report_df)} events are broadcast, expected {expected_event} with tolerance = {tolerance}.\n")

    ids_to_broadcast = {}
    ids_to_broadcast_potential = {}
    for f_id in relationship_map:
        found = 0
        for relationship in relationship_types:
            relationship_index = sti_s.inv_relationship_table[relationship]
            if len(relationship_map[f_id][relationship_index]) > max_partner - found:
                for id_to_broadcast in relationship_map[f_id][relationship_index]:
                    found += 1
                    if id_to_broadcast in ids_to_broadcast_potential:
                        ids_to_broadcast_potential[id_to_broadcast] += 1
                    else:
                        ids_to_broadcast_potential[id_to_broadcast] = 1
            elif len(relationship_map[f_id][relationship_index]) <= max_partner - found:
                for id_to_broadcast in relationship_map[f_id][relationship_index]:
                    found += 1
                    if id_to_broadcast in ids_to_broadcast:
                        ids_to_broadcast[id_to_broadcast] += 1
                    else:
                        ids_to_broadcast[id_to_broadcast] = 1
            if found >= max_partner:
                break
    return succeed, report_df, ids_to_broadcast, ids_to_broadcast_potential


def test_num_event(report_df, ids_to_broadcast, ids_to_broadcast_potential, succeed, output_report_file, male_id):
    """
    Test if the number of event in report_df match the expected count for each individual
    :param report_df:
    :param ids_to_broadcast:
    :param ids_to_broadcast_potential:
    :param succeed:
    :param output_report_file:
    :param male_id:
    :return:
    """
    report_id_count = report_df[Constant.individual_id].value_counts().to_dict()
    report_id_count = collections.OrderedDict(sorted(report_id_count.items(), key=operator.itemgetter(1)))
    ids_to_broadcast = collections.OrderedDict(sorted(ids_to_broadcast.items(), key=operator.itemgetter(1)))
    if any(id not in report_id_count.keys() for id in ids_to_broadcast.keys()):
        succeed = False
        output_report_file.write(f"BAD: some ids in ids_to_broadcast didn't receive any event.\n")
    else:
        output_report_file.write(f"GOOD: all ids in ids_to_broadcast receive at least one event.\n")

    counts_upper_bound, expected_counts = [], []
    for id, count in report_id_count.items():
        if id not in ids_to_broadcast:
            expected = 0
            if id not in ids_to_broadcast_potential:
                upper_bound = 0
            else:
                upper_bound = ids_to_broadcast_potential[id]
        else:
            expected = ids_to_broadcast.pop(id)
            if id not in ids_to_broadcast_potential:
                upper_bound = 0
            else:
                upper_bound = ids_to_broadcast_potential[id]

        expected_counts.append(expected)
        counts_upper_bound.append(upper_bound)
        if count > expected + upper_bound or count < expected:
            succeed = False
            output_report_file.write(f"BAD: male {id} should receive at least {expected} and at most "
                                     f"{expected + upper_bound} events but he receives {count} events.\n")
    if len(ids_to_broadcast):
        succeed = False
        output_report_file.write(f"BAD: ids in ids_to_broadcast who didn't receive any event are: "
                                 f"{ids_to_broadcast}\n")

    for ind_id in set(male_id):
        if not any(report_df[Constant.individual_id] == ind_id):
            output_report_file.write(f"WARNING: male {ind_id} may receive an event but he does not, "
                                     f"this may be OK.\n")
    if succeed:
        output_report_file.write("GOOD: all male individual who are expected to receive the event have received the"
                                 " exact number of events.\n")
    return succeed, report_id_count, expected_counts, counts_upper_bound


def add_age_plot(report_df):
    # plot age of male individuals who receive the event
    report_df['Age'] = report_df['Age'] / dtk_sft.DAYS_IN_YEAR
    report_df.plot(y='Age', kind='hist')
    plt.title("Age-hist")
    plt.savefig("Age.png")
    if dtk_sft.check_for_plotting():
        plt.show()
    plt.close()


def add_event_count_plot(report_id_count, expected_counts, counts_upper_bound):
    # plot actual and expected event count for each male individual
    fig = plt.figure()
    plt.plot(report_id_count.values(), 'r*', label='actual event count')
    plt.errorbar(x=np.arange(len(expected_counts)), y=expected_counts, yerr=counts_upper_bound, lolims=True,
                 label='expected event count')
    plt.legend()
    # plt.xticks(np.arange(len(expected_counts)), report_id_count.keys(), rotation=45)
    fig.autofmt_xdate()
    plt.savefig('count.png')
    if dtk_sft.check_for_plotting():
        plt.show()
    plt.close()
