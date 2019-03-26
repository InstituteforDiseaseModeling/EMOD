import dtk_test.dtk_sft as sft
import dtk_test.dtk_serialization_support as d_ss

def application(output_folder="output", config_filename="config.json",
                report_name=sft.sft_output_filename, debug=False):
    if debug:
        print("output_folder: {0}".format(output_folder))
        print("config_filename: {0}".format(config_filename))
        print("report_name: {0}".format(report_name))
        print("debug: {0}".format(debug))

    config_object = d_ss.load_config_file(config_filename=config_filename, debug=debug)
    sft.start_report_file(output_filename=report_name, config_name=config_object[d_ss.ConfigKeys.CONFIGNAME_KEY])

    success_message_formatter = sft.format_success_msg
    d_ss.generate_report_file(config_object, report_name, output_folder=output_folder,
                              success_formatter=success_message_formatter,
                              debug=debug)


if __name__=="__main__":
    import argparse
    p = argparse.ArgumentParser()
    p.add_argument('-o', '--output', default="output", help="Folder to load outputs from (output)")
    p.add_argument('-c', '--configname', default="config.json", help="config filename (config.json")
    p.add_argument('-r', '--reportname', default=sft.sft_output_filename, help="report filename ({0})".format(sft.sft_output_filename))
    args = p.parse_args()

    application(output_folder=args.output, report_name=args.reportname, config_filename=args.configname,
                debug=True)
