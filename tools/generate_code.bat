@echo off

pcode_autog.exe

ping /n 2 127.0.1>nul && goto :eof
