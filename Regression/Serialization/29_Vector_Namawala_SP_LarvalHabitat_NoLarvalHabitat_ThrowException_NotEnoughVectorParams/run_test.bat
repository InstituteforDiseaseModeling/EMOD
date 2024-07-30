@ECHO OFF

REM include directories
set Include_Dir="..\..\..\..\DtkInput\Namawala"

REM create config
python ..\..\regression_utils.py flatten-config .\param_overrides.json

REM remove species from Vector_Species_Params
python remove_Vector_Species_Params.py

REM run eradictaion with changed config
..\..\..\Eradication\x64\Release\eradication.exe -C config_flattened.json -I %Include_Dir% > ..\StdOut_29_Vector_Namawala_SP_LarvalHabitat_NoLarvalHabitat_ThrowException_NotEnoughVectorParams.txt

Echo If an exception is displayed that says "The current configuration does not comply with the state of the simulation. A habitat of type 'TEMPORARY_RAINFALL' for species 'funestus' does not exist in the simulation." this test passed.

REM PAUSE