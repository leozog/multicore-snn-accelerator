@echo off
setlocal enabledelayedexpansion

set CPP_OUTPUT_DIR=output_cpp/flatbuffers
set PY_OUTPUT_DIR=output_py/flatbuffers

set "SOURCE_DIR=%~dp0"
if not exist "%CPP_OUTPUT_DIR%" mkdir "%CPP_OUTPUT_DIR%"
if not exist "%PY_OUTPUT_DIR%" mkdir "%PY_OUTPUT_DIR%"

set "FILES="
for %%F in ("%SOURCE_DIR%*.fbs") do (
	set "FILES=!FILES! "%%~fF""
    echo %%~fF
)

flatc --cpp -o "%CPP_OUTPUT_DIR%" !FILES! --gen-object-api
flatc --python -o "%PY_OUTPUT_DIR%" !FILES! --gen-object-api

echo Compilation complete!

endlocal