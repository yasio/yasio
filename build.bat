@echo off

powershell scripts/build.ps1 -p win32 -a x64 -cm "'-Bbuild','-DCXX_STD=20'"
