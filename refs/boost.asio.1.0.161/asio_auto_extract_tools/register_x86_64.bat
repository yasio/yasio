@echo off

:begin
set version=%1

if "%version%"=="8.0" goto :l2 else (
if "%version%"=="9.0" goto :l2 else (
if "%version%"=="10.0" goto :l2 else (
echo input error, press any key to input again... & pause>nul && goto :begin
)
)
)

:l2


set tag_item="HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\VisualStudio\%version%\Languages\Extensionless Files\{B2F072B0-ABC1-11D0-9D62-00C04FD9DFD9}"

set file_name=%2
rem set /p file_name=please input file name:

REG ADD %tag_item% /v %file_name% /t REG_SZ /f 1>nul 2>nul

if %errorlevel%==0 echo register file: '%file_name%' succeed. && goto :end

echo register file: %file_name% failed.

:end
rem echo please input any key to exit... & pause>nul
goto :eof
