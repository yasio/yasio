@rem THIS tool is work with ndkr16b
@echo off
set boost_dir=D:\develop\inetlibs\boost_1_66_0
set boost_asio_dir=D:\develop\inetlibs\boost.asio.1.0.166
echo NDK_ROOT=%NDK_ROOT%

setlocal enabledelayedexpansion

:L_Loop
call %NDK_ROOT%\ndk-build.cmd 2>compile_log.txt
rem pause
rem cl /nologo /O2 /W0 /EHsc  /c /I"%boost_asio_dir%" /DBOOST_ERROR_CODE_HEADER_ONLY /DBOOST_SYSTEM_NO_DEPRECATED /DBOOST_SYSTEM_NO_LIB /DBOOST_DATE_TIME_NO_LIB /DBOOST_REGEX_NO_LIB /DBOOST_ASIO_DISABLE_THREADS pseudo.cpp 1>compile_error.txt
set error_code=%errorlevel%

if %error_code% == 2 (
   rem for /f "usebackq delims=" %%i in ("compile_log.txt") do (  
   rem  echo %%i | findstr /C:"fatal error"
   rem  if /i !errorlevel!==0 set error_msg=%%i && goto :error_got
   rem )
   rem :error_got
   rem echo error message: "!error_msg!"
   rem if not "!error_msg!"=="" goto :error_ok
   rem pause
   echo find error message...
   findstr /C:"No such file or directory" compile_log.txt >compile_error.txt
   set /p error_msg=<compile_error.txt
   
   if not "!error_msg!"=="" goto :error_ok
   echo find error message again...
   findstr /C:"file not found" compile_log.txt >compile_error.txt
   set /p error_msg=<compile_error.txt
   
   :error_ok
   echo error message: "!error_msg!"
   
   rem goto :eof
   
   rem for /f "usebackq tokens=5* delims=:  " %%i in ('!error_msg!') do set missing_dir=%%i
   for /f "usebackq tokens=4* delims= " %%i in ('!error_msg!') do set missing_dir=%%i
   echo !missing_dir!
   for /f "usebackq tokens=1* delims=:" %%i in ('!missing_dir!') do set missing_dir=%%i
   echo !missing_dir!
   for /f "usebackq tokens=1* delims='" %%i in ('!missing_dir!') do set missing_dir=%%i
   echo !missing_dir!
   
   rem convert to windows path style
   call set missing_dir=%%missing_dir:/=\%%
   
   set target="%boost_asio_dir%\!missing_dir!"
   set source=%boost_dir%\!missing_dir!
   rem call set source=%%source:/=\%%
   
   echo !source!
   if not exist !source! echo source not exist & pause && goto :eof
   
   echo copy: !source! --^> !target!...

   echo F | xcopy /y /f !source! !target!
   
   rem goto :eof
   
   set error_msg=
   goto :L_Loop
   rem echo source:%boost_dir%\!missing!, target:%boost_asio_dir%\!missing!
   rem xcopy "%boost_dir%\!missing!" "%boost_asio_dir%\!missing!" /Y /R /F
)

:end
echo congrantulation, complete.
rem del /f /q pseudo.obj

echo ÕýÔÚÍË³ö¡£¡£¡£& ping /n 3 127.0.1 >nul
goto :eof


