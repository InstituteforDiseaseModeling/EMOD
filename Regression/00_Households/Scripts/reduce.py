import sys, os, json, collections

def ShowUsage():
    print ('\nUsage: %s [InsetChart.json]' % os.path.basename(sys.argv[0]))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        ShowUsage()
        exit(0)

    input_fn=sys.argv[1]
    output_fn = "InsetChart_Reduced.json"

    fopen=open(input_fn,'r')
    input_json = json.load(fopen)
    fopen.close()

    input_dt = float(input_json["Header"]["Simulation_Timestep"])
    input_num_timesteps = int(input_json["Header"]["Timesteps"])

    steps_per_day = int( 1.0 / input_dt )

    output_num_timesteps = int( float(input_num_timesteps) * input_dt )

    output_json = collections.OrderedDict([])
    output_json["Header"] = input_json["Header"]
    output_json["Header"]["Timesteps"] = output_num_timesteps
    output_json["Header"]["Simulation_Timestep"] = 1.0
    output_json["Channels"] = {}

    for channel in input_json["Channels"]:
        print channel
        output_json["Channels"][ channel ] = {}
        output_json["Channels"][ channel ]["Units"] = input_json["Channels"][ channel ]["Units"]
        output_json["Channels"][ channel ]["Data"] = []
        step_counter = 0
        sum = 0
        for val in input_json["Channels"][ channel ]["Data"]:
            sum += val
            step_counter += 1
            if step_counter == steps_per_day:
                if channel != "New Severe Cases" and \
                   channel != "New Infections" and \
                   channel != "New Reported Infections" and \
                   channel != "New Clinical Cases" and \
                   channel != "Daily Bites per Human" and \
                   channel != "Daily EIR" and \
                   channel != "Probability of New Infection" and \
                   channel != "Rainfall":
                    sum = float( sum ) * input_dt
                output_json["Channels"][ channel]["Data"].append( sum )
                sum = 0
                step_counter = 0

    with open(output_fn, 'w') as file:
        json.dump(output_json, file, indent=4)
