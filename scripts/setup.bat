@echo off
setlocal EnableDelayedExpansion

echo Setting up ESP-IDF environment...

set ACTION=%1
if "%ACTION%"=="" set ACTION=build

:: Try common locations
set ESP_IDF_PATHS="%USERPROFILE%\esp\esp-idf\export.bat" "%USERPROFILE%\esp\v6.0.1\esp-idf\export.bat" "C:\Espressif\frameworks\esp-idf-v6.0.1\export.bat" "C:\esp\esp-idf\export.bat"

set EXPORT_SCRIPT=
for %%i in (%ESP_IDF_PATHS%) do (
    if exist %%i (
        set EXPORT_SCRIPT=%%i
        goto :found
    )
)

if defined IDF_PATH (
    if exist "%IDF_PATH%\export.bat" (
        set EXPORT_SCRIPT="%IDF_PATH%\export.bat"
        goto :found
    )
)

echo Error: Could not find ESP-IDF export.bat. Please ensure ESP-IDF is installed.
exit /b 1

:found
echo Found ESP-IDF at %EXPORT_SCRIPT%
call %EXPORT_SCRIPT%

if "%ACTION%"=="build" (
    echo Building project...
    idf.py build
) else if "%ACTION%"=="flash" (
    echo Building and flashing project...
    idf.py build flash monitor
) else (
    echo Running custom idf.py command: idf.py %ACTION%
    idf.py %ACTION%
)
