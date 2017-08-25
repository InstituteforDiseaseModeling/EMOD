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
set INPUT_SRC=c:\EMOD\input_data

cd 10_Generic_Version_2_Save
..\..\..\Eradication\x64\Release\Eradication.exe -C config.json -I . > ..\StdOut_10_Generic_Version_2_Save.txt

cd ..\11_Generic_Version_2_Load
..\..\..\Eradication\x64\Release\Eradication.exe -C config.json -I . > ..\StdOut_11_Generic_Version_2_Load.txt

cd ..\12_Vector_Version_2_Save
..\..\..\Eradication\x64\Release\Eradication.exe -C config.json -I %INPUT_SRC%\Namawala > ..\StdOut_12_Vector_Version_2_Save.txt

cd ..\13_Vector_Version_2_Load
..\..\..\Eradication\x64\Release\Eradication.exe -C config.json -I %INPUT_SRC%\Namawala > ..\StdOut_13_Vector_Version_2_Load.txt

cd ..\14_Malaria_Version_2_Save
..\..\..\Eradication\x64\Release\Eradication.exe -C config.json -I %INPUT_SRC%\Namawala > ..\StdOut_14_Malaria_Version_2_Save.txt

cd ..\15_Malaria_Version_2_Load
..\..\..\Eradication\x64\Release\Eradication.exe -C config.json -I %INPUT_SRC%\Namawala > ..\StdOut_15_Malaria_Version_2_Load.txt

cd ..\20_Generic_Multicore_Save
mpiexec -n 2 ..\..\..\Eradication\x64\Release\Eradication.exe -C config.json -I . 1> ..\StdOut_20_Generic_Multicore_Save.txt

cd ..\21_Generic_Multicore_Load
mpiexec -n 2 ..\..\..\Eradication\x64\Release\Eradication.exe -C config.json -I . 1> ..\StdOut_21_Generic_Multicore_Load.txt

cd ..\22_Nosibe_Multicore_Save
mpiexec -n 2 ..\..\..\Eradication\x64\Release\Eradication.exe -C config.json -I %INPUT_SRC%\test 1> ..\StdOut_22_Nosibe_Multicore_Save.txt

cd ..\23_Nosibe_Multicore_Load
mpiexec -n 2 ..\..\..\Eradication\x64\Release\Eradication.exe -C config.json -I %INPUT_SRC%\test 1> ..\StdOut_23_Nosibe_Multicore_Load.txt

cd ..