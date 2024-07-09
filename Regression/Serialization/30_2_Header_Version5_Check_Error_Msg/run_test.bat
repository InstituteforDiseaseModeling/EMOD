@ECHO OFF

REM include directories
set Include_Dir="..\..\..\..\DtkInput\Namawala"

REM Change version of sp in header to cause an exception
python change_header.py

REM run eradictaion with changed config
..\..\..\Eradication\x64\Release\eradication.exe -C config_flattened.json -I %Include_Dir% > ..\StdOut_30_2_Header_Version5_Check_Error_Msg.txt

Echo The test passed if an SerializationException is displayed saying that the EMOD version the serialized population was created with is not compatible with the current EMOD version.
Echo The exception must display the EMOD version the population was created with and the version of the serialized population interface of the current EMOD version.
