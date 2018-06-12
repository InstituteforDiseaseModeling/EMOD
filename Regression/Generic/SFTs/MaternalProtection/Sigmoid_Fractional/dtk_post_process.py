#!/usr/bin/python
"""
The test methodology is trying to match the susceptibility correction for all agents for all maternal_type, as follows:

Both "FRACTIONAL" implementations assign an equal susceptibility to all agents. This value is a function of age only:
Linear: Susceptibility = Slope * Age + SusZero
Sigmoid: Susceptibility = SusInit + (1.0 - SusInit) / (1.0 + EXP( (HalfMaxAge - Age) / SteepFac) )
None: Susceptibility = 1.0

Both "BINARY" implementations assign an age cutoff to each agent. If the agent age is less than the cutoff age, then the agent has susceptibility reduced to zero.
If the agent age is greater than or equal to the cutoff age, the agent does not have reduced susceptibility. The age assigned to each agent is randomly assigned:
Linear: AgeCutoff = (RAND - SusZero) / Slope
Sigmoid: AgeCutoff = HalfMaxAge - SteepFac * LOG( (1.0 - SusInit) / (RAND - SusInit) - 1.0 + 0.001)
None: AgeCutoff = 0.0

if the SFT passed, the output file (normally scientific_feature_report.txt) will show params for run as well as "Success=True", as follows:
    Simulation parmaters: simulation_duration=5, simulation_timestep=1.0, enable_birth=1, base_infectivity=0.25
    maternal_protection_type = LINEAR:
    susceptibility_type = BINARY:
    It's a linear type with linear_slope:0.012 and linear_susZero:0.0
    SUMMARY: Success=True

if the SFT failed, the output file (normally scientific_feature_report.txt) will show params for run, "Success=False", as well as which test(s) failed, as follows:
    Simulation parmaters: simulation_duration=5, simulation_timestep=1.0, enable_birth=1, base_infectivity=0.25
    maternal_protection_type = NONE:
    susceptibility_type = BINARY:
    It's a NONE type
    BAD: actual mod_acquire for individual 1.0 at time step 0.0 is 2.0, expected 1.0.
    SUMMARY: Success=False

"""

import dtk_MaternalProtection_Support as MP_Support
import dtk_sft

KEY_SIMULATION_TIMESTEP = "Simulation_Timestep"

def application( output_folder="output", stdout_filename="test.txt",
                 config_filename="config.json",
                 insetchart_name="InsetChart.json",
                 report_name=dtk_sft.sft_output_filename,
                 debug=False):
    if debug:
        print ("output_folder: " + output_folder)
        print ("stdout_filename: " + stdout_filename+ "\n")
        print ("config_filename: " + config_filename + "\n")
        print ("insetchart_name: " + insetchart_name + "\n")
        print ("report_name: " + report_name + "\n")
        print ("debug: " + str(debug) + "\n")
    dtk_sft.wait_for_done()
    param_obj = MP_Support.load_emod_parameters(config_filename)
    print (param_obj)
    simulation_timestep = param_obj[KEY_SIMULATION_TIMESTEP]
    if debug:
        print("simulation time step is : {0} days".format(simulation_timestep))
    output_df = MP_Support.parse_output_file(stdout_filename, debug)
    MP_Support.create_report_file(param_obj, output_df, report_name, debug)

if __name__ == "__main__":
    # execute only if run as a script
    application( "output" )
