@echo off
set NDK_ROOT=%ANDROID_NDK_ROOT%
setlocal enabledelayedexpansion
call %NDK_ROOT%\ndk-build.cmd
echo Exiting...& ping /n 3 127.0.1 >nul
goto :eof