@ECHO OFF

REM include directories
set Include_Dir="..\..\..\..\DtkInput\Namawala"

REM create config
python ..\..\regression_utils.py flatten-config .\param_overrides.json

REM run eradictaion with changed config
..\..\..\Eradication\x64\Release\eradication.exe -C config_flattened.json -I %Include_Dir% > ..\StdOut_28_Vector_Namawala_SP_LarvalHabitat_WriteReadSp_1.txt

REM copy and rename param_overides file
copy config_flattened.json config_load_sp.json

REM run python script and set parameters for serialization
python set_load_serialize_parameters.py

REM run eradictaion and load from dtk
..\..\..\Eradication\x64\Release\eradication.exe -C config_load_sp.json -I %Include_Dir% > ..\StdOut_28_Vector_Namawala_SP_LarvalHabitat_WriteReadSp_2.txt

python check_sp_larval_habitat.py

REM PAUSE