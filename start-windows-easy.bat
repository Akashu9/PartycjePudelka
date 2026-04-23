@echo off
setlocal EnableExtensions

title Partycje Prostopadloscianu - Start
cd /d "%~dp0"

set "PORT=8080"

echo ===================================================
echo   Partycje Prostopadloscianu - szybki start
echo ===================================================
echo.
echo Ten skrypt:
echo   1. Sprawdzi, czy masz Python 3
echo   2. Uruchomi aplikacje lokalnie
echo   3. Otworzy przegladarke
echo.

call :find_python
if not defined PY_CMD goto :no_python

echo [OK] Python znaleziony: %PY_CMD%
echo [OK] Uruchamiam aplikacje pod adresem:
echo      http://localhost:%PORT%
echo.
echo Aby zatrzymac aplikacje, zamknij to okno.
echo.

start "" "http://localhost:%PORT%"
%PY_CMD% -m http.server %PORT%

goto :after_server

:no_python
echo [BLAD] Nie znaleziono Python 3.
echo.
echo Za chwile otworzy sie strona pobierania Pythona dla Windows.
echo Po instalacji uruchom ten plik ponownie.
echo.
echo W instalatorze koniecznie zaznacz opcje:
echo   Add python.exe to PATH
echo.
pause
start "" "https://www.python.org/downloads/windows/"
goto :end

:find_python
set "PY_CMD="
where py >nul 2>nul
if %ERRORLEVEL% EQU 0 (
  set "PY_CMD=py -3"
  goto :eof
)

where python >nul 2>nul
if %ERRORLEVEL% EQU 0 (
  set "PY_CMD=python"
)
goto :eof

:after_server
echo.
echo Serwer zostal zatrzymany.
pause

:end
endlocal
