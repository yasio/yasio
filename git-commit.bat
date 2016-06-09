@echo off
set /p comment=please input comment:
if not defined comment set comment=default

rem cd x-studio365-ldb
git add --all .
git commit -m "%comment%"
git push
rem cd ..
ping /n 3 127.0.1 >nul
goto :eof

