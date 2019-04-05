import dtk_test.dtk_ImportPressure_Support as ips
import numpy as np

MAX_DURATION = 800
NUM_OF_BUCKETS = np.random.randint(2, 10)
MAX_RATE = 20

def application(config_filename="config.json", debug = True):
    campaign_filename = "campaign.json"
    campaign_template_filename = "campaign_template.json"
    if debug:
        print( "campaign_filename: " + campaign_filename + "\n" )
        print( "campaign_template_filename: " + campaign_template_filename + "\n" )
        print( "debug: " + str(debug) + "\n" )
    durations = ips.generate_durations(NUM_OF_BUCKETS + 1, MAX_DURATION)
    if sum(durations) != MAX_DURATION:
        print( "total duration is {0}, expected {1}.\n".format(sum(durations), MAX_DURATION) )
    rates = ips.generate_rates(NUM_OF_BUCKETS, MAX_RATE)
    ips.set_random_campaign_file(durations, rates, campaign_filename, campaign_template_filename, debug)
    return config_filename

if __name__ == "__main__":
    # execute only if run as a script
    application("config.json")
