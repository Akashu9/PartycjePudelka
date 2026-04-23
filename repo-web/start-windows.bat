@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "PORT=%~1"
if "%PORT%"=="" set "PORT=8080"

for /f "delims=0123456789" %%A in ("%PORT%") do set "NONNUM=1"
if defined NONNUM (
  echo [ERROR] Port musi byc liczba calkowita. Uzycie: start-windows.bat [port]
  exit /b 1
)

if %PORT% LSS 1 (
  echo [ERROR] Port musi byc w zakresie 1-65535.
  exit /b 1
)
if %PORT% GTR 65535 (
  echo [ERROR] Port musi byc w zakresie 1-65535.
  exit /b 1
)

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

set "PY_CMD="
where py >nul 2>nul
if %ERRORLEVEL% EQU 0 set "PY_CMD=py -3"

if not defined PY_CMD (
  where python >nul 2>nul
  if %ERRORLEVEL% EQU 0 set "PY_CMD=python"
)

if not defined PY_CMD (
  echo [ERROR] Nie znaleziono Pythona. Zainstaluj Python 3 i dodaj do PATH.
  exit /b 1
)

echo Uruchamiam serwer w: %SCRIPT_DIR%
echo Adres: http://localhost:%PORT%
start "" "http://localhost:%PORT%"

%PY_CMD% -m http.server %PORT%
