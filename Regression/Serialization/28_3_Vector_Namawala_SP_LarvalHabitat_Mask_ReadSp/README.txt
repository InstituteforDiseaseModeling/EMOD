0)  create config.json e.g. by running regresion_test.py or 
    python ..\..\regression_utils.py flatten-config .\param_overrides.json

1)  Run run_test.bat

2)  The test will create a dtk file, no read or write mask is set i.e. default settings.
    A copy of config_flattened.json is created. set_load_serialize_parameters.py changes some serialization parameters and a larval habitat parameter.
    Because no read/write mask is set the changes in the configuration must not effect the simulation.

3) Check the output  