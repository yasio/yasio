@echo off

:begin
set version=%1

if "%version%"=="8.0" goto :l2 else (
if "%version%"=="9.0" goto :l2 else (
if "%version%"=="10.0" goto :l2 else (
if "%version%"=="11.0" goto :l2 else (
if "%version%"=="12.0" goto :l2 else (
if "%version%"=="14.0" goto :l2 else (
echo input error, press any key to input again... & pause>nul && goto :begin
)
)
)
)
)
)

:l2

rem vs2013 or later
REG COPY "HKEY_CURRENT_USER\Software\Microsoft\VisualStudio\%version%_Config\Languages\File Extensions\.inl" "HKEY_CURRENT_USER\Software\Microsoft\VisualStudio\%version%_Config\Languages\File Extensions\.ipp" /s /f
rem REG COPY "HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\VisualStudio\11.0\Languages\File Extensions\.inl" "HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\VisualStudio\11.0\Languages\File Extensions\.ipp" /s /f
rem vs2012 or before, REG COPY "HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\VisualStudio\%version%\Languages\File Extensions\.inl" "HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\VisualStudio\%version%\Languages\File Extensions\.ipp" /s /f

if %errorlevel%==0 echo register succeed. && goto :end

echo register file: %file_name% failed.

:end

echo please input any key to exit... & pause>nul
goto :eof
