
import dtk_test.dtk_InfectivityScalingBoxcar_Support as isb_support

def application(config_filename="config.json", debug = True):
    if debug:
        print( "config filename: " + config_filename + "\n" )
        print( "debug: " + str(debug) + "\n" )
    mode = isb_support.Modes.EQUAL
    isb_support.set_random_config_file(config_filename, mode, debug)
    return config_filename

if __name__ == "__main__":
    # execute only if run as a script
    application("config.json")
