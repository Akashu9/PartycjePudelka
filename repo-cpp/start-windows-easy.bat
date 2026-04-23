@echo off
setlocal EnableExtensions

title Partycje Prostopadloscianu - C++
cd /d "%~dp0"

echo ===================================================
echo   Partycje Prostopadloscianu - szybki start C++
echo ===================================================
echo.
echo Ten skrypt:
echo   1. Skonfiguruje projekt CMake
echo   2. Zbuduje aplikacje C++
echo   3. Uruchomi jedno samodzielne okno programu
echo.

call "%~dp0start-windows.bat" Release
if %ERRORLEVEL% NEQ 0 (
  echo.
  echo [BLAD] Nie udalo sie uruchomic aplikacji.
  pause
  exit /b %ERRORLEVEL%
)

echo.
echo [OK] Aplikacja uruchomiona.
endlocal
