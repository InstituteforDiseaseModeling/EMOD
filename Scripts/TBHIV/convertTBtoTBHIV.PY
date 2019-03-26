import argparse
import os
import json
import copy


def find_key(needles, haystack, delete=False):
    found = {}
    if type(needles) != type([]):
        needles = [needles]

    if type(haystack) == type(dict()):
        for needle in needles:
            if needle in haystack.keys():
                found[needle] = haystack[needle]
                if delete:
                    del haystack[needle]
            elif len(haystack.keys()) > 0:
                for key in haystack.keys():
                    result = find_key(needle, haystack[key], delete)
                    if result:
                        for k, v in result.items():
                            found[k] = v
    elif type(haystack) == type([]):
        for node in haystack:
            result = find_key(needles, node, delete)
            if result:
                for k, v in result.items():
                    found[k] = v
    return found


def find_value_context(needles, haystack):
    found = {}
    if type(needles) != type([]):
        needles = [needles]

    if type(haystack) == type(dict()):
        for needle in needles:
            if needle in haystack.values():
                found = haystack
            elif len(haystack.keys()) > 0:
                for key in haystack.keys():
                    result = find_value_context(needle, haystack[key])
                    found = result if result else found
    elif type(haystack) == type([]):
        for node in haystack:
            result = find_value_context(needles, node)
            if result == 'leaf':
                found = haystack
            elif result:
                found = result
    else:
        for needle in needles:
            if needle == haystack:
                found = 'leaf'
    return found


def fix_tandem_hsb_updateable(campaign):
    # See if there's a HealthSeekingBehaviorUpdateable to fix
    context = find_value_context("HealthSeekingBehaviorUpdateable", campaign)
    if not context:
        return

    # See if there's a HealthSeekingBehaviorUpdate
    if not find_value_context("HealthSeekingBehaviorUpdate", campaign):
        # If not, then change HealthSeekingBehaviorUpdateable to SimpleHealthSeekingBehavior
        context["class"] = "SimpleHealthSeekingBehavior"


def smear_diag_to_diag_treat_neg(campaign):
    # Look for the context that SmearDiagnostic is couched in
    context = find_value_context("SmearDiagnostic", campaign)

    # Change the class
    context["class"] = "DiagnosticTreatNeg"

    # Add values
    if "Defaulters_Event" not in context.keys():
        context["Defaulters_Event"] = "TBTestDefault"
    if  "Negative_Diagnosis_Event" not in context.keys():
        context["Negative_Diagnosis_Event"] = "TBTestNegative"


def transform_drug_params(campaign, config):
    # Find drug params in config
    try:
        drug_params = config["parameters"]["TB_Drug_Params"]
    except:
        return

    # For each drug specified...
    for drug in drug_params.keys():

        params_to_expand = [
            "TB_Drug_Cure_Rate",
            "TB_Drug_Inactivation_Rate",
            "TB_Drug_Mortality_Rate",
            "TB_Drug_Relapse_Rate",
            "TB_Drug_Resistance_Rate"
        ]

        # For each parameter to expand...
        for param in params_to_expand:

            # Expand that param
            drug_params[drug][param + "_HIV"] = drug_params[drug][param]
            drug_params[drug][param + "_MDR"] = drug_params[drug][param]

        # Special case: get rid of TB_Drug_Resistance_Rate_MDR
        del drug_params[drug]["TB_Drug_Resistance_Rate_MDR"]

        params_to_move = [
            ("Primary_Decay_Time_Constant", "TB_Drug_Primary_Decay_Time_Constant"),
            ("Reduced_Acquire", "TB_Reduced_Acquire"),
            ("Reduced_Transmit", "TB_Reduced_Transmit")
        ]

        # Move parameters from campaign to config
        for param in params_to_move:
            found_param = find_key(param[0], campaign)
            if found_param:
                found_param = next(iter(found_param.values()))
                drug_params[drug][param[1]] = found_param

    # Delete the params we copied from the campaign
    for param in params_to_move:
        find_key(param[0], campaign, True)


def restructure_campaign(campaign):
    # For each event in the campaign...
    events = campaign["Events"]
    new_events = copy.deepcopy(events)
    for event in events:

        # Look for interventions of class AntiTBPropDebDrug
        intervention = find_value_context("AntiTBPropDepDrug", event)

        # Keep going if this isn't an event to restructure
        if not intervention or "Drug_Type_by_Property" not in intervention:
            continue

        # Pull out all the drugs
        drugs = []
        for drug in intervention["Drug_Type_by_Property"]:
            drugs.append((next(iter(drug)), next(iter(drug.values()))))

        # For each drug...
        for drug in drugs:

            # Add a copy of the event
            new_event = copy.deepcopy(event)
            new_events.append(new_event)

            # Break apart the property restriction
            restriction_key = drug[0].split(':')[0]
            restriction_val = drug[0].split(':')[1]
            restriction = {restriction_key: restriction_val}

            # Modify values
            new_intervention = find_value_context("AntiTBPropDepDrug", new_event)
            del new_intervention['Drug_Type_by_Property']
            new_intervention['TB_Drug_Name'] = drug[1]
            new_intervention['class'] = "TBHIVConfigurableTBdrug"
            new_event['Event_Coordinator_Config']['Intervention_Config']["Property_Restrictions_Within_Node"] = [restriction]

        # Remove the original event
        new_events.remove(event)

    # Replace the old events with the new structure
    campaign["Events"] = new_events


def replace_triggers(campaign):
    while True:
        # Find trigger context
        context = find_value_context("TBActivationPostRelapse", campaign)

        # Stop when we run out
        if not context:
            return

        # Replace them
        for key in context:
            context[0] = "TBActivation"


def apply_rules(campaign, config):
    # Change sim type
    config["parameters"]["Simulation_Type"] = "TBHIV_SIM"

    # Drug param transformation
    transform_drug_params(campaign, config)

    # Restructure campaign
    restructure_campaign(campaign)

    # Swap SmearDiagnostic for DiagnosticTreatNeg
    smear_diag_to_diag_treat_neg(campaign)

    # HealthSeekingBehaviorUpdateable should have an Update, or be replaced
    fix_tandem_hsb_updateable(campaign)

    # Replace triggers
    replace_triggers(campaign)


def convert_test(path):
    # Construct file paths
    campaign_path = os.path.join(path, 'campaign.json')
    config_path = os.path.join(path, 'config.json')

    # Get data from test files
    with open(campaign_path) as f:
        campaign_data = json.load(f)
    with open(config_path) as f:
        config_data = json.load(f)

    # Apply conversion rules
    apply_rules(campaign_data, config_data)

    # Write data back to json files
    with open(campaign_path, 'w') as f:
        json.dump(campaign_data, f, sort_keys=True, indent=4, separators=(',', ': '))
    with open(config_path, 'w') as f:
        json.dump(config_data, f, sort_keys=True, indent=4, separators=(',', ': '))


def is_dir(path):
    if not os.path.isdir(path):
        raise argparse.ArgumentTypeError("Path is not a directory")
    return path


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=is_dir, help="Directory path to a TB test that will be converted to TBHIV")
    args = parser.parse_args()

    convert_test(args.path)
