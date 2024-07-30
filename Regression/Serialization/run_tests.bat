@echo off
rem The “Load” scenarios are configured to locally pick up the files written by the
rem corresponding “Save” scenario (e.g. 10 writes a file and 11 picks up
rem  ..\10_Generic_Version_2_Save\output\state-00010.dtk).
rem
rem There is no comparison of output, just that the loading scenarios pick up the saved state
rem and run to completion without an error.

rem setlocal INPUT_SRC=\src\input
set INPUT_SRC=C:\Users\thomasfi\Github\DtkInput

if not exist %INPUT_SRC%\ (
  echo Directory %INPUT_SRC does not exist. Please edit this file and correct the path to point to DtkInput directory.
  exit /b 1	
) 

REM echo Deleting all *.dtk files in this directory
del *.dtk /S

echo on

setlocal


cd 10_Generic_Version_2_Save
python ..\..\regression_utils.py flatten-config .\param_overrides.json
..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I . > ..\StdOut_10_Generic_Version_2_Save.txt

cd ..\11_Generic_Version_2_Load
python ..\..\regression_utils.py flatten-config .\param_overrides.json
..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I . > ..\StdOut_11_Generic_Version_2_Load.txt

cd ..\12_Vector_Version_2_Save
python ..\..\regression_utils.py flatten-config .\param_overrides.json
..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I %INPUT_SRC%\Namawala > ..\StdOut_12_Vector_Version_2_Save.txt

cd ..\13_Vector_Version_2_Load
python ..\..\regression_utils.py flatten-config .\param_overrides.json
..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I %INPUT_SRC%\Namawala > ..\StdOut_13_Vector_Version_2_Load.txt

cd ..\14_Malaria_Version_2_Save
python ..\..\regression_utils.py flatten-config .\param_overrides.json
..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I %INPUT_SRC%\Namawala > ..\StdOut_14_Malaria_Version_2_Save.txt

cd ..\15_Malaria_Version_2_Load
python ..\..\regression_utils.py flatten-config .\param_overrides.json
..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I %INPUT_SRC%\Namawala > ..\StdOut_15_Malaria_Version_2_Load.txt

cd ..\20_Generic_Multicore_Save
python ..\..\regression_utils.py flatten-config .\param_overrides.json
mpiexec -n 2 ..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I . 1> ..\StdOut_20_Generic_Multicore_Save.txt

cd ..\21_Generic_Multicore_Load
python ..\..\regression_utils.py flatten-config .\param_overrides.json
mpiexec -n 2 ..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I . 1> ..\StdOut_21_Generic_Multicore_Load.txt

cd ..\22_Nosibe_Multicore_Save
python ..\..\regression_utils.py flatten-config .\param_overrides.json
mpiexec -n 2 ..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I %INPUT_SRC%\test 1> ..\StdOut_22_Nosibe_Multicore_Save.txt

cd ..\23_Nosibe_Multicore_Load
python ..\..\regression_utils.py flatten-config .\param_overrides.json
mpiexec -n 2 ..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I %INPUT_SRC%\test 1> ..\StdOut_23_Nosibe_Multicore_Load.txt

@echo off

echo.
echo ------ 25_Vector_Namawala_SP_LarvalHabitat_MaskWrite ------
cd ..\25_Vector_Namawala_SP_LarvalHabitat_MaskWrite
python ..\..\regression_utils.py flatten-config .\param_overrides.json
..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I %INPUT_SRC%\Namawala > ..\StdOut_25_Vector_Namawala_SP_LarvalHabitat_MaskWrite.txt
python check_sp_larval_habitat.py

rem copy, create directory and overwrite 
xcopy /i /y output\state-00050.dtk ..\26_Vector_Namawala_SP_LarvalHabitat_ReadConfig\testing\
xcopy /i /y output\state-00050.dtk ..\27_Vector_Namawala_SP_LarvalHabitat_NoLarvalHabitat_ThrowException\testing\
xcopy /i /y output\state-00050.dtk ..\29_Vector_Namawala_SP_LarvalHabitat_NoLarvalHabitat_ThrowException_NotEnoughVectorParams\testing\

echo.
echo.
echo ------ 26_Vector_Namawala_SP_LarvalHabitat_ReadConfig ------
cd ..\26_Vector_Namawala_SP_LarvalHabitat_ReadConfig
python ..\..\regression_utils.py flatten-config .\param_overrides.json
..\..\..\Eradication\x64\Release\Eradication.exe -C config_flattened.json -I %INPUT_SRC%\Namawala > ..\StdOut_26_Vector_Namawala_SP_LarvalHabitat_ReadConfig.txt
python check_sp_larval_habitat.py

echo.
echo.
echo ------ Running 27_Vector_Namawala_SP_LarvalHabitat_NoLarvalHabitat_ThrowException ------
cd ..\27_Vector_Namawala_SP_LarvalHabitat_NoLarvalHabitat_ThrowException
CALL run_test.bat

echo.
echo.
echo ------ Running 28_1_Vector_Namawala_SP_LarvalHabitat_WriteSp ------
cd ..\28_1_Vector_Namawala_SP_LarvalHabitat_WriteSp
CALL run_test.bat

echo.
echo.
echo ------ Running 28_2_Vector_Namawala_SP_LarvalHabitat_ReadSp ------
cd ..\28_2_Vector_Namawala_SP_LarvalHabitat_ReadSp
python ..\..\regression_utils.py flatten-config .\param_overrides.json
xcopy /i /y ..\28_1_Vector_Namawala_SP_LarvalHabitat_WriteSp\output\state-00010.dtk .\testing\
xcopy /i /y ..\28_1_Vector_Namawala_SP_LarvalHabitat_WriteSp\output\state-00021.dtk .\testing\
CALL run_test.bat

echo.
echo.
echo ------ Running 28_3_Vector_Namawala_SP_LarvalHabitat_Mask_ReadSp ------
cd ..\28_3_Vector_Namawala_SP_LarvalHabitat_Mask_ReadSp
python ..\..\regression_utils.py flatten-config .\param_overrides.json
xcopy /i /y ..\28_1_Vector_Namawala_SP_LarvalHabitat_WriteSp\output\state-00010.dtk .\testing\
CALL run_test.bat

echo.
echo.
echo ------ Running 29_Vector_Namawala_SP_LarvalHabitat_NoLarvalHabitat_ThrowException_NotEnoughVectorParams ------
cd ..\29_Vector_Namawala_SP_LarvalHabitat_NoLarvalHabitat_ThrowException_NotEnoughVectorParams
CALL run_test.bat

echo.
echo.
echo ------ Running 30_1_Header_Version5 ------
cd ..\30_1_Header_Version5
python ..\..\regression_utils.py flatten-config .\param_overrides.json
CALL run_test.bat

echo.
echo.
echo ------ Running 30_2_Header_Version5_Check_Error_Msg ------
cd ..\30_2_Header_Version5_Check_Error_Msg
python ..\..\regression_utils.py flatten-config .\param_overrides.json
xcopy /i /y ..\30_1_Header_Version5\output\state-00002.dtk .\testing\
CALL run_test.bat


cd ..
