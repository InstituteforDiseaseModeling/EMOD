import json
import dtk_test.dtk_sft as sft


def application(report_file, debug=False):
    sft.wait_for_done()

    cdj = json.loads(open("config.json").read())["parameters"]
    tcer = float(cdj["Typhoid_Contact_Exposure_Rate"])
    teer = float(cdj["Typhoid_Environmental_Exposure_Rate"])

    num_exposures_contact = []
    num_exposures_enviro = []
    with open("test.txt") as logfile:
        for line in logfile:
            if "Exposing " in line and "route 'environment'" in line:
                # collect # of exposures for route contact
                num_exp_e = int(sft.get_val("num_exposures=", line))
                num_exposures_enviro.append(num_exp_e)
            elif "Exposing " and "route 'contact'" in line:
                # collect # of exposures for route environment
                num_exp_c = int(sft.get_val("num_exposures=", line))
                num_exposures_contact.append(num_exp_c)

    success = True
    with open(sft.sft_output_filename, "w") as report_file:
        # report_file.write("len1={0}, len2={1}, teer = {2}, tcer = {3}.\n".format(len(num_exposures_enviro),
        # len(num_exposures_contact), teer, tcer))
        if not num_exposures_enviro:
            success = False
            report_file.write("BAD: Found no individual exposed from route environment.\n" )
        elif not sft.test_poisson(num_exposures_enviro, teer, report_file, 'environment'):
            success = False
        if not num_exposures_contact:
            success = False
            report_file.write("BAD: Found no individual exposed from route contact.\n" )
        elif not sft.test_poisson(num_exposures_contact, tcer, report_file, 'contact'):
            success = False
        report_file.write("num_exposures_enviro = {0}, num_exposures_contact = {1}"
                          "\n".format(len(num_exposures_enviro), len(num_exposures_contact)))

        # Write summary message to report file
        report_file.write(sft.format_success_msg(success))

        with open("error_larger_than_tolerance_log.txt", "w") as my_file:
            sft.plot_poisson_probability(teer, num_exposures_enviro, my_file, 'environmental route',
                                         xlabel="Number of Exposures",
                                         ylabel="Probability",
                                         label2="Emod data",
                                         title="Poisson Probability Mass Funciton\n"
                                               "environmental route: Exposure_Rate = {}".format(teer))
            sft.plot_poisson_probability(tcer, num_exposures_contact, my_file, 'contact route', xlabel="Number of Exposures",
                                         ylabel="Probability",
                                         label2="Emod data",
                                         title="Poisson Probability Mass Funciton\n"
                                               "contact route: Exposure_Rate = {}".format(tcer)
                                         )


if __name__ == "__main__":
    # execute only if run as a script
    application("")
