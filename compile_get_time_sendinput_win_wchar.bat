@echo off
rem ================================================================
rem  get_time_sendinput_win_wchar.c をコンパイル (ワイド文字版)
rem ================================================================

set SRC_FILE=get_time_sendinput_win_wchar.c
set EXE_FILE=get_time_sendinput_win_wchar.exe
set COMPILER=gcc
set C_FLAGS=-Wall -O2 -s
set LIBS=-luser32 -lgdi32 -mwindows

where %COMPILER% > nul 2> nul
if errorlevel 1 (
    echo Error: '%COMPILER%' command not found.
    goto :error_end
)

echo Compiling "%SRC_FILE%" to "%EXE_FILE%" (Wide Char Version, Pre Alt+Tab)...
%COMPILER% "%SRC_FILE%" %C_FLAGS% -o "%EXE_FILE%" %LIBS%

if errorlevel 1 (
    echo. & echo ********** COMPILE FAILED **********
) else (
    echo. & echo ********** COMPILE SUCCESSFUL **********
    echo Executable file: "%EXE_FILE%" created.
)

echo.
pause
goto :eof

:error_end
echo.
pause

:eof