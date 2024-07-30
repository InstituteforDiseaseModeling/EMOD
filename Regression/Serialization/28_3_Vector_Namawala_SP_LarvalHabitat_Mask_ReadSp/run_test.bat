@ECHO OFF

REM include directories
set Include_Dir="..\..\..\..\DtkInput\Namawala"

REM run eradictaion with changed config
..\..\..\Eradication\x64\Release\eradication.exe -C config_flattened.json -I %Include_Dir% > ..\StdOut_28_3_Vector_Namawala_SP_LarvalHabitat_Mask_ReadSp_1.txt

python check_sp_larval_habitat.py

REM PAUSE