@echo off
set workdir=%cd%\
set boost_dir=D:\develop\inetlibs\boost_1_61_0
set boost_asio_dir=D:\develop\inetlibs\boost.asio.1.0.161
rem setup compile env, use vs2013
set programdir=%ProgramFiles%
if exist "%ProgramFiles% (x86)" set programdir=%programdir% (x86)
rem @set vcPath="%programdir%\Microsoft Visual Studio 12.0\VC\bin"
rem set path=%path%;%programdir%\Microsoft Visual Studio 12.0\VC\bin
rem "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"
call "%programdir%\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"

setlocal enabledelayedexpansion

:L_Loop
cl /nologo /O2 /W0 /EHsc  /c /I"%boost_asio_dir%" /D_WIN32 /D_DEBUG /DBOOST_ERROR_CODE_HEADER_ONLY /DBOOST_SYSTEM_NO_DEPRECATED /DBOOST_SYSTEM_NO_LIB /DBOOST_DATE_TIME_NO_LIB /DBOOST_REGEX_NO_LIB /DBOOST_ASIO_DISABLE_IOCP /DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE pseudo.cpp 1>compile_error.txt
set error_code=%errorlevel%

if %error_code% == 2 (
   set /a n=0 
   for /f "usebackq delims=" %%i in ("compile_error.txt") do ( 
     if /i !n!==5 set error_msg=%%i && goto :error_got
     set /a n+=1
   )
   
   :error_got
   if "!error_msg!"=="" (
       set /a n=0 
       for /f "usebackq delims=" %%i in ("compile_error.txt") do ( 
       if /i !n!==1 set error_msg=%%i && goto :error_ok
       set /a n+=1
     )
   )
   
   :error_ok
   echo error message: "!error_msg!"
   
   for /f "usebackq tokens=2,3* delims=¡°¡±" %%i in ('!error_msg!') do set missing=%%i && set missing_dir=%%~dpi
   for /f "usebackq tokens=1,2 delims= " %%i in ('!missing!') do set missing=%%i
   
   call set missing_dir=%%missing_dir:%workdir%=%%
   echo relative mssing: "!missing!"
   echo relative missing dir:!missing_dir!
   set missing_dir="%boost_asio_dir%\!missing_dir!"
   set source="%boost_dir%\!missing!"
   call set source=%%source:/=\%%
   
   if not exist !missing_dir! echo make dir:!missing_dir! && mkdir !missing_dir!
   
   echo copy: !source! --^> !missing_dir!...

   copy /y !source! !missing_dir!
   
   set error_msg=
   goto :L_Loop
   rem echo source:%boost_dir%\!missing!, target:%boost_asio_dir%\!missing!
   rem xcopy "%boost_dir%\!missing!" "%boost_asio_dir%\!missing!" /Y /R /F
)

:end
echo congrantulation, complete.
del /f /q pseudo.obj

echo ÕýÔÚÍË³ö¡£¡£¡£& ping /n 3 127.0.1 >nul
goto :eof


