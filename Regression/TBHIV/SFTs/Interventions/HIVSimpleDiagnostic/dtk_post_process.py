import json
import os.path as path
import dtk_sft

"""
This is HIVSimpleDiagnostic test. 
The test does several things: 
Part1: Verifies that HIVSimpleDiagnostic does not run on Disqualifying_Properties (InterventionStatus:None)
Part2: Verifies that HIVSimpleDiagnostic can update Property_Values 
(to InterventionStatusTest_Status_For_Single_Negative_Test_Then_Expire) and verifies that test correctly tests 
negative results and send out the event ("Tested_Negative_Because_No_Outbreak_Yet")
<HIV Outbreak>
Part3: Verifies that HIVSimpleDiagnostic changes the Property_Value again and tests positively and sends out an event,
tests only the demographic coverage percentage

I'm keeping this simple by not reading in or analyzing the campaign file

Sweep suggestions:
Run_Number, Base_Population_Scale_Factor

Improvement suggestions for a more detailed test:
Read data out of the campaign file: day of intervention, cost_to_consumer
add demographic_coverage for the interventions
add demographic_coverage for outbreak and track positive/negative results vs Infected/Naive
add separate tests for error state testing (things that cause errors)

"""
config_name = "Config_Name"
sim_duration = "Simulation_Duration"
statistical_population = "Statistical Population"
campaign_cost = "Campaign Cost"
property_stat_pop = "Statistical Population:InterventionStatus:"
property_none = "None"
property_till_positive = "Test_Status_For_Recurrent_Testing_Till_Positive"
property_negative_test = "Test_Status_For_Single_Negative_Test_Then_Expire"
property_never_here = "Test_Status_Never_Get_Here"
event_tested_negative = "Tested_Negative_Because_No_Outbreak_Yet"
event_there_it_is = "There_It_Is"
event_not_happen1 = "BAD_Individual_Tested_From_Abort_State"
event_not_happen2 = "BAD_Individual_Tested_Positive_Before_Outbreak"
event_not_happen3 = "BAD_Individual_Tested_Negative_At_100_Prevalence"


def load_emod_parameters(config_filename="config.json"):
    """reads config file and populates params_obj

    :param config_filename: name of config file (config.json)
    :returns param_obj:     dictionary with keys
    """

    with open(config_filename) as infile:
        cdj = json.load(infile)["parameters"]
    param_obj = {config_name: cdj[config_name],
                 sim_duration: cdj[sim_duration]}
    return param_obj


def parse_property_report_tb(output_folder="output", property_report_name="PropertyReportTB.json"):
    """creates property_report object

    :param property_report_name: PropertyReportTB.json file with location (output/PropertyReportTB.jso)
    :param output_folder: folder in relation to the main folder that the file is in
    :param debug: Flag, when set to true writes out relevant to us data into filtered_lines.txt
    :returns: property_report_obj structure
    """
    report_path = path.join(output_folder, property_report_name)

    with open(report_path) as infile:
        prj = json.load(infile)["Channels"]
        property_report_obj = {property_none: prj[property_stat_pop+property_none]["Data"],
                               property_till_positive: prj[property_stat_pop+property_till_positive]["Data"],
                               property_negative_test: prj[property_stat_pop+property_negative_test]["Data"],
                               property_never_here: prj[property_stat_pop+property_never_here]["Data"]}

    return property_report_obj


def parse_report_event_counter(output_folder="output", event_report_name="ReportEventCounter.json"):
    """creates property_report object

    :param event_report_name: PropertyReportTB.json file with location (output/PropertyReportTB.jso)
    :param output_folder: folder in relation to the main folder that the file is in
    :returns: property_report_obj structure
    """
    report_path = path.join(output_folder, event_report_name)

    with open(report_path) as infile:
        prj = json.load(infile)["Channels"]
        event_counter_obj = {event_not_happen1: prj[event_not_happen1]["Data"],
                             event_not_happen2: prj[event_not_happen2]["Data"],
                             event_not_happen3: prj[event_not_happen3]["Data"],
                             event_tested_negative: prj[event_tested_negative]["Data"],
                             event_there_it_is: prj[event_there_it_is]["Data"]}

    return event_counter_obj


def parse_inset_chart(output_folder="output", inset_chart_name="InsetChart.json"):
    """creates campaign_cost list where each entry is the cumulative cost so far

    :param output_folder: folder where the file is located relative to the working folder
    :param inset_chart_name: InsetChart.json file with location (output/InsetChart.json)
    :returns: inset_days structure
    """
    inset_chart_path = path.join(output_folder, inset_chart_name)
    with open(inset_chart_path) as infile:
        icj = json.load(infile)["Channels"]

    inset_chart_obj = {statistical_population: icj[statistical_population]["Data"],
                       campaign_cost: icj[campaign_cost]["Data"]}

    return inset_chart_obj


def create_report_file(param_obj, property_report_obj, event_counter_obj, inset_chart_obj,
                       report_name=dtk_sft.sft_output_filename):
    with open(report_name, "w") as outfile:
        success = True
        sft_name = param_obj[config_name]
        outfile.write("SFT: {}.\n".format(sft_name))
        diagnostic_2_day = 3  # ideally this is gotten from campaign.json
        diagnostic_3_day = 6  # diagnostic_1 should not run at all so we don't worry about it
        diagnostic_2_cost = diagnostic_3_cost = 1 # ideally gotten from campain.json
        if not property_report_obj:
            outfile.write("BAD: Property report data not found. \n")
        elif not event_counter_obj:
            outfile.write("BAD: Events counter data not found. \n")
        elif not inset_chart_obj:
            outfile.write("BAD: Test output data not found. \n")
        else:
            total_cost = 0
            for day in range(len(inset_chart_obj[statistical_population])):
                if property_report_obj[property_never_here][day] != 0:
                    success = False
                    outfile.write("BAD: On day {}, the property is set to {}, "
                                  "shouldn't happen.\n".format(day, property_never_here))
                elif event_counter_obj[event_not_happen1][day] != 0:
                    success = False
                    outfile.write("BAD: On day {}, we see {} {} events, shouldn't happen."
                                  "\n".format(day, event_counter_obj[event_not_happen1][day], event_not_happen1))
                elif event_counter_obj[event_not_happen2][day] != 0:
                    success = False
                    outfile.write("BAD: On day {}, we see {} {} events, shouldn't happen."
                                  "\n".format(day, event_counter_obj[event_not_happen2][day], event_not_happen2))
                elif event_counter_obj[event_not_happen3][day] != 0:
                    success = False
                    outfile.write("BAD: On day {}, we see {} {} events, shouldn't happen."
                                  "\n".format(day, event_counter_obj[event_not_happen3][day], event_not_happen3))
                else:
                    stat_pop = inset_chart_obj[statistical_population][day]
                    # checking properties
                    if day < diagnostic_2_day:
                        if property_report_obj[property_none][day] != stat_pop:
                            success = False
                            outfile.write("BAD: On day {}, property {} is should have {} members, "
                                          "but it has {}.\n".format(day, poperty_none, stat_pop,
                                                                    property_report_obj[property_none][day]))
                        elif property_report_obj[property_negative_test][day] != 0:
                            success = False
                            outfile.write("BAD: On day {}, property {} has {} members, should be 0."
                                          "\n".format(day, property_negative_test,
                                                      property_report_obj[property_negative_test][day]))
                        elif property_report_obj[property_till_positive][day] != 0:
                            success = False
                            outfile.write("BAD: On day (), property {} has {} members, should be 0."
                                          "\n".format(day, property_till_positive,
                                                      property_report_obj[property_till_positive][day]))
                    elif day < diagnostic_3_day:
                        if property_report_obj[property_negative_test][day] != stat_pop:
                            success = False
                            outfile.write("BAD: On day {}, property {} is should have {} members, "
                                          "but it has {}.\n".format(day, property_negative_test, stat_pop,
                                                                    property_report_obj[property_negative_test][day]))
                        elif property_report_obj[property_none][day] != 0:
                            success = False
                            outfile.write("BAD: On day {}, property {} has {} members, should be 0."
                                          "\n".format(day, property_none,
                                                      property_report_obj[property_none][day]))
                        elif property_report_obj[property_till_positive][day] != 0:
                            success = False
                            outfile.write("BAD: On day (), property {} has {} members, should be 0."
                                          "\n".format(day, property_till_positive,
                                                      property_report_obj[property_till_positive][day]))
                    else:
                        if property_report_obj[property_till_positive][day] != stat_pop:
                            success = False
                            outfile.write("BAD: On day {}, property {} is should have {} members, "
                                          "but it has {}.\n".format(day, property_till_positive, stat_pop,
                                                                    property_report_obj[property_till_positive][day]))
                        elif property_report_obj[property_none][day] != 0:
                            success = False
                            outfile.write("BAD: On day {}, property {} has {} members, should be 0."
                                          "\n".format(day, property_none,
                                                      property_report_obj[property_none][day]))
                        elif property_report_obj[property_negative_test][day] != 0:
                            success = False
                            outfile.write("BAD: On day (), property {} has {} members, should be 0."
                                          "\n".format(day, property_till_positive,
                                                      property_report_obj[property_till_positive][day]))
                    # checking events
                    if day == diagnostic_2_day:
                        total_cost += stat_pop * diagnostic_2_cost
                        if event_counter_obj[event_tested_negative][day] != stat_pop:
                            success = False
                            outfile.write("BAD: On day {}, event {} is should have {} occurrences, "
                                          "but it has {}.\n".format(day, event_tested_negative, stat_pop,
                                                                    property_report_obj[property_negative_test][day]))
                    else:
                        if event_counter_obj[event_tested_negative][day] != 0:
                            success = False
                            outfile.write("BAD: On day {}, event {} is should have {} occurrences, "
                                          "but it has {}.\n".format(day, event_tested_negative, 0,
                                                                    property_report_obj[property_negative_test][day]))
                    if day == diagnostic_3_day:
                        total_cost += stat_pop * diagnostic_3_cost
                        if event_counter_obj[event_there_it_is][day] != stat_pop:
                            success = False
                            outfile.write("BAD: On day {}, event {} is should have {} occurrences, "
                                          "but it has {}.\n".format(day, event_tested_negative, stat_pop,
                                                                    property_report_obj[property_negative_test][day]))
                    else:
                        if event_counter_obj[event_there_it_is][day] != 0:
                            success = False
                            outfile.write("BAD: On day {}, event {} is should have {} occurrences, "
                                          "but it has {}.\n".format(day, event_tested_negative, 0,
                                                                    property_report_obj[property_negative_test][day]))
                    # checking that campaign costs make sense
                    if inset_chart_obj[campaign_cost][day] != total_cost:
                        success = False
                        outfile.write("BAD: On day {}, accrued campaign cost is calculated to be {}, but reported as "
                                      "{}.\n".format(day, total_cost, inset_chart_obj[campaign_cost][day]))

            outfile.write("SUMMARY: Success={0}\n".format(success))


def application(output_folder="output",
                property_report_name="PropertyReportTB.json",
                event_report_name="ReportEventCounter.json",
                config_filename="config.json",
                inset_chart_name="InsetChart.json",
                report_name=dtk_sft.sft_output_filename,
                debug=False):
    if debug:
        print("output_folder: " + output_folder)
        print("property_report_name: " + property_report_name)
        print("event_report_name: " + event_report_name)
        print("config_filename: " + config_filename + "\n")
        print("inset_chart_name: " + inset_chart_name + "\n")
        print("report_name: " + report_name + "\n")
        print("debug: " + str(debug) + "\n")

    param_obj = load_emod_parameters(config_filename)
    property_report_obj = parse_property_report_tb(output_folder, property_report_name)
    event_counter_obj = parse_report_event_counter(output_folder, event_report_name)
    inset_chart_obj = parse_inset_chart(output_folder, inset_chart_name)
    create_report_file(param_obj, property_report_obj, event_counter_obj, inset_chart_obj, report_name)


if __name__ == "__main__":
    # execute only if run as a script
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    parser.add_argument('-i', '--inset', default="InsetChart.json", help="Json report to load (InsetChart.json)")
    parser.add_argument('-c', '--config', default="config.json", help="Config name to load (config.json)")
    parser.add_argument('-r', '--report', default=dtk_sft.sft_output_filename, help="Report file to generate")
    parser.add_argument('-p', '--property', default="PropertyReportTB.json",
                        help="Json report to load (PropertyReportTB.json)")
    parser.add_argument('-e', '--event', default="ReportEventCounter.json",
                        help="Json report to load (ReportEventCounter.json)")
    args = parser.parse_args()

    application(output_folder=args.output, property_report_name=args.property, event_report_name=args.event,
                config_filename=args.config, inset_chart_name=args.inset,
                report_name=args.report, debug=True)
