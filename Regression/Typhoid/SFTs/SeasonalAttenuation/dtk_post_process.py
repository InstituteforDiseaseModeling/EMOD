#!/usr/bin/python
# This SFT test the following statement:
# Seasonality. The amount of environmental contagion (dose) to which an individual is exposed is attenuated/amplified
#  by a seasonality factor
#              that varies throughout the year according to a trapezoidal pattern.
#              The ramp durantions and cutoff days need to follow a rule where: ramp_up_duration + ramp_down_duration
#                + cutoff_days < 365
#              check environmental amplification and exposure for each individual, in each time step

if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../../shared_embedded_py_scripts').resolve().absolute()) )

import re
import json
import math
import dtk_test.dtk_sft as sft
import dtk_test.dtk_General_Support as general_support
from dtk_test.dtk_General_Support import InsetKeys

# C version: infectiousness = exp( -1 * _infectiousness_param_1 * pow(duration - _infectiousness_param_2,2) ) / _infectiousness_param_3;

def application(report_file, debug=True):
    sft.wait_for_done()
    """
    Parse this line from test.txt:
    00:00:00 [0] [V] [IndividualTyphoid] amplification calculated as 0.997059: day of year=1, start=360.000000,
    end=365.000000, ramp_up=30.000000, ramp_down=170.000000, cutoff=160.000000.
    00:00:00 [0] [V] [IndividualTyphoid] Exposing individual 2 age 8582.488281 on route 'environment': prob=0.000000,
    infects=0.000008, immunity=1.000000, num_exposures=0, exposure=0.997059, environment=1.000000, iv_mult=1.000000.
    """
    #print( "Post-processing: " + report_file )
    # get params from config.json
    cdj = json.loads( open( "config.json" ).read() )["parameters"]
    ncdr = cdj["Node_Contagion_Decay_Rate"]
    start_time=cdj["Start_Time"]
    lines = []
    timestep=start_time
    count =0
    amp = {}
    exposure = {}
    environment = {}
    cum_shedding = cum = 0
    cum_shedding_all = {}
    Statpop = []

    with open( "test.txt" ) as logfile:
        for line in logfile:
            if "Update(): Time:" in line:
                # store the accumulated shedding and reset the counter at the end of each time step.
                cum_shedding_all[timestep] = cum_shedding
                pop = float(sft.get_val("StatPop: ", line))
                Statpop.append(pop)
                # environmental cp decay
                cum_shedding *= 1.0 - ncdr
                # resetting shedding variables
                shedding = 0
                #calculate time step
                timestep += 1
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
            elif "amplification calculated as" in line:
                count += 1
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
                amp[timestep] = float(sft.get_val("amplification calculated as ", line))
            elif ("Exposing" in line) and ("environment" in line):
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)
                if timestep not in exposure:
                    exposure[timestep] = float(sft.get_val("exposure=", line))
                    environment[timestep] = float(sft.get_val("environment=", line))
            elif ("depositing" in line) and ("route environment" in line):
                # get shedding of contact route and add it to accumulated shedding
                shedding = float(sft.get_val("depositing ", line))
                cum_shedding += shedding
                line = "TimeStep: " + str(timestep) + " " + line
                lines.append(line)

    if debug:
        with open("DEBUG_filtered_line.txt","w") as filter_file:
            filter_file.write("".join(lines))

    #more params from config file
    rud=cdj["Environmental_Ramp_Up_Duration"]
    rdd=cdj["Environmental_Ramp_Down_Duration"]
    ecd=cdj["Environmental_Cutoff_Days"]
    eps=cdj["Environmental_Peak_Start"]

    peak_duration=365-rud-rdd-ecd
    # for eps > 365
    peak_starttime = eps % 365
    peak_endtime = peak_starttime + peak_duration
    cutoff_starttime = peak_starttime + peak_duration + rdd
    #cutoff_endtime = peak_starttime + peak_duration + rdd + ecd

    success = True

    expected_e_contagion = {}
    environmental_amp = {}

    inset_chart_obj = general_support.parse_inset_chart("output", "InsetChart.json",
                                                        insetkey_list=[InsetKeys.ChannelsKeys.Environmental_Contagion_Population])
    with open( sft.sft_output_filename, "w" ) as report_file:
        report_file.write("Peak_Start={0}, Peak_Duration={1}, Peak_End={2}, Ramp_Down={3}, Ramp_Up={4},"
                          "Cutoff_Start={5}, Cutoff_Duration={6}.\n".format(peak_starttime, peak_duration, peak_endtime,
                                                                            rdd, rud, cutoff_starttime, ecd))
        if ncdr != 1:
            report_file.write("WARNING: Node_Contagion_Decay_Rate is {}, suggest to set it to 1.\n".format(
                ncdr
            ))
        if count == 0:
            success = False
            report_file.write( "Found no data matching test case.\n" )
        elif peak_duration < 0:
            success = False
            report_file.write("BAD: Environmental peak duration should be larger or equal to 0, the actual value is {}."
                              "\n The ramp durations and cutoff days need to follow a rule where: ramp_up_duration + "
                              "ramp_down_duration + cutoff_days < 365.\n".format(peak_duration))
        else:
            # adjust the times so that the ramp up starts at time 0, which means the cut off ends at time 0 too.
            adjust_time = peak_starttime - rud
            peak_starttime -= adjust_time
            peak_endtime -= adjust_time
            cutoff_starttime -= adjust_time
            # cutoff_endtime -= adjust_time
            with open("DEBUG_contagion_data.txt", "w") as debug_file:

                for t in range(timestep - 2):
                    amplification = environmental_amplification = None
                    if t in amp:
                        TimeStep = t - adjust_time
                        day_in_year = TimeStep%365
                        amplification = amp[t]
                        # Environment Ramp Up
                        if day_in_year < peak_starttime:
                            environmental_amplification = day_in_year/float(rud)
                        # Environment peak
                        elif peak_starttime <= day_in_year <= peak_endtime:
                            environmental_amplification = 1
                        # Environment Ramp Down
                        elif peak_endtime < day_in_year < cutoff_starttime:
                            environmental_amplification = (cutoff_starttime - day_in_year) / float(rdd)
                        # Environment cutoff
                        elif day_in_year >= cutoff_starttime:
                            environmental_amplification = 0
                        if math.fabs(amplification - environmental_amplification) > 5e-2 * environmental_amplification:
                            success =False
                            report_file.write("BAD: at time {0}, day of year = {1}, the environmental amplification is {2},"
                                              " expected {3}.\n".format(t, t%365, amplification,
                                                                        environmental_amplification))
                        environmental_amp[t] = environmental_amplification
                    if t in exposure:
                        exposure_t = exposure[t]
                        if amplification is not None:
                            environment_contagion = cum_shedding_all[t-start_time] / Statpop[t -start_time]
                            expected_exposure = environment_contagion * amplification
                            if math.fabs(expected_exposure - exposure_t) > 5e-2 * expected_exposure:
                                success =False
                                report_file.write("BAD: at time {0}, day of year = {1}, the amount of environmental contagion "
                                                  "that individual is exposed is {2}, expected {3}"
                                                  ".\n".format(t, t%365,
                                                               exposure_t, expected_exposure))
                            expected_e_contagion[t] = expected_exposure

                            if debug:
                                environment_t = environment[t]
                                inserchart_contagion_e = inset_chart_obj \
                                    [InsetKeys.ChannelsKeys.Environmental_Contagion_Population][t]
                                debug_file.write("At time step {0}: environment={1}, exposure={2} from loggind, "
                                                 "{3}={4} from InsetChart, expected value={5}."
                                                 "accumulated shedding after decay={6} calculated from logging, "
                                                 "environmental amplification={7}.\n".format(
                                    t, environment_t,exposure_t, InsetKeys.ChannelsKeys.Environmental_Contagion_Population,
                                    inserchart_contagion_e, expected_exposure, environment_contagion, amplification
                                ))

        sft.plot_data(amp.values(), environmental_amp.values(),
                      label1="Actual Seasonal Attenuation",
                      label2="Expected Seasonal Attenuation",
                      title="Seasonal Attenuation", xlabel="Time",
                      ylabel="Attenuation",
                      category='seasonal_attenuation')

        for t in range(len(amp)):
            if t not in exposure:
                exposure[t] = 0
                expected_e_contagion[t] = 0

        sft.plot_data([exposure[key] for key in sorted(exposure.keys())],
                      [expected_e_contagion[key] for key in sorted(expected_e_contagion.keys())],
                             label1="exposure contagion(from StdOut)",
                             label2="Expected",
                             title="Environmental Contagion", xlabel="Time",
                             ylabel="Environmental Contagion",
                             category='Environmental_Contagion')

        report_file.write( sft.format_success_msg( success ) )


if __name__ == "__main__":
    application( "" )
