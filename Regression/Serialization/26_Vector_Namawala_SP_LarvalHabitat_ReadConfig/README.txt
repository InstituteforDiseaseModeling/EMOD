0) create config.json e.g. by running regresion_test.py

1) Run Eradication .exe with I parameter set to path to Namawala_single_node_air_temperature_daily.bin, e.g.
Eradication.exe -C config.json -I C:\Users\tfischle\Github\DtkInput\Namawala -O testing

2) use state-00050.dtk from 25_Vector_Namawala_SP_LarvalHabitat_Read, if not present run the test

2) run Python script check_sp_larval_habitat.py
The script will check larval habitats is read in from config.json and serilizationMask was set to the correct value

3) Check to output of the Python scrip!!