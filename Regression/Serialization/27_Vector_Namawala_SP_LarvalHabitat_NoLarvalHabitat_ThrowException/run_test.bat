@ECHO OFF

REM include directories
set Include_Dir="..\..\..\..\DtkInput\Namawala"

REM create config
python ..\..\regression_utils.py flatten-config .\param_overrides.json

REM run eradictaion with changed config
..\..\..\Eradication\x64\Release\eradication.exe -C config_flattened.json -I %Include_Dir% > ..\StdOut_27_Vector_Namawala_SP_LarvalHabitat_NoLarvalHabitat_ThrowException.txt

Echo If an exception is displayed this test passed.

REM PAUSE