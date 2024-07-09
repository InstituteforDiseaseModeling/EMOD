#!/usr/bin/python
from enum import Enum, auto
from functools import partial
from collections import Counter
"""
This is a support library for relationship targeting SFTs
        
"""

class Event:
    using_condoms = "UsingCondoms"
    not_using_condoms = "NotUsingCondoms"
    i_am_risk_high = "IAmRiskHigh"
    my_partner_in_risk_high = "MyPartnerInRiskHigh"
    my_marital_partner_in_risk_high = "MyMaritalPartnerInRiskHigh"
    my_relationship_just_started = "MyRelationshipJustStarted"
    my_relationship_just_ended = "MyRelationshipJustEnded"
    relationship_created = "Relationship_Created"


class GenderStrings:
    female = "F"
    male = "M"
    female_id = "B_ID"
    female_id_end = "female_ID"
    male_id = "A_ID"
    male_id_end = "male_ID"


class Constant:
    rel_actual_end_time = "Rel_actual_end_time"
    rel_start_time = "Rel_start_time"
    rel_id = "Rel_ID"
    is_rel_outside_pfa = "Is_rel_outside_PFA"
    event_name = "Event_Name"
    ind = "Individual_ID"
    gender = "Gender"
    transitory_type = 0
    informal_type = 1
    marital_type = 2
    commercial_type = 3
    rel_type = "Rel_type"
    transitory = "TRANSITORY"
    informal = "INFORMAL"
    marital = "MARITAL"
    commercial = "COMMERCIAL"
    rel_type_long = "Rel_type (0 = TRANSITORY; 1 = INFORMAL; 2 = MARITAL; 3 = COMMERCIAL)"


class MyCompareOperation(Enum):
    LessThan = auto()
    MoreThan = auto()


map_relationship_type = {Constant.transitory: Constant.transitory_type,
                         Constant.informal:   Constant.informal_type,
                         Constant.marital:    Constant.marital_type,
                         Constant.commercial: Constant.commercial_type
                         }


def get_relationship_reverse_counter(relationship_df, count_who_has_no_partner=False, id_start_from=1, population_size=0):
    """
    Collect a dictionary for N: list of ids who have N relationships/partners, N is from 0 to maximum number of
    relationships
    """
    # Collect ids and put them into bins based on how many relationships they have(Individual should has only
    # one relationship with the same partner)
    relationship_list = relationship_df[GenderStrings.male_id].tolist() + relationship_df[GenderStrings.female_id].tolist()
    relationship_counter = Counter(relationship_list)
    if count_who_has_no_partner:
        # Collect individual ids who has no partner
        for id in range(id_start_from, population_size + id_start_from):
            if id not in relationship_counter:
                relationship_counter[id] = 0
    # Reverse the relationship counter, num_of_relationship is the key and id_list are the values.
    relationship_reverse_counter = dict()
    for id, num_of_relationship in relationship_counter.items():
        if num_of_relationship in relationship_reverse_counter:
            relationship_reverse_counter[num_of_relationship].append(id)
        else:
            relationship_reverse_counter[num_of_relationship] = [id]
    return relationship_reverse_counter

def get_ids_based_on_num_relationship(relationship_reverse_counter, num, operation: MyCompareOperation) -> set:
    def less_than(num, n):
        return n < num

    def more_than(num, n):
        return n > num

    if isinstance(operation, MyCompareOperation):
        if operation == MyCompareOperation.LessThan:
            compare_operation = partial(less_than, num)
        elif operation == MyCompareOperation.MoreThan:
            compare_operation = partial(more_than, num)
        else:
            raise ValueError(f"operation should be either {MyCompareOperation.LessThan.name} or "
                             f"{MyCompareOperation.MoreThan.name}")
    else:
        raise ValueError("operation is not a MyCompareOperation type.")

    expected_ids = list()
    for num_of_relationship, id_list in relationship_reverse_counter.items():
        if compare_operation(num_of_relationship):
            expected_ids.extend(id_list)
    return set(expected_ids)


def compare_ids(expected_id_set, actual_id_set, event, output_report_file):
    if expected_id_set != actual_id_set:
        result = False
        output_report_file.write(f"\tBAD: {event} event didn't broadcast to the right individuals "
                                 f"based on their number of relationships.\n")
        if len(expected_id_set - actual_id_set):
            output_report_file.write(f"\tBAD: these individuals should receive {event} event but they "
                                     f"didn't receive it: {expected_id_set - actual_id_set}.\n")
        if len(actual_id_set - expected_id_set):
            output_report_file.write(f"\tBAD: these individuals should not receive {event} event but they "
                                     f"did receive it: {actual_id_set - expected_id_set}.\n")
    else:
        result = True
        output_report_file.write(f"\tGOOD: {event} event broadcast to the right individuals "
                                 f"based on their number of relationships.\n")
    return result


def compare_ids_with_2_events(ids_1, ids_2, event_1, event_2, output_report_file):
    output_report_file.write(f"Compare {event_1} with {event_2}:\n")
    if ids_1 != ids_2:
        result = False
        if len(ids_1 - ids_2):
            output_report_file.write(f"\tBAD: these individuals received {event_1} event but they "
                                     f"didn't receive {event_2} event: {ids_1 - ids_2}.\n")
        if len(ids_2 - ids_1):
            output_report_file.write(f"\tBAD: these individuals received {event_2} event but they "
                                     f"didn't receive {event_1} event: {ids_2 - ids_1}.\n")
    else:
        result = True
        output_report_file.write(f"\tGOOD: Individual who received {event_1} also received {event_2}.\n")

    return result


def hasRelationship_test(partner_df, target_df, relationship_df, testing_options, event, print_string, output_report_file):
    """
    This is the main test in each HasRelationship_** SFT (STI\SFTs\Relationship_Targeting). It compares a set of
    individual ids for event in ReportEventRecorder.csv with a set of individual ids that the event should target.
    Args:
        partner_df:      a dataframe contains id and gender information for individuals in Risk:High group
        target_df:       a dataframe contains information for individuals who receive the event
        relationship_df: a dataframe contains all relationships that qualify the targeting logic(relationship start/end day,
                         type, etc.)
        testing_options: list of options that under testing
        event:           targeting event
        print_string:    custom string in the error message for each test
        output_report_file:

    Returns: True or False

    """
    # Create an empty list to store the individual ids that should be targeted.
    target_ids = list()
    # Collect female ids and male ids from relationships that qualify the targeting logic.
    females_in_relationships = relationship_df[GenderStrings.female_id].tolist()
    males_in_relationships = relationship_df[GenderStrings.male_id].tolist()

    # Loop through everyone in Risk:High group
    for row in partner_df.itertuples(index=False):
        ind_id = row.Individual_ID
        gender = row.Gender
        # Get partner's ID for individual in Risk:High group
        if gender == GenderStrings.female:
            if ind_id in females_in_relationships:
                target_ids += relationship_df[relationship_df[GenderStrings.female_id] == ind_id][GenderStrings.male_id].tolist()
        else:
            if ind_id in males_in_relationships:
                target_ids += relationship_df[relationship_df[GenderStrings.male_id] == ind_id][GenderStrings.female_id].tolist()

    # Compare two sets of ids(actual vs. expect)
    actual_target_set_from_event_report = set(target_df[Constant.ind].tolist())
    target_set_from_relationship_report = set(target_ids)

    if actual_target_set_from_event_report != target_set_from_relationship_report:
        result = False
        output_report_file.write(f"BAD: HasRelationship Targeting doesn't work with {testing_options}:\n")
        if len(actual_target_set_from_event_report - target_set_from_relationship_report):
            output_report_file.write(f"\tThere are individuals receive the intervention("
                                     f"{event}) but they don't have "
                                     f"any partner in {print_string}. They are: "
                                     f"({sorted(actual_target_set_from_event_report - target_set_from_relationship_report)}).\n")
        if len(target_set_from_relationship_report - actual_target_set_from_event_report):
            output_report_file.write(f"\tThere are individuals do not receive the intervention("
                                     f"{event}) but they have "
                                     f"partner in {print_string}. They are: "
                                     f"({sorted(target_set_from_relationship_report - actual_target_set_from_event_report)})\n")
    else:
        result = True
        output_report_file.write(f"GOOD: HasRelationship Targeting works fine with {testing_options}.\n")
    return result


def get_highriskpartner_and_target_df(target_event, event_df, event_report_name, output_report_file):
    """
    Collect partner_df and target_df from ReportEventRecorder.csv and perform some sanity check.
    partner_df and target_df are two inputs for hasRelationship_test() method.

    """
    result = True
    partner_df = event_df[event_df[Constant.event_name] == Event.i_am_risk_high]
    target_df = event_df[event_df[Constant.event_name] == target_event]
    if partner_df.empty:
        result = False
        output_report_file.write(f"BAD: There is no {Event.i_am_risk_high} event in the {event_report_name}, "
                                 f"please fix the test.\n")
    if target_df.empty:
        result = False
        output_report_file.write(f"BAD: There is no {target_event} event in the {event_report_name}, "
                                 f"please fix the test.\n")
    return result, partner_df, target_df