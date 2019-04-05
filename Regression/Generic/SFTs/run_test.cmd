@echo off
@copy nul > "%HOMEDRIVE%%HOMEPATH%.rt_show.sft"

set current_path=%cd%

REM replace "Regression" with "@" then split at "@" see https://stackoverflow.com/questions/13469939/replacing-characters-in-string and https://www.computerhope.com/forum/index.php?topic=134599.0
set temppath=%current_path:Regression=@%		
FOR /f "tokens=1 delims=@" %%a IN ("%temppath%") do set EMOD_ROOT=%%a

REM set EMOD_ROOT=C:\Users\jbloedow\DtkTrunk\

@IF "%EMOD_ROOT%"=="" (
ECHO EMOD_ROOT is NOT defined in this script yet. Please edit run_test.cmd directly.
EXIT /B
)

@ECHO EMOD_ROOT=%EMOD_ROOT%

@del *.txt
@%EMOD_ROOT%build\x64\Release\Eradication\Eradication.exe -C config.json -I %EMOD_ROOT%Regression\Generic\ -P %EMOD_ROOT%Regression\shared_embedded_py_scripts  -D %EMOD_ROOT%build\ > test.txt
@type scientific_feature_report.txt

@del "%HOMEDRIVE%%HOMEPATH%.rt_show.sft"

