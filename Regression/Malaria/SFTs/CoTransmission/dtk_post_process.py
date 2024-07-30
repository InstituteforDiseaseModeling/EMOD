import json
import os
import numpy as np
import pandas as pd
import dtk_test.dtk_sft as dtk_sft

"""
Once upon a time, we wanted to test that a vector only bites once per day, but to make things more like the base
malaria model, we allow it (it helps to avoid premature elimination of the parasite).  Hence, this test verifies that
each vector does not bite more often than its feeding cycle (feeding cycle needs to be an integer), unless there is a 
bednet intervention since the vector might be blocked by the bednet. At the time of the outbreak, there is a "vector 0"
that distributes the infections and that "vector" is ignored in our test. 
"""


def parse_campaign_file(campaign_filename="campaign.json", debug=False):
    with open(campaign_filename) as campaign_file:
        interventions = json.load(campaign_file)["Events"]

    outbreak_start = None
    bednet_start = None
    bednet_end = None
    for intervention in interventions:
        # find outbreak
        if intervention["Event_Name"] == "Outbreak":
            outbreak_start = intervention["Start_Day"]
        # find bednet
        elif intervention["Event_Name"] == "SimpleBednet":
            bednet_start = intervention["Start_Day"]
            bednet_duration = intervention["Event_Coordinator_Config"]["Intervention_Config"]["Blocking_Config"][
                "Box_Duration"]
            bednet_end = bednet_start + bednet_duration

    intervention_dict = {"outbreak_start": outbreak_start, "bednet_start": bednet_start, "bednet_end": bednet_end}

    if debug:
        with open("DEBUG_intervention_dictionary.txt", "w") as output_file:
            for key, value in intervention_dict:
                output_file.write(f"{key} - {value}\n")

    return intervention_dict


def parse_report_file(report_filename="ReportSimpleMalariaTransmission.csv", output_folder="output", debug=False):
    report_fn = os.path.join(output_folder,report_filename)
    report_df = pd.read_csv( report_fn )
    
    missing_infections_errors = []
    for index, transmission in report_df.iterrows():
        if (transmission["acquireInfectionIds"] == 0) or (transmission["acquireIndividualId"] == 0):
            missing_infections_errors.append(
                f"BAD: Either acquireInfectionIds or acquireIndividualId are empty for the"
                f"transmission at acquireTime {transmission['acquireTime']}, for vectorId {transmission['vectorId']}\n")
    
    if debug:
        report_df.to_csv("DEBUG_report_malaria_transmission_df.csv")
        with open("DEBUG_missing_infections_errors.txt", 'w') as outfile:
            for error in missing_infections_errors:
                outfile.write(error)

    return report_df, missing_infections_errors


def create_report_file(report_df, missing_infections_errors, intervention_dict, report_name="report.txt", debug=False):
    success = True
    with open(report_name, "w") as outfile:
        # look at the repeating bites
        vector_ids = report_df["vectorId"].tolist()
        repeating_set = {x for x in vector_ids if vector_ids.count(x) > 1}
        time_between_feeds = 3
        outbreak_start = intervention_dict["outbreak_start"]
        for id in repeating_set:
            bite_days = report_df.loc[report_df["vectorId"] == id]["acquireTime"].tolist()
            for day_index in range(len(bite_days) - 1):
                days_between_bites = bite_days[day_index + 1] - bite_days[day_index]
                if days_between_bites == 0:
                    # allow multiple bites of the same vector on the same day
                    continue
                elif days_between_bites < time_between_feeds:
                    # we should not see vectors biting more often than their time between feeds
                    success = False
                    outfile.write(f"BAD: For vector #{id}, We expected time between infecting feeds to be a multiple of "
                                  f"{time_between_feeds} or more but we got day {bite_days[day_index]} and day "
                                  f"{bite_days[day_index + 1]}, with "
                                  f"difference of {days_between_bites}, which is shorter than the feeding cycle.\n")
                elif days_between_bites % time_between_feeds != 0:
                    # days between bites should be a multiple of time_between_feeds, if we're seeing this, this is
                    # potentially bad, but if bednets are deployed, that happens
                    bednet_start = intervention_dict["bednet_start"]
                    bednet_end = intervention_dict["bednet_end"]
                    if bednet_start <= bite_days[day_index + 1] <= bednet_end or \
                            bednet_start <= bite_days[day_index] <= bednet_end or \
                            (bite_days[day_index] < bednet_start and bednet_end < bite_days[day_index + 1]):
                        # if either one of the bites was inside the bednet intervention time, then it's probably
                        # what the issue was
                        pass
                    else:
                        # this means we're seeing this issue outside of bednet intervention, that is bad
                        success = False
                        outfile.write(f"BAD: For vector #{id}, We expected time between infecting feeds to be a multiple"
                                      f" of {time_between_feeds} or more but we got day {bite_days[day_index]} and day "
                                      f"{bite_days[day_index + 1]}, with "
                                      f"difference of {days_between_bites}, which is longer than the feeding cycle and "
                                      f"cannot be "
                                      f"explained by the presence of bednets, which could cause longer feed cycle times"
                                      f".\n")
                elif debug:
                    outfile.write(f"GOOD: For vector #{id}, We expected time between infecting feeds to be a multiple "
                                  f"of {time_between_feeds} and we got day {bite_days[day_index]} and day "
                                  f"{bite_days[day_index + 1]}, with "
                                  f"difference of {days_between_bites}, so good.\n")

        for mi in missing_infections_errors:
            # the outbreak vector bites have no acquireInfectionIds or acquireIndividualId, so it's only bad
            # if we are seeing this outside of the outbreak_start time
            if f"acquireTime {outbreak_start}" not in mi:
                success = False
                outfile.write(mi)

        outfile.write(dtk_sft.format_success_msg(success))
        return

def application(output_folder="output",
                config_filename="config.json",
                campaign_filename="campaign.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder + "\n")
        print("config_filename: " + config_filename + "\n")
        print("campaign_filename: " + campaign_filename + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    dtk_sft.wait_for_done()
    intervention_dict = parse_campaign_file(campaign_filename, debug)
    report_df, missing_infections_errors = parse_report_file()
    create_report_file(report_df, missing_infections_errors, intervention_dict, report_name, debug=debug)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-ca', '--campaign', default="campaign.json", help="Campaign name to load (config.json)")
    parser.add_argument('-r', '--reportname', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-d', '--debug', action='store_true', help="Turns on debugging")
    args = parser.parse_args()

    application()
