cd ..
set TEMP_VERSIONINFO_FILE="%TEMP%\version_info.%RANDOM%.h"
call SubWCRev.exe "." "utils\version_info.tmpl" %TEMP_VERSIONINFO_FILE% -f

if %ERRORLEVEL% NEQ 0  echo Skipping write of version_info.h file & goto final

fc "utils\version_info.h" %TEMP_VERSIONINFO_FILE%
if %ERRORLEVEL% NEQ 0 (
    echo Writing new version_info.h file
    move /Y %TEMP_VERSIONINFO_FILE% "utils\version_info.h"
) else (
    echo Skipping write of version_info.h file; nothing changed
    del %TEMP_VERSIONINFO_FILE%
)

:final
EXIT 0