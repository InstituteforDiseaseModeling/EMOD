@copy nul > "%HOMEDRIVE%%HOMEPATH%\.rt_show.sft"

set EMOD_ROOT=C:\GitHub\TBHIV\DtkTrunk



@IF "%EMOD_ROOT%"=="" (
ECHO EMOD_ROOT is NOT defined in this script yet. Please edit run_test.cmd directly.
EXIT /B
)

@ECHO %EMOD_ROOT%

@del *.txt
@%EMOD_ROOT%\build\x64\Release\Eradication\Eradication.exe -C config.json -I %EMOD_ROOT%\Regression\TBHIV\ -P %EMOD_ROOT%\Regression\shared_embedded_py_scripts --dll-path %EMOD_ROOT%\x64\Release> test.txt
@type scientific_feature_report.txt

@del "%HOMEDRIVE%%HOMEPATH%\.rt_show.sft"

