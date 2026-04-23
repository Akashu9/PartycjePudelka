@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set "BUILD_DIR=%SCRIPT_DIR%build"
set "TARGET=partycje_pudelka_cpp.exe"
set "CONFIG=Release"

if not "%~1"=="" set "CONFIG=%~1"

where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
  echo [ERROR] Nie znaleziono cmake.
  exit /b 1
)

echo [INFO] Konfiguracja CMake...
cmake -S "%SCRIPT_DIR%" -B "%BUILD_DIR%"
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo [INFO] Budowanie aplikacji (%CONFIG%)...
cmake --build "%BUILD_DIR%" --config %CONFIG%
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

set "APP_PATH=%BUILD_DIR%\%CONFIG%\%TARGET%"
if not exist "%APP_PATH%" set "APP_PATH=%BUILD_DIR%\%TARGET%"

if not exist "%APP_PATH%" (
  echo [ERROR] Nie znaleziono pliku wykonywalnego:
  echo         %APP_PATH%
  exit /b 1
)

echo [INFO] Uruchamianie: %APP_PATH%
start "" "%APP_PATH%"

endlocal
