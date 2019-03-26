import re
import json
import math
import dtk_test.dtk_sft as sft
from scipy import stats

"""
This SFT tests the duration of the acute stage and subclinical infection stage for individuals shall be set at the onset 
of the stage by a draw from a log-normal distribution with one of two (hard-coded) mu & sigma value sets depending on 
whether or not the individual is under or over 30 years of age at time of stage onset

Acute infection stage:
Category	Mu	    Sigma
Under 30	1.172	0.483
Over 30	    1.258	0.788

Subclinical infection stage:
Category	Mu	    Sigma
Under 30	1.172	0.483
Over 30	    1.258	0.788
"""
def application(report_file, debug=False):
    sft.wait_for_done()
    # print( "Post-processing: " + report_file )
    lines = []
    with open("test.txt") as logfile:
        for line in logfile:
            # collect all lines of
            if re.search("Infection stage transition", line):
                lines.append(line)

    acute_mu_over_30 = 1.258
    acute_sigma_over_30 = 0.788
    acute_mu_under_30 = 1.172
    acute_sigma_under_30 = 0.483
    subc_mu_over_30 = 1.258
    subc_sigma_over_30 = 0.788
    subc_mu_under_30 = 1.172
    subc_sigma_under_30 = 0.483

    success = True
    Timers_acute_over_30 = []
    Timers_acute_under_30 = []
    Timers_subc_over_30 = []
    Timers_subc_under_30 = []

    with open(sft.sft_output_filename, "w") as report_file:
        if len(lines) == 0:
            success = False
            report_file.write("Found no data matching test case.\n")
        else:
            for line in lines:
                if re.search("->Acute", line):
                    # get duration and convert to week
                    duration = float(sft.get_val("dur=", line))
                    duration /= 7
                    age = float(sft.get_val("Age=", line))
                    if age > 30:
                        Timers_acute_over_30.append(duration)
                    else:
                        Timers_acute_under_30.append(duration)
                elif re.search("->SubC", line):
                    # get duration and convert to week
                    duration = float(sft.get_val("dur=", line))
                    duration /= 7
                    age = float(sft.get_val("Age=", line))
                    if age > 30:
                        Timers_subc_over_30.append(duration)
                    else:
                        Timers_subc_under_30.append(duration)

            # Don't want to print anything for troubleshooting until the test.txt processing is done.
            # print( str( Timers_acute_over_30 ) )
            # print( str( Timers_acute_under_30 ) )

            if Timers_acute_over_30: #implicit boolean, non-empty = true, empty = false
                if not sft.test_lognorm(Timers_acute_over_30, acute_mu_over_30, acute_sigma_over_30, report_file,
                                        "Acute_over_30"):
                    success = False
            else:
                "BAD: Data for Acute Duration for over 30 is missing.\n"
            if Timers_acute_under_30:
                if not sft.test_lognorm(Timers_acute_under_30, acute_mu_under_30, acute_sigma_under_30, report_file,
                                        "Acute_under_30"):
                    success = False
            else:
                "BAD: Data for Acute Duration for under 30 is missing.\n"
            if Timers_subc_over_30:
                if not sft.test_lognorm(Timers_subc_over_30, subc_mu_over_30, subc_sigma_over_30, report_file,
                                        "SubClinical_over_30"):
                    success = False
            else:
                "BAD: Data for SubClinical Duration for over 30 is missing.\n"
            if Timers_subc_under_30:
                if not sft.test_lognorm(Timers_subc_under_30, subc_mu_under_30, subc_sigma_under_30, report_file,
                                        "SubClinical_under_30"):
                    success = False
            else:
                "BAD: Data for SubClinical Duration for under 30 is missing.\n"

        if debug:
            # dump data in sorted_duration.json
            report_file.write("Timers and logs in sorted_duration.json\n")
            myjson = {}
            Duration_Acute_Over_30 = {"Data": sorted(Timers_acute_over_30)}
            Duration_Acute_Under_30 = {"Data": sorted(Timers_acute_under_30)}
            Duration_Subc_Over_30 = {"Data": sorted(Timers_subc_over_30)}
            Duration_Subc_Under_30 = {"Data": sorted(Timers_subc_under_30)}
            myjson["Duration_Acute_Over_30"] = Duration_Acute_Over_30
            myjson["Duration_Acute_Under_30"] = Duration_Acute_Under_30
            myjson["Duration_Subc_Over_30"] = Duration_Subc_Over_30
            myjson["Duration_Subc_Under_30"] = Duration_Subc_Under_30
            with open("sorted_duration.json", "w") as outfile:
                outfile.write(json.dumps(myjson, indent=4))

        # plotting
        dist_lognormal_scipy1 = stats.lognorm.rvs(s=acute_sigma_over_30, loc=0, scale=math.exp(acute_mu_over_30),
                                                  size=len(Timers_acute_over_30))
        sft.plot_data(sorted(Timers_acute_over_30), sorted(dist_lognormal_scipy1), label1="Actual (Sorted)",
                      label2="Expected (Sorted)", title="Duration of Acute for over 30",
                      xlabel="Occurrences",
                      ylabel="Duration in Weeks",
                      category="acute_over_30",
                      alpha=0.5,
                      overlap=True)

        dist_lognormal_scipy2 = stats.lognorm.rvs(s=acute_sigma_under_30, loc=0, scale=math.exp(acute_mu_under_30),
                                                  size=len(Timers_acute_under_30))
        sft.plot_data(sorted(Timers_acute_under_30), sorted(dist_lognormal_scipy2), label1="Actual (Sorted)",
                      label2="Expected (Sorted)", title="Duration of Acute for under 30",
                      xlabel="Occurrences",
                      ylabel="Duration in Weeks",
                      category="acute_under_30",
                      alpha=0.5,
                      overlap=True)

        dist_lognormal_scipy3 = stats.lognorm.rvs(s=subc_sigma_over_30, loc=0, scale=math.exp(subc_mu_over_30),
                                                  size=len(Timers_subc_over_30))
        sft.plot_data(sorted(Timers_subc_over_30), sorted(dist_lognormal_scipy3), label1="Actual (Sorted)",
                      label2="Expected (Sorted)", title="Duration of Subclinical for over 30",
                      xlabel="Occurrences",
                      ylabel="Duration in Weeks",
                      category="subc_over_30",
                      alpha=0.5,
                      overlap=True)

        dist_lognormal_scipy4 = stats.lognorm.rvs(s=subc_sigma_under_30, loc=0, scale=math.exp(subc_mu_under_30),
                                                  size=len(Timers_subc_under_30))
        sft.plot_data(sorted(Timers_subc_under_30), sorted(dist_lognormal_scipy4), label1="Actual (Sorted)",
                      label2="Expected (Sorted)", title="Duration of Subclinical under 30",
                      xlabel="Occurrences",
                      ylabel="Duration in Weeks",
                      category="subc_under_30",
                      alpha=0.5,
                      overlap=True)


        report_file.write(sft.format_success_msg(success))


if __name__ == "__main__":
    # execute only if run as a script
    application("")
