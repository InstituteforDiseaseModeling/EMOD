echo off
rem The “Load” scenarios are configured to locally pick up the files written by the
rem corresponding “Save” scenario (e.g. 10 writes a file and 11 picks up
rem  ..\10_Generic_Version_2_Save\output\state-00010.dtk).
rem
rem There is no comparison of output, just that the loading scenarios pick up the saved state
rem and run to completion without an error.

echo on

setlocal
rem setlocal INPUT_SRC=\src\input
set INPUT_SRC=C:\Users\tfischle\Github\DtkInput


cd 11_Generic_Version_2_Load
python ..\..\regression_utils.py flatten-config .\param_overrides.json
..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I . > ..\StdOut_11_Generic_Version_2_Load.txt

cd ..\13_Vector_Version_2_Load
python ..\..\regression_utils.py flatten-config .\param_overrides.json
..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I %INPUT_SRC%\Namawala > ..\StdOut_13_Vector_Version_2_Load.txt

cd ..\15_Malaria_Version_2_Load
python ..\..\regression_utils.py flatten-config .\param_overrides.json
..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I %INPUT_SRC%\Namawala > ..\StdOut_15_Malaria_Version_2_Load.txt

cd ..\21_Generic_Multicore_Load
python ..\..\regression_utils.py flatten-config .\param_overrides.json
mpiexec -n 2 ..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I . 1> ..\StdOut_21_Generic_Multicore_Load.txt

cd ..\23_Nosibe_Multicore_Load
python ..\..\regression_utils.py flatten-config .\param_overrides.json
mpiexec -n 2 ..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I %INPUT_SRC%\test 1> ..\StdOut_23_Nosibe_Multicore_Load.txt

rem copy all dtk files created in 25_Vector_Namawala_SP_LarvalHabitat_MaskWrite
cd ..\25_Vector_Namawala_SP_LarvalHabitat_MaskWrite

copy output\state-00050.dtk ..\26_Vector_Namawala_SP_LarvalHabitat_ReadConfig\testing
copy output\state-00050.dtk ..\27_Vector_Namawala_SP_LarvalHabitat_NoLarvalHabitat_ThrowException\testing
copy output\state-00050.dtk ..\29_Vector_Namawala_SP_LarvalHabitat_NoLarvalHabitat_ThrowException_NotEnoughVectorParams\testing

cd ..\26_Vector_Namawala_SP_LarvalHabitat_ReadConfig
python ..\..\regression_utils.py flatten-config .\param_overrides.json
..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I %INPUT_SRC%\Namawala > ..\StdOut_26_Vector_Namawala_SP_LarvalHabitat_ReadConfig.txt
python check_sp_larval_habitat.py

cd ..\27_Vector_Namawala_SP_LarvalHabitat_NoLarvalHabitat_ThrowException
CALL run_test.bat

rem copy dtk file created in 28_1_Vector_Namawala_SP_LarvalHabitat_WriteSp for load test
cd ..\28_1_Vector_Namawala_SP_LarvalHabitat_WriteSp
copy output\state-00010.dtk ..\28_2_Vector_Namawala_SP_LarvalHabitat_ReadSp/testing

cd ..\28_2_Vector_Namawala_SP_LarvalHabitat_ReadSp
CALL run_test.bat

cd ..\29_Vector_Namawala_SP_LarvalHabitat_NoLarvalHabitat_ThrowException_NotEnoughVectorParams
CALL run_test.bat


cd ..
