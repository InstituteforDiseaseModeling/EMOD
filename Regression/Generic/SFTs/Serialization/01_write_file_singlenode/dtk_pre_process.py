import json
import dtk_test.dtk_serialization_support as d_ss
import dtk_test.dtk_sft as sft


def application(config_filename="config.json", debug=False):
    with open(config_filename) as infile:
        config_json = json.load(infile)
        config_params = config_json[d_ss.ConfigKeys.PARAMETERS_KEY]
    config_object = d_ss.load_config_file(config_filename=config_filename, debug=debug)
    if d_ss.ConfigKeys.Serialization.POPULATION_FILENAMES_KEY in config_params:
        # TODO: replace this code when issue 2850 is resolved
        if len(config_params[d_ss.ConfigKeys.Serialization.POPULATION_FILENAMES_KEY]) == 0:
            config_params.pop(d_ss.ConfigKeys.Serialization.POPULATION_FILENAMES_KEY)
    with open(config_filename, 'w') as outfile:
        temp_config = {}
        temp_config[d_ss.ConfigKeys.PARAMETERS_KEY] = config_params
        json.dump(temp_config, outfile, indent=4)

if __name__=="__main__":
    import argparse
    p = argparse.ArgumentParser()
    p.add_argument('-c', '--configname', default="config.json", help="config filename (config.json")
    args = p.parse_args()

    application(config_filename=args.configname,
                debug=True)
