0) create config.json e.g. by running regresion_test.py

1) Run Eradication .exe with I parameter set to path to Namawala_single_node_air_temperature_daily.bin, e.g.
Eradication.exe -C config.json -I C:\Users\tfischle\Github\DtkInput\Namawala -O testing

state-00050.dtk should appear in testing directory

2) run Python script check_sp_larval_habitat.py
The script will check if serilization excluded larval habitats, serilizationMask was set to the correct value, and if EggQueues exist.

3) Check to output of the Python scrip!!